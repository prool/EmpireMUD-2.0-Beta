/* ************************************************************************
*   File: vehicles.c                                      EmpireMUD 2.0b5 *
*  Usage: DB and OLC for vehicles (including ships)                       *
*                                                                         *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2015                      *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  EmpireMUD based upon CircleMUD 3.0, bpl 17, by Jeremy Elson.           *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"
#include "comm.h"
#include "olc.h"
#include "skills.h"
#include "handler.h"
#include "dg_scripts.h"

/**
* Contents:
*   Helpers
*   Utilities
*   Database
*   2.0b3.8 Converter
*   OLC Handlers
*   Displays
*   Edit Modules
*/

// local data
const char *default_vehicle_keywords = "vehicle unnamed";
const char *default_vehicle_short_desc = "an unnamed vehicle";
const char *default_vehicle_long_desc = "An unnamed vehicle is parked here.";

// local protos
void add_room_to_vehicle(room_data *room, vehicle_data *veh);
void clear_vehicle(vehicle_data *veh);
void finish_dismantle_vehicle(char_data *ch, vehicle_data *veh);
int get_new_vehicle_construction_id();
void ruin_vehicle(vehicle_data *veh, char *message);
char_data *unharness_mob_from_vehicle(struct vehicle_attached_mob *vam, vehicle_data *veh);
bool vehicle_allows_climate(vehicle_data *veh, room_data *room);

// external consts
extern const char *climate_flags[];
extern const bitvector_t climate_flags_order[];
extern const char *designate_flags[];
extern const char *function_flags[];
extern const char *interact_types[];
extern const char *mob_move_types[];
extern const char *room_aff_bits[];
extern const char *veh_custom_types[];
extern const char *vehicle_flags[];
extern const char *vehicle_speed_types[];

// external funcs
void adjust_vehicle_tech(vehicle_data *veh, bool add);
extern struct resource_data *copy_resource_list(struct resource_data *input);
extern room_data *create_room(room_data *home);
void get_resource_display(struct resource_data *list, char *save_buffer);
void scale_item_to_level(obj_data *obj, int level);
extern char *show_color_codes(char *string);
extern bool validate_icon(char *icon);


 //////////////////////////////////////////////////////////////////////////////
//// HELPERS /////////////////////////////////////////////////////////////////

/**
* Cancels vehicle ownership on vehicles whose empires are gone, allowing those
* vehicles to be cleaned up by players.
*/
void abandon_lost_vehicles(void) {
	vehicle_data *veh;
	empire_data *emp;
	
	DL_FOREACH(vehicle_list, veh) {
		if (!(emp = VEH_OWNER(veh))) {
			continue;	// only looking to abandon owned vehs
		}
		if (EMPIRE_IMM_ONLY(emp)) {
			continue;	// imm empire vehicles could be disastrous
		}
		if (EMPIRE_MEMBERS(emp) > 0 || EMPIRE_TERRITORY(emp, TER_TOTAL) > 0) {
			continue;	// skip empires that still have territory or members
		}
		
		// found!
		VEH_OWNER(veh) = NULL;
		
		if (VEH_INTERIOR_HOME_ROOM(veh)) {
			abandon_room(VEH_INTERIOR_HOME_ROOM(veh));
		}
		
		if (VEH_IS_COMPLETE(veh)) {
			qt_empire_players(emp, qt_lose_vehicle, VEH_VNUM(veh));
			et_lose_vehicle(emp, VEH_VNUM(veh));
		}
	}
}


/**
* Checks the allowed-climates on every vehicle in the room. This should be
* called if the terrain changes.
*
* @param room_data *room The room to check vehicles in.
*/
void check_vehicle_climate_change(room_data *room) {
	vehicle_data *veh, *next_veh;
	char *msg;
	
	DL_FOREACH_SAFE2(ROOM_VEHICLES(room), veh, next_veh, next_in_room) {
		if (!vehicle_allows_climate(veh, IN_ROOM(veh))) {
			// this will extract it (usually)
			msg = veh_get_custom_message(veh, VEH_CUSTOM_CLIMATE_CHANGE_TO_ROOM);
			ruin_vehicle(veh, msg ? msg : "$V falls into ruin!");
		}
	}
}


/**
* @param vehicle_data *veh Any vehicle instance.
* @return int The number of animals harnessed to it.
*/
int count_harnessed_animals(vehicle_data *veh) {
	struct vehicle_attached_mob *iter;
	int count;
	LL_COUNT(VEH_ANIMALS(veh), iter, count);
	return count;
}


/**
* Counts how many building-vehicles are in the room (testing using the
* VEH_CLAIMS_WITH_ROOM() macro). Optionally, you can check the owner, too.
*
* @param room_data *room The location.
* @param empire_data *only_owner Optional: Only count ones owned by this person (NULL for any-owner).
* @return int The number of building-vehicles in the room.
*/
int count_building_vehicles_in_room(room_data *room, empire_data *only_owner) {
	vehicle_data *veh;
	int count = 0;
	if (room) {
		DL_FOREACH2(ROOM_VEHICLES(room), veh, next_in_room) {
			if (VEH_CLAIMS_WITH_ROOM(veh) && (!only_owner || VEH_OWNER(veh) == only_owner)) {
				++count;
			}
		}
	}
	return count;
}


/**
* Determines how many players are inside a vehicle.
*
* @param vehicle_data *veh The vehicle to check.
* @param bool ignore_invis_imms If TRUE, does not count immortals who can't be seen by players.
* @return int How many players were inside.
*/
int count_players_in_vehicle(vehicle_data *veh, bool ignore_invis_imms) {
	struct vehicle_room_list *vrl, *next_vrl;
	vehicle_data *viter;
	char_data *iter;
	int count = 0;
	
	LL_FOREACH_SAFE(VEH_ROOM_LIST(veh), vrl, next_vrl) {
		DL_FOREACH2(ROOM_PEOPLE(vrl->room), iter, next_in_room) {
			if (IS_NPC(iter)) {
				continue;
			}
			if (ignore_invis_imms && IS_IMMORTAL(iter) && (GET_INVIS_LEV(iter) > 1 || PRF_FLAGGED(iter, PRF_WIZHIDE))) {
				continue;
			}
			
			++count;
		}
		
		// check nested vehicles
		DL_FOREACH2(ROOM_VEHICLES(vrl->room), viter, next_in_room) {
			count += count_players_in_vehicle(viter, ignore_invis_imms);
		}
	}
	
	return count;
}


/**
* Deletes the interior rooms of a vehicle e.g. before an extract or dismantle.
*
* @param vehicle_data *veh The vehicle whose interior must go.
*/
void delete_vehicle_interior(vehicle_data *veh) {
	void relocate_players(room_data *room, room_data *to_room);
	
	struct vehicle_room_list *vrl, *next_vrl;
	room_data *main_room;
	
	if ((main_room = VEH_INTERIOR_HOME_ROOM(veh)) || VEH_ROOM_LIST(veh)) {
		LL_FOREACH_SAFE(VEH_ROOM_LIST(veh), vrl, next_vrl) {
			if (vrl->room == main_room) {
				continue;	// do this one last
			}
			
			relocate_players(vrl->room, IN_ROOM(veh));
			delete_room(vrl->room, FALSE);	// MUST check_all_exits later
		}
		
		if (main_room) {
			relocate_players(main_room, IN_ROOM(veh));
			delete_room(main_room, FALSE);
		}
		check_all_exits();
	}
}


/**
* Empties the contents (items) of a vehicle into the room it's in (if any) or
* extracts them.
*
* @param vehicle_data *veh The vehicle to empty.
*/
void empty_vehicle(vehicle_data *veh) {
	obj_data *obj, *next_obj;
	
	DL_FOREACH_SAFE2(VEH_CONTAINS(veh), obj, next_obj, next_content) {
		if (IN_ROOM(veh)) {
			obj_to_room(obj, IN_ROOM(veh));
		}
		else {
			extract_obj(obj);
		}
	}
}


/**
* Finds the craft recipe entry for a given building vehicle.
*
* @param room_data *room The building location to find.
* @param byte type FIND_BUILD_UPGRADE or FIND_BUILD_NORMAL.
* @return craft_data* The craft for that building, or NULL.
*/
craft_data *find_craft_for_vehicle(vehicle_data *veh) {
	craft_data *craft, *next_craft;
	any_vnum recipe;
	
	if ((recipe = get_vehicle_extra_data(veh, ROOM_EXTRA_BUILD_RECIPE)) > 0 && (craft = craft_proto(recipe))) {
		return craft;
	}
	else {
		HASH_ITER(hh, craft_table, craft, next_craft) {
			if (CRAFT_FLAGGED(craft, CRAFT_IN_DEVELOPMENT) || !CRAFT_IS_VEHICLE(craft)) {
				continue;	// not a valid target
			}
			if (GET_CRAFT_OBJECT(craft) != VEH_VNUM(veh)) {
				continue;
			}
		
			// we have a match!
			return craft;
		}
	}
	
	return NULL;	// if none
}


/**
* Finds a vehicle in the room that is mid-dismantle. Optionally, finds a
* specific one rather than ANY one.
*
* @param room_data *room The room to find the vehicle in.
* @param int with_id Optional: Find a vehicle with this construction-id (pass NOTHING to find any mid-dismantle vehicle).
* @return vehicle_data* The found vehicle, if any (otherwise NULL).
*/
vehicle_data *find_dismantling_vehicle_in_room(room_data *room, int with_id) {
	vehicle_data *veh;
	
	DL_FOREACH2(ROOM_VEHICLES(room), veh, next_in_room) {
		if (!VEH_IS_DISMANTLING(veh)) {
			continue;	// not being dismantled
		}
		if (with_id != NOTHING && VEH_CONSTRUCTION_ID(veh) != with_id) {
			continue;	// wrong id
		}
		
		// found!
		return veh;
	}
	
	return NULL;	// not found
}


/**
* Finishes the actual dismantle for a vehicle.
*
* @param char_data *ch Optional: The dismantler.
* @param vehicle_data *veh The vehicle being dismantled.
*/
void finish_dismantle_vehicle(char_data *ch, vehicle_data *veh) {
	extern bool check_autostore(obj_data *obj, bool force, empire_data *override_emp);
	extern struct empire_chore_type chore_data[NUM_CHORES];
	
	obj_data *newobj, *proto;
	craft_data *type;
	char_data *iter;
	
	if (ch) {
		act("You finish dismantling $V.", FALSE, ch, NULL, veh, TO_CHAR);
		act("$n finishes dismantling $V.", FALSE, ch, NULL, veh, TO_ROOM);
	}
	
	if (IN_ROOM(veh)) {
		DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(veh)), iter, next_in_room) {
			if (!IS_NPC(iter) && GET_ACTION(iter) == ACT_DISMANTLE_VEHICLE && GET_ACTION_VNUM(iter, 1) == VEH_CONSTRUCTION_ID(veh)) {
				cancel_action(iter);
			}
			else if (IS_NPC(iter) && GET_MOB_VNUM(iter) == chore_data[CHORE_BUILDING].mob) {
				SET_BIT(MOB_FLAGS(iter), MOB_SPAWNED);
			}
		}
	}
	
	// check for required obj and return it
	if (IN_ROOM(veh) && (type = find_craft_for_vehicle(veh)) && CRAFT_FLAGGED(type, CRAFT_TAKE_REQUIRED_OBJ) && GET_CRAFT_REQUIRES_OBJ(type) != NOTHING && (proto = obj_proto(GET_CRAFT_REQUIRES_OBJ(type))) && !OBJ_FLAGGED(proto, OBJ_SINGLE_USE)) {
		newobj = read_object(GET_CRAFT_REQUIRES_OBJ(type), TRUE);
		
		// scale item to minimum level
		scale_item_to_level(newobj, 0);
		
		if (!ch || IS_NPC(ch)) {
			obj_to_room(newobj, IN_ROOM(veh));
			check_autostore(newobj, TRUE, VEH_OWNER(veh));
		}
		else {
			obj_to_char(newobj, ch);
			act("You get $p.", FALSE, ch, newobj, 0, TO_CHAR);
			
			// ensure binding
			if (OBJ_FLAGGED(newobj, OBJ_BIND_FLAGS)) {
				bind_obj_to_player(newobj, ch);
			}
			load_otrigger(newobj);
		}
	}
			
	extract_vehicle(veh);
}


/**
* This runs after the vehicle is finished (or, in some cases, if it moves).
*
* @param vehicle_data *veh The vehicle being finished.
*/
void finish_vehicle_setup(vehicle_data *veh) {
	void init_mine(room_data *room, char_data *ch, empire_data *emp);
	
	if (!veh || !VEH_IS_COMPLETE(veh)) {
		return;	// no work
	}
	
	// mine setup
	if (room_has_function_and_city_ok(IN_ROOM(veh), FNC_MINE)) {
		init_mine(IN_ROOM(veh), NULL, VEH_OWNER(veh));
	}
}


/**
* Removes everyone/everything from inside a vehicle, and puts it on the outside
* if possible.
*
* @param vehicle_data *veh The vehicle to empty.
*/
void fully_empty_vehicle(vehicle_data *veh) {
	vehicle_data *iter, *next_iter;
	struct vehicle_room_list *vrl;
	obj_data *obj, *next_obj;
	char_data *ch, *next_ch;
	
	if (VEH_ROOM_LIST(veh)) {
		LL_FOREACH(VEH_ROOM_LIST(veh), vrl) {
			// remove other vehicles
			DL_FOREACH_SAFE2(ROOM_VEHICLES(vrl->room), iter, next_iter, next_in_room) {
				if (IN_ROOM(veh)) {
					vehicle_to_room(iter, IN_ROOM(veh));
				}
				else {
					vehicle_from_room(iter);
					extract_vehicle(iter);
				}
			}
			
			// remove people
			DL_FOREACH_SAFE2(ROOM_PEOPLE(vrl->room), ch, next_ch, next_in_room) {
				act("You are ejected from $V!", FALSE, ch, NULL, veh, TO_CHAR);
				if (IN_ROOM(veh)) {
					char_to_room(ch, IN_ROOM(veh));
					qt_visit_room(ch, IN_ROOM(ch));
					look_at_room(ch);
					act("$n is ejected from $V!", TRUE, ch, NULL, veh, TO_ROOM);
					msdp_update_room(ch);
				}
				else {
					extract_char(ch);
				}
			}
			
			// remove items
			DL_FOREACH_SAFE2(ROOM_CONTENTS(vrl->room), obj, next_obj, next_content) {
				if (IN_ROOM(veh)) {
					obj_to_room(obj, IN_ROOM(veh));
				}
				else {
					extract_obj(obj);
				}
			}
		}
	}
	
	// dump contents
	empty_vehicle(veh);
}


/**
* This returns (or creates, if necessary) the start of the interior of the
* vehicle. Some vehicles don't have this feature.
*
* @param vehicle_data *veh The vehicle to get the interior for.
* @return room_data* The interior home room, if it exists (may be NULL).
*/
room_data *get_vehicle_interior(vehicle_data *veh) {
	room_data *room;
	bld_data *bld;
	
	// already have one?
	if (VEH_INTERIOR_HOME_ROOM(veh)) {
		return VEH_INTERIOR_HOME_ROOM(veh);
	}
	// this vehicle has no interior available
	if (!VEH_IS_COMPLETE(veh) || !(bld = building_proto(VEH_INTERIOR_ROOM_VNUM(veh)))) {
		return NULL;
	}
	
	// otherwise, create the interior
	room = create_room(NULL);
	attach_building_to_room(bld, room, TRUE);
	COMPLEX_DATA(room)->home_room = NULL;
	SET_BIT(ROOM_BASE_FLAGS(room), ROOM_AFF_IN_VEHICLE);
	affect_total_room(room);
	
	// attach
	COMPLEX_DATA(room)->vehicle = veh;
	VEH_INTERIOR_HOME_ROOM(veh) = room;
	add_room_to_vehicle(room, veh);
	
	if (VEH_OWNER(veh)) {
		claim_room(room, VEH_OWNER(veh));
	}
	
	complete_wtrigger(room);
		
	return room;
}


/**
* Finds a mob (using multi-isname) attached to a vehicle, and returns the
* vehicle_attached_mob entry for it.
*
* @param vehicle_data *veh The vehicle to check.
* @param char *name The typed-in name (may contain "#.name" and/or multiple keywords).
* @return struct vehicle_attached_mob* The found entry, or NULL.
*/
struct vehicle_attached_mob *find_harnessed_mob_by_name(vehicle_data *veh, char *name) {
	struct vehicle_attached_mob *iter;
	char tmpname[MAX_INPUT_LENGTH];
	char *tmp = tmpname;
	char_data *proto;
	int number;
	
	// safety first
	if (!veh || !*name) {
		return NULL;
	}
	
	strcpy(tmp, name);
	number = get_number(&tmp);
	
	LL_FOREACH(VEH_ANIMALS(veh), iter) {
		if (!(proto = mob_proto(iter->mob))) {
			continue;
		}
		if (!multi_isname(tmp, GET_PC_NAME(proto))) {
			continue;
		}
		
		// found
		if (--number == 0) {
			return iter;
		}
	}
	
	return NULL;
}


/**
* Finds a vehicle that will be shown in the room. The vehicle must have an icon
* to qualify for this, and must also either be complete or be size > 0.
* Vehicles in buildings are never shown. Only the largest valid vehicle is
* returned.
*
* @param char_data *ch The player looking.
* @param room_data *room The room to check.
* @return vehicle_data* A vehicle to show, if any (NULL if not).
*/
vehicle_data *find_vehicle_to_show(char_data *ch, room_data *room) {
	extern bool vehicle_is_chameleon(vehicle_data *veh, room_data *from);
	
	vehicle_data *iter, *in_veh, *found = NULL;
	bool is_on_vehicle = ((in_veh = GET_ROOM_VEHICLE(IN_ROOM(ch))) && room == IN_ROOM(in_veh));
	int found_size = -1;
	
	// we don't show vehicles in buildings or closed tiles (unless the player is on a vehicle in that room, in which case we override)
	if (!is_on_vehicle && (IS_ANY_BUILDING(room) || ROOM_IS_CLOSED(room))) {
		return NULL;
	}
	
	DL_FOREACH2(ROOM_VEHICLES(room), iter, next_in_room) {
		if (!VEH_ICON(iter) || !*VEH_ICON(iter)) {
			continue;	// no icon
		}
		if (!VEH_IS_COMPLETE(iter) && VEH_SIZE(iter) < 1) {
			continue;	// skip incomplete unless it has size
		}
		if (vehicle_is_chameleon(iter, IN_ROOM(ch))) {
			continue;	// can't see from here
		}
		
		// valid to show! only if first/bigger
		if (!found || VEH_SIZE(iter) > found_size) {
			found = iter;
			found_size = VEH_SIZE(iter);
		}
	}
	
	return found;	// if any
}


/**
* Attaches an animal to a vehicle, and extracts the animal.
*
* @param char_data *mob The mob to attach.
* @param vehicle_data *veh The vehicle to attach it to.
*/
void harness_mob_to_vehicle(char_data *mob, vehicle_data *veh) {
	struct vehicle_attached_mob *vam;
	
	// safety first
	if (!mob || !IS_NPC(mob) || !veh) {
		return;
	}
	
	CREATE(vam, struct vehicle_attached_mob, 1);
	vam->mob = GET_MOB_VNUM(mob);
	vam->scale_level = GET_CURRENT_SCALE_LEVEL(mob);
	vam->flags = MOB_FLAGS(mob);
	vam->empire = GET_LOYALTY(mob) ? EMPIRE_VNUM(GET_LOYALTY(mob)) : NOTHING;
	
	LL_PREPEND(VEH_ANIMALS(veh), vam);
	extract_char(mob);
}


/**
* Gets the short description of a vehicle as seen by a particular person.
*
* @param vehicle_data *veh The vehicle to show.
* @param char_data *to Optional: A person who must be able to see it (or they get "something").
* @return char* The short description of the vehicle for that person.
*/
char *get_vehicle_short_desc(vehicle_data *veh, char_data *to) {
	if (!veh) {
		return "<nothing>";
	}
	
	if (to && !CAN_SEE_VEHICLE(to, veh)) {
		return "something";
	}
	
	return VEH_SHORT_DESC(veh);
}


