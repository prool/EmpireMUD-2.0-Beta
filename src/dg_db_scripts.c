/* ************************************************************************
*   File: dg_db_scripts.c                                 EmpireMUD 2.0b5 *
*  Usage: Contains routines to handle db functions for scripts and trigs  *
*                                                                         *
*  DG Scripts code by egreen, 1996/09/30 21:27:54, revision 3.7           *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2024                      *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  EmpireMUD based upon CircleMUD 3.0, bpl 17, by Jeremy Elson.           *
*  Death's Gate MUD is based on CircleMUD, Copyright (C) 1993, 94.        *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "dg_scripts.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "dg_event.h"
#include "comm.h"
#include "olc.h"
#include "interpreter.h"

// external vars
extern int top_script_uid;


/**
* Uses strtok() to compile a list of trigger commands.
*
* @param char *input The raw trigger script.
* @return struc cmdlist_element* The compiled command list.
*/
struct cmdlist_element *compile_command_list(char *input) {
	struct cmdlist_element *list, *cmd;
	
	CREATE(list, struct cmdlist_element, 1);
	if (input) {
		char *t = strtok(input, "\n\r"); /* strtok returns NULL if str is "\r\n" */
		if (t)
			list->cmd = strdup(t);
		else
			list->cmd = strdup("* No script");

		cmd = list;
		while ((input = strtok(NULL, "\n\r"))) {
			CREATE(cmd->next, struct cmdlist_element, 1);
			cmd = cmd->next;
			cmd->cmd = strdup(input);
		}
	}
	else {
		list->cmd = strdup("* No Script");
	}

	return list;
}


void parse_trigger(FILE *trig_f, int nr) {
	int t[2], k, attach_type;
	char line[256], *cmds, *s, flags[256], errors[MAX_INPUT_LENGTH];
	struct cmdlist_element *cle;
	trig_data *trig;

	CREATE(trig, trig_data, 1);
	trig->vnum = nr;
	add_trigger_to_table(trig);

	safe_snprintf(errors, sizeof(errors), "trig vnum %d", nr);

	trig->name = fread_string(trig_f, errors);

	get_line(trig_f, line);
	k = sscanf(line, "%d %s %d", &attach_type, flags, t);
	trig->attach_type = (byte)attach_type;
	trig->trigger_type = asciiflag_conv(flags);
	trig->narg = (k == 3) ? t[0] : 0;

	trig->arglist = fread_string(trig_f, errors);

	cmds = s = fread_string(trig_f, errors);

	CREATE(trig->cmdlist, struct cmdlist_element, 1);
	trig->cmdlist->cmd = strdup(strtok(s, "\n\r"));
	cle = trig->cmdlist;

	while ((s = strtok(NULL, "\n\r"))) {
		CREATE(cle->next, struct cmdlist_element, 1);
		cle = cle->next;
		cle->cmd = strdup(s);
	}

	free(cmds);
}


/**
* create a new trigger from a prototype.
*
* @param trig_vnum vnum Trigger to instantiate
* @return trig_data* The trigger, or NULL if it doesn't exit
*/
trig_data *read_trigger(trig_vnum vnum) {
	trig_data *proto, *trig;

	if (vnum < 0 || vnum == NOTHING) {
		return NULL;
	}
	if ((proto = real_trigger(vnum)) == NULL) {
		return NULL;
	}

	CREATE(trig, trig_data, 1);
	trig_data_copy(trig, proto);

	return trig;
}


void trig_data_init(trig_data *this_data) {
	this_data->vnum = NOTHING;
	this_data->data_type = 0;
	this_data->name = NULL;
	this_data->trigger_type = 0;
	this_data->cmdlist = NULL;
	this_data->curr_state = NULL;
	this_data->narg = 0;
	this_data->arglist = NULL;
	this_data->depth = 0;
	this_data->wait_event = NULL;
	this_data->purged = FALSE;
	this_data->var_list = NULL;

	this_data->next = NULL;  
}


void trig_data_copy(trig_data *this_data, const trig_data *trg) {
	trig_data_init(this_data);

	this_data->vnum = trg->vnum;
	this_data->attach_type = trg->attach_type;
	this_data->data_type = trg->data_type;
	if (trg->name) 
		this_data->name = strdup(trg->name);
	else {
		this_data->name = strdup("unnamed trigger");
		log("Trigger with no name! (%d)", trg->vnum);
	}
	this_data->trigger_type = trg->trigger_type;
	this_data->cmdlist = trg->cmdlist;
	this_data->narg = trg->narg;
	if (trg->arglist)
		this_data->arglist = strdup(trg->arglist);
	
	this_data->purge_tracker = NULL;
}


