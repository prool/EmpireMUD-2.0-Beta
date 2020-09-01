/* ************************************************************************
*   File: act.survival.c                                  EmpireMUD 2.0b5 *
*  Usage: code related to the Survival skill and its abilities            *
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
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "dg_scripts.h"
#include "vnums.h"

/**
* Contents:
*   Helpers
*   Hunt Helpers
*   Mount Commands
*   Commands
*/

// external vars

// external funcs
extern room_data *dir_to_room(room_data *room, int dir, bool ignore_entrance);
void scale_item_to_level(obj_data *obj, int level);
extern bool validate_spawn_location(room_data *room, bitvector_t spawn_flags, int x_coord, int y_coord, bool in_city);

// local protos
ACMD(do_dismount);


 //////////////////////////////////////////////////////////////////////////////
//// HELPERS /////////////////////////////////////////////////////////////////

INTERACTION_FUNC(butcher_interact) {
	obj_data *fillet = NULL;
	int num;
	
	if (!has_player_tech(ch, PTECH_BUTCHER_UPGRADE) && number(1, 100) > 60) {
		return FALSE;	// 60% chance of failure without the ability
	}
	
	for (num = 0; num < interaction->quantity; ++num) {
		fillet = read_object(interaction->vnum, TRUE);
		scale_item_to_level(fillet, 1);	// minimum level
		obj_to_char(fillet, ch);
		load_otrigger(fillet);
	}
	
	// mark gained
	if (GET_LOYALTY(ch)) {
		add_production_total(GET_LOYALTY(ch), interaction->vnum, interaction->quantity);
	}
	
	if (fillet) {
		if (interaction->quantity != 1) {
			sprintf(buf, "You skillfully butcher $p (x%d) from the corpse!", interaction->quantity);
			act(buf, FALSE, ch, fillet, NULL, TO_CHAR);
			
			sprintf(buf, "$n butchers a corpse and gets $p (x%d).", interaction->quantity);
			act(buf, FALSE, ch, fillet, NULL, TO_ROOM);
		}
		else {
			act("You skillfully butcher $p from the corpse!", FALSE, ch, fillet, NULL, TO_CHAR);
			act("$n butchers a corpse and gets $p.", FALSE, ch, fillet, NULL, TO_ROOM);
		}
		return TRUE;
	}

	// nothing gained?
	return FALSE;
}


/**
* Finds the best saddle in a player's inventory.
*
* @param char_data *ch the person
* @return obj_data *the best saddle in inventory, or NULL if none
*/
obj_data *find_best_saddle(char_data *ch) {
	extern bool can_wear_item(char_data *ch, obj_data *item, bool send_messages);
	
	obj_data *obj, *best = NULL;
	double best_score = 0, this;
	
	DL_FOREACH2(ch->carrying, obj, next_content) {
		if (CAN_WEAR(obj, ITEM_WEAR_SADDLE) && can_wear_item(ch, obj, FALSE)) {
			this = rate_item(obj);
			
			// give a slight bonus to items that are bound ONLY to this character
			if (OBJ_BOUND_TO(obj) && OBJ_BOUND_TO(obj)->next == NULL && bind_ok(obj, ch)) {
				this *= 1.1;
			}
			
			if (this >= best_score) {
				best = obj;
				best_score = this;
			}
		}
	}
	
	return best;
}


/**
* Determines if a room qualifies for No Trace (outdoors/wilderness).
*
* @param room_data *room Where to check.
* @return bool TRUE if No Trace works here.
*/
bool valid_no_trace(room_data *room) {
	if (IS_ADVENTURE_ROOM(room)) {
		return FALSE;	// adventures do not trigger this ability
	}
	if (!IS_OUTDOOR_TILE(room) || IS_ROAD(room) || IS_ANY_BUILDING(room)) {
		return FALSE;	// not outdoors
	}
	
	// all other cases?
	return TRUE;
}


 //////////////////////////////////////////////////////////////////////////////
//// HUNT HELPERS ////////////////////////////////////////////////////////////
	
// helper data: stores spawn lists for do_hunt
struct hunt_helper {
	struct spawn_info *list;
	struct hunt_helper *prev, *next;	// doubly-linked list
};

// for finding global spawn lists
struct hunt_global_bean {
	room_data *room;
	int x_coord, y_coord;
	struct hunt_helper **helpers;	// pointer to existing DLL of helpers
};


GLB_VALIDATOR(validate_global_hunt_for_map_spawns) {
	struct hunt_global_bean *data = (struct hunt_global_bean*)other_data;
	if (!other_data) {
		return FALSE;
	}
	return validate_spawn_location(data->room, GET_GLOBAL_SPARE_BITS(glb), data->x_coord, data->y_coord, FALSE);
}