/**
* Gets a nicely-formatted comma-separated list of all the animals leading
* the vehicle.
*
* @param vehicle_data *veh The vehicle.
* @return char* The list of animals pulling it.
*/
char *list_harnessed_mobs(vehicle_data *veh) {
	static char output[MAX_STRING_LENGTH];
	struct vehicle_attached_mob *iter, *next_iter, *tmp;
	int count, num = 0;
	size_t size = 0;
	char mult[256];
	bool skip;
	
	*output = '\0';
	
	LL_FOREACH_SAFE(VEH_ANIMALS(veh), iter, next_iter) {
		// stacking: determine if already listed
		skip = FALSE;
		count = 1;
		LL_FOREACH(VEH_ANIMALS(veh), tmp) {
			if (tmp == iter) {
				break;	// stop when we find this one
			}
			if (tmp->mob == iter->mob) {
				skip = TRUE;
				break;
			}
		}
		if (skip) {
			continue;	// already showed this one
		}
		
		// count how many to show
		count = 1;	// this one
		LL_FOREACH(iter->next, tmp) {
			if (tmp->mob == iter->mob) {
				++count;
			}
		}
		
		if (count > 1) {
			snprintf(mult, sizeof(mult), " (x%d)", count);
		}
		else {
			*mult = '\0';
		}
	
		size += snprintf(output+size, sizeof(output)-size, "%s%s%s", ((num == 0) ? "" : (next_iter ? ", " : (num > 1 ? ", and " : " and "))), get_mob_name_by_proto(iter->mob, TRUE), mult);
		++num;
	}
	
	return output;
}


/**
* Destroys a vehicle from disrepair or invalid location. A destroy trigger
* on the vehicle can prevent this.
*
* @param vehicle_data *veh The vehicle (will usually be extracted).
* @param char *message Optional: An act string (using $V for the vehicle) to send to the room. (NULL for none)
*/
void ruin_vehicle(vehicle_data *veh, char *message) {
	void delete_room_npcs(room_data *room, struct empire_territory_data *ter, bool make_homeless);
	
	struct vehicle_room_list *vrl;
	
	if (!destroy_vtrigger(veh)) {
		VEH_HEALTH(veh) = MAX(1, VEH_HEALTH(veh));	// ensure health
		return;
	}
	
	if (message && ROOM_PEOPLE(IN_ROOM(veh))) {
		act(message, FALSE, ROOM_PEOPLE(IN_ROOM(veh)), NULL, veh, TO_CHAR | TO_ROOM);
	}
	
	// delete the NPCs who live here first so they don't do an 'ejected-then-leave'
	LL_FOREACH(VEH_ROOM_LIST(veh), vrl) {
		delete_room_npcs(vrl->room, NULL, TRUE);
	}
	
	fully_empty_vehicle(veh);
	extract_vehicle(veh);
}


/**
* @param vehicle_data *veh The vehicle to scale.
* @param int level What level to scale it to (passing 0 will trigger auto-detection).
*/
void scale_vehicle_to_level(vehicle_data *veh, int level) {
	struct instance_data *inst = NULL;
	
	// detect level if we weren't given a strong level
	if (!level) {
		if (IN_ROOM(veh) && (inst = ROOM_INSTANCE(IN_ROOM(veh)))) {
			if (INST_LEVEL(inst) > 0) {
				level = INST_LEVEL(inst);
			}
		}
	}
	
	// outside constraints
	if (inst || (IN_ROOM(veh) && (inst = ROOM_INSTANCE(IN_ROOM(veh))))) {
		if (GET_ADV_MIN_LEVEL(INST_ADVENTURE(inst)) > 0) {
			level = MAX(level, GET_ADV_MIN_LEVEL(INST_ADVENTURE(inst)));
		}
		if (GET_ADV_MAX_LEVEL(INST_ADVENTURE(inst)) > 0) {
			level = MIN(level, GET_ADV_MAX_LEVEL(INST_ADVENTURE(inst)));
		}
	}
	
	// vehicle constraints
	if (VEH_MIN_SCALE_LEVEL(veh) > 0) {
		level = MAX(level, VEH_MIN_SCALE_LEVEL(veh));
	}
	if (VEH_MAX_SCALE_LEVEL(veh) > 0) {
		level = MIN(level, VEH_MAX_SCALE_LEVEL(veh));
	}
	
	// set the level
	VEH_SCALE_LEVEL(veh) = level;
}


/**
* Begins the dismantle process on a vehicle including setting its
* VEH_DISMANTLING flag and its VEH_CONSTRUCTION_ID().
*
* @param bool vehicle_data *veh The vehicle to dismantle.
*/
void start_dismantle_vehicle(vehicle_data *veh) {
	void reduce_dismantle_resources(int damage, int max_health, struct resource_data **list);
	
	struct resource_data *res, *next_res;
	obj_data *proto;
	
	// remove from goals/tech
	if (VEH_OWNER(veh) && VEH_IS_COMPLETE(veh)) {
		qt_empire_players(VEH_OWNER(veh), qt_lose_vehicle, VEH_VNUM(veh));
		et_lose_vehicle(VEH_OWNER(veh), VEH_VNUM(veh));
		adjust_vehicle_tech(veh, FALSE);
	}
	
	// clear it out
	fully_empty_vehicle(veh);
	delete_vehicle_interior(veh);
	
	// set up flags
	SET_BIT(VEH_FLAGS(veh), VEH_DISMANTLING);
	VEH_CONSTRUCTION_ID(veh) = get_new_vehicle_construction_id();
	
	// remove any existing resources remaining to build/maintain
	if (VEH_NEEDS_RESOURCES(veh)) {
		free_resource_list(VEH_NEEDS_RESOURCES(veh));
		VEH_NEEDS_RESOURCES(veh) = NULL;
	}
	
	// set up refundable resources: use the actual materials that built this thing
	if (VEH_BUILT_WITH(veh)) {
		VEH_NEEDS_RESOURCES(veh) = VEH_BUILT_WITH(veh);
		VEH_BUILT_WITH(veh) = NULL;
	}
	// remove liquids, etc
	LL_FOREACH_SAFE(VEH_NEEDS_RESOURCES(veh), res, next_res) {
		// RES_x: only RES_OBJECT is refundable
		if (res->type != RES_OBJECT) {
			LL_DELETE(VEH_NEEDS_RESOURCES(veh), res);
			free(res);
		}
		else {	// is object -- check for single_use
			if (!(proto = obj_proto(res->vnum)) || OBJ_FLAGGED(proto, OBJ_SINGLE_USE)) {
				LL_DELETE(VEH_NEEDS_RESOURCES(veh), res);
				free(res);
			}
		}
	}
	// reduce resource: they don't get it all back
	reduce_dismantle_resources(VEH_MAX_HEALTH(veh) - VEH_HEALTH(veh), VEH_MAX_HEALTH(veh), &VEH_NEEDS_RESOURCES(veh));
	
	// ensure it has no people/mobs on it
	if (VEH_LED_BY(veh)) {
		GET_LEADING_VEHICLE(VEH_LED_BY(veh)) = NULL;
		VEH_LED_BY(veh) = NULL;
	}
	if (VEH_SITTING_ON(veh)) {
		unseat_char_from_vehicle(VEH_SITTING_ON(veh));
	}
	if (VEH_DRIVER(veh)) {
		GET_DRIVING(VEH_DRIVER(veh)) = NULL;
		VEH_DRIVER(veh) = NULL;
	}
	while (VEH_ANIMALS(veh)) {
		unharness_mob_from_vehicle(VEH_ANIMALS(veh), veh);
	}
	
	affect_total_room(IN_ROOM(veh));
}


/**
* Sets a vehicle ablaze!
*
* @param vehicle_data *veh The vehicle to ignite.
*/
void start_vehicle_burning(vehicle_data *veh) {
	void do_unseat_from_vehicle(char_data *ch);
	
	if (VEH_OWNER(veh)) {
		log_to_empire(VEH_OWNER(veh), ELOG_HOSTILITY, "Your %s has caught on fire at (%d, %d)", skip_filler(VEH_SHORT_DESC(veh)), X_COORD(IN_ROOM(veh)), Y_COORD(IN_ROOM(veh)));
	}
	msg_to_vehicle(veh, TRUE, "It seems %s has caught fire!\r\n", VEH_SHORT_DESC(veh));
	SET_BIT(VEH_FLAGS(veh), VEH_ON_FIRE);

	if (VEH_SITTING_ON(veh)) {
		do_unseat_from_vehicle(VEH_SITTING_ON(veh));
	}
	if (VEH_LED_BY(veh)) {
		act("You stop leading $V.", FALSE, VEH_LED_BY(veh), NULL, veh, TO_CHAR);
		GET_LEADING_VEHICLE(VEH_LED_BY(veh)) = NULL;
		VEH_LED_BY(veh) = NULL;
	}
}


/**
* Determines the total size of all vehicles in the room.
*
* @param room_data *room The room to check.
* @return int The total size of vehicles there.
*/
int total_vehicle_size_in_room(room_data *room) {
	vehicle_data *veh;
	int size = 0;
	
	DL_FOREACH2(ROOM_VEHICLES(room), veh, next_in_room) {
		size += VEH_SIZE(veh);
	}
	
	return size;
}


/**
* Unharnesses a mob and loads it back into the game. If it fails to load the
* mob, it will still remove 'vam' from the animals list.
*
* @param struct vehicle_attached_mob *vam The attached-mob entry to unharness.
* @param vehicle_data *veh The vehicle to remove it from.
* @return char_data* A pointer to the mob if one was loaded, or NULL if not.
*/
char_data *unharness_mob_from_vehicle(struct vehicle_attached_mob *vam, vehicle_data *veh) {
	void scale_mob_to_level(char_data *mob, int level);
	void setup_generic_npc(char_data *mob, empire_data *emp, int name, int sex);	
	
	char_data *mob;
	
	// safety first
	if (!vam || !veh) {
		return NULL;
	}
	
	// remove the vam entry now
	LL_DELETE(VEH_ANIMALS(veh), vam);
	
	// things that keep us from spawning the mob
	if (!IN_ROOM(veh) || !mob_proto(vam->mob)) {
		free(vam);
		return NULL;
	}
	
	mob = read_mobile(vam->mob, TRUE);
	MOB_FLAGS(mob) = vam->flags;
	setup_generic_npc(mob, real_empire(vam->empire), NOTHING, NOTHING);
	if (vam->scale_level > 0) {
		scale_mob_to_level(mob, vam->scale_level);
	}
	char_to_room(mob, IN_ROOM(veh));
	load_mtrigger(mob);
	
	free(vam);
	return mob;
}


/**
* Updates the island/id and map location for the rooms inside a vehicle.
*
* @param vehicle_data *veh Which vehicle to update.
* @param room_data *loc Where the vehicle is.
*/
void update_vehicle_island_and_loc(vehicle_data *veh, room_data *loc) {
	struct vehicle_room_list *vrl;
	vehicle_data *iter;
	
	if (!veh) {
		return;
	}
	
	LL_FOREACH(VEH_ROOM_LIST(veh), vrl) {
		GET_ISLAND_ID(vrl->room) = GET_ISLAND_ID(loc);
		GET_ISLAND(vrl->room) = GET_ISLAND(loc);
		GET_MAP_LOC(vrl->room) = GET_MAP_LOC(loc);
		
		// check vehicles inside and cascade
		DL_FOREACH2(ROOM_VEHICLES(vrl->room), iter, next_in_room) {
			update_vehicle_island_and_loc(iter, loc);
		}
	}
}


/**
* Determines if a vehicle is allowed to be in a room based on climate. This
* only applies on the map. Open map buildings use the base sector; interiors
* are always allowed.
*
* @param vehicle_data *veh The vehicle to test.
* @param room_data *room What room to check for climate.
* @return bool TRUE if the vehicle allows it; FALSE if not.
*/
bool vehicle_allows_climate(vehicle_data *veh, room_data *room) {
	bitvector_t climate = NOBITS;
	
	if (!veh || !room) {
		return TRUE;	// junk in, junk out
	}
	if (ROOM_IS_CLOSED(room)) {
		return TRUE;	// closed rooms always allowed
	}
	
	// determine which climate to use
	if (IS_MAP_BUILDING(room) || IS_ROAD(room)) {
		// open map buildings use base sect
		climate = GET_SECT_CLIMATE(BASE_SECT(room));
	}
	else {
		// all other tiles use regular sect
		climate = GET_SECT_CLIMATE(SECT(room));
	}
	
	// compare
	if (VEH_REQUIRES_CLIMATE(veh) && !(VEH_REQUIRES_CLIMATE(veh) & climate)) {
		return FALSE;	// required climate type is missing
	}
	if (VEH_FORBID_CLIMATE(veh) && (VEH_FORBID_CLIMATE(veh) & climate)) {
		return FALSE;	// has a forbidden climate type
	}
	
	// otherwise, we made it!
	return TRUE;
}


/**
* Determines if a vehicle can be seen at a distance or not, using the chameleon
* flag and whether or not the vehicle is complete.
*
* @param vehicle_data *veh The vehicle.
* @param room_data *from Where it's being viewed from (to compute distance).
* @return bool TRUE if the vehicle should be hidden, FALSE if not.
*/
bool vehicle_is_chameleon(vehicle_data *veh, room_data *from) {
	if (!veh || !from || !IN_ROOM(veh)) {
		return FALSE;	// safety
	}
	if (!VEH_IS_COMPLETE(veh) || VEH_NEEDS_RESOURCES(veh)) {
		return FALSE;	// incomplete or unrepaired vehicles are not chameleon
	}
	if (!VEH_FLAGGED(veh, VEH_CHAMELEON) && (!IN_ROOM(veh) || !ROOM_AFF_FLAGGED(IN_ROOM(veh), ROOM_AFF_CHAMELEON))) {
		return FALSE;	// missing chameleon flags
	}
	
	// ok chameleon: now check distance
	return (compute_distance(from, IN_ROOM(veh)) >= 2);
}


 //////////////////////////////////////////////////////////////////////////////
//// UTILITIES ///////////////////////////////////////////////////////////////

/**
* Adds a room to a vehicle's tracking list.
*
* @param room_data *room The room.
* @param vehicle_data *veh The vehicle to add it to.
*/
void add_room_to_vehicle(room_data *room, vehicle_data *veh) {
	struct vehicle_room_list *vrl;
	CREATE(vrl, struct vehicle_room_list, 1);
	vrl->room = room;
	LL_APPEND(VEH_ROOM_LIST(veh), vrl);
	
	// initial island data
	if (IN_ROOM(veh)) {
		update_vehicle_island_and_loc(veh, IN_ROOM(veh));
	}
}


/**
* Checks for common vehicle problems and reports them to ch.
*
* @param vehicle_data *veh The item to audit.
* @param char_data *ch The person to report to.
* @return bool TRUE if any problems were reported; FALSE if all good.
*/
bool audit_vehicle(vehicle_data *veh, char_data *ch) {
	extern bool audit_extra_descs(any_vnum vnum, struct extra_descr_data *list, char_data *ch);
	extern bool audit_interactions(any_vnum vnum, struct interaction_item *list, int attach_type, char_data *ch);
	extern bool audit_spawns(any_vnum vnum, struct spawn_info *list, char_data *ch);
	
	char temp[MAX_STRING_LENGTH], *ptr;
	bld_data *interior = building_proto(VEH_INTERIOR_ROOM_VNUM(veh));
	bool problem = FALSE;
	
	if (!str_cmp(VEH_KEYWORDS(veh), default_vehicle_keywords)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Keywords not set");
		problem = TRUE;
	}
	
	ptr = VEH_KEYWORDS(veh);
	do {
		ptr = any_one_arg(ptr, temp);
		if (*temp && !str_str(VEH_SHORT_DESC(veh), temp) && !str_str(VEH_LONG_DESC(veh), temp)) {
			olc_audit_msg(ch, VEH_VNUM(veh), "Keyword '%s' not found in strings", temp);
			problem = TRUE;
		}
	} while (*ptr);
	
	if (!str_cmp(VEH_LONG_DESC(veh), default_vehicle_long_desc)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Long desc not set");
		problem = TRUE;
	}
	if (!ispunct(VEH_LONG_DESC(veh)[strlen(VEH_LONG_DESC(veh)) - 1])) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Long desc missing punctuation");
		problem = TRUE;
	}
	if (islower(*VEH_LONG_DESC(veh))) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Long desc not capitalized");
		problem = TRUE;
	}
	if (!str_cmp(VEH_SHORT_DESC(veh), default_vehicle_short_desc)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Short desc not set");
		problem = TRUE;
	}
	any_one_arg(VEH_SHORT_DESC(veh), temp);
	if ((fill_word(temp) || reserved_word(temp)) && isupper(*temp)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Short desc capitalized");
		problem = TRUE;
	}
	if (ispunct(VEH_SHORT_DESC(veh)[strlen(VEH_SHORT_DESC(veh)) - 1])) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Short desc has punctuation");
		problem = TRUE;
	}
	
	ptr = VEH_SHORT_DESC(veh);
	do {
		ptr = any_one_arg(ptr, temp);
		// remove trailing punctuation
		while (*temp && ispunct(temp[strlen(temp)-1])) {
			temp[strlen(temp)-1] = '\0';
		}
		if (*temp && !fill_word(temp) && !reserved_word(temp) && !isname(temp, VEH_KEYWORDS(veh))) {
			olc_audit_msg(ch, VEH_VNUM(veh), "Suggested missing keyword '%s'", temp);
			problem = TRUE;
		}
	} while (*ptr);
	
	if (!VEH_LOOK_DESC(veh) || !*VEH_LOOK_DESC(veh) || !str_cmp(VEH_LOOK_DESC(veh), "Nothing.\r\n")) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Look desc not set");
		problem = TRUE;
	}
	else if (!strn_cmp(VEH_LOOK_DESC(veh), "Nothing.", 8)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Look desc starting with 'Nothing.'");
		problem = TRUE;
	}
	
	if (VEH_ICON(veh) && !validate_icon(VEH_ICON(veh))) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Bad icon set");
		problem = TRUE;
	}
	
	if (VEH_MAX_HEALTH(veh) < 1) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Hitpoints set lower than 1");
		problem = TRUE;
	}
	
	if (VEH_INTERIOR_ROOM_VNUM(veh) != NOTHING && (!interior || !IS_SET(GET_BLD_FLAGS(interior), BLD_ROOM))) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Interior room set to invalid building vnum");
		problem = TRUE;
	}
	if (VEH_MAX_ROOMS(veh) > 0 && !interior) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Extra rooms set but vehicle has no interior");
		problem = TRUE;
	}
	if (VEH_DESIGNATE_FLAGS(veh) && !interior) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Designate flags set but vehicle has no interior");
		problem = TRUE;
	}
	if (!VEH_YEARLY_MAINTENANCE(veh)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Requires no maintenance");
		problem = TRUE;
	}
	
	// flags
	if (VEH_FLAGGED(veh, VEH_INCOMPLETE)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "INCOMPLETE flag");
		problem = TRUE;
	}
	if (VEH_FLAGGED(veh, VEH_DISMANTLING)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "DISMANTLING flag");
		problem = TRUE;
	}
	if (VEH_FLAGGED(veh, VEH_CONTAINER | VEH_SHIPPING) && VEH_CAPACITY(veh) < 1) {
		olc_audit_msg(ch, VEH_VNUM(veh), "No capacity set when container or shipping flag present");
		problem = TRUE;
	}
	if (!VEH_FLAGGED(veh, VEH_CONTAINER | VEH_SHIPPING) && VEH_CAPACITY(veh) > 0) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Capacity set without container or shipping flag");
		problem = TRUE;
	}
	if (VEH_FLAGGED(veh, VEH_ALLOW_ROUGH) && !VEH_FLAGGED(veh, VEH_DRIVING | VEH_DRAGGABLE)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "ALLOW-ROUGH set without driving/draggable");
		problem = TRUE;
	}
	if (VEH_FLAGGED(veh, VEH_IN) && !VEH_FLAGGED(veh, VEH_SIT) && VEH_INTERIOR_ROOM_VNUM(veh) == NOTHING) {
		olc_audit_msg(ch, VEH_VNUM(veh), "IN flag set without SIT or interior room");
		problem = TRUE;
	}
	if (VEH_FLAGGED(veh, VEH_CARRY_VEHICLES | VEH_CARRY_MOBS) && VEH_INTERIOR_ROOM_VNUM(veh) == NOTHING) {
		olc_audit_msg(ch, VEH_VNUM(veh), "CARRY-* flag present without interior room");
		problem = TRUE;
	}
	if (VEH_FLAGGED(veh, VEH_ON_FIRE)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "ON-FIRE flag");
		problem = TRUE;
	}
	if (VEH_FLAGGED(veh, VEH_BUILDING) && VEH_FLAGGED(veh, MOVABLE_VEH_FLAGS)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Has both BUILDING flag and at least 1 movement flag");
		problem = TRUE;
	}
	if (VEH_FLAGGED(veh, VEH_BUILDING) && !VEH_FLAGGED(veh, VEH_VISIBLE_IN_DARK)) {
		olc_audit_msg(ch, VEH_VNUM(veh), "Has BUILDING flag but not VISIBLE-IN-DARK");
		problem = TRUE;
	}
	
	problem |= audit_extra_descs(VEH_VNUM(veh), VEH_EX_DESCS(veh), ch);
	problem |= audit_interactions(VEH_VNUM(veh), VEH_INTERACTIONS(veh), TYPE_ROOM, ch);
	problem |= audit_spawns(VEH_VNUM(veh), VEH_SPAWNS(veh), ch);
	
	return problem;
}