/**
* Parses a trigger proto from a line already read from a file. These genrally
* appear as:
*
* T 12345
*
* @param char *line The line already read from the file.
* @param struct trig_proto_list **list The list to append to.
* @param char *error_str How to report if there is an error.
*/
void parse_trig_proto(char *line, struct trig_proto_list **list, char *error_str) {
	struct trig_proto_list *trig;
	any_vnum vnum;
	char junk;
	
	if (!line || !list) {
		log("SYSERR: parse_trig_proto called without %s, from %s", line ? "list" : "line", error_str ? error_str : "UNKNOWN");
		exit(1);
	}
	if (sscanf(line, "%c %d", &junk, &vnum) != 2) {
		log("SYSERR: format error in trigger line of %s", error_str ? error_str : "UNKNOWN");
		exit(1);
	}
	
	CREATE(trig, struct trig_proto_list, 1);
	trig->vnum = vnum;
	
	LL_APPEND(*list, trig);
}


void dg_obj_trigger(char *line, obj_data *obj) {
	char junk[8];
	int vnum, count;
	trig_data *trproto;
	struct trig_proto_list *new_trg;

	count = sscanf(line, "%s %d", junk, &vnum);

	if (count != 2) {
		syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: dg_obj_trigger() : Error assigning trigger! - Line was:\n  %s", line);
		return;
	}

	trproto = real_trigger(vnum);
	if (!trproto) {
		syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: Trigger vnum #%d asked for but non-existant! (Object: %s - %d)", vnum, GET_OBJ_SHORT_DESC(obj), GET_OBJ_VNUM(obj));
		return;
	}

	CREATE(new_trg, struct trig_proto_list, 1);
	new_trg->vnum = vnum;
	LL_APPEND(obj->proto_script, new_trg);
}

void assign_triggers(void *i, int type) {
	vehicle_data *veh = NULL;
	char_data *mob = NULL;
	obj_data *obj = NULL;
	room_data *room = NULL;
	trig_data *trproto;
	struct trig_proto_list *trg_proto;

	switch (type) {
		case MOB_TRIGGER:
			mob = (char_data*)i;
			trg_proto = mob->proto_script;
			while (trg_proto) {
				trproto = real_trigger(trg_proto->vnum);
				if (!trproto) {
					syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: trigger #%d non-existant, for mob #%d", trg_proto->vnum, GET_MOB_VNUM(mob));
				}
				else {
					if (!SCRIPT(mob)) {
						create_script_data(mob, MOB_TRIGGER);
					}
					add_trigger(SCRIPT(mob), read_trigger(trg_proto->vnum), -1);
				}
				trg_proto = trg_proto->next;
			}
			reread_companion_trigs(mob);
			request_char_save_in_world(mob);
			break;
		case OBJ_TRIGGER:
			obj = (obj_data*)i;
			trg_proto = obj->proto_script;
			while (trg_proto) {
				trproto = real_trigger(trg_proto->vnum);
				if (!trproto) {
					log("SYSERR: trigger #%d non-existant, for obj #%d", trg_proto->vnum, obj->vnum);
				}
				else {
					if (!SCRIPT(obj)) {
						create_script_data(obj, OBJ_TRIGGER);
					}
					add_trigger(SCRIPT(obj), read_trigger(trg_proto->vnum), -1);
				}
				trg_proto = trg_proto->next;
			}
			request_obj_save_in_world(obj);
			break;
		case WLD_TRIGGER:
		case ADV_TRIGGER:
		case RMT_TRIGGER:
		case BLD_TRIGGER:
			room = (room_data*)i;
			trg_proto = room->proto_script;
			while (trg_proto) {
				trproto = real_trigger(trg_proto->vnum);
				if (!trproto) {
					syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: trigger #%d non-existant, for room #%d",
					trg_proto->vnum, GET_ROOM_VNUM(room));
				}
				else {
					if (!SCRIPT(room)) {
						create_script_data(room, WLD_TRIGGER);
					}
					add_trigger(SCRIPT(room), read_trigger(trg_proto->vnum), -1);
				}
				trg_proto = trg_proto->next;
			}
			request_world_save(GET_ROOM_VNUM(room), WSAVE_ROOM);
			break;
		case VEH_TRIGGER: {
			veh = (vehicle_data*)i;
			trg_proto = veh->proto_script;
			while (trg_proto) {
				trproto = real_trigger(trg_proto->vnum);
				if (!trproto) {
					syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: trigger #%d non-existant, for vehicle #%d", trg_proto->vnum, VEH_VNUM(veh));
				}
				else {
					if (!SCRIPT(veh)) {
						create_script_data(veh, VEH_TRIGGER);
					}
					add_trigger(SCRIPT(veh), read_trigger(trg_proto->vnum), -1);
				}
				trg_proto = trg_proto->next;
			}
			request_vehicle_save_in_world(veh);
			break;
		}
		case EMP_TRIGGER: {
			syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: unable to assign triggers to empires in assign_triggers()");
			break;
		}
		default:
			syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: unknown type for assign_triggers()");
			break;
	}
}