GLB_FUNCTION(run_global_hunt_for_map_spawns) {
	struct hunt_global_bean *data = (struct hunt_global_bean*)other_data;
	struct hunt_helper *hlp;
	
	if (data && data->helpers) {
		CREATE(hlp, struct hunt_helper, 1);
		hlp->list = GET_GLOBAL_SPAWNS(glb);
		DL_APPEND(*(data->helpers), hlp);
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// MOUNT COMMANDS //////////////////////////////////////////////////////////

// mount your current mount
void do_mount_current(char_data *ch) {
	obj_data *saddle;
	char_data *mob;
	
	if (IS_RIDING(ch)) {
		msg_to_char(ch, "You're already mounted.\r\n");
	}
	else if (GET_MOUNT_VNUM(ch) == NOTHING || !mob_proto(GET_MOUNT_VNUM(ch))) {
		msg_to_char(ch, "You don't have a current mount set.\r\n");
	}
	else if (IS_MORPHED(ch)) {
		msg_to_char(ch, "You can't ride anything in this form.\r\n");
	}
	else if (AFF_FLAGGED(ch, AFF_FLY)) {
		msg_to_char(ch, "You can't mount while flying.\r\n");
	}
	else if (AFF_FLAGGED(ch, AFF_SNEAK)) {
		msg_to_char(ch, "You can't mount while sneaking.\r\n");
	}
	else if (IS_COMPLETE(IN_ROOM(ch)) && !BLD_ALLOWS_MOUNTS(IN_ROOM(ch))) {
		msg_to_char(ch, "You can't mount here.\r\n");
	}
	else if (GET_SITTING_ON(ch)) {
		msg_to_char(ch, "You're already sitting %s something.\r\n", IN_OR_ON(GET_SITTING_ON(ch)));
	}
	else if (MOUNT_FLAGGED(ch, MOUNT_FLYING) && !CAN_RIDE_FLYING_MOUNT(ch)) {
		msg_to_char(ch, "You don't have the correct ability to ride %s! (see HELP RIDE)\r\n", get_mob_name_by_proto(GET_MOUNT_VNUM(ch), TRUE));
	}
	else if (run_ability_triggers_by_player_tech(ch, PTECH_RIDING, NULL, NULL)) {
		return;
	}
	else {
		// attempt to use a saddle
		if (!(saddle = GET_EQ(ch, WEAR_SADDLE))) {
			saddle = find_best_saddle(ch);
			if (saddle) {
				equip_char(ch, saddle, WEAR_SADDLE);
				determine_gear_level(ch);
			}
		}
		
		// load a copy of the mount mob, for messaging
		mob = read_mobile(GET_MOUNT_VNUM(ch), TRUE);
		char_to_room(mob, IN_ROOM(ch));
		
		// messaging
		if (saddle) {
			// prevents mounting "someone" in the dark
			sprintf(buf, "You throw on $p and clamber onto %s.", PERS(mob, mob, FALSE));
			act(buf, FALSE, ch, saddle, mob, TO_CHAR);
			
			act("$n throws on $p and clambers onto $N.", TRUE, ch, saddle, mob, TO_NOTVICT);
		}
		else {
			// prevents mounting "someone" in the dark
			sprintf(buf, "You clamber onto %s.", PERS(mob, mob, FALSE));
			act(buf, FALSE, ch, saddle, mob, TO_CHAR);
			
			act("$n clambers onto $N's back.", TRUE, ch, saddle, mob, TO_NOTVICT);
		}
		
		// clear any stale mob flags
		GET_MOUNT_FLAGS(ch) = 0;
		
		// hard work! this will un-load the mob
		perform_mount(ch, mob);
		command_lag(ch, WAIT_OTHER);
	}
}


// list/search mounts
void do_mount_list(char_data *ch, char *argument) {
	extern const char *mount_flags[];
	
	char buf[MAX_STRING_LENGTH], part[MAX_STRING_LENGTH], temp[MAX_STRING_LENGTH];
	struct mount_data *mount, *next_mount;
	bool any = FALSE, cur;
	char_data *proto;
	size_t size = 0;
	int count = 0;
	
	if (!GET_MOUNT_LIST(ch)) {
		msg_to_char(ch, "You have no mounts.\r\n");
		return;
	}
	
	// header
	if (!*argument) {
		size = snprintf(buf, sizeof(buf), "Your mounts:\r\n");
	}
	else {
		size = snprintf(buf, sizeof(buf), "Your mounts matching '%s':\r\n", argument);
	}
	
	HASH_ITER(hh, GET_MOUNT_LIST(ch), mount, next_mount) {
		if (size >= sizeof(buf)) {	// overflow
			break;
		}
		if (!(proto = mob_proto(mount->vnum))) {
			continue;
		}
		if (*argument && !multi_isname(argument, GET_PC_NAME(proto))) {
			continue;
		}
		
		// build line
		cur = (GET_MOUNT_VNUM(ch) == mount->vnum);
		if (mount->flags) {
			prettier_sprintbit(mount->flags, mount_flags, temp);
			snprintf(part, sizeof(part), "%s (%s)%s", skip_filler(GET_SHORT_DESC(proto)), temp, (cur && PRF_FLAGGED(ch, PRF_SCREEN_READER) ? " [current]" : ""));
		}
		else {
			snprintf(part, sizeof(part), "%s%s", skip_filler(GET_SHORT_DESC(proto)), (cur && PRF_FLAGGED(ch, PRF_SCREEN_READER) ? " [current]" : ""));
		}
		
		++count;
		size += snprintf(buf + size, sizeof(buf) - size, " %s%-38s%s%s", (cur ? "&l" : ""), part, (cur ? "&0" : ""), PRF_FLAGGED(ch, PRF_SCREEN_READER) ? "\r\n" : (!(count % 2) ? "\r\n" : " "));
		any = TRUE;
	}
	
	if (!PRF_FLAGGED(ch, PRF_SCREEN_READER) && (count % 2)) {
		size += snprintf(buf + size, sizeof(buf) - size, "\r\n");
	}
	
	if (!any) {
		size += snprintf(buf + size, sizeof(buf) - size, " no matches\r\n");
	}
	else {
		size += snprintf(buf + size, sizeof(buf) - size, " (%d total mount%s)\r\n", count, PLURAL(count));
	}
	
	if (ch->desc) {
		page_string(ch->desc, buf, TRUE);
	}
}


// attempt to add a mount
void do_mount_new(char_data *ch, char *argument) {
	struct mount_data *mount;
	char_data *mob, *proto;
	bool only;
	
	if (!can_use_room(ch, IN_ROOM(ch), MEMBERS_ONLY)) {
		msg_to_char(ch, "You don't have permission to mount anything here!\r\n");
	}
	else if (!*argument) {
		msg_to_char(ch, "What did you want to mount?\r\n");
	}
	else if (!(mob = get_char_vis(ch, argument, FIND_CHAR_ROOM))) {
		// special case: mount/ride a vehicle
		if (get_vehicle_in_room_vis(ch, arg)) {
			void do_sit_on_vehicle(char_data *ch, char *argument);
			do_sit_on_vehicle(ch, arg);
		}
		else {
			send_config_msg(ch, "no_person");
		}
	}
	else if (ch == mob) {
		msg_to_char(ch, "You can't mount yourself.\r\n");
	}
	else if (!IS_NPC(mob)) {
		msg_to_char(ch, "You can't ride on other players.\r\n");
	}
	else if (find_mount_data(ch, GET_MOB_VNUM(mob))) {
		act("You already have $N in your stable.", FALSE, ch, NULL, mob, TO_CHAR);
	}
	else if (!MOB_FLAGGED(mob, MOB_MOUNTABLE) && !IS_IMMORTAL(ch)) {
		act("You can't ride $N!", FALSE, ch, 0, mob, TO_CHAR);
	}
	else if (AFF_FLAGGED(mob, AFF_FLY) && !CAN_RIDE_FLYING_MOUNT(ch)) {
		act("You don't have the correct ability to ride $N! (see HELP RIDE)", FALSE, ch, 0, mob, TO_CHAR);
	}
	else if (mob->desc || (GET_PC_NAME(mob) && (proto = mob_proto(GET_MOB_VNUM(mob))) && GET_PC_NAME(mob) != GET_PC_NAME(proto))) {
		act("You can't ride $N!", FALSE, ch, 0, mob, TO_CHAR);
	}
	else if (GET_LED_BY(mob)) {
		msg_to_char(ch, "You can't ride someone who's being led around.\r\n");
	}
	else if (GET_POS(mob) < POS_STANDING) {
		act("You can't mount $N right now.", FALSE, ch, NULL, mob, TO_CHAR);
	}
	else if (run_ability_triggers_by_player_tech(ch, PTECH_RIDING, mob, NULL)) {
		return;
	}
	else {
		// will immediately attempt to ride if they have no current mount
		only = (GET_MOUNT_VNUM(ch) == NOTHING);
		
		// add mob to pool
		add_mount(ch, GET_MOB_VNUM(mob), get_mount_flags_by_mob(mob));
		
		if (only && (mount = find_mount_data(ch, GET_MOB_VNUM(mob)))) {
			// NOTE: this deliberately has no carriage return (will get another message from do_mount_current)
			msg_to_char(ch, "You gain %s as a mount and attempt to ride %s: ", PERS(mob, mob, FALSE), HMHR(mob));
			act("$n gains $N as a mount.", FALSE, ch, NULL, mob, TO_NOTVICT);
			
			GET_MOUNT_VNUM(ch) = mount->vnum;
			GET_MOUNT_FLAGS(ch) = mount->flags;
			do_mount_current(ch);
		}
		else {	// has other mobs
			act("You gain $N as a mount and send $M back to your stable.", FALSE, ch, NULL, mob, TO_CHAR);
			act("$n gains $N as a mount and sends $M back to $s stable.", FALSE, ch, NULL, mob, TO_NOTVICT);
		}
		
		// remove mob
		extract_char(mob);
	}
}


// release your current mount
void do_mount_release(char_data *ch, char *argument) {
	void setup_generic_npc(char_data *mob, empire_data *emp, int name, int sex);
	
	struct mount_data *mount;
	char_data *mob;
	
	if (!can_use_room(ch, IN_ROOM(ch), MEMBERS_ONLY)) {
		msg_to_char(ch, "You don't have permission to release mounts here (you wouldn't be able to re-mount it)!\r\n");
	}
	else if (!has_ability(ch, ABIL_STABLEMASTER)) {
		msg_to_char(ch, "You need the Stablemaster ability to release a mount.\r\n");
	}
	else if (*argument) {
		msg_to_char(ch, "You can only release your active mount (you get this error if you type a name).\r\n");
	}
	else if (GET_MOUNT_VNUM(ch) == NOTHING || !mob_proto(GET_MOUNT_VNUM(ch))) {
		msg_to_char(ch, "You have no active mount to release.\r\n");
	}
	else {
		if (IS_RIDING(ch)) {
			do_dismount(ch, "", 0, 0);
		}
		
		mob = read_mobile(GET_MOUNT_VNUM(ch), TRUE);
		char_to_room(mob, IN_ROOM(ch));
		setup_generic_npc(mob, GET_LOYALTY(ch), NOTHING, NOTHING);
		SET_BIT(AFF_FLAGS(mob), AFF_NO_DRINK_BLOOD);
		SET_BIT(MOB_FLAGS(mob), MOB_NO_EXPERIENCE);
		
		act("You drop $N's lead and release $M.", FALSE, ch, NULL, mob, TO_CHAR);
		act("$n drops $N's lead and releases $M.", TRUE, ch, NULL, mob, TO_NOTVICT);
		
		// remove data
		if ((mount = find_mount_data(ch, GET_MOB_VNUM(mob)))) {
			HASH_DEL(GET_MOUNT_LIST(ch), mount);
			free(mount);
		}
		
		// unset current-mount
		GET_MOUNT_VNUM(ch) = NOTHING;
		GET_MOUNT_FLAGS(ch) = NOBITS;
		queue_delayed_update(ch, CDU_SAVE);	// prevent mob duplication
		
		load_mtrigger(mob);
	}
}


// change to a stabled mount
void do_mount_swap(char_data *ch, char *argument) {
	struct mount_data *mount, *iter, *next_iter;
	char tmpname[MAX_INPUT_LENGTH], *tmp = tmpname;
	bool was_mounted = FALSE;
	char_data *proto;
	int number;
	
	if (!has_ability(ch, ABIL_STABLEMASTER) && !room_has_function_and_city_ok(IN_ROOM(ch), FNC_STABLE)) {
		msg_to_char(ch, "You can only swap mounts in a stable unless you have the Stablemaster ability.\r\n");
		return;
	}
	if (!has_ability(ch, ABIL_STABLEMASTER) && !check_in_city_requirement(IN_ROOM(ch), TRUE)) {
		msg_to_char(ch, "This stable must be in a city for you to swap mounts without the Stablemaster ability.\r\n");
		return;
	}
	if (!has_ability(ch, ABIL_STABLEMASTER) && !IS_COMPLETE(IN_ROOM(ch))) {
		msg_to_char(ch, "You must complete the stable first.\r\n");
		return;
	}
	if (!has_ability(ch, ABIL_STABLEMASTER) && !check_in_city_requirement(IN_ROOM(ch), TRUE)) {
		msg_to_char(ch, "This building must be in a city to swap mounts here.\r\n");
		return;
	}
	if (!has_ability(ch, ABIL_STABLEMASTER) && !can_use_room(ch, IN_ROOM(ch), GUESTS_ALLOWED)) {
		msg_to_char(ch, "You don't have permission to mount anything here!\r\n");
		return;
	}
	if (!*argument) {
		msg_to_char(ch, "Swap to which mount?\r\n");
		return;
	}
	
	// look up mount by argument
	mount = NULL;
	strcpy(tmp, argument);
	number = get_number(&tmp);
	HASH_ITER(hh, GET_MOUNT_LIST(ch), iter, next_iter) {
		if (!(proto = mob_proto(iter->vnum))) {
			continue;
		}
		if (!multi_isname(tmp, GET_PC_NAME(proto))) {
			continue;
		}
		
		// match (barring #.x)
		if (--number != 0) {
			continue;
		}
		
		// match!
		mount = iter;
		break;
	}
	
	// did we find one?
	if (!mount) {
		msg_to_char(ch, "You don't seem to have a mount called '%s'.\r\n", argument);
		return;
	}
	if (mount->vnum == GET_MOUNT_VNUM(ch)) {
		msg_to_char(ch, "You're already using that mount.\r\n");
		return;
	}
	
	// Ok go:
	if (IS_RIDING(ch)) {
		do_dismount(ch, "", 0, 0);
		was_mounted = TRUE;
	}
	
	// change current mount to that
	GET_MOUNT_VNUM(ch) = mount->vnum;
	GET_MOUNT_FLAGS(ch) = mount->flags;
	msg_to_char(ch, "You change your active mount to %s.\r\n", get_mob_name_by_proto(mount->vnum, TRUE));
	
	if (was_mounted) {
		do_mount_current(ch);
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// COMMANDS ////////////////////////////////////////////////////////////////

ACMD(do_butcher) {
	char_data *proto;
	obj_data *corpse;
	
	one_argument(argument, arg);
	
	if (!*arg) {
		msg_to_char(ch, "What would you like to butcher?\r\n");
		return;
	}
	
	// find in inventory
	corpse = get_obj_in_list_vis(ch, arg, ch->carrying);
	
	// find in room
	if (!corpse) {
		corpse = get_obj_in_list_vis(ch, arg, ROOM_CONTENTS(IN_ROOM(ch)));
	}
	
	if (!corpse) {
		msg_to_char(ch, "You don't see a %s here.\r\n", arg);
	}
	else if (!IS_CORPSE(corpse)) {
		msg_to_char(ch, "You can only butcher a corpse.\r\n");
	}
	else if (!bind_ok(corpse, ch)) {
		msg_to_char(ch, "You can't butcher a corpse that is bound to someone else.\r\n");
	}
	else if (GET_CORPSE_NPC_VNUM(corpse) == NOTHING || !(proto = mob_proto(GET_CORPSE_NPC_VNUM(corpse)))) {
		msg_to_char(ch, "You can't get any good meat out of that.\r\n");
	}
	else if (!has_tool(ch, TOOL_KNIFE)) {
		msg_to_char(ch, "You need to equip a good knife to butcher with.\r\n");
	}
	else if (run_ability_triggers_by_player_tech(ch, PTECH_BUTCHER_UPGRADE, NULL, corpse)) {
		return;
	}
	else {
		if (!IS_SET(GET_CORPSE_FLAGS(corpse), CORPSE_NO_LOOT) && run_interactions(ch, proto->interactions, INTERACT_BUTCHER, IN_ROOM(ch), NULL, corpse, butcher_interact)) {
			// success
			gain_player_tech_exp(ch, PTECH_BUTCHER_UPGRADE, 15);
		}
		else {
			act("You butcher $p but get no useful meat.", FALSE, ch, corpse, NULL, TO_CHAR);
			act("$n butchers $p but gets no useful meat.", FALSE, ch, corpse, NULL, TO_ROOM);
		}
		
		empty_obj_before_extract(corpse);
		extract_obj(corpse);
		command_lag(ch, WAIT_OTHER);
	}
}


ACMD(do_dismount) {
	void do_unseat_from_vehicle(char_data *ch);
	
	char_data *mount;
	
	if (IS_RIDING(ch)) {
		mount = mob_proto(GET_MOUNT_VNUM(ch));
		
		msg_to_char(ch, "You jump down off of %s.\r\n", mount ? GET_SHORT_DESC(mount) : "your mount");
		
		sprintf(buf, "$n jumps down off of %s.", mount ? GET_SHORT_DESC(mount) : "$s mount");
		act(buf, FALSE, ch, NULL, NULL, TO_ROOM);
		
		perform_dismount(ch);
	}
	else if (GET_SITTING_ON(ch)) {
		do_unseat_from_vehicle(ch);
	}
	else {
		msg_to_char(ch, "You're not riding anything right now.\r\n");
	}
}


ACMD(do_fish) {
	extern const char *dirs[];
	
	room_data *room = IN_ROOM(ch);
	char buf[MAX_STRING_LENGTH];
	int dir = NO_DIR;
	
	any_one_arg(argument, arg);
	
	if (IS_NPC(ch)) {
		msg_to_char(ch, "You can't fish.\r\n");
	}
	else if (GET_ACTION(ch) == ACT_FISHING && !*arg) {
		msg_to_char(ch, "You stop fishing.\r\n");
		act("$n stops fishing.", TRUE, ch, 0, 0, TO_ROOM);
		cancel_action(ch);
	}
	else if (FIGHTING(ch) && GET_POS(ch) == POS_FIGHTING) {
		msg_to_char(ch, "You can't do that now!\r\n");
	}
	else if (!has_player_tech(ch, PTECH_FISH)) {
		msg_to_char(ch, "You don't have the correct ability to fish for anything.\r\n");
	}
	else if (!can_use_ability(ch, NOTHING, NOTHING, 0, NOTHING)) {
		// own messages
	}
	else if (GET_ACTION(ch) != ACT_NONE && GET_ACTION(ch) != ACT_FISHING) {
		msg_to_char(ch, "You're really too busy to do that.\r\n");
	}
	else if (!CAN_SEE_IN_DARK_ROOM(ch, IN_ROOM(ch))) {
		msg_to_char(ch, "It's too dark to fish for anything here.\r\n");
	}
	else if (*arg && (dir = parse_direction(ch, arg)) == NO_DIR) {
		msg_to_char(ch, "Fish in what direction?\r\n");
	}
	else if (dir != NO_DIR && !(room = dir_to_room(IN_ROOM(ch), dir, FALSE))) {
		msg_to_char(ch, "You can't fish in that direction.\r\n");
	}
	else if (!can_interact_room(room, INTERACT_FISH)) {
		msg_to_char(ch, "You can't fish for anything %s!\r\n", (room == IN_ROOM(ch)) ? "here" : "there");
	}
	else if (!can_use_room(ch, room, MEMBERS_ONLY)) {
		msg_to_char(ch, "You don't have permission to fish %s.\r\n", (room == IN_ROOM(ch)) ? "here" : "there");
	}
	else if (!has_tool(ch, TOOL_FISHING)) {
		msg_to_char(ch, "You aren't using any fishing equipment.\r\n");
	}
	else if (run_ability_triggers_by_player_tech(ch, PTECH_FISH, NULL, NULL)) {
		return;
	}
	else {
		if (dir != NO_DIR) {
			sprintf(buf, " to the %s", dirs[get_direction_for_char(ch, dir)]);
		}
		else {
			*buf = '\0';
		}
		
		msg_to_char(ch, "You begin looking for fish%s...\r\n", buf);
		act("$n begins looking for fish.", TRUE, ch, NULL, NULL, TO_ROOM);
		
		start_action(ch, ACT_FISHING, config_get_int("fishing_timer") / (player_tech_skill_check(ch, PTECH_FISH, DIFF_EASY) ? 2 : 1));
		GET_ACTION_VNUM(ch, 0) = dir;
	}
}


ACMD(do_hunt) {
	struct hunt_helper *helpers = NULL, *item, *next_item;
	struct spawn_info *spawn, *found_spawn = NULL;
	char_data *mob, *found_proto = NULL;
	struct hunt_global_bean *data;
	vehicle_data *veh, *next_veh;
	bool junk, non_animal = FALSE;
	int count, x_coord, y_coord;
	
	double min_percent = 1.0;	// won't find things below 1% spawn
	
	skip_spaces(&argument);
	
	if (GET_ACTION(ch) == ACT_HUNTING && !*argument) {
		msg_to_char(ch, "You stop hunting.\r\n");
		cancel_action(ch);
		return;
	}
	if (!has_player_tech(ch, PTECH_HUNT_ANIMALS)) {
		msg_to_char(ch, "You don't have the right ability to hunt anything.\r\n");
		return;
	}
	if (!CAN_SEE_IN_DARK_ROOM(ch, IN_ROOM(ch))) {
		msg_to_char(ch, "It's too dark to hunt anything right now.\r\n");
		return;
	}
	if (!IS_OUTDOORS(ch)) {
		msg_to_char(ch, "You can only hunt while outdoors.\r\n");
		return;
	}
	if (ROOM_OWNER(IN_ROOM(ch)) && is_in_city_for_empire(IN_ROOM(ch), ROOM_OWNER(IN_ROOM(ch)), TRUE, &junk)) {
		// note: if you remove the in-city restriction, the validate_spawn_location() below will need in-city info
		msg_to_char(ch, "You can't hunt in cities.\r\n");
		return;
	}
	if (AFF_FLAGGED(ch, AFF_ENTANGLED)) {
		msg_to_char(ch, "You can't hunt anything while entangled.\r\n");
		return;
	}
	if (GET_ACTION(ch) != ACT_NONE && GET_ACTION(ch) != ACT_HUNTING) {
		msg_to_char(ch, "You're too busy to hunt right now.\r\n");
		return;
	}
	if (!*argument) {
		msg_to_char(ch, "Hunt what?\r\n");
		return;
	}
	
	// count how many people are in the room and also check for a matching animal here
	count = 0;
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(ch)), mob, next_in_room) {
		++count;
		
		if (!IS_NPC(mob) || !MOB_FLAGGED(mob, MOB_ANIMAL)) {
			continue;
		}
		
		if (multi_isname(argument, GET_PC_NAME(mob))) {
			act("You can see $N right here!", FALSE, ch, NULL, mob, TO_CHAR);
			return;
		}
	}
	
	if (count > 4) {
		msg_to_char(ch, "The area is too crowded to hunt for anything.\r\n");
		return;
	}
	
	// build lists: vehicles
	DL_FOREACH_SAFE2(ROOM_VEHICLES(IN_ROOM(ch)), veh, next_veh, next_in_room) {
		if (VEH_SPAWNS(veh)) {
			CREATE(item, struct hunt_helper, 1);
			item->list = VEH_SPAWNS(veh);
			DL_PREPEND(helpers, item);
		}
	}
	// build lists: building
	if (GET_BUILDING(IN_ROOM(ch))) {
		// only find a spawn list here if the building is complete; otherwise no list = no spawn
		if (IS_COMPLETE(IN_ROOM(ch)) && GET_BLD_SPAWNS(GET_BUILDING(IN_ROOM(ch)))) {
			CREATE(item, struct hunt_helper, 1);
			item->list = GET_BLD_SPAWNS(GET_BUILDING(IN_ROOM(ch)));
			DL_PREPEND(helpers, item);
		}
	}
	// build lists: crop
	else if (ROOM_SECT_FLAGGED(IN_ROOM(ch), SECTF_CROP) && ROOM_CROP(IN_ROOM(ch))) {
		CREATE(item, struct hunt_helper, 1);
		item->list = GET_CROP_SPAWNS(ROOM_CROP(IN_ROOM(ch)));
		DL_PREPEND(helpers, item);
	}
	// build lists: sect
	else {
		CREATE(item, struct hunt_helper, 1);
		item->list = GET_SECT_SPAWNS(SECT(IN_ROOM(ch)));
		DL_PREPEND(helpers, item);
	}
	
	// prepare data for validation (calling these here prevents multiple function calls)
	x_coord = X_COORD(IN_ROOM(ch));
	y_coord = Y_COORD(IN_ROOM(ch));
	
	// build lists: global spawns
	CREATE(data, struct hunt_global_bean, 1);
	data->room = IN_ROOM(ch);
	data->x_coord = x_coord;
	data->y_coord = y_coord;
	data->helpers = &helpers;	// reference current list
	run_globals(GLOBAL_MAP_SPAWNS, run_global_hunt_for_map_spawns, TRUE, GET_SECT_CLIMATE(BASE_SECT(IN_ROOM(ch))), NULL, NULL, 0, validate_global_hunt_for_map_spawns, data);
	free(data);
	
	// find the thing to hunt
	DL_FOREACH_SAFE(helpers, item, next_item) {
		LL_FOREACH(item->list, spawn) {
			if (spawn->percent < min_percent) {
				continue;	// too low
			}
			if (!validate_spawn_location(IN_ROOM(ch), spawn->flags, x_coord, y_coord, FALSE)) {
				continue;	// cannot spawn here
			}
			if (!(mob = mob_proto(spawn->vnum))) {
				continue;	// no proto
			}
			if (!multi_isname(argument, GET_PC_NAME(mob))) {
				continue;	// name mismatch
			}
			if (!MOB_FLAGGED(mob, MOB_ANIMAL)) {
				non_animal = TRUE;
				continue;	// only animals	-- check this last because it triggers an error message
			}
			
			// seems ok:
			found_proto = mob;
			found_spawn = spawn;	// records the percent etc
		}
		
		// and free the temporary data while we're here
		DL_DELETE(helpers, item);
		free(item);
	}
	
	// did we find anything?
	if (found_proto) {
		act("You see signs that $N has been here recently, and crouch low to stalk it.", FALSE, ch, NULL, found_proto, TO_CHAR);
		act("$n crouches low and begins to hunt.", TRUE, ch, NULL, NULL, TO_ROOM);
		
		start_action(ch, ACT_HUNTING, 0);
		GET_ACTION_VNUM(ch, 0) = GET_MOB_VNUM(found_proto);
		GET_ACTION_VNUM(ch, 1) = found_spawn->percent * 100;	// 10000 = 100.00%
	}
	else if (non_animal) {	// and also not success
		msg_to_char(ch, "You can only hunt animals.\r\n");
	}
	else {
		msg_to_char(ch, "You can't find a trail for anything like that here.\r\n");
	}
}


ACMD(do_mount) {
	char arg[MAX_INPUT_LENGTH];
	
	argument = one_argument(argument, arg);
	skip_spaces(&argument);
	
	if (IS_NPC(ch)) {
		msg_to_char(ch, "You can't ride anything!\r\n");
	}
	else if (!has_player_tech(ch, PTECH_RIDING)) {
		msg_to_char(ch, "You don't have the ability to ride anything.\r\n");
	}
	
	// list requires no position
	else if (!str_cmp(arg, "list") || !str_cmp(arg, "search")) {
		do_mount_list(ch, argument);
	}
	
	else if (GET_POS(ch) < POS_STANDING) {
		msg_to_char(ch, "You can't do that right now.\r\n");
	}
	
	// other sub-commands require standing
	else if (!*arg) {
		do_mount_current(ch);
	}
	else if (!str_cmp(arg, "swap") || !str_cmp(arg, "change")) {
		do_mount_swap(ch, argument);
	}
	else if (!str_cmp(arg, "release")) {
		do_mount_release(ch, argument);
	}
	else {
		// arg provided
		do_mount_new(ch, arg);
	}
}


ACMD(do_track) {
	extern const char *dirs[];
	
	char_data *vict, *proto;
	struct track_data *track;
	bool found = FALSE;
	byte dir = NO_DIR;
	
	int tracks_lifespan = config_get_int("tracks_lifespan");
	
	one_argument(argument, arg);
	
	if (!can_use_ability(ch, ABIL_TRACK, NOTHING, 0, NOTHING)) {
		return;
	}
	else if (!CAN_SEE_IN_DARK_ROOM(ch, IN_ROOM(ch))) {
		msg_to_char(ch, "It's too dark to track for anything here.\r\n");
		return;
	}
	else if (!*arg) {
		msg_to_char(ch, "Track whom? Or what?\r\n");
		return;
	}
	else if (ABILITY_TRIGGERS(ch, NULL, NULL, ABIL_TRACK)) {
		return;
	}

	for (track = ROOM_TRACKS(IN_ROOM(ch)); !found && track; track = track->next) {
		// skip already-expired tracks
		if (time(0) - track->timestamp > tracks_lifespan * SECS_PER_REAL_MIN) {
			continue;
		}
		
		if (track->player_id != NOTHING && (vict = is_playing(track->player_id))) {
			// TODO: this is pretty similar to the MATCH macro in handler.c and could be converted to use it
			if (isname(arg, GET_PC_NAME(vict)) || isname(arg, PERS(vict, vict, 0)) || isname(arg, PERS(vict, vict, 1)) || (!IS_NPC(vict) && GET_LASTNAME(vict) && isname(arg, GET_LASTNAME(vict)))) {
				found = TRUE;
				dir = track->dir;
			}
		}
		else if (track->mob_num != NOTHING && (proto = mob_proto(track->mob_num)) && isname(arg, GET_PC_NAME(proto))) {
			found = TRUE;
			dir = track->dir;
		}
	}
	
	if (found) {
		msg_to_char(ch, "You find a trail heading %s!\r\n", dirs[get_direction_for_char(ch, dir)]);
		gain_ability_exp(ch, ABIL_TRACK, 20);
	}
	else {
		msg_to_char(ch, "You can't seem to find a trail.\r\n");
	}
	
	command_lag(ch, WAIT_ABILITY);
}