/**
* This is called when a vehicle no longer needs resources. It checks flags and
* health, and it will also remove the INCOMPLETE flag if present. It can safely
* be called on vehicles that are done being repaired, too.
*
* WARNING: Calling this on a vehicle that is being dismantled will result in
* it passing to finish_dismantle_vehicle(), which can purge the vehicle.
*
* @param vehicle_data *veh The vehicle.
*/
void complete_vehicle(vehicle_data *veh) {
	if (VEH_IS_DISMANTLING(veh)) {
		// short-circuit out to dismantling
		finish_dismantle_vehicle(NULL, veh);
		return;
	}
	
	// restore
	VEH_HEALTH(veh) = VEH_MAX_HEALTH(veh);
	
	// remove resources
	free_resource_list(VEH_NEEDS_RESOURCES(veh));
	VEH_NEEDS_RESOURCES(veh) = NULL;
	
	// cancel construction id
	VEH_CONSTRUCTION_ID(veh) = NOTHING;
	
	// only if it was incomplete:
	if (VEH_FLAGGED(veh, VEH_INCOMPLETE)) {
		REMOVE_BIT(VEH_FLAGS(veh), VEH_INCOMPLETE);
		
		if (VEH_OWNER(veh)) {
			qt_empire_players(VEH_OWNER(veh), qt_gain_vehicle, VEH_VNUM(veh));
			et_gain_vehicle(VEH_OWNER(veh), VEH_VNUM(veh));
			adjust_vehicle_tech(veh, TRUE);
		}
		
		finish_vehicle_setup(veh);
		
		// build the interior
		get_vehicle_interior(veh);
		
		// run triggers
		load_vtrigger(veh);
	}
	
	affect_total_room(IN_ROOM(veh));
}


/**
* Saves the vehicles list for a room to the room file. This is called
* recursively to save in reverse-order and thus load in correct order.
*
* @param vehicle_data *veh The vehicle to save.
* @param FILE *fl The file open for writing.
*/
void Crash_save_vehicles(vehicle_data *veh, FILE *fl) {
	void store_one_vehicle_to_file(vehicle_data *veh, FILE *fl);
	
	// store next first so the order is right on reboot
	if (veh->next_in_room) {
		Crash_save_vehicles(veh->next_in_room, fl);
	}
	store_one_vehicle_to_file(veh, fl);
}


/**
* Quick way to turn a vnum into a name, safely.
*
* @param any_vnum vnum The vehicle vnum to look up.
* @return char* A name for the vnum, or "UNKNOWN".
*/
char *get_vehicle_name_by_proto(obj_vnum vnum) {
	vehicle_data *proto = vehicle_proto(vnum);
	return proto ? VEH_SHORT_DESC(proto) : "UNKNOWN";
}


/**
* Looks for dead vehicle interiors at startup, and deletes them.
*/
void link_and_check_vehicles(void) {
	vehicle_data *veh, *next_veh;
	room_data *room, *next_room;
	bool found = FALSE;
	
	// reverse-link the home-room of vehicles to this one
	DL_FOREACH_SAFE(vehicle_list, veh, next_veh) {
		if (VEH_INTERIOR_HOME_ROOM(veh)) {
			COMPLEX_DATA(VEH_INTERIOR_HOME_ROOM(veh))->vehicle = veh;
		}
	}
	
	LL_FOREACH_SAFE2(interior_room_list, room, next_room, next_interior) {
		// check for orphaned ship rooms
		if (ROOM_AFF_FLAGGED(room, ROOM_AFF_IN_VEHICLE) && HOME_ROOM(room) == room && !GET_ROOM_VEHICLE(room)) {
			delete_room(room, FALSE);	// must check_all_exits later
			found = TRUE;
		}
		// otherwise add the room to the vehicle's interior list
		else if (GET_ROOM_VEHICLE(room)) {
			add_room_to_vehicle(room, GET_ROOM_VEHICLE(room));
		}
	}
	
	// only bother this if we deleted anything
	if (found) {
		check_all_exits();
	}
	
	// need to update territory counts
	read_empire_territory(NULL, FALSE);
}


/**
* For the .list command.
*
* @param vehicle_data *veh The thing to list.
* @param bool detail If TRUE, provide additional details
* @return char* The line to show (without a CRLF).
*/
char *list_one_vehicle(vehicle_data *veh, bool detail) {
	static char output[MAX_STRING_LENGTH];
	// char part[MAX_STRING_LENGTH], applies[MAX_STRING_LENGTH];
	
	if (detail) {
		snprintf(output, sizeof(output), "[%5d] %s", VEH_VNUM(veh), VEH_SHORT_DESC(veh));
	}
	else {
		snprintf(output, sizeof(output), "[%5d] %s", VEH_VNUM(veh), VEH_SHORT_DESC(veh));
	}
		
	return output;
}


/**
* Searches for all uses of a vehicle and displays them.
*
* @param char_data *ch The player.
* @param any_vnum vnum The vehicle vnum.
*/
void olc_search_vehicle(char_data *ch, any_vnum vnum) {
	extern bool find_quest_giver_in_list(struct quest_giver *list, int type, any_vnum vnum);
	extern bool find_requirement_in_list(struct req_data *list, int type, any_vnum vnum);
	
	char buf[MAX_STRING_LENGTH];
	vehicle_data *veh = vehicle_proto(vnum);
	craft_data *craft, *next_craft;
	quest_data *quest, *next_quest;
	progress_data *prg, *next_prg;
	room_template *rmt, *next_rmt;
	social_data *soc, *next_soc;
	shop_data *shop, *next_shop;
	struct adventure_spawn *asp;
	int size, found;
	bool any;
	
	if (!veh) {
		msg_to_char(ch, "There is no vehicle %d.\r\n", vnum);
		return;
	}
	
	found = 0;
	size = snprintf(buf, sizeof(buf), "Occurrences of vehicle %d (%s):\r\n", vnum, VEH_SHORT_DESC(veh));
	
	// crafts
	HASH_ITER(hh, craft_table, craft, next_craft) {
		if (CRAFT_IS_VEHICLE(craft) && GET_CRAFT_OBJECT(craft) == vnum) {
			++found;
			size += snprintf(buf + size, sizeof(buf) - size, "CFT [%5d] %s\r\n", GET_CRAFT_VNUM(craft), GET_CRAFT_NAME(craft));
		}
	}
	
	// progress
	HASH_ITER(hh, progress_table, prg, next_prg) {
		if (size >= sizeof(buf)) {
			break;
		}
		// REQ_x: requirement search
		any = find_requirement_in_list(PRG_TASKS(prg), REQ_OWN_VEHICLE, vnum);
		
		if (any) {
			++found;
			size += snprintf(buf + size, sizeof(buf) - size, "PRG [%5d] %s\r\n", PRG_VNUM(prg), PRG_NAME(prg));
		}
	}
	
	// quests
	HASH_ITER(hh, quest_table, quest, next_quest) {
		if (size >= sizeof(buf)) {
			break;
		}
		any = find_requirement_in_list(QUEST_TASKS(quest), REQ_OWN_VEHICLE, vnum);
		any |= find_requirement_in_list(QUEST_PREREQS(quest), REQ_OWN_VEHICLE, vnum);
		any |= find_quest_giver_in_list(QUEST_STARTS_AT(quest), QG_VEHICLE, vnum);
		any |= find_quest_giver_in_list(QUEST_ENDS_AT(quest), QG_VEHICLE, vnum);
		
		if (any) {
			++found;
			size += snprintf(buf + size, sizeof(buf) - size, "QST [%5d] %s\r\n", QUEST_VNUM(quest), QUEST_NAME(quest));
		}
	}
	
	// room templates
	HASH_ITER(hh, room_template_table, rmt, next_rmt) {
		LL_FOREACH(GET_RMT_SPAWNS(rmt), asp) {
			if (asp->type == ADV_SPAWN_VEH && asp->vnum == vnum) {
				++found;
				size += snprintf(buf + size, sizeof(buf) - size, "RMT [%5d] %s\r\n", GET_RMT_VNUM(rmt), GET_RMT_TITLE(rmt));
				break;	// only need 1
			}
		}
	}
	
	// on shops
	HASH_ITER(hh, shop_table, shop, next_shop) {
		if (size >= sizeof(buf)) {
			break;
		}
		// QG_x: shop types
		any = find_quest_giver_in_list(SHOP_LOCATIONS(shop), QG_VEHICLE, vnum);
		
		if (any) {
			++found;
			size += snprintf(buf + size, sizeof(buf) - size, "SHOP [%5d] %s\r\n", SHOP_VNUM(shop), SHOP_NAME(shop));
		}
	}
	
	// socials
	HASH_ITER(hh, social_table, soc, next_soc) {
		if (size >= sizeof(buf)) {
			break;
		}
		any = find_requirement_in_list(SOC_REQUIREMENTS(soc), REQ_OWN_VEHICLE, vnum);
		
		if (any) {
			++found;
			size += snprintf(buf + size, sizeof(buf) - size, "SOC [%5d] %s\r\n", SOC_VNUM(soc), SOC_NAME(soc));
		}
	}
	
	if (found > 0) {
		size += snprintf(buf + size, sizeof(buf) - size, "%d location%s shown\r\n", found, PLURAL(found));
	}
	else {
		size += snprintf(buf + size, sizeof(buf) - size, " none\r\n");
	}
	
	page_string(ch->desc, buf, TRUE);
}


/**
* Periodic action for character dismantling a vehicle.
*
* @param char_data *ch The person doing the dismantling.
*/
void process_dismantle_vehicle(char_data *ch) {
	extern const char *pool_types[];
	
	struct resource_data *res, *find_res, *next_res, *copy;
	char buf[MAX_STRING_LENGTH];
	vehicle_data *veh;
	
	if (!(veh = find_dismantling_vehicle_in_room(IN_ROOM(ch), GET_ACTION_VNUM(ch, 1)))) {
		cancel_action(ch);	// no vehicle
		return;
	}
	else if (!can_use_vehicle(ch, veh, MEMBERS_ONLY)) {
		msg_to_char(ch, "You can't dismantle a %s you don't own.\r\n", VEH_OR_BLD(veh));
		cancel_action(ch);
		return;
	}
	else if (!CAN_SEE_VEHICLE(ch, veh)) {
		msg_to_char(ch, "You can't dismantle a %s you can't see.\r\n", VEH_OR_BLD(veh));
		cancel_action(ch);
		return;
	}
	
	// sometimes zeroes end up in here ... just clear them
	res = NULL;
	LL_FOREACH_SAFE(VEH_NEEDS_RESOURCES(veh), find_res, next_res) {
		// things we can't refund
		if (find_res->amount <= 0 || find_res->type == RES_COMPONENT) {
			LL_DELETE(VEH_NEEDS_RESOURCES(veh), find_res);
			free(find_res);
			continue;
		}
		
		// we actually only want the last valid thing, so save res now (and overwrite it on the next loop)
		res = find_res;
	}
	
	// did we find something to refund?
	if (res) {
		// RES_x: messaging
		switch (res->type) {
			// RES_COMPONENT (stored as obj), RES_ACTION, RES_TOOL (stored as obj), and RES_CURRENCY aren't possible here
			case RES_OBJECT: {
				snprintf(buf, sizeof(buf), "You carefully remove %s from $V.", get_obj_name_by_proto(res->vnum));
				act(buf, FALSE, ch, NULL, veh, TO_CHAR | TO_SPAMMY);
				snprintf(buf, sizeof(buf), "$n removes %s from $V.", get_obj_name_by_proto(res->vnum));
				act(buf, FALSE, ch, NULL, veh, TO_ROOM | TO_SPAMMY);
				break;
			}
			case RES_LIQUID: {
				snprintf(buf, sizeof(buf), "You carefully retrieve %d unit%s of %s from $V.", res->amount, PLURAL(res->amount), get_generic_string_by_vnum(res->vnum, GENERIC_LIQUID, GSTR_LIQUID_NAME));
				act(buf, FALSE, ch, NULL, veh, TO_CHAR | TO_SPAMMY);
				snprintf(buf, sizeof(buf), "$n retrieves some %s from $V.", get_generic_string_by_vnum(res->vnum, GENERIC_LIQUID, GSTR_LIQUID_NAME));
				act(buf, FALSE, ch, NULL, veh, TO_ROOM | TO_SPAMMY);
				break;
			}
			case RES_COINS: {
				snprintf(buf, sizeof(buf), "You retrieve %s from $V.", money_amount(real_empire(res->vnum), res->amount));
				act(buf, FALSE, ch, NULL, veh, TO_CHAR | TO_SPAMMY);
				snprintf(buf, sizeof(buf), "$n retrieves %s from $V.", res->amount == 1 ? "a coin" : "some coins");
				act(buf, FALSE, ch, NULL, veh, TO_ROOM | TO_SPAMMY);
				break;
			}
			case RES_POOL: {
				snprintf(buf, sizeof(buf), "You regain %d %s point%s from $V.", res->amount, pool_types[res->vnum], PLURAL(res->amount));
				act(buf, FALSE, ch, NULL, veh, TO_CHAR | TO_SPAMMY);
				snprintf(buf, sizeof(buf), "$n retrieves %s from $V.", pool_types[res->vnum]);
				act(buf, FALSE, ch, NULL, veh, TO_ROOM | TO_SPAMMY);
				break;
			}
			case RES_CURRENCY: {
				snprintf(buf, sizeof(buf), "You retrieve %d %s from $V.", res->amount, get_generic_string_by_vnum(res->vnum, GENERIC_CURRENCY, WHICH_CURRENCY(res->amount)));
				act(buf, FALSE, ch, NULL, veh, TO_CHAR | TO_SPAMMY);
				snprintf(buf, sizeof(buf), "$n retrieves %d %s from $V.", res->amount, get_generic_string_by_vnum(res->vnum, GENERIC_CURRENCY, WHICH_CURRENCY(res->amount)));
				act(buf, FALSE, ch, NULL, veh, TO_ROOM | TO_SPAMMY);
				break;
			}
		}
		
		// make a copy to pass to give_resources
		CREATE(copy, struct resource_data, 1);
		*copy = *res;
		copy->next = NULL;
		
		if (copy->type == RES_OBJECT) {
			// for items, refund 1 at a time
			copy->amount = MIN(1, copy->amount);
			res->amount -= 1;
		}
		else {
			// all other types refund the whole thing
			res->amount = 0;
		}
		
		// give the thing(s)
		give_resources(ch, copy, FALSE);
		free(copy);
		
		if (res->amount <= 0) {
			LL_DELETE(VEH_NEEDS_RESOURCES(veh), res);
			free(res);
		}
	}
	
	// done?
	if (!VEH_NEEDS_RESOURCES(veh)) {
		finish_dismantle_vehicle(ch, veh);
	}
}


/**
* Create a new vehicle from a prototype. You should almost always call this
* with with_triggers = TRUE.
*
* @param any_vnum vnum The vehicle vnum to load.
* @param bool with_triggers If TRUE, attaches all triggers.
* @return vehicle_data* The instantiated vehicle.
*/
vehicle_data *read_vehicle(any_vnum vnum, bool with_triggers) {
	vehicle_data *veh, *proto;
	
	if (!(proto = vehicle_proto(vnum))) {
		log("Vehicle vnum %d does not exist in database.", vnum);
		// grab first one (bug)
		proto = vehicle_table;
	}

	CREATE(veh, vehicle_data, 1);
	clear_vehicle(veh);
	
	// fix memory leak because attributes was allocated by clear_vehicle then overwritten by the next line
	if (veh->attributes) {
		free(veh->attributes);
	}

	*veh = *proto;
	DL_PREPEND(vehicle_list, veh);
	
	// new vehicle setup
	VEH_OWNER(veh) = NULL;
	VEH_SCALE_LEVEL(veh) = 0;	// unscaled
	VEH_HEALTH(veh) = VEH_MAX_HEALTH(veh);
	VEH_CONTAINS(veh) = NULL;
	VEH_CARRYING_N(veh) = 0;
	VEH_ANIMALS(veh) = NULL;
	VEH_NEEDS_RESOURCES(veh) = NULL;
	VEH_BUILT_WITH(veh) = NULL;
	IN_ROOM(veh) = NULL;
	REMOVE_BIT(VEH_FLAGS(veh), VEH_INCOMPLETE | VEH_DISMANTLING);	// ensure not marked incomplete/dismantle
	
	veh->script_id = 0;	// initialize later
	
	if (with_triggers) {
		veh->proto_script = copy_trig_protos(proto->proto_script);
		assign_triggers(veh, VEH_TRIGGER);
	}
	else {
		veh->proto_script = NULL;
	}
	
	return veh;
}


/**
* Removes a room from a vehicle's tracking list, if it's present.
*
* @param room_data *room The room.
* @param vehicle_data *veh The vehicle to remove it from.
*/
void remove_room_from_vehicle(room_data *room, vehicle_data *veh) {
	struct vehicle_room_list *vrl, *next_vrl;
	
	LL_FOREACH_SAFE(VEH_ROOM_LIST(veh), vrl, next_vrl) {
		if (vrl->room == room) {
			LL_DELETE(VEH_ROOM_LIST(veh), vrl);
			free(vrl);
		}
	}
}


// Simple vnum sorter for the vehicle hash
int sort_vehicles(vehicle_data *a, vehicle_data *b) {
	return VEH_VNUM(a) - VEH_VNUM(b);
}