/**
* Fetches the char's script id -- may also set it here if it's not set yet.
*
* @param char_data *ch The character.
* @return int The unique ID.
*/
int char_script_id(char_data *ch) {
	if (ch->script_id == 0 && IS_NPC(ch)) {
		ch->script_id = top_script_uid++;
		add_to_lookup_table(ch->script_id, (void *)ch, TYPE_MOB);
		ch->in_lookup_table = TRUE;
		
		if (top_script_uid == INT_MAX && (!REBOOT_IS_SET() || reboot_control.time > config_get_int("reboot_warning_minutes") + 1)) {
			reboot_control.time = config_get_int("reboot_warning_minutes") + 1;
			if (reboot_control.type != REBOOT_SHUTDOWN) {
				reboot_control.type = REBOOT_REBOOT;
			}
			syslog(SYS_ERROR, 0, TRUE, "SYSERR: Script IDs for mobiles has exceeded the limit, scheduling an auto-reboot");
			top_script_uid = OTHER_ID_BASE;
		}
	}
	else if (ch->script_id == 0 && !IS_NPC(ch) && GET_IDNUM(ch) > 0) {
		ch->script_id = GET_IDNUM(ch);
		add_to_lookup_table(ch->script_id, (void *)ch, TYPE_MOB);
		ch->in_lookup_table = TRUE;
	}
	return ch->script_id;
}


/**
* Fetches the object's script id -- may also set it here if it's not set yet.
*
* @param obj_data *obj The object.
* @return int The unique ID.
*/
int obj_script_id(obj_data *obj) {
	if (obj->script_id == 0) {
		obj->script_id = top_script_uid++;
		add_to_lookup_table(obj->script_id, (void *)obj, TYPE_OBJ);
		
		if (top_script_uid == INT_MAX && (!REBOOT_IS_SET() || reboot_control.time > config_get_int("reboot_warning_minutes") + 1)) {
			reboot_control.time = config_get_int("reboot_warning_minutes") + 1;
			if (reboot_control.type != REBOOT_SHUTDOWN) {
				reboot_control.type = REBOOT_REBOOT;
			}
			syslog(SYS_ERROR, 0, TRUE, "SYSERR: Script IDs for objects has exceeded the limit, scheduling an auto-reboot");
			top_script_uid = OTHER_ID_BASE;
		}
	}
	return obj->script_id;
}


/**
* Fetches the vehicle's script idd -- may also set it here if it's not set yet.
*
* @param vehicle_data *veh The vehicle.
* @return int The unique ID.
*/
int veh_script_id(vehicle_data *veh) {
	if (veh->script_id == 0) {
		// the -1 here is because vehicle idnums start with 1
		veh->script_id = VEHICLE_ID_BASE - 1 + VEH_IDNUM(veh);
		add_to_lookup_table(veh->script_id, (void *)veh, TYPE_VEH);
		
		if (veh->script_id >= ROOM_ID_BASE) {
			// warn
			log("SCRIPT ERR: Vehicle idnum for copy of vehicle [%d] %s is beyond maximum safe vnum for scripts", VEH_VNUM(veh), VEH_SHORT_DESC(veh));
		}
	}
	
	return veh->script_id;
}