/**
* store_one_vehicle_to_file: Write a vehicle to a tagged save file. Vehicle
* tags start with %VNUM instead of #VNUM because they may co-exist with items
* in the file.
*
* @param vehicle_data *veh The vehicle to save.
* @param FILE *fl The file to save to (open for writing).
*/
void store_one_vehicle_to_file(vehicle_data *veh, FILE *fl) {
	void Crash_save(obj_data *obj, FILE *fp, int location);
	
	struct room_extra_data *red, *next_red;
	struct vehicle_attached_mob *vam;
	char temp[MAX_STRING_LENGTH];
	struct resource_data *res;
	struct trig_var_data *tvd;
	vehicle_data *proto;
	
	if (!fl || !veh) {
		log("SYSERR: store_one_vehicle_to_file called without %s", fl ? "vehicle" : "file");
		return;
	}
	
	proto = vehicle_proto(VEH_VNUM(veh));
	
	fprintf(fl, "%%%d\n", VEH_VNUM(veh));
	fprintf(fl, "Flags: %s\n", bitv_to_alpha(VEH_FLAGS(veh)));

	if (!proto || VEH_KEYWORDS(veh) != VEH_KEYWORDS(proto)) {
		fprintf(fl, "Keywords:\n%s~\n", NULLSAFE(VEH_KEYWORDS(veh)));
	}
	if (!proto || VEH_SHORT_DESC(veh) != VEH_SHORT_DESC(proto)) {
		fprintf(fl, "Short-desc:\n%s~\n", NULLSAFE(VEH_SHORT_DESC(veh)));
	}
	if (!proto || VEH_LONG_DESC(veh) != VEH_LONG_DESC(proto)) {
		fprintf(fl, "Long-desc:\n%s~\n", NULLSAFE(VEH_LONG_DESC(veh)));
	}
	if (!proto || VEH_LOOK_DESC(veh) != VEH_LOOK_DESC(proto)) {
		strcpy(temp, NULLSAFE(VEH_LOOK_DESC(veh)));
		strip_crlf(temp);
		fprintf(fl, "Look-desc:\n%s~\n", temp);
	}
	if (!proto || VEH_ICON(veh) != VEH_ICON(proto)) {
		fprintf(fl, "Icon:\n%s~\n", NULLSAFE(VEH_ICON(veh)));
	}

	if (VEH_OWNER(veh)) {
		fprintf(fl, "Owner: %d\n", EMPIRE_VNUM(VEH_OWNER(veh)));
	}
	if (VEH_SCALE_LEVEL(veh)) {
		fprintf(fl, "Scale: %d\n", VEH_SCALE_LEVEL(veh));
	}
	if (VEH_HEALTH(veh) < VEH_MAX_HEALTH(veh)) {
		fprintf(fl, "Health: %.2f\n", VEH_HEALTH(veh));
	}
	if (VEH_INTERIOR_HOME_ROOM(veh)) {
		fprintf(fl, "Interior-home: %d\n", GET_ROOM_VNUM(VEH_INTERIOR_HOME_ROOM(veh)));
	}
	if (VEH_CONSTRUCTION_ID(veh) != NOTHING) {
		fprintf(fl, "Construction-id: %d\n", VEH_CONSTRUCTION_ID(veh));
	}
	if (VEH_CONTAINS(veh)) {
		fprintf(fl, "Contents:\n");
		Crash_save(VEH_CONTAINS(veh), fl, LOC_INVENTORY);
		fprintf(fl, "Contents-end\n");
	}
	if (VEH_LAST_FIRE_TIME(veh)) {
		fprintf(fl, "Last-fired: %ld\n", VEH_LAST_FIRE_TIME(veh));
	}
	if (VEH_LAST_MOVE_TIME(veh)) {
		fprintf(fl, "Last-moved: %ld\n", VEH_LAST_MOVE_TIME(veh));
	}
	if (VEH_SHIPPING_ID(veh) >= 0) {
		fprintf(fl, "Shipping-id: %d\n", VEH_SHIPPING_ID(veh));
	}
	LL_FOREACH(VEH_ANIMALS(veh), vam) {
		fprintf(fl, "Animal: %d %d %s %d\n", vam->mob, vam->scale_level, bitv_to_alpha(vam->flags), vam->empire);
	}
	LL_FOREACH(VEH_NEEDS_RESOURCES(veh), res) {
		// arg order is backwards-compatible to pre-2.0b3.17
		fprintf(fl, "Needs-res: %d %d %d %d\n", res->vnum, res->amount, res->type, res->misc);
	}
	if (!VEH_FLAGGED(veh, VEH_NEVER_DISMANTLE)) {
		// only save built-with if it's dismantlable
		LL_FOREACH(VEH_BUILT_WITH(veh), res) {
			fprintf(fl, "Built-with: %d %d %d %d\n", res->vnum, res->amount, res->type, res->misc);
		}
	}
	HASH_ITER(hh, VEH_EXTRA_DATA(veh), red, next_red) {
		fprintf(fl, "Extra-data: %d %d\n", red->type, red->value);
	}
	
	// scripts
	if (SCRIPT(veh)) {
		trig_data *trig;
		
		for (trig = TRIGGERS(SCRIPT(veh)); trig; trig = trig->next) {
			fprintf(fl, "Trigger: %d\n", GET_TRIG_VNUM(trig));
		}
		
		LL_FOREACH (SCRIPT(veh)->global_vars, tvd) {
			if (*tvd->name == '-' || !*tvd->value) { // don't save if it begins with - or is empty
				continue;
			}
			
			fprintf(fl, "Variable: %s %ld\n%s\n", tvd->name, tvd->context, tvd->value);
		}
	}
	
	fprintf(fl, "Vehicle-end\n");
}


/**
* Reads a vehicle from a tagged data file.
*
* @param FILE *fl The file open for reading, just after the %VNUM line.
* @param any_vnum vnum The vnum already read from the file.
* @return vehicle_data* The loaded vehicle, if possible.
*/
vehicle_data *unstore_vehicle_from_file(FILE *fl, any_vnum vnum) {
	extern obj_data *Obj_load_from_file(FILE *fl, obj_vnum vnum, int *location, char_data *notify);

	char line[MAX_INPUT_LENGTH], error[MAX_STRING_LENGTH], s_in[MAX_INPUT_LENGTH];
	obj_data *load_obj, *obj, *next_obj, *cont_row[MAX_BAG_ROWS];
	struct vehicle_attached_mob *vam, *last_vam = NULL;
	int length, iter, i_in[4], location = 0, timer;
	struct resource_data *res, *last_res = NULL;
	vehicle_data *proto = vehicle_proto(vnum);
	bool end = FALSE, seek_end = FALSE;
	any_vnum load_vnum;
	vehicle_data *veh;
	long long_in[2];
	double dbl_in;
	
	// load based on vnum or, if NOTHING, create anonymous object
	if (proto) {
		veh = read_vehicle(vnum, FALSE);
	}
	else {
		veh = NULL;
		seek_end = TRUE;	// signal it to skip the whole vehicle
	}
		
	// for fread_string
	sprintf(error, "unstore_vehicle_from_file %d", vnum);
	
	// for more readable if/else chain	
	#define OBJ_FILE_TAG(src, tag, len)  (!strn_cmp((src), (tag), ((len) = strlen(tag))))

	while (!end) {
		if (!get_line(fl, line)) {
			log("SYSERR: Unexpected end of pack file in unstore_vehicle_from_file");
			exit(1);
		}
		
		if (OBJ_FILE_TAG(line, "Vehicle-end", length)) {
			end = TRUE;
			continue;
		}
		else if (seek_end) {
			// are we looking for the end of the vehicle? ignore this line
			// WARNING: don't put any ifs that require "veh" above seek_end; obj is not guaranteed
			continue;
		}
		
		// normal tags by letter
		switch (UPPER(*line)) {
			case 'A': {
				if (OBJ_FILE_TAG(line, "Animal:", length)) {
					if (sscanf(line + length + 1, "%d %d %s %d", &i_in[0], &i_in[1], s_in, &i_in[2]) == 4) {
						CREATE(vam, struct vehicle_attached_mob, 1);
						vam->mob = i_in[0];
						vam->scale_level = i_in[1];
						vam->flags = asciiflag_conv(s_in);
						vam->empire = i_in[2];
						
						// append
						if (last_vam) {
							last_vam->next = vam;
						}
						else {
							VEH_ANIMALS(veh) = vam;
						}
						last_vam = vam;
					}
				}
				break;
			}
			case 'B': {
				if (OBJ_FILE_TAG(line, "Built-with:", length)) {
					if (sscanf(line + length + 1, "%d %d %d %d", &i_in[0], &i_in[1], &i_in[2], &i_in[3]) != 4) {
						// unknown number of args?
						break;
					}
					
					CREATE(res, struct resource_data, 1);
					res->vnum = i_in[0];
					res->amount = i_in[1];
					res->type = i_in[2];
					res->misc = i_in[3];
					LL_APPEND(VEH_BUILT_WITH(veh), res);
				}
				break;
			}
			case 'C': {
				if (OBJ_FILE_TAG(line, "Construction-id:", length)) {
					if (sscanf(line + length + 1, "%d", &i_in[0])) {
						VEH_CONSTRUCTION_ID(veh) = i_in[0];
					}
				}
				else if (OBJ_FILE_TAG(line, "Contents:", length)) {
					// empty container lists
					for (iter = 0; iter < MAX_BAG_ROWS; iter++) {
						cont_row[iter] = NULL;
					}

					// load contents until we find an end
					for (;;) {
						if (!get_line(fl, line)) {
							log("SYSERR: Format error in pack file with vehicle %d", vnum);
							extract_vehicle(veh);
							return NULL;
						}
						
						if (*line == '#') {
							if (sscanf(line, "#%d", &load_vnum) < 1) {
								log("SYSERR: Format error in vnum line of pack file with vehicle %d", vnum);
								extract_vehicle(veh);
								return NULL;
							}
							if ((load_obj = Obj_load_from_file(fl, load_vnum, &location, NULL))) {
								// Obj_load_from_file may return a NULL for deleted objs
				
								// Not really an inventory, but same idea.
								if (location > 0) {
									location = LOC_INVENTORY;
								}

								// store autostore timer through obj_to_room
								timer = GET_AUTOSTORE_TIMER(load_obj);
								obj_to_vehicle(load_obj, veh);
								GET_AUTOSTORE_TIMER(load_obj) = timer;
				
								for (iter = MAX_BAG_ROWS - 1; iter > -location; --iter) {
									// No container, back to vehicle
									DL_FOREACH_SAFE2(cont_row[iter], obj, next_obj, next_content) {
										DL_DELETE2(cont_row[iter], obj, prev_content, next_content);
										timer = GET_AUTOSTORE_TIMER(obj);
										obj_to_vehicle(obj, veh);
										GET_AUTOSTORE_TIMER(obj) = timer;
									}
								}
								if (iter == -location && cont_row[iter]) {			/* Content list exists. */
									if (GET_OBJ_TYPE(load_obj) == ITEM_CONTAINER || IS_CORPSE(load_obj)) {
										/* Take the item, fill it, and give it back. */
										obj_from_room(load_obj);
										load_obj->contains = NULL;
										DL_FOREACH_SAFE2(cont_row[iter], obj, next_obj, next_content) {
											DL_DELETE2(cont_row[iter], obj, prev_content, next_content);
											obj_to_obj(obj, load_obj);
										}
										timer = GET_AUTOSTORE_TIMER(load_obj);
										obj_to_vehicle(load_obj, veh);			/* Add to vehicle first. */
										GET_AUTOSTORE_TIMER(load_obj) = timer;
									}
									else {				/* Object isn't container, empty content list. */
										DL_FOREACH_SAFE2(cont_row[iter], obj, next_obj, next_content) {
											DL_DELETE2(cont_row[iter], obj, prev_content, next_content);
											timer = GET_AUTOSTORE_TIMER(obj);
											obj_to_vehicle(obj, veh);
											GET_AUTOSTORE_TIMER(obj) = timer;
										}
									}
								}
								if (location < 0 && location >= -MAX_BAG_ROWS) {
									obj_from_room(load_obj);
									DL_APPEND2(cont_row[-location - 1], load_obj, prev_content, next_content);
								}
							}
						}
						else if (!strn_cmp(line, "Contents-end", 12)) {
							// done
							break;
						}
						else {
							log("SYSERR: Format error in pack file for vehicle %d: %s", vnum, line);
							extract_vehicle(veh);
							return NULL;
						}
					}
				}
				break;
			}
			case 'E': {
				if (OBJ_FILE_TAG(line, "Extra-data:", length)) {
					if (sscanf(line + length + 1, "%d %d", &i_in[0], &i_in[1]) == 2) {
						set_extra_data(&VEH_EXTRA_DATA(veh), i_in[0], i_in[1]);
					}
				}
				break;
			}
			case 'F': {
				if (OBJ_FILE_TAG(line, "Flags:", length)) {
					if (sscanf(line + length + 1, "%s", s_in)) {
						if (proto) {	// prefer to keep flags from the proto
							VEH_FLAGS(veh) = VEH_FLAGS(proto) & ~SAVABLE_VEH_FLAGS;
							VEH_FLAGS(veh) |= asciiflag_conv(s_in) & SAVABLE_VEH_FLAGS;
						}
						else {	// no proto
							VEH_FLAGS(veh) = asciiflag_conv(s_in);
						}
					}
				}
				break;
			}
			case 'H': {
				if (OBJ_FILE_TAG(line, "Health:", length)) {
					if (sscanf(line + length + 1, "%lf", &dbl_in)) {
						VEH_HEALTH(veh) = MIN(dbl_in, VEH_MAX_HEALTH(veh));
					}
				}
				break;
			}
			case 'I': {
				if (OBJ_FILE_TAG(line, "Icon:", length)) {
					if (VEH_ICON(veh) && (!proto || VEH_ICON(veh) != VEH_ICON(proto))) {
						free(VEH_ICON(veh));
					}
					VEH_ICON(veh) = fread_string(fl, error);
				}
				else if (OBJ_FILE_TAG(line, "Interior-home:", length)) {
					if (sscanf(line + length + 1, "%d", &i_in[0])) {
						VEH_INTERIOR_HOME_ROOM(veh) = real_room(i_in[0]);
					}
				}
				break;
			}
			case 'K': {
				if (OBJ_FILE_TAG(line, "Keywords:", length)) {
					if (VEH_KEYWORDS(veh) && (!proto || VEH_KEYWORDS(veh) != VEH_KEYWORDS(proto))) {
						free(VEH_KEYWORDS(veh));
					}
					VEH_KEYWORDS(veh) = fread_string(fl, error);
				}
				break;
			}
			case 'L': {
				if (OBJ_FILE_TAG(line, "Last-fired:", length)) {
					if (sscanf(line + length + 1, "%ld", &long_in[0])) {
						VEH_LAST_FIRE_TIME(veh) = long_in[0];
					}
				}
				else if (OBJ_FILE_TAG(line, "Last-moved:", length)) {
					if (sscanf(line + length + 1, "%ld", &long_in[0])) {
						VEH_LAST_MOVE_TIME(veh) = long_in[0];
					}
				}
				else if (OBJ_FILE_TAG(line, "Long-desc:", length)) {
					if (VEH_LONG_DESC(veh) && (!proto || VEH_LONG_DESC(veh) != VEH_LONG_DESC(proto))) {
						free(VEH_LONG_DESC(veh));
					}
					VEH_LONG_DESC(veh) = fread_string(fl, error);
				}
				else if (OBJ_FILE_TAG(line, "Look-desc:", length)) {
					if (VEH_LOOK_DESC(veh) && (!proto || VEH_LOOK_DESC(veh) != VEH_LOOK_DESC(proto))) {
						free(VEH_LOOK_DESC(veh));
					}
					VEH_LOOK_DESC(veh) = fread_string(fl, error);
				}
				break;
			}
			case 'N': {
				if (OBJ_FILE_TAG(line, "Needs-res:", length)) {
					if (sscanf(line + length + 1, "%d %d %d %d", &i_in[0], &i_in[1], &i_in[2], &i_in[3]) == 4) {
						// all args present
					}
					else if (sscanf(line + length + 1, "%d %d", &i_in[0], &i_in[1]) == 2) {
						// backwards-compatible to pre-2.0b3.17
						i_in[2] = RES_OBJECT;
						i_in[3] = 0;
					}
					else {
						// unknown number of args?
						break;
					}
					
					CREATE(res, struct resource_data, 1);
					res->vnum = i_in[0];
					res->amount = i_in[1];
					res->type = i_in[2];
					res->misc = i_in[3];
					
					// append
					if (last_res) {
						last_res->next = res;
					}
					else {
						VEH_NEEDS_RESOURCES(veh) = res;
					}
					last_res = res;
				}
				break;
			}
			case 'O': {
				if (OBJ_FILE_TAG(line, "Owner:", length)) {
					if (sscanf(line + length + 1, "%d", &i_in[0])) {
						VEH_OWNER(veh) = real_empire(i_in[0]);
					}
				}
				break;
			}
			case 'S': {
				if (OBJ_FILE_TAG(line, "Scale:", length)) {
					if (sscanf(line + length + 1, "%d", &i_in[0])) {
						VEH_SCALE_LEVEL(veh) = i_in[0];
					}
				}
				else if (OBJ_FILE_TAG(line, "Shipping-id:", length)) {
					if (sscanf(line + length + 1, "%d", &i_in[0])) {
						VEH_SHIPPING_ID(veh) = i_in[0];
					}
				}
				else if (OBJ_FILE_TAG(line, "Short-desc:", length)) {
					if (VEH_SHORT_DESC(veh) && (!proto || VEH_SHORT_DESC(veh) != VEH_SHORT_DESC(proto))) {
						free(VEH_SHORT_DESC(veh));
					}
					VEH_SHORT_DESC(veh) = fread_string(fl, error);
				}
				break;
			}
			case 'T': {
				if (OBJ_FILE_TAG(line, "Trigger:", length)) {
					if (sscanf(line + length + 1, "%d", &i_in[0]) && real_trigger(i_in[0])) {
						if (!SCRIPT(veh)) {
							create_script_data(veh, VEH_TRIGGER);
						}
						add_trigger(SCRIPT(veh), read_trigger(i_in[0]), -1);
					}
				}
				break;
			}
			case 'V': {
				if (OBJ_FILE_TAG(line, "Variable:", length)) {
					if (sscanf(line + length + 1, "%s %d", s_in, &i_in[0]) != 2 || !get_line(fl, line)) {
						log("SYSERR: Bad variable format in unstore_vehicle_from_file: #%d", VEH_VNUM(veh));
						exit(1);
					}
					if (!SCRIPT(veh)) {
						create_script_data(veh, VEH_TRIGGER);
					}
					add_var(&(SCRIPT(veh)->global_vars), s_in, line, i_in[0]);
				}
			}
		}
	}
	
	return veh;	// if any
}


 //////////////////////////////////////////////////////////////////////////////
//// DATABASE ////////////////////////////////////////////////////////////////

/**
* Puts a vehicle into the hash table.
*
* @param vehicle_data *veh The vehicle data to add to the table.
*/
void add_vehicle_to_table(vehicle_data *veh) {
	vehicle_data *find;
	any_vnum vnum;
	
	if (veh) {
		vnum = VEH_VNUM(veh);
		HASH_FIND_INT(vehicle_table, &vnum, find);
		if (!find) {
			HASH_ADD_INT(vehicle_table, vnum, veh);
			HASH_SORT(vehicle_table, sort_vehicles);
		}
	}
}


/**
* @param any_vnum vnum Any vehicle vnum
* @return vehicle_data* The vehicle, or NULL if it doesn't exist
*/
vehicle_data *vehicle_proto(any_vnum vnum) {
	vehicle_data *veh;
	
	if (vnum < 0 || vnum == NOTHING) {
		return NULL;
	}
	
	HASH_FIND_INT(vehicle_table, &vnum, veh);
	return veh;
}


/**
* Removes a vehicle from the hash table.
*
* @param vehicle_data *veh The vehicle data to remove from the table.
*/
void remove_vehicle_from_table(vehicle_data *veh) {
	HASH_DEL(vehicle_table, veh);
}


/**
* Initializes a new vehicle. This clears all memory for it, so set the vnum
* AFTER.
*
* @param vehicle_data *veh The vehicle to initialize.
*/
void clear_vehicle(vehicle_data *veh) {
	struct vehicle_attribute_data *attr = veh->attributes;
	
	memset((char *) veh, 0, sizeof(vehicle_data));
	
	VEH_VNUM(veh) = NOTHING;
	VEH_SHIPPING_ID(veh) = -1;
	
	veh->attributes = attr;	// stored from earlier
	if (veh->attributes) {
		memset((char *)(veh->attributes), 0, sizeof(struct vehicle_attribute_data));
	}
	else {
		CREATE(veh->attributes, struct vehicle_attribute_data, 1);
	}
	
	// attributes init
	VEH_INTERIOR_ROOM_VNUM(veh) = NOTHING;
	VEH_CONSTRUCTION_ID(veh) = NOTHING;
	
	// Since we've wiped out the attributes above, we need to set the speed to the default of VSPEED_NORMAL (two bonuses).
	VEH_SPEED_BONUSES(veh) = VSPEED_NORMAL;
}


/**
* frees up memory for a vehicle data item.
*
* See also: olc_delete_vehicle
*
* @param vehicle_data *veh The vehicle data to free.
*/
void free_vehicle(vehicle_data *veh) {
	void free_custom_messages(struct custom_message *mes);
	
	vehicle_data *proto = vehicle_proto(VEH_VNUM(veh));
	struct vehicle_attached_mob *vam;
	struct spawn_info *spawn;
	
	// strings
	if (VEH_KEYWORDS(veh) && (!proto || VEH_KEYWORDS(veh) != VEH_KEYWORDS(proto))) {
		free(VEH_KEYWORDS(veh));
	}
	if (VEH_SHORT_DESC(veh) && (!proto || VEH_SHORT_DESC(veh) != VEH_SHORT_DESC(proto))) {
		free(VEH_SHORT_DESC(veh));
	}
	if (VEH_LONG_DESC(veh) && (!proto || VEH_LONG_DESC(veh) != VEH_LONG_DESC(proto))) {
		free(VEH_LONG_DESC(veh));
	}
	if (VEH_LOOK_DESC(veh) && (!proto || VEH_LOOK_DESC(veh) != VEH_LOOK_DESC(proto))) {
		free(VEH_LOOK_DESC(veh));
	}
	if (VEH_ICON(veh) && (!proto || VEH_ICON(veh) != VEH_ICON(proto))) {
		free(VEH_ICON(veh));
	}
	
	// pointers
	if (VEH_NEEDS_RESOURCES(veh)) {
		free_resource_list(VEH_NEEDS_RESOURCES(veh));
	}
	if (VEH_BUILT_WITH(veh)) {
		free_resource_list(VEH_BUILT_WITH(veh));
	}
	while ((vam = VEH_ANIMALS(veh))) {
		VEH_ANIMALS(veh) = vam->next;
		free(vam);
	}
	empty_vehicle(veh);
	
	// free any assigned scripts and vars
	if (SCRIPT(veh)) {
		extract_script(veh, VEH_TRIGGER);
	}
	if (veh->proto_script && (!proto || veh->proto_script != proto->proto_script)) {
		free_proto_scripts(&veh->proto_script);
	}
	
	// attributes
	if (veh->attributes && (!proto || veh->attributes != proto->attributes)) {
		if (VEH_YEARLY_MAINTENANCE(veh)) {
			free_resource_list(VEH_YEARLY_MAINTENANCE(veh));
		}
		if (VEH_EX_DESCS(veh)) {
			free_extra_descs(&VEH_EX_DESCS(veh));
		}
		if (VEH_INTERACTIONS(veh) && (!proto || VEH_INTERACTIONS(veh) != VEH_INTERACTIONS(proto))) {
			free_interactions(&VEH_INTERACTIONS(veh));
		}
		if (VEH_SPAWNS(veh)) {
			while ((spawn = VEH_SPAWNS(veh))) {
				VEH_SPAWNS(veh) = spawn->next;
				free(spawn);
			}
		}
		if (VEH_CUSTOM_MSGS(veh) && (!proto || VEH_CUSTOM_MSGS(veh) != VEH_CUSTOM_MSGS(proto))) {
			free_custom_messages(VEH_CUSTOM_MSGS(veh));
		}
		
		free(veh->attributes);
	}
	
	free(veh);
}


/**
* Increments the last construction id by 1 and returns it, for use on a vehicle
* that's being constructed or dismantled. If it hits max-int, it wraps back to
* zero. The chances of a conflict doing this are VERY low since these are
* usually short-lived ids and it would take billions of vehicles to create a
* collision.
*
* @return int A new construction id.
*/
int get_new_vehicle_construction_id(void) {
	int id = data_get_int(DATA_LAST_CONSTRUCTION_ID);
	
	if (id < INT_MAX) {
		++id;
	}
	else {
		id = 0;
	}
	data_set_int(DATA_LAST_CONSTRUCTION_ID, id);
	return id;
}


/**
* Read one vehicle from file.
*
* @param FILE *fl The open .veh file
* @param any_vnum vnum The vehicle vnum
*/
void parse_vehicle(FILE *fl, any_vnum vnum) {
	void parse_custom_message(FILE *fl, struct custom_message **list, char *error);
	void parse_extra_desc(FILE *fl, struct extra_descr_data **list, char *error_part);
	void parse_interaction(char *line, struct interaction_item **list, char *error_part);
	void parse_resource(FILE *fl, struct resource_data **list, char *error_str);

	char line[256], error[256], str_in[256], str_in2[256], str_in3[256];
	struct spawn_info *spawn;
	vehicle_data *veh, *find;
	double dbl_in;
	int int_in[7];
	
	CREATE(veh, vehicle_data, 1);
	clear_vehicle(veh);
	VEH_VNUM(veh) = vnum;
	
	HASH_FIND_INT(vehicle_table, &vnum, find);
	if (find) {
		log("WARNING: Duplicate vehicle vnum #%d", vnum);
		// but have to load it anyway to advance the file
	}
	add_vehicle_to_table(veh);
		
	// for error messages
	sprintf(error, "vehicle vnum %d", vnum);
	
	// lines 1-5: strings
	VEH_KEYWORDS(veh) = fread_string(fl, error);
	VEH_SHORT_DESC(veh) = fread_string(fl, error);
	VEH_LONG_DESC(veh) = fread_string(fl, error);
	VEH_LOOK_DESC(veh) = fread_string(fl, error);
	VEH_ICON(veh) = fread_string(fl, error);
	
	// line 6: flags move_type maxhealth capacity animals_required functions fame military size
	if (!get_line(fl, line)) {
		log("SYSERR: Missing line 6 of %s", error);
		exit(1);
	}
	if (sscanf(line, "%s %d %d %d %d %s %d %d %d %s", str_in, &int_in[0], &int_in[1], &int_in[2], &int_in[3], str_in2, &int_in[4], &int_in[5], &int_in[6], str_in3) != 10) {
		int_in[6] = 0;	// b5.104 backwards-compatible: size
		strcpy(str_in3, "0");	// affects
		
		if (sscanf(line, "%s %d %d %d %d %s %d %d", str_in, &int_in[0], &int_in[1], &int_in[2], &int_in[3], str_in2, &int_in[4], &int_in[5]) != 8) {
			strcpy(str_in2, "0");	// backwards-compatible: functions
			int_in[4] = 0;	// fame
			int_in[5] = 0;	// military
		
			if (sscanf(line, "%s %d %d %d %d", str_in, &int_in[0], &int_in[1], &int_in[2], &int_in[3]) != 5) {
				log("SYSERR: Format error in line 6 of %s", error);
				exit(1);
			}
		}
	}
	
	VEH_FLAGS(veh) = asciiflag_conv(str_in);
	VEH_MOVE_TYPE(veh) = int_in[0];
	VEH_HEALTH(veh) = VEH_MAX_HEALTH(veh) = int_in[1];
	VEH_CAPACITY(veh) = int_in[2];
	VEH_ANIMALS_REQUIRED(veh) = int_in[3];
	VEH_FUNCTIONS(veh) = asciiflag_conv(str_in2);
	VEH_FAME(veh) = int_in[4];
	VEH_MILITARY(veh) = int_in[5];
	VEH_SIZE(veh) = int_in[6];
	VEH_ROOM_AFFECTS(veh) = asciiflag_conv(str_in3);
	
	// optionals
	for (;;) {
		if (!get_line(fl, line)) {
			log("SYSERR: Format error in %s, expecting alphabetic flags", error);
			exit(1);
		}
		switch (*line) {
			case 'C': {	// scalable
				if (!get_line(fl, line) || sscanf(line, "%d %d", &int_in[0], &int_in[1]) != 2) {
					log("SYSERR: Format error in C line of %s", error);
					exit(1);
				}
				
				VEH_MIN_SCALE_LEVEL(veh) = int_in[0];
				VEH_MAX_SCALE_LEVEL(veh) = int_in[1];
				break;
			}
			case 'D': {	// designate/rooms
				if (!get_line(fl, line) || sscanf(line, "%d %d %s", &int_in[0], &int_in[1], str_in) != 3) {
					log("SYSERR: Format error in D line of %s", error);
					exit(1);
				}
				
				VEH_INTERIOR_ROOM_VNUM(veh) = int_in[0];
				VEH_MAX_ROOMS(veh) = int_in[1];
				VEH_DESIGNATE_FLAGS(veh) = asciiflag_conv(str_in);
				break;
			}
			case 'E': {	// extra descs
				parse_extra_desc(fl, &VEH_EX_DESCS(veh), error);
				break;
			}
			case 'I': {	// interaction item
				parse_interaction(line, &VEH_INTERACTIONS(veh), error);
				break;
			}
			case 'K': {	// custom messages
				parse_custom_message(fl, &VEH_CUSTOM_MSGS(veh), error);
				break;
			}
			case 'L': {	// climate require/forbids
				if (sscanf(line, "L %s %s", str_in, str_in2) != 2) {
					log("SYSERR: Format error in L line of %s", error);
					exit(1);
				}
				
				VEH_REQUIRES_CLIMATE(veh) = asciiflag_conv(str_in);
				VEH_FORBID_CLIMATE(veh) = asciiflag_conv(str_in2);
				break;
			}
			case 'M': {	// mob spawn
				if (!get_line(fl, line) || sscanf(line, "%d %lf %s", &int_in[0], &dbl_in, str_in) != 3) {
					log("SYSERR: Format error in M line of %s", error);
					exit(1);
				}
				
				CREATE(spawn, struct spawn_info, 1);
				spawn->vnum = int_in[0];
				spawn->percent = dbl_in;
				spawn->flags = asciiflag_conv(str_in);
				
				LL_APPEND(VEH_SPAWNS(veh), spawn);
				break;
			}
			
			case 'P': { // speed bonuses (default is VSPEED_NORMAL, set above in clear_vehicle(veh)
				if (!get_line(fl, line) || sscanf(line, "%d", &int_in[0]) != 1) {
					log("SYSERR: Format error in P line of %s", error);
					exit(1);
				}
				
				VEH_SPEED_BONUSES(veh) = int_in[0];
				break;
			}
			
			case 'R': {	// resources/yearly maintenance
				parse_resource(fl, &VEH_YEARLY_MAINTENANCE(veh), error);
				break;
			}
			
			case 'T': {	// trigger
				parse_trig_proto(line, &(veh->proto_script), error);
				break;
			}
			
			// end
			case 'S': {
				return;
			}
			
			default: {
				log("SYSERR: Format error in %s, expecting alphabetic flags", error);
				exit(1);
			}
		}
	}
}


// writes entries in the vehicle index
void write_vehicle_index(FILE *fl) {
	vehicle_data *veh, *next_veh;
	int this, last;
	
	last = -1;
	HASH_ITER(hh, vehicle_table, veh, next_veh) {
		// determine "zone number" by vnum
		this = (int)(VEH_VNUM(veh) / 100);
	
		if (this != last) {
			fprintf(fl, "%d%s\n", this, VEH_SUFFIX);
			last = this;
		}
	}
}


/**
* Outputs one vehicle item in the db file format, starting with a #VNUM and
* ending with an S.
*
* @param FILE *fl The file to write it to.
* @param vehicle_data *veh The thing to save.
*/
void write_vehicle_to_file(FILE *fl, vehicle_data *veh) {
	void write_custom_messages_to_file(FILE *fl, char letter, struct custom_message *list);
	void write_extra_descs_to_file(FILE *fl, struct extra_descr_data *list);
	void write_interactions_to_file(FILE *fl, struct interaction_item *list);
	void write_resources_to_file(FILE *fl, char letter, struct resource_data *list);
	void write_trig_protos_to_file(FILE *fl, char letter, struct trig_proto_list *list);
	
	char temp[MAX_STRING_LENGTH], temp2[MAX_STRING_LENGTH], temp3[MAX_STRING_LENGTH];
	struct spawn_info *spawn;
	
	if (!fl || !veh) {
		syslog(SYS_ERROR, LVL_START_IMM, TRUE, "SYSERR: write_vehicle_to_file called without %s", !fl ? "file" : "vehicle");
		return;
	}
	
	fprintf(fl, "#%d\n", VEH_VNUM(veh));
	
	// 1-5. strings
	fprintf(fl, "%s~\n", NULLSAFE(VEH_KEYWORDS(veh)));
	fprintf(fl, "%s~\n", NULLSAFE(VEH_SHORT_DESC(veh)));
	fprintf(fl, "%s~\n", NULLSAFE(VEH_LONG_DESC(veh)));
	strcpy(temp, NULLSAFE(VEH_LOOK_DESC(veh)));
	strip_crlf(temp);
	fprintf(fl, "%s~\n", temp);
	fprintf(fl, "%s~\n", NULLSAFE(VEH_ICON(veh)));
	
	// 6. flags move_type maxhealth capacity animals_required functions fame military size room-affects
	strcpy(temp, bitv_to_alpha(VEH_FLAGS(veh)));
	strcpy(temp2, bitv_to_alpha(VEH_FUNCTIONS(veh)));
	strcpy(temp3, bitv_to_alpha(VEH_ROOM_AFFECTS(veh)));
	fprintf(fl, "%s %d %d %d %d %s %d %d %d %s\n", temp, VEH_MOVE_TYPE(veh), VEH_MAX_HEALTH(veh), VEH_CAPACITY(veh), VEH_ANIMALS_REQUIRED(veh), temp2, VEH_FAME(veh), VEH_MILITARY(veh), VEH_SIZE(veh), temp3);
	
	// C: scaling
	if (VEH_MIN_SCALE_LEVEL(veh) > 0 || VEH_MAX_SCALE_LEVEL(veh) > 0) {
		fprintf(fl, "C\n");
		fprintf(fl, "%d %d\n", VEH_MIN_SCALE_LEVEL(veh), VEH_MAX_SCALE_LEVEL(veh));
	}
	
	// 'D': designate/rooms
	if (VEH_INTERIOR_ROOM_VNUM(veh) != NOTHING || VEH_MAX_ROOMS(veh) || VEH_DESIGNATE_FLAGS(veh)) {
		strcpy(temp, bitv_to_alpha(VEH_DESIGNATE_FLAGS(veh)));
		fprintf(fl, "D\n%d %d %s\n", VEH_INTERIOR_ROOM_VNUM(veh), VEH_MAX_ROOMS(veh), temp);
	}
	
	// 'E': extra descs
	write_extra_descs_to_file(fl, VEH_EX_DESCS(veh));
	
	// I: interactions
	write_interactions_to_file(fl, VEH_INTERACTIONS(veh));
	
	// K: custom messages
	write_custom_messages_to_file(fl, 'K', VEH_CUSTOM_MSGS(veh));
	
	// L: climate restrictions
	if (VEH_REQUIRES_CLIMATE(veh) || VEH_FORBID_CLIMATE(veh)) {
		strcpy(temp, bitv_to_alpha(VEH_REQUIRES_CLIMATE(veh)));
		strcpy(temp2, bitv_to_alpha(VEH_FORBID_CLIMATE(veh)));
		fprintf(fl, "L %s %s\n", temp, temp2);
	}
	
	// 'M': mob spawns
	LL_FOREACH(VEH_SPAWNS(veh), spawn) {
		fprintf(fl, "M\n%d %.2f %s\n", spawn->vnum, spawn->percent, bitv_to_alpha(spawn->flags));
	}
	
	// 'P': speed bonuses
	fprintf(fl, "P\n%d\n", VEH_SPEED_BONUSES(veh));
	
	// 'R': resources
	write_resources_to_file(fl, 'R', VEH_YEARLY_MAINTENANCE(veh));
	
	// T, V: triggers
	write_trig_protos_to_file(fl, 'T', veh->proto_script);
	
	// end
	fprintf(fl, "S\n");
}


 //////////////////////////////////////////////////////////////////////////////
//// 2.0b3.8 CONVERTER ///////////////////////////////////////////////////////

// this system converts a set of objects to vehicles, including all the boats,
// catapults, carts, and ships.
	
// list of vnums to convert directly from obj to vehicle
any_vnum convert_list[] = {
	900,	// a rickety cart
	901,	// a carriage
	902,	// a covered wagon
	903,	// the catapult
	904,	// a chair
	905,	// a wooden bench
	906,	// a long table
	907,	// a stool
	917,	// the throne
	920,	// a wooden canoe
	952,	// the pinnace
	953,	// the brigantine
	954,	// the galley
	955,	// the argosy
	956,	// the galleon
	10715,	// the sleigh
	NOTHING	// end list
};

struct convert_vehicle_data {
	char_data *mob;	// mob to attach
	any_vnum vnum;	// vehicle vnum
	struct convert_vehicle_data *next;
};

struct convert_vehicle_data *list_of_vehicles_to_convert = NULL;

/**
* Stores data for a mob that was supposed to be attached to a vehicle.
*/
void add_convert_vehicle_data(char_data *mob, any_vnum vnum) {
	struct convert_vehicle_data *cvd;
	
	CREATE(cvd, struct convert_vehicle_data, 1);
	cvd->mob = mob;
	cvd->vnum = vnum;
	LL_PREPEND(list_of_vehicles_to_convert, cvd);
}


/**
* Processes any temporary data for mobs that should be attached to a vehicle.
* This basically assumes you're in the middle of upgrading to 2.0 b3.8 and
* works on any data it found. Mobs are only removed if they become attached
* to a vehicle.
*
* @return int the number converted
*/
int run_convert_vehicle_list(void) {
	struct convert_vehicle_data *cvd;
	vehicle_data *veh;
	int changed = 0;
	
	while ((cvd = list_of_vehicles_to_convert)) {
		list_of_vehicles_to_convert = cvd->next;
		
		if (cvd->mob && IN_ROOM(cvd->mob)) {
			DL_FOREACH2(ROOM_VEHICLES(IN_ROOM(cvd->mob)), veh, next_in_room) {
				if (VEH_VNUM(veh) == cvd->vnum && count_harnessed_animals(veh) < VEH_ANIMALS_REQUIRED(veh)) {
					harness_mob_to_vehicle(cvd->mob, veh);
					++changed;
					break;
				}
			}
		}
		
		free(cvd);
	}
	
	return changed;
}

/**
* Replaces an object with a vehicle of the same VNUM, and converts the traits
* that it can. This will result in partially-completed ships becoming fully-
* completed.
* 
* @param obj_data *obj The object to convert (will be extracted).
*/
void convert_one_obj_to_vehicle(obj_data *obj) {
	extern room_data *obj_room(obj_data *obj);
	
	obj_data *obj_iter, *next_obj;
	room_data *room, *room_iter, *main_room;
	vehicle_data *veh;
	
	// if there isn't a room or vehicle involved, just remove the object
	if (!(room = obj_room(obj)) || !vehicle_proto(GET_OBJ_VNUM(obj))) {
		extract_obj(obj);
		return;
	}
	
	// create the vehicle
	veh = read_vehicle(GET_OBJ_VNUM(obj), TRUE);
	vehicle_to_room(veh, room);
	
	// move inventory
	DL_FOREACH_SAFE2(obj->contains, obj_iter, next_obj, next_content) {
		obj_to_vehicle(obj_iter, veh);
	}
	
	// convert traits
	VEH_OWNER(veh) = real_empire(obj->last_empire_id);
	VEH_SCALE_LEVEL(veh) = GET_OBJ_CURRENT_SCALE_LEVEL(obj);
	
	// type-based traits
	switch (GET_OBJ_TYPE(obj)) {
		case ITEM_SHIP: {
			if ((main_room = real_room(GET_OBJ_VAL(obj, 2)))) {	// obj val 2: was ship's main-room-vnum
				VEH_INTERIOR_HOME_ROOM(veh) = main_room;
				
				// detect owner from room
				if (ROOM_OWNER(main_room)) {
					VEH_OWNER(veh) = ROOM_OWNER(main_room);
				}
				
				// apply vehicle aff
				LL_FOREACH2(interior_room_list, room_iter, next_interior) {
					if (room_iter == main_room || HOME_ROOM(room_iter) == main_room) {
						SET_BIT(ROOM_BASE_FLAGS(room_iter), ROOM_AFF_IN_VEHICLE);
						affect_total_room(room_iter);
					}
				}
			}
			break;
		}
		case ITEM_CART: {
			// nothing to convert?
			break;
		}
	}
	
	// did we successfully get an owner? try the room it's in
	if (!VEH_OWNER(veh)) {
		VEH_OWNER(veh) = ROOM_OWNER(room);
	}
	
	// remove the object
	extract_obj(obj);
}


/**
* Converts a list of objects into vehicles with the same vnum. This converter
* was used during the initial implementation of vehicles in 2.0 b3.8.
*
* @return int the number converted
*/
int convert_to_vehicles(void) {
	obj_data *obj, *next_obj;
	int iter, changed = 0;
	bool found;
	
	DL_FOREACH_SAFE(object_list, obj, next_obj) {
		// determine if it's in the list to replace
		found = FALSE;
		for (iter = 0; convert_list[iter] != NOTHING && !found; ++iter) {
			if (convert_list[iter] == GET_OBJ_VNUM(obj)) {
				found = TRUE;
			}
		}
		if (!found) {
			continue;
		}
		
		// success
		convert_one_obj_to_vehicle(obj);
		++changed;
	}
	
	return changed;
}


/**
* Removes the old room affect flag that hinted when to show a ship in pre-
* b3.8.
*/
void b3_8_ship_update(void) {
	void save_whole_world();
	
	room_data *room, *next_room;
	int changed = 0;
	
	bitvector_t ROOM_AFF_SHIP_PRESENT = BIT(10);	// old bit to remove
	
	HASH_ITER(hh, world_table, room, next_room) {
		if (IS_SET(ROOM_AFF_FLAGS(room) | ROOM_BASE_FLAGS(room), ROOM_AFF_SHIP_PRESENT)) {
			REMOVE_BIT(ROOM_BASE_FLAGS(room), ROOM_AFF_SHIP_PRESENT);
			affect_total_room(room);
			++changed;
		}
	}
	
	changed += convert_to_vehicles();
	changed += run_convert_vehicle_list();
	
	if (changed > 0) {
		save_whole_world();
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// OLC HANDLERS ////////////////////////////////////////////////////////////

/**
* Creates a new vehicle entry.
* 
* @param any_vnum vnum The number to create.
* @return vehicle_data* The new vehicle's prototype.
*/
vehicle_data *create_vehicle_table_entry(any_vnum vnum) {
	vehicle_data *veh;
	
	// sanity
	if (vehicle_proto(vnum)) {
		log("SYSERR: Attempting to insert vehicle at existing vnum %d", vnum);
		return vehicle_proto(vnum);
	}
	
	CREATE(veh, vehicle_data, 1);
	clear_vehicle(veh);
	VEH_VNUM(veh) = vnum;
	
	VEH_KEYWORDS(veh) = str_dup(default_vehicle_keywords);
	VEH_SHORT_DESC(veh) = str_dup(default_vehicle_short_desc);
	VEH_LONG_DESC(veh) = str_dup(default_vehicle_long_desc);
	add_vehicle_to_table(veh);

	// save index and vehicle file now
	save_index(DB_BOOT_VEH);
	save_library_file_for_vnum(DB_BOOT_VEH, vnum);

	return veh;
}


/**
* WARNING: This function actually deletes a vehicle.
*
* @param char_data *ch The person doing the deleting.
* @param any_vnum vnum The vnum to delete.
*/
void olc_delete_vehicle(char_data *ch, any_vnum vnum) {
	extern bool delete_from_spawn_template_list(struct adventure_spawn **list, int spawn_type, mob_vnum vnum);
	extern bool delete_quest_giver_from_list(struct quest_giver **list, int type, any_vnum vnum);
	extern bool delete_requirement_from_list(struct req_data **list, int type, any_vnum vnum);
	
	vehicle_data *veh, *iter, *next_iter;
	craft_data *craft, *next_craft;
	quest_data *quest, *next_quest;
	progress_data *prg, *next_prg;
	room_template *rmt, *next_rmt;
	social_data *soc, *next_soc;
	shop_data *shop, *next_shop;
	descriptor_data *desc;
	bool found;
	
	if (!(veh = vehicle_proto(vnum))) {
		msg_to_char(ch, "There is no such vehicle %d.\r\n", vnum);
		return;
	}
	
	// remove live vehicles
	DL_FOREACH_SAFE(vehicle_list, iter, next_iter) {
		if (VEH_VNUM(iter) != vnum) {
			continue;
		}
		
		if (ROOM_PEOPLE(IN_ROOM(iter))) {
			act("$V vanishes.", FALSE, ROOM_PEOPLE(IN_ROOM(iter)), NULL, iter, TO_CHAR | TO_ROOM);
		}
		extract_vehicle(iter);
	}
	
	// remove it from the hash table first
	remove_vehicle_from_table(veh);
	
	// save index and vehicle file now
	save_index(DB_BOOT_VEH);
	save_library_file_for_vnum(DB_BOOT_VEH, vnum);
	
	// update crafts
	HASH_ITER(hh, craft_table, craft, next_craft) {
		found = FALSE;
		if (CRAFT_IS_VEHICLE(craft) && GET_CRAFT_OBJECT(craft) == vnum) {
			GET_CRAFT_OBJECT(craft) = NOTHING;
			found = TRUE;
		}
		
		if (found) {
			SET_BIT(GET_CRAFT_FLAGS(craft), CRAFT_IN_DEVELOPMENT);
			save_library_file_for_vnum(DB_BOOT_CRAFT, GET_CRAFT_VNUM(craft));
		}
	}
	
	// update progress
	HASH_ITER(hh, progress_table, prg, next_prg) {
		found = delete_requirement_from_list(&PRG_TASKS(prg), REQ_OWN_VEHICLE, vnum);
		
		if (found) {
			SET_BIT(PRG_FLAGS(prg), PRG_IN_DEVELOPMENT);
			save_library_file_for_vnum(DB_BOOT_PRG, PRG_VNUM(prg));
			need_progress_refresh = TRUE;
		}
	}
	
	// update quests
	HASH_ITER(hh, quest_table, quest, next_quest) {
		found = delete_requirement_from_list(&QUEST_TASKS(quest), REQ_OWN_VEHICLE, vnum);
		found |= delete_requirement_from_list(&QUEST_PREREQS(quest), REQ_OWN_VEHICLE, vnum);
		found |= delete_quest_giver_from_list(&QUEST_STARTS_AT(quest), QG_VEHICLE, vnum);
		found |= delete_quest_giver_from_list(&QUEST_ENDS_AT(quest), QG_VEHICLE, vnum);
		
		if (found) {
			SET_BIT(QUEST_FLAGS(quest), QST_IN_DEVELOPMENT);
			save_library_file_for_vnum(DB_BOOT_QST, QUEST_VNUM(quest));
		}
	}
	
	// update room templates
	HASH_ITER(hh, room_template_table, rmt, next_rmt) {
		found = delete_from_spawn_template_list(&GET_RMT_SPAWNS(rmt), ADV_SPAWN_VEH, vnum);
		if (found) {
			save_library_file_for_vnum(DB_BOOT_RMT, GET_RMT_VNUM(rmt));
		}
	}
	
	// update shops
	HASH_ITER(hh, shop_table, shop, next_shop) {
		// QG_x: quest types
		found = delete_quest_giver_from_list(&SHOP_LOCATIONS(shop), QG_VEHICLE, vnum);
		
		if (found) {
			SET_BIT(SHOP_FLAGS(shop), SHOP_IN_DEVELOPMENT);
			save_library_file_for_vnum(DB_BOOT_SHOP, SHOP_VNUM(shop));
		}
	}
	
	// update socials
	HASH_ITER(hh, social_table, soc, next_soc) {
		found = delete_requirement_from_list(&SOC_REQUIREMENTS(soc), REQ_OWN_VEHICLE, vnum);
		
		if (found) {
			SET_BIT(SOC_FLAGS(soc), SOC_IN_DEVELOPMENT);
			save_library_file_for_vnum(DB_BOOT_SOC, SOC_VNUM(soc));
		}
	}
	
	// olc editor updates
	LL_FOREACH(descriptor_list, desc) {
		if (GET_OLC_CRAFT(desc)) {
			found = FALSE;
			if (CRAFT_IS_VEHICLE(GET_OLC_CRAFT(desc)) && GET_OLC_CRAFT(desc)->object == vnum) {
				GET_OLC_CRAFT(desc)->object = NOTHING;
				found = TRUE;
			}
		
			if (found) {
				SET_BIT(GET_OLC_CRAFT(desc)->flags, CRAFT_IN_DEVELOPMENT);
				msg_to_char(desc->character, "The vehicle made by the craft you're editing was deleted.\r\n");
			}
		}
		if (GET_OLC_PROGRESS(desc)) {
			found = delete_requirement_from_list(&PRG_TASKS(GET_OLC_PROGRESS(desc)), REQ_OWN_VEHICLE, vnum);
		
			if (found) {
				SET_BIT(QUEST_FLAGS(GET_OLC_PROGRESS(desc)), PRG_IN_DEVELOPMENT);
				msg_to_desc(desc, "A vehicle used by the progression goal you're editing has been deleted.\r\n");
			}
		}
		if (GET_OLC_QUEST(desc)) {
			found = delete_requirement_from_list(&QUEST_TASKS(GET_OLC_QUEST(desc)), REQ_OWN_VEHICLE, vnum);
			found |= delete_requirement_from_list(&QUEST_PREREQS(GET_OLC_QUEST(desc)), REQ_OWN_VEHICLE, vnum);
		
			if (found) {
				SET_BIT(QUEST_FLAGS(GET_OLC_QUEST(desc)), QST_IN_DEVELOPMENT);
				msg_to_desc(desc, "A vehicle used by the quest you are editing was deleted.\r\n");
			}
		}
		if (GET_OLC_ROOM_TEMPLATE(desc)) {
			if (delete_from_spawn_template_list(&GET_OLC_ROOM_TEMPLATE(desc)->spawns, ADV_SPAWN_VEH, vnum)) {
				msg_to_char(desc->character, "One of the vehicles that spawns in the room template you're editing was deleted.\r\n");
			}
		}
		if (GET_OLC_SHOP(desc)) {
			// QG_x: quest types
			found = delete_quest_giver_from_list(&SHOP_LOCATIONS(GET_OLC_SHOP(desc)), QG_VEHICLE, vnum);
		
			if (found) {
				SET_BIT(SHOP_FLAGS(GET_OLC_SHOP(desc)), SHOP_IN_DEVELOPMENT);
				msg_to_desc(desc, "A vehicle used by the shop you are editing was deleted.\r\n");
			}
		}
		if (GET_OLC_SOCIAL(desc)) {
			found = delete_requirement_from_list(&SOC_REQUIREMENTS(GET_OLC_SOCIAL(desc)), REQ_OWN_VEHICLE, vnum);
		
			if (found) {
				SET_BIT(SOC_FLAGS(GET_OLC_SOCIAL(desc)), SOC_IN_DEVELOPMENT);
				msg_to_desc(desc, "A vehicle required by the social you are editing was deleted.\r\n");
			}
		}
	}
	
	syslog(SYS_OLC, GET_INVIS_LEV(ch), TRUE, "OLC: %s has deleted vehicle %d", GET_NAME(ch), vnum);
	msg_to_char(ch, "Vehicle %d deleted.\r\n", vnum);
	
	free_vehicle(veh);
}


/**
* Searches properties of vehicles.
*
* @param char_data *ch The person searching.
* @param char *argument The argument they entered.
*/
void olc_fullsearch_vehicle(char_data *ch, char *argument) {
	char buf[MAX_STRING_LENGTH * 2], line[MAX_STRING_LENGTH], type_arg[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH], find_keywords[MAX_INPUT_LENGTH];
	int count;
	
	char only_icon[MAX_INPUT_LENGTH];
	bitvector_t only_designate = NOBITS, only_flags = NOBITS, only_functions = NOBITS, only_affs = NOBITS;
	bitvector_t find_interacts = NOBITS, not_flagged = NOBITS, found_interacts = NOBITS, find_custom = NOBITS, found_custom = NOBITS;
	int only_animals = NOTHING, only_cap = NOTHING, cap_over = NOTHING, cap_under = NOTHING;
	int only_fame = NOTHING, fame_over = NOTHING, fame_under = NOTHING, only_speed = NOTHING;
	int only_hitpoints = NOTHING, hitpoints_over = NOTHING, hitpoints_under = NOTHING, only_level = NOTHING;
	int only_military = NOTHING, military_over = NOTHING, military_under = NOTHING;
	int only_rooms = NOTHING, rooms_over = NOTHING, rooms_under = NOTHING, only_move = NOTHING;
	int size_under = NOTHING, size_over = NOTHING;
	struct custom_message *cust;
	bool needs_animals = FALSE;
	
	struct interaction_item *inter;
	vehicle_data *veh, *next_veh;
	size_t size;
	
	*only_icon = '\0';
	
	if (!*argument) {
		msg_to_char(ch, "See HELP VEDIT FULLSEARCH for syntax.\r\n");
		return;
	}
	
	// process argument
	*find_keywords = '\0';
	while (*argument) {
		// figure out a type
		argument = any_one_arg(argument, type_arg);
		
		if (!strcmp(type_arg, "-")) {
			continue;	// just skip stray dashes
		}
		
		// else-ifs defined in olc.h process these args:
		FULLSEARCH_FLAGS("affects", only_affs, room_aff_bits)
		FULLSEARCH_INT("animalsrequired", only_animals, 0, INT_MAX)
		FULLSEARCH_BOOL("anyanimalsrequired", needs_animals)
		FULLSEARCH_INT("capacity", only_cap, 0, INT_MAX)
		FULLSEARCH_INT("capacityover", cap_over, 0, INT_MAX)
		FULLSEARCH_INT("capacityunder", cap_under, 0, INT_MAX)
		FULLSEARCH_FLAGS("custom", find_custom, veh_custom_types)
		FULLSEARCH_FLAGS("designate", only_designate, designate_flags)
		FULLSEARCH_INT("fame", only_fame, 0, INT_MAX)
		FULLSEARCH_INT("fameover", fame_over, 0, INT_MAX)
		FULLSEARCH_INT("fameunder", fame_under, 0, INT_MAX)
		FULLSEARCH_FLAGS("flagged", only_flags, vehicle_flags)
		FULLSEARCH_FLAGS("flags", only_flags, vehicle_flags)
		FULLSEARCH_FLAGS("unflagged", not_flagged, vehicle_flags)
		FULLSEARCH_FLAGS("functions", only_functions, function_flags)
		FULLSEARCH_STRING("icon", only_icon)
		FULLSEARCH_FLAGS("interaction", find_interacts, interact_types)
		FULLSEARCH_INT("hitpoints", only_hitpoints, 0, INT_MAX)
		FULLSEARCH_INT("hitpointsover", hitpoints_over, 0, INT_MAX)
		FULLSEARCH_INT("hitpointsunder", hitpoints_under, 0, INT_MAX)
		FULLSEARCH_LIST("movetype", only_move, mob_move_types)
		FULLSEARCH_INT("level", only_level, 0, INT_MAX)
		FULLSEARCH_INT("rooms", only_rooms, 0, INT_MAX)
		FULLSEARCH_INT("roomsover", rooms_over, 0, INT_MAX)
		FULLSEARCH_INT("roomsunder", rooms_under, 0, INT_MAX)
		FULLSEARCH_INT("military", only_military, 0, INT_MAX)
		FULLSEARCH_INT("militaryover", military_over, 0, INT_MAX)
		FULLSEARCH_INT("militaryunder", military_under, 0, INT_MAX)
		FULLSEARCH_INT("sizeover", size_over, 0, INT_MAX)
		FULLSEARCH_INT("sizeunder", size_under, 0, INT_MAX)
		FULLSEARCH_LIST("speed", only_speed, vehicle_speed_types)
		
		else {	// not sure what to do with it? treat it like a keyword
			sprintf(find_keywords + strlen(find_keywords), "%s%s", *find_keywords ? " " : "", type_arg);
		}
		
		// prepare for next loop
		skip_spaces(&argument);
	}
	
	size = snprintf(buf, sizeof(buf), "Vehicle fullsearch: %s\r\n", find_keywords);
	count = 0;
	
	// okay now look up items
	HASH_ITER(hh, vehicle_table, veh, next_veh) {
		if (only_affs != NOBITS && (VEH_ROOM_AFFECTS(veh) & only_affs) != only_affs) {
			continue;
		}
		if (only_animals != NOTHING && VEH_ANIMALS_REQUIRED(veh) != only_animals) {
			continue;
		}
		if (needs_animals && VEH_ANIMALS_REQUIRED(veh) == 0) {
			continue;
		}
		if (only_cap != NOTHING && VEH_CAPACITY(veh) != only_cap) {
			continue;
		}
		if (cap_over != NOTHING && VEH_CAPACITY(veh) < cap_over) {
			continue;
		}
		if (cap_under != NOTHING && (VEH_CAPACITY(veh) > cap_under || VEH_CAPACITY(veh) == 0)) {
			continue;
		}
		if (only_designate != NOBITS && (VEH_DESIGNATE_FLAGS(veh) & only_designate) != only_designate) {
			continue;
		}
		if (only_fame != NOTHING && VEH_FAME(veh) != only_fame) {
			continue;
		}
		if (fame_over != NOTHING && VEH_FAME(veh) < fame_over) {
			continue;
		}
		if (fame_under != NOTHING && (VEH_FAME(veh) > fame_under || VEH_FAME(veh) == 0)) {
			continue;
		}
		if (not_flagged != NOBITS && VEH_FLAGGED(veh, not_flagged)) {
			continue;
		}
		if (only_flags != NOBITS && (VEH_FLAGS(veh) & only_flags) != only_flags) {
			continue;
		}
		if (only_functions != NOBITS && (VEH_FUNCTIONS(veh) & only_functions) != only_functions) {
			continue;
		}
		if (only_hitpoints != NOTHING && VEH_MAX_HEALTH(veh) != only_hitpoints) {
			continue;
		}
		if (hitpoints_over != NOTHING && VEH_MAX_HEALTH(veh) < hitpoints_over) {
			continue;
		}
		if (hitpoints_under != NOTHING && VEH_MAX_HEALTH(veh) > hitpoints_under) {
			continue;
		}
		if (*only_icon && !VEH_ICON(veh)) {
			continue;
		}
		if (*only_icon && !strstr(only_icon, VEH_ICON(veh)) && !strstr(only_icon, strip_color(VEH_ICON(veh)))) {
			continue;
		}
		if (find_interacts) {	// look up its interactions
			found_interacts = NOBITS;
			LL_FOREACH(VEH_INTERACTIONS(veh), inter) {
				found_interacts |= BIT(inter->type);
			}
			if ((find_interacts & found_interacts) != find_interacts) {
				continue;
			}
		}
		if (only_level != NOTHING) {	// level-based checks
			if (VEH_MAX_SCALE_LEVEL(veh) != 0 && only_level > VEH_MAX_SCALE_LEVEL(veh)) {
				continue;
			}
			if (VEH_MIN_SCALE_LEVEL(veh) != 0 && only_level < VEH_MIN_SCALE_LEVEL(veh)) {
				continue;
			}
		}
		if (only_military != NOTHING && VEH_MILITARY(veh) != only_military) {
			continue;
		}
		if (military_over != NOTHING && VEH_MILITARY(veh) < military_over) {
			continue;
		}
		if (military_under != NOTHING && (VEH_MILITARY(veh) > military_under || VEH_MILITARY(veh) == 0)) {
			continue;
		}
		if (only_move != NOTHING && VEH_MOVE_TYPE(veh) != only_move) {
			continue;
		}
		if (only_rooms != NOTHING && VEH_MAX_ROOMS(veh) != only_rooms) {
			continue;
		}
		if (rooms_over != NOTHING && VEH_MAX_ROOMS(veh) < rooms_over) {
			continue;
		}
		if (rooms_under != NOTHING && (VEH_MAX_ROOMS(veh) > rooms_under || VEH_MAX_ROOMS(veh) == 0)) {
			continue;
		}
		if (size_over != NOTHING && VEH_SIZE(veh) < size_over) {
			continue;
		}
		if (size_under != NOTHING && VEH_SIZE(veh) > size_under) {
			continue;
		}
		if (only_speed != NOTHING && VEH_SPEED_BONUSES(veh) != only_speed) {
			continue;
		}
		if (find_custom) {	// look up its custom messages
			found_custom = NOBITS;
			LL_FOREACH(VEH_CUSTOM_MSGS(veh), cust) {
				found_custom |= BIT(cust->type);
			}
			if ((find_custom & found_custom) != find_custom) {
				continue;
			}
		}
		
		if (*find_keywords && !multi_isname(find_keywords, VEH_KEYWORDS(veh)) && !multi_isname(find_keywords, VEH_LONG_DESC(veh)) && !multi_isname(find_keywords, VEH_LOOK_DESC(veh)) && !multi_isname(find_keywords, VEH_SHORT_DESC(veh)) && !search_extra_descs(find_keywords, VEH_EX_DESCS(veh)) && !search_custom_messages(find_keywords, VEH_CUSTOM_MSGS(veh))) {
			continue;
		}
		
		// show it
		snprintf(line, sizeof(line), "[%5d] %s\r\n", VEH_VNUM(veh), VEH_SHORT_DESC(veh));
		if (strlen(line) + size < sizeof(buf)) {
			size += snprintf(buf + size, sizeof(buf) - size, "%s", line);
			++count;
		}
		else {
			size += snprintf(buf + size, sizeof(buf) - size, "OVERFLOW\r\n");
			break;
		}
	}
	
	if (count > 0 && (size + 14) < sizeof(buf)) {
		size += snprintf(buf + size, sizeof(buf) - size, "(%d vehicles)\r\n", count);
	}
	else if (count == 0) {
		size += snprintf(buf + size, sizeof(buf) - size, " none\r\n");
	}
	
	if (ch->desc) {
		page_string(ch->desc, buf, TRUE);
	}
}


/**
* Function to save a player's changes to a vehicle (or a new one).
*
* @param descriptor_data *desc The descriptor who is saving.
*/
void save_olc_vehicle(descriptor_data *desc) {
	void prune_extra_descs(struct extra_descr_data **list);
	
	vehicle_data *proto, *veh = GET_OLC_VEHICLE(desc), *iter;
	any_vnum vnum = GET_OLC_VNUM(desc);
	struct spawn_info *spawn;
	struct quest_lookup *ql;
	struct shop_lookup *sl;
	bitvector_t old_flags;
	UT_hash_handle hh;

	// have a place to save it?
	if (!(proto = vehicle_proto(vnum))) {
		proto = create_vehicle_table_entry(vnum);
	}
	
	// sanity
	if (!VEH_KEYWORDS(veh) || !*VEH_KEYWORDS(veh)) {
		if (VEH_KEYWORDS(veh)) {
			free(VEH_KEYWORDS(veh));
		}
		VEH_KEYWORDS(veh) = str_dup(default_vehicle_keywords);
	}
	if (!VEH_SHORT_DESC(veh) || !*VEH_SHORT_DESC(veh)) {
		if (VEH_SHORT_DESC(veh)) {
			free(VEH_SHORT_DESC(veh));
		}
		VEH_SHORT_DESC(veh) = str_dup(default_vehicle_short_desc);
	}
	if (!VEH_LONG_DESC(veh) || !*VEH_LONG_DESC(veh)) {
		if (VEH_LONG_DESC(veh)) {
			free(VEH_LONG_DESC(veh));
		}
		VEH_LONG_DESC(veh) = str_dup(default_vehicle_long_desc);
	}
	if (VEH_ICON(veh) && !*VEH_ICON(veh)) {
		free(VEH_ICON(veh));
		VEH_ICON(veh) = NULL;
	}
	VEH_HEALTH(veh) = VEH_MAX_HEALTH(veh);
	prune_extra_descs(&VEH_EX_DESCS(veh));
	
	// update live vehicles
	DL_FOREACH(vehicle_list, iter) {
		if (VEH_VNUM(iter) != vnum) {
			continue;
		}
		
		// flags (preserve the state of the savable flags only)
		old_flags = VEH_FLAGS(iter) & SAVABLE_VEH_FLAGS;
		VEH_FLAGS(iter) = (VEH_FLAGS(veh) & ~SAVABLE_VEH_FLAGS) | old_flags;
		
		// update pointers
		if (VEH_KEYWORDS(iter) == VEH_KEYWORDS(proto)) {
			VEH_KEYWORDS(iter) = VEH_KEYWORDS(veh);
		}
		if (VEH_SHORT_DESC(iter) == VEH_SHORT_DESC(proto)) {
			VEH_SHORT_DESC(iter) = VEH_SHORT_DESC(veh);
		}
		if (VEH_ICON(iter) == VEH_ICON(proto)) {
			VEH_ICON(iter) = VEH_ICON(veh);
		}
		if (VEH_LONG_DESC(iter) == VEH_LONG_DESC(proto)) {
			VEH_LONG_DESC(iter) = VEH_LONG_DESC(veh);
		}
		if (VEH_LOOK_DESC(iter) == VEH_LOOK_DESC(proto)) {
			VEH_LOOK_DESC(iter) = VEH_LOOK_DESC(veh);
		}
		if (iter->attributes == proto->attributes) {
			iter->attributes = veh->attributes;
		}
		
		// remove old scripts
		if (SCRIPT(iter)) {
			remove_all_triggers(iter, VEH_TRIGGER);
		}
		if (iter->proto_script && iter->proto_script != proto->proto_script) {
			free_proto_scripts(&iter->proto_script);
		}
		
		// re-attach scripts
		iter->proto_script = copy_trig_protos(veh->proto_script);
		assign_triggers(iter, VEH_TRIGGER);
		
		// sanity checks
		if (VEH_HEALTH(iter) > VEH_MAX_HEALTH(iter)) {
			VEH_HEALTH(iter) = VEH_MAX_HEALTH(iter);
		}
		
		affect_total_room(IN_ROOM(iter));
	}
	
	// free prototype strings and pointers
	if (VEH_KEYWORDS(proto)) {
		free(VEH_KEYWORDS(proto));
	}
	if (VEH_SHORT_DESC(proto)) {
		free(VEH_SHORT_DESC(proto));
	}
	if (VEH_ICON(proto)) {
		free(VEH_ICON(proto));
	}
	if (VEH_LONG_DESC(proto)) {
		free(VEH_LONG_DESC(proto));
	}
	if (VEH_LOOK_DESC(proto)) {
		free(VEH_LOOK_DESC(proto));
	}
	if (VEH_YEARLY_MAINTENANCE(proto)) {
		free_resource_list(VEH_YEARLY_MAINTENANCE(proto));
	}
	free_interactions(&VEH_INTERACTIONS(proto));
	while ((spawn = VEH_SPAWNS(proto))) {
		VEH_SPAWNS(proto) = spawn->next;
		free(spawn);
	}
	free_extra_descs(&VEH_EX_DESCS(proto));
	free(proto->attributes);
	
	// free old script?
	if (proto->proto_script) {
		free_proto_scripts(&proto->proto_script);
	}
	
	// save data back over the proto-type
	hh = proto->hh;	// save old hash handle
	ql = proto->quest_lookups;	// save lookups
	sl = proto->shop_lookups;
	
	*proto = *veh;	// copy over all data
	proto->vnum = vnum;	// ensure correct vnum
	
	proto->hh = hh;	// restore old hash handle
	proto->quest_lookups = ql;	// restore lookups
	proto->shop_lookups = sl;
		
	// and save to file
	save_library_file_for_vnum(DB_BOOT_VEH, vnum);
}


/**
* Creates a copy of a vehicle, or clears a new one, for editing.
* 
* @param vehicle_data *input The vehicle to copy, or NULL to make a new one.
* @return vehicle_data* The copied vehicle.
*/
vehicle_data *setup_olc_vehicle(vehicle_data *input) {
	extern struct extra_descr_data *copy_extra_descs(struct extra_descr_data *list);
	
	vehicle_data *new;
	
	CREATE(new, vehicle_data, 1);
	clear_vehicle(new);
	
	if (input) {
		free(new->attributes);	// created by clear_vehicle
		
		// copy normal data
		*new = *input;
		CREATE(new->attributes, struct vehicle_attribute_data, 1);
		*(new->attributes) = *(input->attributes);

		// copy things that are pointers
		VEH_KEYWORDS(new) = VEH_KEYWORDS(input) ? str_dup(VEH_KEYWORDS(input)) : NULL;
		VEH_SHORT_DESC(new) = VEH_SHORT_DESC(input) ? str_dup(VEH_SHORT_DESC(input)) : NULL;
		VEH_ICON(new) = VEH_ICON(input) ? str_dup(VEH_ICON(input)) : NULL;
		VEH_LONG_DESC(new) = VEH_LONG_DESC(input) ? str_dup(VEH_LONG_DESC(input)) : NULL;
		VEH_LOOK_DESC(new) = VEH_LOOK_DESC(input) ? str_dup(VEH_LOOK_DESC(input)) : NULL;
		
		// copy lists
		VEH_YEARLY_MAINTENANCE(new) = copy_resource_list(VEH_YEARLY_MAINTENANCE(input));
		VEH_EX_DESCS(new) = copy_extra_descs(VEH_EX_DESCS(input));
		VEH_INTERACTIONS(new) = copy_interaction_list(VEH_INTERACTIONS(input));
		VEH_SPAWNS(new) = copy_spawn_list(VEH_SPAWNS(input));
		
		// copy scripts
		SCRIPT(new) = NULL;
		new->proto_script = copy_trig_protos(input->proto_script);
	}
	else {
		// brand new: some defaults
		VEH_KEYWORDS(new) = str_dup(default_vehicle_keywords);
		VEH_SHORT_DESC(new) = str_dup(default_vehicle_short_desc);
		VEH_LONG_DESC(new) = str_dup(default_vehicle_long_desc);
		VEH_MAX_HEALTH(new) = 1;
		VEH_MOVE_TYPE(new) = MOB_MOVE_DRIVES;
		SCRIPT(new) = NULL;
		new->proto_script = NULL;
	}
	
	// done
	return new;	
}


 //////////////////////////////////////////////////////////////////////////////
//// DISPLAYS ////////////////////////////////////////////////////////////////

/**
* For vstat.
*
* @param char_data *ch The player requesting stats.
* @param vehicle_data *veh The vehicle to display.
*/
void do_stat_vehicle(char_data *ch, vehicle_data *veh) {
	void get_interaction_display(struct interaction_item *list, char *save_buffer);
	extern char *get_room_name(room_data *room, bool color);
	void script_stat (char_data *ch, struct script_data *sc);
	void show_spawn_summary_to_char(char_data *ch, struct spawn_info *list);
	extern const char *room_extra_types[];
	
	char buf[MAX_STRING_LENGTH * 2], part[MAX_STRING_LENGTH];
	struct room_extra_data *red, *next_red;
	struct custom_message *custm;
	obj_data *obj;
	size_t size;
	int found;
	
	if (!veh) {
		return;
	}
	
	// first line
	size = snprintf(buf, sizeof(buf), "VNum: [\tc%d\t0], S-Des: \tc%s\t0, Keywords: %s\r\n", VEH_VNUM(veh), VEH_SHORT_DESC(veh), VEH_KEYWORDS(veh));
	
	size += snprintf(buf + size, sizeof(buf) - size, "L-Des: %s\r\n", VEH_LONG_DESC(veh));
	
	if (VEH_LOOK_DESC(veh) && *VEH_LOOK_DESC(veh)) {
		size += snprintf(buf + size, sizeof(buf) - size, "%s", VEH_LOOK_DESC(veh));
	}
	
	if (VEH_EX_DESCS(veh)) {
		struct extra_descr_data *desc;
		size += snprintf(buf + size, sizeof(buf) - size, "Extra descs:\tc");
		LL_FOREACH(VEH_EX_DESCS(veh), desc) {
			size += snprintf(buf + size, sizeof(buf) - size, " %s", desc->keyword);
		}
		size += snprintf(buf + size, sizeof(buf) - size, "\t0\r\n");
	}
	
	if (VEH_ICON(veh)) {
		size += snprintf(buf + size, sizeof(buf) - size, "Map Icon: %s\t0 %s\r\n", VEH_ICON(veh), show_color_codes(VEH_ICON(veh)));
	}
	
	// stats lines
	size += snprintf(buf + size, sizeof(buf) - size, "Health: [\tc%d\t0/\tc%d\t0], Capacity: [\tc%d\t0/\tc%d\t0], Animals Req: [\tc%d\t0], Move Type: [\ty%s\t0]\r\n", (int) VEH_HEALTH(veh), VEH_MAX_HEALTH(veh), VEH_CARRYING_N(veh), VEH_CAPACITY(veh), VEH_ANIMALS_REQUIRED(veh), mob_move_types[VEH_MOVE_TYPE(veh)]);
	size += snprintf(buf + size, sizeof(buf) - size, "Fame: [\tc%d\t0], Military: [\tc%d\t0], Speed: [\ty%s\t0], Size: [\tc%d\t0]\r\n", VEH_FAME(veh), VEH_MILITARY(veh), vehicle_speed_types[VEH_SPEED_BONUSES(veh)], VEH_SIZE(veh));
	
	if (VEH_INTERIOR_ROOM_VNUM(veh) != NOTHING || VEH_MAX_ROOMS(veh) || VEH_DESIGNATE_FLAGS(veh)) {
		sprintbit(VEH_DESIGNATE_FLAGS(veh), designate_flags, part, TRUE);
		size += snprintf(buf + size, sizeof(buf) - size, "Interior: [\tc%d\t0 - \ty%s\t0], Rooms: [\tc%d\t0], Designate: \ty%s\t0\r\n", VEH_INTERIOR_ROOM_VNUM(veh), building_proto(VEH_INTERIOR_ROOM_VNUM(veh)) ? GET_BLD_NAME(building_proto(VEH_INTERIOR_ROOM_VNUM(veh))) : "none", VEH_MAX_ROOMS(veh), part);
	}
	
	sprintbit(VEH_FLAGS(veh), vehicle_flags, part, TRUE);
	size += snprintf(buf + size, sizeof(buf) - size, "Flags: \tg%s\t0\r\n", part);
	
	sprintbit(VEH_ROOM_AFFECTS(veh), room_aff_bits, part, TRUE);
	size += snprintf(buf + size, sizeof(buf) - size, "Affects: \tc%s\t0\r\n", part);
	
	sprintbit(VEH_FUNCTIONS(veh), function_flags, part, TRUE);
	size += snprintf(buf + size, sizeof(buf) - size, "Functions: \tg%s\t0\r\n", part);
	
	ordered_sprintbit(VEH_REQUIRES_CLIMATE(veh), climate_flags, climate_flags_order, FALSE, part);
	size += snprintf(buf + size, sizeof(buf) - size, "Requires climate: \tc%s\t0\r\n", part);
	ordered_sprintbit(VEH_FORBID_CLIMATE(veh), climate_flags, climate_flags_order, FALSE, part);
	size += snprintf(buf + size, sizeof(buf) - size, "Forbid climate: \tg%s\t0\r\n", part);
	
	if (VEH_INTERACTIONS(veh)) {
		send_to_char("Interactions:\r\n", ch);
		get_interaction_display(VEH_INTERACTIONS(veh), part);
		strcat(buf, part);
		size += strlen(part);
	}
	
	if (VEH_YEARLY_MAINTENANCE(veh)) {
		get_resource_display(VEH_YEARLY_MAINTENANCE(veh), part);
		size += snprintf(buf + size, sizeof(buf) - size, "Yearly maintenance:\r\n%s", part);
	}
	
	size += snprintf(buf + size, sizeof(buf) - size, "Scaled to level: [\tc%d (%d-%d)\t0], Owner: [%s%s\t0]\r\n", VEH_SCALE_LEVEL(veh), VEH_MIN_SCALE_LEVEL(veh), VEH_MAX_SCALE_LEVEL(veh), VEH_OWNER(veh) ? EMPIRE_BANNER(VEH_OWNER(veh)) : "", VEH_OWNER(veh) ? EMPIRE_NAME(VEH_OWNER(veh)) : "nobody");
	
	if (VEH_INTERIOR_HOME_ROOM(veh) || VEH_INSIDE_ROOMS(veh) > 0) {
		size += snprintf(buf + size, sizeof(buf) - size, "Interior location: [\ty%d\t0], Added rooms: [\tg%d\t0/\tg%d\t0]\r\n", VEH_INTERIOR_HOME_ROOM(veh) ? GET_ROOM_VNUM(VEH_INTERIOR_HOME_ROOM(veh)) : NOTHING, VEH_INSIDE_ROOMS(veh), VEH_MAX_ROOMS(veh));
	}
	
	if (IN_ROOM(veh)) {
		size += snprintf(buf + size, sizeof(buf) - size, "In room: %s, Led by: %s, ", get_room_name(IN_ROOM(veh), FALSE), VEH_LED_BY(veh) ? PERS(VEH_LED_BY(veh), ch, TRUE) : "nobody");
		size += snprintf(buf + size, sizeof(buf) - size, "Sitting on: %s, ", VEH_SITTING_ON(veh) ? PERS(VEH_SITTING_ON(veh), ch, TRUE) : "nobody");
		size += snprintf(buf + size, sizeof(buf) - size, "Driven by: %s\r\n", VEH_DRIVER(veh) ? PERS(VEH_DRIVER(veh), ch, TRUE) : "nobody");
	}
	
	if (VEH_CONTAINS(veh)) {
		sprintf(part, "Contents:\tg");
		found = 0;
		DL_FOREACH2(VEH_CONTAINS(veh), obj, next_content) {
			sprintf(part + strlen(part), "%s %s", found++ ? "," : "", GET_OBJ_DESC(obj, ch, OBJ_DESC_SHORT));
			if (strlen(part) >= 62) {
				if (obj->next_content) {
					strcat(part, ",");
				}
				size += snprintf(buf + size, sizeof(buf) - size, "%s\r\n", part);
				*part = '\0';
				found = 0;
			}
		}
		if (*part) {
			size += snprintf(buf + size, sizeof(buf) - size, "%s\t0\r\n", part);
		}
		else {
			size += snprintf(buf + size, sizeof(buf) - size, "\t0");
		}
	}
	
	if (VEH_NEEDS_RESOURCES(veh)) {
		get_resource_display(VEH_NEEDS_RESOURCES(veh), part);
		size += snprintf(buf + size, sizeof(buf) - size, "%s resources:\r\n%s", VEH_IS_DISMANTLING(veh) ? "Dismantle" : "Needs", part);
	}
	
	if (VEH_CUSTOM_MSGS(veh)) {
		size += snprintf(buf + size, sizeof(buf) - size, "Custom messages:\r\n");
		
		LL_FOREACH(VEH_CUSTOM_MSGS(veh), custm) {
			size += snprintf(buf + size, sizeof(buf) - size, " %s: %s\r\n", veh_custom_types[custm->type], custm->msg);
		}
	}
	
	send_to_char(buf, ch);
	show_spawn_summary_to_char(ch, VEH_SPAWNS(veh));
	
	if (VEH_EXTRA_DATA(veh)) {
		msg_to_char(ch, "Extra data:\r\n");
		HASH_ITER(hh, VEH_EXTRA_DATA(veh), red, next_red) {
			sprinttype(red->type, room_extra_types, buf);
			msg_to_char(ch, " %s: %d\r\n", buf, red->value);
		}
	}
	
	// script info
	msg_to_char(ch, "Script information:\r\n");
	if (SCRIPT(veh)) {
		script_stat(ch, SCRIPT(veh));
	}
	else {
		msg_to_char(ch, "  None.\r\n");
	}
}


/**
* Perform a look-at-vehicle.
*
* @param vehicle_data *veh The vehicle to look at.
* @param char_data *ch The person to show the output to.
*/
void look_at_vehicle(vehicle_data *veh, char_data *ch) {
	char lbuf[MAX_STRING_LENGTH];
	vehicle_data *proto;
	
	if (!veh || !ch || !ch->desc) {
		return;
	}
	
	proto = vehicle_proto(VEH_VNUM(veh));
	
	if (VEH_LOOK_DESC(veh) && *VEH_LOOK_DESC(veh)) {
		sprintf(lbuf, "%s:\r\n%s", VEH_SHORT_DESC(veh), VEH_LOOK_DESC(veh));
		msg_to_char(ch, "%s", CAP(lbuf));
	}
	else {
		act("You look at $V but see nothing special.", FALSE, ch, NULL, veh, TO_CHAR);
	}
	
	if (VEH_SHORT_DESC(veh) != VEH_SHORT_DESC(proto)) {
		msg_to_char(ch, "Type: %s\r\n", skip_filler(VEH_SHORT_DESC(proto)));
	}
	
	if (VEH_OWNER(veh)) {
		msg_to_char(ch, "Owner: %s%s\t0", EMPIRE_BANNER(VEH_OWNER(veh)), EMPIRE_NAME(VEH_OWNER(veh)));
		
		if (VEH_OWNER(veh) == GET_LOYALTY(ch)) {
			if (VEH_FLAGGED(veh, VEH_PLAYER_NO_WORK)) {
				send_to_char(" (no-work)", ch);
			}
			if (VEH_FLAGGED(veh, VEH_PLAYER_NO_DISMANTLE)) {
				send_to_char(" (no-dismantle)", ch);
			}
		}
		send_to_char("\r\n", ch);
	}
	
	if (VEH_NEEDS_RESOURCES(veh)) {
		show_resource_list(VEH_NEEDS_RESOURCES(veh), lbuf);
		
		if (VEH_IS_COMPLETE(veh)) {
			msg_to_char(ch, "Maintenance needed: %s\r\n", lbuf);
		}
		else if (VEH_IS_DISMANTLING(veh)) {
			msg_to_char(ch, "Remaining to dismantle: %s\r\n", lbuf);
		}
		else {
			msg_to_char(ch, "Resources to completion: %s\r\n", lbuf);
		}
	}
}


/**
* This is the main recipe display for vehicle OLC. It displays the user's
* currently-edited vehicle.
*
* @param char_data *ch The person who is editing a vehicle and will see its display.
*/
void olc_show_vehicle(char_data *ch) {
	void get_extra_desc_display(struct extra_descr_data *list, char *save_buffer);
	void get_interaction_display(struct interaction_item *list, char *save_buffer);
	void get_script_display(struct trig_proto_list *list, char *save_buffer);
	
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	char buf[MAX_STRING_LENGTH], lbuf[MAX_STRING_LENGTH];
	struct custom_message *custm;
	struct spawn_info *spawn;
	int count;
	
	if (!veh) {
		return;
	}
	
	*buf = '\0';
	
	sprintf(buf + strlen(buf), "[%s%d\t0] %s%s\t0\r\n", OLC_LABEL_CHANGED, GET_OLC_VNUM(ch->desc), OLC_LABEL_UNCHANGED, !vehicle_proto(VEH_VNUM(veh)) ? "new vehicle" : VEH_SHORT_DESC(vehicle_proto(VEH_VNUM(veh))));

	sprintf(buf + strlen(buf), "<%skeywords\t0> %s\r\n", OLC_LABEL_STR(VEH_KEYWORDS(veh), default_vehicle_keywords), NULLSAFE(VEH_KEYWORDS(veh)));
	sprintf(buf + strlen(buf), "<%sshortdescription\t0> %s\r\n", OLC_LABEL_STR(VEH_SHORT_DESC(veh), default_vehicle_short_desc), NULLSAFE(VEH_SHORT_DESC(veh)));
	sprintf(buf + strlen(buf), "<%slongdescription\t0>\r\n%s\r\n", OLC_LABEL_STR(VEH_LONG_DESC(veh), default_vehicle_long_desc), NULLSAFE(VEH_LONG_DESC(veh)));
	sprintf(buf + strlen(buf), "<%slookdescription\t0>\r\n%s", OLC_LABEL_STR(VEH_LOOK_DESC(veh), ""), NULLSAFE(VEH_LOOK_DESC(veh)));
	sprintf(buf + strlen(buf), "<%sicon\t0> %s\t0 %s\r\n", OLC_LABEL_STR(VEH_ICON(veh), ""), VEH_ICON(veh) ? VEH_ICON(veh) : "none", VEH_ICON(veh) ? show_color_codes(VEH_ICON(veh)) : "");
	
	sprintbit(VEH_FLAGS(veh), vehicle_flags, lbuf, TRUE);
	sprintf(buf + strlen(buf), "<%sflags\t0> %s\r\n", OLC_LABEL_VAL(VEH_FLAGS(veh), NOBITS), lbuf);
	
	sprintf(buf + strlen(buf), "<%shitpoints\t0> %d\r\n", OLC_LABEL_VAL(VEH_MAX_HEALTH(veh), 1), VEH_MAX_HEALTH(veh));
	sprintf(buf + strlen(buf), "<%smovetype\t0> %s\r\n", OLC_LABEL_VAL(VEH_MOVE_TYPE(veh), 0), mob_move_types[VEH_MOVE_TYPE(veh)]);
	sprintf(buf + strlen(buf), "<%sspeed\t0> %s, <%ssize\t0> %d\r\n", OLC_LABEL_VAL(VEH_SPEED_BONUSES(veh), VSPEED_NORMAL), vehicle_speed_types[VEH_SPEED_BONUSES(veh)], OLC_LABEL_VAL(VEH_SIZE(veh), 0), VEH_SIZE(veh));
	sprintf(buf + strlen(buf), "<%scapacity\t0> %d item%s\r\n", OLC_LABEL_VAL(VEH_CAPACITY(veh), 0), VEH_CAPACITY(veh), PLURAL(VEH_CAPACITY(veh)));
	sprintf(buf + strlen(buf), "<%sanimalsrequired\t0> %d\r\n", OLC_LABEL_VAL(VEH_ANIMALS_REQUIRED(veh), 0), VEH_ANIMALS_REQUIRED(veh));
	
	if (VEH_MIN_SCALE_LEVEL(veh) > 0) {
		sprintf(buf + strlen(buf), "<%sminlevel\t0> %d\r\n", OLC_LABEL_CHANGED, VEH_MIN_SCALE_LEVEL(veh));
	}
	else {
		sprintf(buf + strlen(buf), "<%sminlevel\t0> none\r\n", OLC_LABEL_UNCHANGED);
	}
	if (VEH_MAX_SCALE_LEVEL(veh) > 0) {
		sprintf(buf + strlen(buf), "<%smaxlevel\t0> %d\r\n", OLC_LABEL_CHANGED, VEH_MAX_SCALE_LEVEL(veh));
	}
	else {
		sprintf(buf + strlen(buf), "<%smaxlevel\t0> none\r\n", OLC_LABEL_UNCHANGED);
	}

	sprintf(buf + strlen(buf), "<%sinteriorroom\t0> %d - %s\r\n", OLC_LABEL_VAL(VEH_INTERIOR_ROOM_VNUM(veh), NOWHERE), VEH_INTERIOR_ROOM_VNUM(veh), building_proto(VEH_INTERIOR_ROOM_VNUM(veh)) ? GET_BLD_NAME(building_proto(VEH_INTERIOR_ROOM_VNUM(veh))) : "none");
	sprintf(buf + strlen(buf), "<%sextrarooms\t0> %d\r\n", OLC_LABEL_VAL(VEH_MAX_ROOMS(veh), 0), VEH_MAX_ROOMS(veh));
	sprintbit(VEH_DESIGNATE_FLAGS(veh), designate_flags, lbuf, TRUE);
	sprintf(buf + strlen(buf), "<%sdesignate\t0> %s\r\n", OLC_LABEL_VAL(VEH_DESIGNATE_FLAGS(veh), NOBITS), lbuf);
	sprintf(buf + strlen(buf), "<%sfame\t0> %d\r\n", OLC_LABEL_VAL(VEH_FAME(veh), 0), VEH_FAME(veh));
	sprintf(buf + strlen(buf), "<%smilitary\t0> %d\r\n", OLC_LABEL_VAL(VEH_MILITARY(veh), 0), VEH_MILITARY(veh));
	
	sprintbit(VEH_ROOM_AFFECTS(veh), room_aff_bits, lbuf, TRUE);
	sprintf(buf + strlen(buf), "<%saffects\t0> %s\r\n", OLC_LABEL_VAL(VEH_ROOM_AFFECTS(veh), NOBITS), lbuf);
	
	sprintbit(VEH_FUNCTIONS(veh), function_flags, lbuf, TRUE);
	sprintf(buf + strlen(buf), "<%sfunctions\t0> %s\r\n", OLC_LABEL_VAL(VEH_FUNCTIONS(veh), NOBITS), lbuf);
	
	ordered_sprintbit(VEH_REQUIRES_CLIMATE(veh), climate_flags, climate_flags_order, FALSE, lbuf);
	sprintf(buf + strlen(buf), "<%srequireclimate\t0> %s\r\n", OLC_LABEL_VAL(VEH_REQUIRES_CLIMATE(veh), NOBITS), lbuf);
	ordered_sprintbit(VEH_FORBID_CLIMATE(veh), climate_flags, climate_flags_order, FALSE, lbuf);
	sprintf(buf + strlen(buf), "<%sforbidclimate\t0> %s\r\n", OLC_LABEL_VAL(VEH_FORBID_CLIMATE(veh), NOBITS), lbuf);
	
	// exdesc
	sprintf(buf + strlen(buf), "Extra descriptions: <%sextra\t0>\r\n", OLC_LABEL_PTR(VEH_EX_DESCS(veh)));
	if (VEH_EX_DESCS(veh)) {
		get_extra_desc_display(VEH_EX_DESCS(veh), lbuf);
		strcat(buf, lbuf);
	}

	sprintf(buf + strlen(buf), "Interactions: <%sinteraction\t0>\r\n", OLC_LABEL_PTR(VEH_INTERACTIONS(veh)));
	if (VEH_INTERACTIONS(veh)) {
		get_interaction_display(VEH_INTERACTIONS(veh), lbuf);
		strcat(buf, lbuf);
	}
	
	// maintenance resources
	sprintf(buf + strlen(buf), "Yearly maintenance resources required: <%sresource\t0>\r\n", OLC_LABEL_PTR(VEH_YEARLY_MAINTENANCE(veh)));
	if (VEH_YEARLY_MAINTENANCE(veh)) {
		get_resource_display(VEH_YEARLY_MAINTENANCE(veh), lbuf);
		strcat(buf, lbuf);
	}
	
	// custom messages
	sprintf(buf + strlen(buf), "Custom messages: <%scustom\t0>\r\n", OLC_LABEL_PTR(VEH_CUSTOM_MSGS(veh)));
	count = 0;
	LL_FOREACH(VEH_CUSTOM_MSGS(veh), custm) {
		sprintf(buf + strlen(buf), " \ty%d\t0. [%s] %s\r\n", ++count, veh_custom_types[custm->type], custm->msg);
	}
	
	// scripts
	sprintf(buf + strlen(buf), "Scripts: <%sscript\t0>\r\n", OLC_LABEL_PTR(veh->proto_script));
	if (veh->proto_script) {
		get_script_display(veh->proto_script, lbuf);
		strcat(buf, lbuf);
	}
	
	// spawns
	sprintf(buf + strlen(buf), "<%sspawns\t0>\r\n", OLC_LABEL_PTR(VEH_SPAWNS(veh)));
	if (VEH_SPAWNS(veh)) {
		count = 0;
		LL_FOREACH(VEH_SPAWNS(veh), spawn) {
			++count;
		}
		sprintf(buf + strlen(buf), " %d spawn%s set\r\n", count, PLURAL(count));
	}
	
	page_string(ch->desc, buf, TRUE);
}


/**
* Searches the vehicle db for a match, and prints it to the character.
*
* @param char *searchname The search string.
* @param char_data *ch The player who is searching.
* @return int The number of matches shown.
*/
int vnum_vehicle(char *searchname, char_data *ch) {
	vehicle_data *iter, *next_iter;
	int found = 0;
	
	HASH_ITER(hh, vehicle_table, iter, next_iter) {
		if (multi_isname(searchname, VEH_KEYWORDS(iter))) {
			msg_to_char(ch, "%3d. [%5d] %s\r\n", ++found, VEH_VNUM(iter), VEH_SHORT_DESC(iter));
		}
	}
	
	return found;
}


 //////////////////////////////////////////////////////////////////////////////
//// OLC MODULES /////////////////////////////////////////////////////////////

OLC_MODULE(vedit_affects) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_ROOM_AFFECTS(veh) = olc_process_flag(ch, argument, "affect", "affects", room_aff_bits, VEH_ROOM_AFFECTS(veh));
}


OLC_MODULE(vedit_animalsrequired) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_ANIMALS_REQUIRED(veh) = olc_process_number(ch, argument, "animals required", "animalsrequired", 0, 100, VEH_ANIMALS_REQUIRED(veh));
}


OLC_MODULE(vedit_capacity) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_CAPACITY(veh) = olc_process_number(ch, argument, "capacity", "capacity", 0, 10000, VEH_CAPACITY(veh));
}


OLC_MODULE(vedit_custom) {
	void olc_process_custom_messages(char_data *ch, char *argument, struct custom_message **list, const char **type_names);
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	
	olc_process_custom_messages(ch, argument, &VEH_CUSTOM_MSGS(veh), veh_custom_types);
}


OLC_MODULE(vedit_speed) {
	// TODO: move this into alphabetic order on some future major version
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_SPEED_BONUSES(veh) = olc_process_type(ch, argument, "speed", "speed", vehicle_speed_types, VEH_SPEED_BONUSES(veh));
}


OLC_MODULE(vedit_designate) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_DESIGNATE_FLAGS(veh) = olc_process_flag(ch, argument, "designate", "designate", designate_flags, VEH_DESIGNATE_FLAGS(veh));
}


OLC_MODULE(vedit_extra_desc) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	olc_process_extra_desc(ch, argument, &VEH_EX_DESCS(veh));
}


OLC_MODULE(vedit_extrarooms) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_MAX_ROOMS(veh) = olc_process_number(ch, argument, "max rooms", "maxrooms", 0, 1000, VEH_MAX_ROOMS(veh));
}


OLC_MODULE(vedit_fame) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_FAME(veh) = olc_process_number(ch, argument, "fame", "fame", -1000, 1000, VEH_FAME(veh));
}


OLC_MODULE(vedit_flags) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_FLAGS(veh) = olc_process_flag(ch, argument, "vehicle", "flags", vehicle_flags, VEH_FLAGS(veh));
}


OLC_MODULE(vedit_forbidclimate) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_FORBID_CLIMATE(veh) = olc_process_flag(ch, argument, "climate", "forbidclimate", climate_flags, VEH_FORBID_CLIMATE(veh));
}


OLC_MODULE(vedit_functions) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_FUNCTIONS(veh) = olc_process_flag(ch, argument, "function", "functions", function_flags, VEH_FUNCTIONS(veh));
}


OLC_MODULE(vedit_hitpoints) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_MAX_HEALTH(veh) = olc_process_number(ch, argument, "hitpoints", "hitpoints", 1, 1000, VEH_MAX_HEALTH(veh));
}


OLC_MODULE(vedit_icon) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	
	delete_doubledollar(argument);
	
	if (!str_cmp(argument, "none")) {
		if (VEH_ICON(veh)) {
			free(VEH_ICON(veh));
		}
		VEH_ICON(veh) = NULL;
		msg_to_char(ch, "The vehicle now has no icon and will not appear on the map.\r\n");
	}
	else if (!validate_icon(argument)) {
		msg_to_char(ch, "You must specify an icon that is 4 characters long, not counting color codes.\r\n");
	}
	else {
		olc_process_string(ch, argument, "icon", &VEH_ICON(veh));
		msg_to_char(ch, "\t0");	// in case color is unterminated
	}
}


OLC_MODULE(vedit_interaction) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	olc_process_interactions(ch, argument, &VEH_INTERACTIONS(veh), TYPE_ROOM);
}


OLC_MODULE(vedit_interiorroom) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	any_vnum old_b;
	bld_data *bld;
	
	if (!str_cmp(argument, "none")) {
		VEH_INTERIOR_ROOM_VNUM(veh) = NOTHING;
		msg_to_char(ch, "It now has no interior room.\r\n");
		return;
	}
	
	old_b = VEH_INTERIOR_ROOM_VNUM(veh);
	VEH_INTERIOR_ROOM_VNUM(veh) = olc_process_number(ch, argument, "interior room vnum", "interiorroom", 0, MAX_VNUM, VEH_INTERIOR_ROOM_VNUM(veh));
	
	if (!(bld = building_proto(VEH_INTERIOR_ROOM_VNUM(veh)))) {
		VEH_INTERIOR_ROOM_VNUM(veh) = old_b;
		msg_to_char(ch, "Invalid room building vnum. Old value restored.\r\n");
	}
	else if (!IS_SET(GET_BLD_FLAGS(bld), BLD_ROOM)) {
		VEH_INTERIOR_ROOM_VNUM(veh) = old_b;
		msg_to_char(ch, "You can only set it to a building template with the ROOM flag. Old value restored.\r\n");
	}	
	else {
		msg_to_char(ch, "It now has interior room '%s'.\r\n", GET_BLD_NAME(bld));
	}
}


OLC_MODULE(vedit_keywords) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	olc_process_string(ch, argument, "keywords", &VEH_KEYWORDS(veh));
}


OLC_MODULE(vedit_longdescription) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	olc_process_string(ch, argument, "long description", &VEH_LONG_DESC(veh));
}


OLC_MODULE(vedit_lookdescription) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	
	if (ch->desc->str) {
		msg_to_char(ch, "You are already editing a string.\r\n");
	}
	else {
		sprintf(buf, "description for %s", VEH_SHORT_DESC(veh));
		start_string_editor(ch->desc, buf, &VEH_LOOK_DESC(veh), MAX_ITEM_DESCRIPTION, TRUE);
	}
}


OLC_MODULE(vedit_maxlevel) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_MAX_SCALE_LEVEL(veh) = olc_process_number(ch, argument, "maximum level", "maxlevel", 0, MAX_INT, VEH_MAX_SCALE_LEVEL(veh));
}


OLC_MODULE(vedit_military) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_MILITARY(veh) = olc_process_number(ch, argument, "military", "military", 0, 1000, VEH_MILITARY(veh));
}


OLC_MODULE(vedit_minlevel) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_MIN_SCALE_LEVEL(veh) = olc_process_number(ch, argument, "minimum level", "minlevel", 0, MAX_INT, VEH_MIN_SCALE_LEVEL(veh));
}


OLC_MODULE(vedit_movetype) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_MOVE_TYPE(veh) = olc_process_type(ch, argument, "move type", "movetype", mob_move_types, VEH_MOVE_TYPE(veh));
}


OLC_MODULE(vedit_requiresclimate) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_REQUIRES_CLIMATE(veh) = olc_process_flag(ch, argument, "climate", "requiresclimate", climate_flags, VEH_REQUIRES_CLIMATE(veh));
}


OLC_MODULE(vedit_resource) {
	void olc_process_resources(char_data *ch, char *argument, struct resource_data **list);
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	olc_process_resources(ch, argument, &VEH_YEARLY_MAINTENANCE(veh));
}


OLC_MODULE(vedit_script) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	olc_process_script(ch, argument, &(veh->proto_script), VEH_TRIGGER);
}


OLC_MODULE(vedit_shortdescription) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	olc_process_string(ch, argument, "short description", &VEH_SHORT_DESC(veh));
}


OLC_MODULE(vedit_size) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	VEH_SIZE(veh) = olc_process_number(ch, argument, "size", "size", 0, config_get_int("vehicle_size_per_tile"), VEH_SIZE(veh));
}


OLC_MODULE(vedit_spawns) {
	vehicle_data *veh = GET_OLC_VEHICLE(ch->desc);
	olc_process_spawns(ch, argument, &VEH_SPAWNS(veh));
}
