/* ************************************************************************
*   File: dg_triggers.c                                   EmpireMUD 2.0b5 *
*  Usage: contains all the trigger functions for scripts.                 *
*                                                                         *
*  DG Scripts code by galion, 1996/08/05 23:32:08, revision 3.9           *
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
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "olc.h"
#include "dg_event.h"
#include "constants.h"

/**
* Contents:
*   General Functions
*   Mob Triggers
*   Object Triggers
*   World Triggers
*   Vehicle Triggers
*   Combo Triggers
*   Finish Quest Triggers
*   Reset Trigger Helper
*/


 //////////////////////////////////////////////////////////////////////////////
//// GENERAL FUNCTIONS ///////////////////////////////////////////////////////

/*
* Copy first phrase into first_arg, returns rest of string
*
* @param char *arg The incoming phrase.
* @param char *first_arg A buffer to store the first argument.
* @return char* The remaining part of arg.
*/
char *one_phrase(char *arg, char *first_arg) {
	skip_spaces(&arg);

	if (!*arg) {
		*first_arg = '\0';
	}
	else if (*arg == '"') {
		char *p, c;

		p = matching_quote(arg);
		c = *p;
		*p = '\0';
		strcpy(first_arg, arg + 1);
		if (c == '\0') {
			return p;
		}
		else {
			return p + 1;
		}
	}
	else {
		char *s, *p;

		s = first_arg;
		p = arg;

		while (*p && !isspace(*p) && *p != '"') {
			*s++ = *p++;
		}

		*s = '\0';
		return p;
	}

	return arg;
}


/**
* Determines if one string is a substring of the other.
*
* @param char *sub The part to try to find in 'string'.
* @param char *string The full string to search.
* @return int 1 if it's a substring; 0 if not.
*/
int is_substring(char *sub, char *string) {
	char *s;

	if ((s = str_str(string, sub))) {
		int len = strlen(string);
		int sublen = strlen(sub);

		if ((s == string || isspace(*(s - 1)) || ispunct(*(s - 1))) && ((s + sublen == string + len) || isspace(s[sublen]) || ispunct(s[sublen]))) {
			return 1;
		}
	}

	return 0;
}


/**
* Attempts to match a input to a command trigger.
*
* @param char *input The player's command input (without args).
* @param char *match The text to match (one or more words).
* @param int mode CMDTRG_EXACT or CMDTRG_ABBREV.
* @return bool TRUE if the input matches, FALSE if not.
*/
bool match_command_trig(char *input, char *match, bool mode) {
	char temp[MAX_STRING_LENGTH];
	
	if (!input || !match) {	// missing input
		return FALSE;
	}
	
	skip_spaces(&match);
	
	if (*match == '*') {	// match anything
		return TRUE;
	}
	if (!strchr(match, ' ')) {	// no spaces? simple match
		if (mode == CMDTRG_EXACT) {
			return !str_cmp(input, match);
		}
		else if (mode == CMDTRG_ABBREV) {
			return (is_abbrev(input, match) && str_cmp(input, match));
		}
	}
	else {	// has spaces (multiple possible words)
		char buffer[MAX_INPUT_LENGTH], word[MAX_INPUT_LENGTH];
		strcpy(buffer, match);
		while (*buffer) {
			half_chop(buffer, word, temp);
			strcpy(buffer, temp);
			if ((mode == CMDTRG_EXACT && !str_cmp(input, word)) || (mode == CMDTRG_ABBREV && is_abbrev(input, word) && str_cmp(input, word))) {
				return TRUE;
			}
		}
	}
	
	// no matches
	return FALSE;
}


/*
* Checks if a word from wordlist is in a string. If wordlist is NULL, then it
* returns 1; if str is NULL, returns 0.
*
* @param char *str The string to search.
* @param char *wordlist A list of words to check. Phrases are in "double quotes". An asterisk matches anything.
* @return bool 1 if a word or phrase from wordlist is in 'str'; 0 if not.
*/
int word_check(char *str, char *wordlist) {
	char words[MAX_INPUT_LENGTH], phrase[MAX_INPUT_LENGTH], *s;

	if (*wordlist == '*') {
		return 1;
	}

	strcpy(words, wordlist);

	for (s = one_phrase(words, phrase); *phrase; s = one_phrase(s, phrase)) {
		if (is_substring(phrase, str)) {
			return 1;
		}
	}

	return 0;
}


 //////////////////////////////////////////////////////////////////////////////
//// MOB TRIGGERS ////////////////////////////////////////////////////////////

/**
* Called before the character actually enters the room, and before they look
* at it, giving time to hide a mob or reject the entry.
*
* @param char_data *actor The person who is going to move.
* @param room_data *room The room they are trying to enter.
* @param int dir The direction they are moving.
* @param char *method How they are moving.
* @return int 1 to allow; 0 to try to prevent the movement.
*/
int pre_greet_mtrigger(char_data *actor, room_data *room, int dir, char *method) {
	trig_data *t, *next_t;
	char_data *ch;
	char buf[MAX_INPUT_LENGTH];
	int intermediate, final=TRUE, any_in_room = -1;

	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return TRUE;
	}
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return TRUE;
	}
	
	DL_FOREACH2(ROOM_PEOPLE(room), ch, next_in_room) {
		if (!SCRIPT_CHECK(ch, MTRIG_PRE_GREET_ALL) || (ch == actor)) {
			continue;
		}

		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
			if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
				continue;
			}
			if (!TRIGGER_CHECK(t, MTRIG_PRE_GREET_ALL)) {
				continue;
			}
			if (TRIG_IS_LOCAL(t)) {
				if (any_in_room == -1) {
					any_in_room = any_players_in_room(room);
				}
				if (!any_in_room) {
					continue;	// requires a player
				}
			}
			if (number(1, 100) <= GET_TRIG_NARG(t)) {
				union script_driver_data_u sdd;
				if (dir >= 0 && dir < NUM_OF_DIRS) {
					add_var(&GET_TRIG_VARS(t), "direction", (char *)dirs[rev_dir[dir]], 0);
				}
				else {
					add_var(&GET_TRIG_VARS(t), "direction", "none", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
				sdd.c = ch;
				intermediate = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
				if (!intermediate) {
					final = FALSE;
				}
				continue;
			}
		}
	}
	return final;
}


/**
* Buy trigger (mob): fires when someone is about to buy.
*
* @param char_data *actor The person trying to buy.
* @param char_data *shopkeeper The mob shopkeeper, if any (many shops have none).
* @param obj_data *buying The item being bought.
* @param int cost The amount to be charged.
* @param any_vnum currency The currency type (NOTHING for coins).
* @return int 0 if a trigger blocked the buy (stop); 1 if not (ok to continue).
*/
int buy_mtrigger(char_data *actor, char_data *shopkeeper, obj_data *buying, int cost, any_vnum currency) {
	union script_driver_data_u sdd;
	char buf[MAX_INPUT_LENGTH];
	char_data *ch, *ch_next;
	trig_data *t;
	
	// gods not affected
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return 1;
	}
	
	DL_FOREACH_SAFE2(ROOM_PEOPLE(IN_ROOM(actor)), ch, ch_next, next_in_room) {
		if (!SCRIPT_CHECK(ch, MTRIG_BUY)) {
			continue;
		}
		if (actor == ch) {
			continue;
		}
		
		LL_FOREACH(TRIGGERS(SCRIPT(ch)), t) {
			if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
				continue;
			}
			if (!TRIGGER_CHECK(t, MTRIG_BUY)) {
				continue;
			}
			
			// vars
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			ADD_UID_VAR(buf, t, obj_script_id(buying), "obj", 0);
			if (shopkeeper) {
				ADD_UID_VAR(buf, t, char_script_id(shopkeeper), "shopkeeper", 0);
			}
			else {
				add_var(&GET_TRIG_VARS(t), "shopkeeper", "", 0);
			}
			
			snprintf(buf, sizeof(buf), "%d", cost);
			add_var(&GET_TRIG_VARS(t), "cost", buf, 0);
			
			if (currency == NOTHING) {
				strcpy(buf, "coins");
			}
			else {
				snprintf(buf, sizeof(buf), "%d", currency);
			}
			add_var(&GET_TRIG_VARS(t), "currency", buf, 0);
			
			// run it:
			sdd.c = ch;
			if (!script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW)) {
				return 0;
			}
		}
	}

	return 1;
}


/**
* Command trigger (mob).
*
* @param char_data *actor The person typing a command.
* @param char *cmd The command as-typed (first word).
* @param char *argument Any arguments (remaining text).
* @param int mode CMDTRG_EXACT or CMDTRG_ABBREV.
* @return int 1 if a trigger ran (stop); 0 if not (ok to continue).
*/
int command_mtrigger(char_data *actor, char *cmd, char *argument, int mode) {
	char_data *ch, *ch_next;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];

	/* prevent people we like from becoming trapped :P */
	if (!valid_dg_target(actor, 0)) {
		return 0;
	}
	
	DL_FOREACH_SAFE2(ROOM_PEOPLE(IN_ROOM(actor)), ch, ch_next, next_in_room) {
		if (SCRIPT_CHECK(ch, MTRIG_COMMAND) && (actor != ch || !AFF_FLAGGED(ch, AFF_ORDERED))) {
			LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
				if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
					continue;
				}
				if (!TRIGGER_CHECK(t, MTRIG_COMMAND)) {
					continue;
				}

				if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
					syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
					continue;
				}
				
				if (match_command_trig(cmd, GET_TRIG_ARG(t), mode)) {
					union script_driver_data_u sdd;
					ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
					skip_spaces(&argument);
					add_var(&GET_TRIG_VARS(t), "arg", argument, 0);
					skip_spaces(&cmd);
					add_var(&GET_TRIG_VARS(t), "cmd", cmd, 0);
					sdd.c = ch;

					if (script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW)) {
						return 1;
					}
				}
			}
		}
	}

	return 0;
}


/**
* Called when a character enters a room to see who they remember there.
*
* @param char_data *ch The person who moved.
*/
void entry_memory_mtrigger(char_data *ch) {
	union script_driver_data_u sdd;
	trig_data *t, *next_t;
	char_data *actor;
	struct script_memory *mem, *next_mem;
	char buf[MAX_INPUT_LENGTH];
	int any_in_room = -1;

	if (!SCRIPT_MEM(ch)) {
		return;
	}
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(ch)), actor, next_in_room) {
		if (!SCRIPT_MEM(ch)) {
			break;
		}
		if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
			continue;
		}
		if (actor!=ch && SCRIPT_MEM(ch)) {
			LL_FOREACH_SAFE(SCRIPT_MEM(ch), mem, next_mem) {
				if (actor->script_id == mem->id) {
					if (mem->cmd) {
						command_interpreter(ch, mem->cmd);
					}
					else {
						LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
							if (!TRIGGER_CHECK(t, MTRIG_MEMORY) || number(1, 100) > GET_TRIG_NARG(t)) {
								continue;	// wrong type or no random roll
							}
							if (TRIG_IS_LOCAL(t)) {
								if (any_in_room == -1) {
									any_in_room = any_players_in_room(IN_ROOM(ch));
								}
								if (!any_in_room) {
									continue;	// requires a player
								}
						    }
						    
						    // ok
							ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
							sdd.c = ch;
							script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
							break;
						}
					}
					/* delete the memory */
					LL_DELETE(SCRIPT_MEM(ch), mem);
					if (mem->cmd) {
						free(mem->cmd);
					}
					free(mem);
				}
			}
		}
	}
}


/**
* Called when a mob walks into a room.
*
* @param char_data *ch The mob who moved.
* @param char *method The way the mob entered.
* @return int 1 to allow the entry; 0 to try to send it back.
*/
int entry_mtrigger(char_data *ch, char *method) {
	union script_driver_data_u sdd;
	trig_data *t, *next_t;
	int any_in_room = -1, val;
	bool multi = FALSE;

	if (!SCRIPT_CHECK(ch, MTRIG_ENTRY)) {
		return 1;
	}
	
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
			continue;
		}
		if (!TRIGGER_CHECK(t, MTRIG_ENTRY) || (number(1, 100) > GET_TRIG_NARG(t))) {
			continue;
		}
		
		if (TRIG_IS_LOCAL(t)) {
			if (any_in_room == -1) {
				any_in_room = any_players_in_room(IN_ROOM(ch));
			}
			if (!any_in_room) {
				continue;	// requires a player
			}
		}
		
		// ok:
		add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
		sdd.c = ch;
		val = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
		
		if (!val || !IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
			return val;
		}
		else {
			multi = TRUE;
		}
	}

	return 1;
}


/**
* Called when an actor tries to turn in a quest.
*
* @param char_data *actor The person trying to finish a quest.
* @param quest_data *quest Which quest they're trying to finish.
* @param struct instance_data *inst What instance the quest is associated with, if any.
* @return int 0 to prevent it; 1 to allow it.
*/
int finish_quest_mtrigger(char_data *actor, quest_data *quest, struct instance_data *inst) {
	char buf[MAX_INPUT_LENGTH];
	char_data *ch;
	trig_data *t;
	
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return TRUE;
	}
	
	// store instance globally to allow %instance.xxx% in scripts
	quest_instance_global = inst;
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(actor)), ch, next_in_room) {
		if (!SCRIPT_CHECK(ch, MTRIG_FINISH_QUEST) || (ch == actor)) {
			continue;
		}
		
		LL_FOREACH(TRIGGERS(SCRIPT(ch)), t) {
			if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
				continue;
			}
			if (TRIGGER_CHECK(t, MTRIG_FINISH_QUEST)) {
				union script_driver_data_u sdd;
				if (quest) {
					snprintf(buf, sizeof(buf), "%d", QUEST_VNUM(quest));
					add_var(&GET_TRIG_VARS(t), "questvnum", buf, 0);
					add_var(&GET_TRIG_VARS(t), "questname", QUEST_NAME(quest), 0);
				}
				else {	// no quest?
					add_var(&GET_TRIG_VARS(t), "questvnum", "0", 0);
					add_var(&GET_TRIG_VARS(t), "questname", "Unknown", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				sdd.c = ch;
				if (!script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW)) {
					quest_instance_global = NULL;	// un-set this
					return FALSE;
				}
			}
		}
	}
	
	quest_instance_global = NULL;	// un-set this
	return TRUE;
}


/**
* Called when a person enters a room to see who remembers them.
*
* @param char_data *actor The person who moved.
*/
void greet_memory_mtrigger(char_data *actor) {
	trig_data *t, *next_t;
	char_data *ch;
	struct script_memory *mem, *next_mem;
	char buf[MAX_INPUT_LENGTH];
	int command_performed = 0, any_in_room = -1;

	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return;
	}
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return;
	}
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(actor)), ch, next_in_room) {
		if (!SCRIPT_MEM(ch) || !AWAKE(ch) || FIGHTING(ch) || (ch == actor)) {
			continue;
		}
		/* find memory line with command only */
		LL_FOREACH_SAFE(SCRIPT_MEM(ch), mem, next_mem) {
			if (actor->script_id != mem->id) {
				continue;
			}
			if (mem->cmd) {
				command_interpreter(ch, mem->cmd); /* no script */
				command_performed = 1;
				break;
			}
			/* if a command was not performed execute the memory script */
			if (mem && !command_performed) {
				LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
					if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
						continue;
					}
					if (!TRIGGER_CHECK(t, MTRIG_MEMORY) || !CAN_SEE(ch, actor)) {
						continue;
					}
					if (TRIG_IS_LOCAL(t)) {
						if (any_in_room == -1) {
							any_in_room = any_players_in_room(IN_ROOM(ch));
						}
						if (!any_in_room) {
							continue;	// requires a player
						}
					}
					if (number(1, 100) <= GET_TRIG_NARG(t)) {
						union script_driver_data_u sdd;
						ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
						sdd.c = ch;
						script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
						break;
					}
				}
			}
			/* delete the memory */
			if (mem) {
				LL_DELETE(SCRIPT_MEM(ch), mem);
				if (mem->cmd) {
					free(mem->cmd);
				}
				free(mem);
			}
		}
	}
}


/**
* Called when a person walks into a room to run triggers on mobs.
*
* @param char_data *ch The person who moved.
* @param int dir Which direction they moved.
* @param char *method The way the person entered.
* @return int 1 to allow the entry; 0 to try to send them back.
*/
int greet_mtrigger(char_data *actor, int dir, char *method) {
	trig_data *t, *next_t;
	char_data *ch;
	char buf[MAX_INPUT_LENGTH];
	int intermediate, final=TRUE, any_in_room = -1;

	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return TRUE;
	}
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return TRUE;
	}
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(actor)), ch, next_in_room) {
		if (!SCRIPT_CHECK(ch, MTRIG_GREET | MTRIG_GREET_ALL) || (ch == actor)) {
			continue;
		}
		if (!SCRIPT_CHECK(ch, MTRIG_GREET_ALL) && (!AWAKE(ch) || FIGHTING(ch) || AFF_FLAGGED(actor, AFF_SNEAK))) {
			continue;
		}

		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
			if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
				continue;
			}
			if (!TRIGGER_CHECK(t, MTRIG_GREET_ALL) && !(TRIGGER_CHECK(t, MTRIG_GREET) && CAN_SEE(ch, actor))) {
				continue;	// wrong types
			}
			if (TRIG_IS_LOCAL(t)) {
				if (any_in_room == -1) {
					any_in_room = any_players_in_room(IN_ROOM(ch));
				}
				if (!any_in_room) {
					continue;	// requires a player
				}
			}
			if (number(1, 100) <= GET_TRIG_NARG(t)) {
				union script_driver_data_u sdd;
				if (dir >= 0 && dir < NUM_OF_DIRS) {
					add_var(&GET_TRIG_VARS(t), "direction", (char *)dirs[rev_dir[dir]], 0);
				}
				else {
					add_var(&GET_TRIG_VARS(t), "direction", "none", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
				sdd.c = ch;
				intermediate = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
				if (!intermediate) {
					final = FALSE;
				}
				continue;
			}
		}
	}
	return final;
}


/**
* @param char_data *actor Person speaking.
* @param char *str String spoken.
* @param generic_data *language Language spoken in.
* @param char_data *only_mob Optional: Only this mob heard the speech (NULL for all mobs in room). Mob MUST be in the same room.
*/
void speech_mtrigger(char_data *actor, char *str, generic_data *language, char_data *only_mob) {
	char_data *ch, *ch_next;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int any_in_room = -1;
	bool multi;
	
	DL_FOREACH_SAFE2(ROOM_PEOPLE(IN_ROOM(actor)), ch, ch_next, next_in_room) {
		if (only_mob && ch != only_mob) {
			continue;
		}
		
		// multi is per-char
		multi = FALSE;
		
		if (SCRIPT_CHECK(ch, MTRIG_SPEECH) && AWAKE(ch) && (actor!=ch)) {
			LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
				if (multi && IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
					continue;	// already did an allow-multi
				}
				if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
					continue;
				}
				if (!TRIGGER_CHECK(t, MTRIG_SPEECH)) {
					continue;
				}
				if (TRIG_IS_LOCAL(t)) {
					if (any_in_room == -1) {
						any_in_room = any_players_in_room(IN_ROOM(actor));
					}
					if (!any_in_room) {
						continue;	// requires a player
					}
				}
				
				if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
					syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: Speech Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
					continue;
				}

				if (*GET_TRIG_ARG(t) == '*' || ((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) || (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str)))) {
					union script_driver_data_u sdd;
					ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
					add_var(&GET_TRIG_VARS(t), "speech", strip_color(str), 0);
					add_var(&GET_TRIG_VARS(t), "lang", language ? GEN_NAME(language) : "", 0);
					sprintf(buf, "%d", language ? GEN_VNUM(language) : NOTHING);
					add_var(&GET_TRIG_VARS(t), "lang_vnum", buf, 0);
					sdd.c = ch;
					script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
					
					if (!IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
						break;
					}
					else {
						multi = TRUE;
					}
				}
			}
		}
	}
}


/**
* Called when an actor tries to start a quest.
*
* @param char_data *actor The person trying to start a quest.
* @param quest_data *quest Which quest they're trying to start.
* @param struct instance_data *inst What instance the quest is associated with, if any.
* @return int 0 to prevent it; 1 to allow it.
*/
int start_quest_mtrigger(char_data *actor, quest_data *quest, struct instance_data *inst) {
	char buf[MAX_INPUT_LENGTH];
	char_data *ch;
	trig_data *t;
	
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return TRUE;
	}
	
	// store instance globally to allow %instance.xxx% in scripts
	quest_instance_global = inst;
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(actor)), ch, next_in_room) {
		if (!SCRIPT_CHECK(ch, MTRIG_START_QUEST) || (ch == actor)) {
			continue;
		}
		
		LL_FOREACH(TRIGGERS(SCRIPT(ch)), t) {
			if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
				continue;
			}
			if (TRIGGER_CHECK(t, MTRIG_START_QUEST)) {
				union script_driver_data_u sdd;
				if (quest) {
					snprintf(buf, sizeof(buf), "%d", QUEST_VNUM(quest));
					add_var(&GET_TRIG_VARS(t), "questvnum", buf, 0);
					add_var(&GET_TRIG_VARS(t), "questname", QUEST_NAME(quest), 0);
				}
				else {	// no quest?
					add_var(&GET_TRIG_VARS(t), "questvnum", "0", 0);
					add_var(&GET_TRIG_VARS(t), "questname", "Unknown", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				sdd.c = ch;
				if (!script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW)) {
					quest_instance_global = NULL;	// un-set this
					return FALSE;
				}
			}
		}
	}
	quest_instance_global = NULL;	// un-set this
	return TRUE;
}


/**
* Called when an act() action message is sent to a mob.
*
* @param const char_data *ch The person receiving the action message.
* @param char *str The string shown.
* @param char_data *actor The person who caused the message to be sent.
* @param char_data *victim The victim/target of the message, if any.
* @param obj_data *object The first object-target of the message, if any.
* @param obj_data *target The secodn object-target of the message, if any.
* @param char *arg The string passed to the target arg, if any. This is NOT USED.
*/
void act_mtrigger(const char_data *ch, char *str, char_data *actor, char_data *victim, obj_data *object, obj_data *target, char *arg) {
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int any_in_room = -1;
	bool multi = FALSE;

	if (SCRIPT_CHECK(ch, MTRIG_ACT) && (actor!=ch)) {
		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
			if (multi && IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
				continue;	// already did an allow-multi
			}
			if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
				continue;
			}
			if (!TRIGGER_CHECK(t, MTRIG_ACT)) {
				continue;
			}
			if (TRIG_IS_LOCAL(t)) {
				if (any_in_room == -1) {
					any_in_room = any_players_in_room(IN_ROOM(ch));
				}
				if (!any_in_room) {
					continue;	// requires a player
				}
			}

			if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
				syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: Act Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
				continue;
			}

			if (((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) || (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str)))) {
				union script_driver_data_u sdd;

				if (actor) {
					ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				}
				if (victim) {
					ADD_UID_VAR(buf, t, char_script_id(victim), "victim", 0);
				}
				if (object) {
					ADD_UID_VAR(buf, t, obj_script_id(object), "object", 0);
				}
				if (target) {
					ADD_UID_VAR(buf, t, obj_script_id(target), "target", 0);
				}
				if (str) {
					/* we're guaranteed to have a string ending with \r\n\0 */
					char *nstr = strdup(str), *fstr = nstr, *p = strchr(nstr, '\r');
					skip_spaces(&nstr);
					*p = '\0';
					add_var(&GET_TRIG_VARS(t), "arg", nstr, 0);
					free(fstr);
				}	  
				sdd.c = (char_data*)ch;
				script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
				
				if (!IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
					break;
				}
				else {
					multi = TRUE;
				}
			}	
		}
	}
}


/**
* Called before damage during any hit().
*
* @param char_data *ch The mob who is fighting.
* @param bool will_hit Indicating whether the mob is expected to hit the target.
* @return int 0 to cancel, 1 to continue the attack.
*/
int fight_mtrigger(char_data *ch, bool will_hit) {
	char_data *actor;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int val = 1;

	if (!SCRIPT_CHECK(ch, MTRIG_FIGHT) || !FIGHTING(ch)) {
		return val;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
			continue;
		}
		if (TRIGGER_CHECK(t, MTRIG_FIGHT) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;

			actor = FIGHTING(ch);
			if (actor) {
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			}
			else {
				// this should be impossible since it was pre-checked
				add_var(&GET_TRIG_VARS(t), "actor", "nobody", 0);
			}
			
			sprintf(buf, "%d", will_hit ? 1 : 0);
			add_var(&GET_TRIG_VARS(t), "hit", buf, 0);

			sdd.c = ch;
			val = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
			break;
		}
	}
	
	return val;
}


/**
* Called after a hit against the mob.
*
* @param char_data *ch The mob being hit.
*/
void hitprcnt_mtrigger(char_data *ch) {
	bool multi = FALSE;
	char_data *actor;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];

	if (!SCRIPT_CHECK(ch, MTRIG_HITPRCNT) || !FIGHTING(ch)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
		if (multi && IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
			continue;
		}
		if (TRIGGER_CHECK(t, MTRIG_HITPRCNT) && GET_MAX_HEALTH(ch) && (((GET_HEALTH(ch) * 100) / MAX(1, GET_MAX_HEALTH(ch))) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;

			actor = FIGHTING(ch);
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sdd.c = ch;
			if (!script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW) || !IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
				break;
			}
			multi = TRUE;
		}
	}
}


/**
* Called when someone is giving an object to a mob.
*
* @param char_data *ch The mob receiving the item.
* @param char_data *actor The person giving the mob the object.
* @param obj_data *obj The item being given.
* @return int 1 to allow it; 0 to cancel the give.
*/
int receive_mtrigger(char_data *ch, char_data *actor, obj_data *obj) {
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int ret_val;

	if (!SCRIPT_CHECK(ch, MTRIG_RECEIVE)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
			continue;
		}
		if (TRIGGER_CHECK(t, MTRIG_RECEIVE) && (number(1, 100) <= GET_TRIG_NARG(t))){
			union script_driver_data_u sdd;

			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			ADD_UID_VAR(buf, t, obj_script_id(obj), "object", 0);
			sdd.c = ch;
			ret_val = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
			if (EXTRACTED(actor) || EXTRACTED(ch) || IS_DEAD(actor) || IS_DEAD(ch) || obj->carried_by != actor) {
				return 0;
			}
			else {
				return ret_val;
			}
		}
	}

	return 1;
}


/**
* Called as a character dies.
*
* @param char_data *ch The mob dying.
* @param char_data *actor The killer, if applicable.
* @return 0 to prevent the death cry; 1 to allow it. (The death cannot be prevented.)
*/
int death_mtrigger(char_data *ch, char_data *actor) {
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int val = 1;
	bool multi = FALSE;

	if (!SCRIPT_CHECK(ch, MTRIG_DEATH)) {
		return val;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
		if (multi && IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
			continue;
		}
		if (TRIGGER_CHECK(t, MTRIG_DEATH) && (number(1, 100) <= GET_TRIG_NARG(t))){
			union script_driver_data_u sdd;

			if (actor && actor != ch) {
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			}
			sdd.c = ch;
			val &= script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
			
			if (!IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
				// the first non-multiple breaks the loop
				break;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return val;
}


/**
* Called when someone gives money to a character.
*
* @param char_data *ch The mob receiving the money.
* @param char_data *actor The person giving the money.
* @param int amount How much is being given.
* @return int 0 to block the bribe, 1 to allow it.
*/
int bribe_mtrigger(char_data *ch, char_data *actor, int amount) {
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int ret_val;

	if (!SCRIPT_CHECK(ch, MTRIG_BRIBE)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
			continue;
		}
		if (TRIGGER_CHECK(t, MTRIG_BRIBE) && (amount >= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			sdd.c = ch;
			snprintf(buf, sizeof(buf), "%d", amount);
			add_var(&GET_TRIG_VARS(t), "amount", buf, 0);
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			ret_val = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
			
			if (EXTRACTED(actor) || EXTRACTED(ch) || IS_DEAD(actor) || IS_DEAD(ch)) {
				return 0;
			}
			else {
				return ret_val;
			}
		}
	}
	
	return 1;
}


/**
* Called when someone tries to attack the mob it's on.
*
* @param char_data *ch The mob to check for triggers.
* @param char_data *actor The person trying to attack.
* @return int 0 to block the fight, 1 to allow it
*/
int can_fight_mtrigger(char_data *ch, char_data *actor) {
	trig_data *trig, *next_t;
	int ret_val;

	if (!SCRIPT_CHECK(ch, MTRIG_CAN_FIGHT)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), trig, next_t) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(trig, MTRIG_CHARMED)) {
			continue;
		}
		if (TRIGGER_CHECK(trig, MTRIG_CAN_FIGHT)) {
			union script_driver_data_u sdd;
			sdd.c = ch;
			ADD_UID_VAR(buf, trig, char_script_id(actor), "actor", 0);
			ret_val = script_driver(&sdd, trig, MOB_TRIGGER, TRIG_NEW);
			
			if (EXTRACTED(actor) || EXTRACTED(ch) || IS_DEAD(actor) || IS_DEAD(ch)) {
				return 0;
			}
			else {
				return ret_val;
			}
		}
	}
	
	return 1;	// allow
}


/**
* Called when the mob is first loaded.
*
* @param char_data *ch The mob.
*/
void load_mtrigger(char_data *ch) {
	trig_data *t, *next_t;
	bool multi = FALSE;

	if (!SCRIPT_CHECK(ch, MTRIG_LOAD)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
		if (multi && IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, MTRIG_LOAD) &&  (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			sdd.c = ch;
			script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
			
			if (!IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
				// the first non-multiple breaks the loop
				break;
			}
			else {
				multi = TRUE;
			}
		}
	}
}


/**
* Called when someone tries to target a mob with an ability.
*
* @param char_data *actor The person using the ability.
* @param char_data *ch The mob being targeted.
* @param any_vnum abil The ability's vnum.
* @return int 1 to allow the ability; 0 to prevent it.
*/
int ability_mtrigger(char_data *actor, char_data *ch, any_vnum abil) {
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	ability_data *ab;
	int val;
	bool multi = FALSE;

	if (ch == NULL || !(ab = find_ability_by_vnum(abil))) {
		return 1;
	}

	if (!SCRIPT_CHECK(ch, MTRIG_ABILITY)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
			continue;
		}
		if (TRIGGER_CHECK(t, MTRIG_ABILITY) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;

			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sprintf(buf, "%d", abil);
			add_var(&GET_TRIG_VARS(t), "ability", buf, 0);
			add_var(&GET_TRIG_VARS(t), "abilityname", ABIL_NAME(ab), 0);
			sdd.c = ch;
			val = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
			
			if (!val || !IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
				return val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return 1;
}


/**
* Called as a character tries to leave the room.
*
* @param char_data *actor The person trying to leave.
* @param int dir The direction they are trying to go (passed through to %direction%).
* @param char *custom_dir Optional: A different value for %direction% (may be NULL).
* @param char *method Optional: The method by which they moved (may be NULL).
* @return int 0 = block the leave, 1 = pass
*/
int leave_mtrigger(char_data *actor, int dir, char *custom_dir, char *method) {
	trig_data *t, *next_t;
	char_data *ch;
	char buf[MAX_INPUT_LENGTH];
	int any_in_room = -1, val;
	bool multi = FALSE;
	
	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return 1;
	}
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(actor)), ch, next_in_room) {
		if (!SCRIPT_CHECK(ch, MTRIG_LEAVE | MTRIG_LEAVE_ALL) || (ch == actor)) {
			continue;
		}
		if (!SCRIPT_CHECK(ch, MTRIG_LEAVE_ALL) && (!AWAKE(ch) || FIGHTING(ch))) {
			continue;
		}

		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
			if (multi && !IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
				continue;	// already did an allow-multi
			}
			if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
				continue;
			}
			if (!TRIGGER_CHECK(t, MTRIG_LEAVE_ALL) && !(TRIGGER_CHECK(t, MTRIG_LEAVE) && CAN_SEE(ch, actor))) {
				continue;
			}
			if (TRIG_IS_LOCAL(t)) {
				if (any_in_room == -1) {
					any_in_room = any_players_in_room(IN_ROOM(actor));
				}
				if (!any_in_room) {
					continue;	// requires a player
				}
			}
			if (number(1, 100) <= GET_TRIG_NARG(t)) {
				union script_driver_data_u sdd;
				if (custom_dir) {
					add_var(&GET_TRIG_VARS(t), "direction", custom_dir, 0);
				}
				else {
					add_var(&GET_TRIG_VARS(t), "direction", (dir >= 0 && dir < NUM_OF_DIRS) ? ((char *)dirs[dir]) : "none", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
				sdd.c = ch;
				val = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
				
				if (!val || !IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
					return val;
				}
				else {
					multi = TRUE;
				}
			}
		}
	}
	return 1;
}


/**
* Called as a character attempts to use a door.
*
* @param char_data *actor The person working a door.
* @param int subcmd Which door action the player is using.
* @param int dir Which direction the door is in.
* @return int 1 to allow the command; 0 to prevent it.
*/
int door_mtrigger(char_data *actor, int subcmd, int dir) {
	trig_data *t, *next_t;
	char_data *ch;
	char buf[MAX_INPUT_LENGTH];
	int val;
	bool multi = FALSE;
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(actor)), ch, next_in_room) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (!SCRIPT_CHECK(ch, MTRIG_DOOR) || !AWAKE(ch) || FIGHTING(ch) || (ch == actor)) {
			continue;
		}

		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
			if (AFF_FLAGGED(ch, AFF_CHARM) && !TRIGGER_CHECK(t, MTRIG_CHARMED)) {
				continue;
			}
			if (TRIGGER_CHECK(t, MTRIG_DOOR) && CAN_SEE(ch, actor) && (number(1, 100) <= GET_TRIG_NARG(t))) {
				union script_driver_data_u sdd;
				add_var(&GET_TRIG_VARS(t), "cmd", (char *)cmd_door[subcmd], 0);
				if (dir>=0 && dir < NUM_OF_DIRS) {
					add_var(&GET_TRIG_VARS(t), "direction", (char *)dirs[dir], 0);
				}
				else {
					add_var(&GET_TRIG_VARS(t), "direction", "none", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				sdd.c = ch;
				val = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
				
				if (!val || !IS_SET(GET_TRIG_TYPE(t), MTRIG_ALLOW_MULTIPLE)) {
					return val;
				}
				else {
					multi = TRUE;
				}
			}
		}
	}
	return 1;
}


/**
* Called on a mob after the mud starts up.
*
* @param char_data *ch The mob.
*/
void reboot_mtrigger(char_data *ch) {
	trig_data *t, *next_t;
	int val;

	if (!SCRIPT_CHECK(ch, MTRIG_REBOOT)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch)), t, next_t) {
		if (TRIGGER_CHECK(t, MTRIG_REBOOT)) {
			union script_driver_data_u sdd;
			sdd.c = ch;
			val = script_driver(&sdd, t, MOB_TRIGGER, TRIG_NEW);
			if (!val) {
				break;
			}
		}
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// OBJECT TRIGGERS /////////////////////////////////////////////////////////

/**
* Fires when an object's timer expired.
*
* @param obj_data *obj The object.
* @return int 1 to continue, 0 to stop, -1 if the object was purged
*/
int timer_otrigger(obj_data *obj) {
	trig_data *t, *next_t;
	int return_val;

	if (!SCRIPT_CHECK(obj, OTRIG_TIMER)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (TRIGGER_CHECK(t, OTRIG_TIMER)) {
			union script_driver_data_u sdd;
			sdd.o = obj;
			return_val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			/* don't allow a wear to take place, if
			* the object is purged.
			*/
			if (!obj || (t->purge_tracker && t->purge_tracker->purged)) {
				return -1;
			}
			else if (!return_val) {
				// break out only on a 0
				return 0;
			}
		}
	}  

	return 1;
}


/**
* Trigger fires when actor tries to get obj, or receives it in some way.
*
* @param obj_data *obj The object trying to be "got".
* @param char_data *actor The person trying to receive the object.
* @param bool preventable If TRUE, the script can block the "get". If FALSE, the character has already received the object and it can only be prevented by moving it in the script.
*/
int get_otrigger(obj_data *obj, char_data *actor, bool preventable) {
	trig_data *t, *next_t;
	bool multi = FALSE;
	char buf[MAX_INPUT_LENGTH];
	int ret_val = 1;
	
	if (!SCRIPT_CHECK(obj, OTRIG_GET)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, OTRIG_GET) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			snprintf(buf, sizeof(buf), "%d", preventable ? 1 : 0);
			add_var(&GET_TRIG_VARS(t), "preventable", buf, 0);
			sdd.o = obj;
			ret_val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			/* don't allow a get to take place, if
			* a) the actor is killed (the mud would choke on obj_to_char).
			* b) the object is purged.
			*/
			if (EXTRACTED(actor) || IS_DEAD(actor) || !obj) {
				return 0;
			}
			else if (!ret_val || !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
				return ret_val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return ret_val;
}


/**
* Called when an actor tries to turn in a quest.
*
* @param obj_data *obj The object to run triggers on.
* @param char_data *actor The person trying to finish a quest.
* @param quest_data *quest Which quest they're trying to finish.
* @param struct instance_data *inst What instance the quest is associated with, if any.
* @return int 0 to prevent it; 1 to allow it.
*/
int finish_quest_otrigger_one(obj_data *obj, char_data *actor, quest_data *quest, struct instance_data *inst) {
	char buf[MAX_INPUT_LENGTH];
	int ret_val = TRUE;
	trig_data *t, *next_t;
	
	if (!SCRIPT_CHECK(obj, OTRIG_FINISH_QUEST)) {
		return TRUE;
	}
	
	// store instance globally to allow %instance.xxx% in scripts
	quest_instance_global = inst;

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (TRIGGER_CHECK(t, OTRIG_FINISH_QUEST)) {
			union script_driver_data_u sdd;
			if (quest) {
				snprintf(buf, sizeof(buf), "%d", QUEST_VNUM(quest));
				add_var(&GET_TRIG_VARS(t), "questvnum", buf, 0);
				add_var(&GET_TRIG_VARS(t), "questname", QUEST_NAME(quest), 0);
			}
			else {	// no quest?
				add_var(&GET_TRIG_VARS(t), "questvnum", "0", 0);
				add_var(&GET_TRIG_VARS(t), "questname", "Unknown", 0);
			}
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sdd.o = obj;
			ret_val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			if (!ret_val) {
				break;
			}
		}
	}
	
	quest_instance_global = NULL;	// un-set this
	return ret_val;
}


/**
* Finish quest trigger (obj).
*
* @param char_data *actor The person trying to get a quest.
* @param quest_data *quest The quest to try to finish
* @param struct instance_data *inst The associated instance, if any.
* @return int 0/FALSE to stop the quest, 1/TRUE to allow it to continue.
*/
int finish_quest_otrigger(char_data *actor, quest_data *quest, struct instance_data *inst) {
	obj_data *obj;
	int i;

	/* prevent people we like from becoming trapped :P */
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return 1;
	}

	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(actor, i) && !finish_quest_otrigger_one(GET_EQ(actor, i), actor, quest, inst)) {
			return 0;
		}
	}
	
	DL_FOREACH2(actor->carrying, obj, next_content) {
		if (!finish_quest_otrigger_one(obj, actor, quest, inst)) {
			return 0;
		}
	}
	
	DL_FOREACH2(ROOM_CONTENTS(IN_ROOM(actor)), obj, next_content) {
		if (!finish_quest_otrigger_one(obj, actor, quest, inst)) {
			return 0;
		}
	}

	return 1;
}


/**
* Runs greet triggers on objects in the room.
*
* @param char_data *actor The person who has entered the room.
* @param int dir Which direction the character moved.
* @param char *method Method of movement.
* @return int 1 to allow the character here, 0 to attempt to send them back.
*/
int greet_otrigger(char_data *actor, int dir, char *method) {
	int intermediate, final = TRUE, any_in_room = -1;
	obj_data *obj, *next_obj;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;

	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return 1;
	}
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return TRUE;
	}
	
	DL_FOREACH_SAFE2(ROOM_CONTENTS(IN_ROOM(actor)), obj, next_obj, next_content) {
		if (!SCRIPT_CHECK(obj, OTRIG_GREET)) {
			continue;
		}
		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
			if (!TRIGGER_CHECK(t, OTRIG_GREET)) {
				continue;
			}
			if (TRIG_IS_LOCAL(t)) {
				if (any_in_room == -1) {
					any_in_room = any_players_in_room(IN_ROOM(actor));
				}
				if (!any_in_room) {
					continue;	// requires a player
				}
			}
			if (number(1, 100) <= GET_TRIG_NARG(t)) {
				union script_driver_data_u sdd;
				if (dir >= 0 && dir < NUM_OF_DIRS) {
					add_var(&GET_TRIG_VARS(t), "direction", (char *)dirs[rev_dir[dir]], 0);
				}
				else {
					add_var(&GET_TRIG_VARS(t), "direction", "none", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
				sdd.o = obj;
				intermediate = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
				if (!intermediate) {
					final = FALSE;
				}
				continue;
			}
		}
	}
	return final;
}


/**
* Buy trigger (obj) sub-processor.
*
* @param obj_data *obj The item to check.
* @param char_data *actor The person trying to buy.
* @param char_data *shopkeeper The mob shopkeeper, if any (many shops have none).
* @param obj_data *buying The item being bought.
* @param int cost The amount to be charged.
* @param any_vnum currency The currency type (NOTHING for coins).
* @param int type Location: OCMD_EQUIP, etc.
* @return int 0 if a trigger blocked the buy (stop); 1 if not (ok to continue).
*/
int buy_otrig(obj_data *obj, char_data *actor, char_data *shopkeeper, obj_data *buying, int cost, any_vnum currency, int type) {
	union script_driver_data_u sdd;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t;

	if (!obj || !SCRIPT_CHECK(obj, OTRIG_BUY)) {
		return 1;
	}
	
	LL_FOREACH(TRIGGERS(SCRIPT(obj)), t) {
		if (!TRIGGER_CHECK(t, OTRIG_BUY)) {
			continue;	// not a buy trigger
		}
		if (!IS_SET(GET_TRIG_NARG(t), type)) {
			continue;	// bad location
		}

		// vars
		ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
		ADD_UID_VAR(buf, t, obj_script_id(buying), "obj", 0);
		if (shopkeeper) {
			ADD_UID_VAR(buf, t, char_script_id(shopkeeper), "shopkeeper", 0);
		}
			else {
				add_var(&GET_TRIG_VARS(t), "shopkeeper", "", 0);
			}
		
		snprintf(buf, sizeof(buf), "%d", cost);
		add_var(&GET_TRIG_VARS(t), "cost", buf, 0);
		
		if (currency == NOTHING) {
			strcpy(buf, "coins");
		}
		else {
			snprintf(buf, sizeof(buf), "%d", currency);
		}
		add_var(&GET_TRIG_VARS(t), "currency", buf, 0);
		
		// run it
		sdd.o = obj;
		if (!script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW)) {
			return 0;
		}
	}

	return 1;
}


/**
* Buy trigger (obj): fires before someone buys something.
*
* @param char_data *actor The person trying to buy.
* @param char_data *shopkeeper The mob shopkeeper, if any (many shops have none).
* @param obj_data *buying The item being bought.
* @param int cost The amount to be charged.
* @param any_vnum currency The currency type (NOTHING for coins).
* @return int 0 if a trigger blocked the buy (stop); 1 if not (ok to continue).
*/
int buy_otrigger(char_data *actor, char_data *shopkeeper, obj_data *buying, int cost, any_vnum currency) {
	obj_data *obj;
	int iter;
	
	// gods not affected
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return 1;
	}
	
	for (iter = 0; iter < NUM_WEARS; iter++) {
		if (!buy_otrig(GET_EQ(actor, iter), actor, shopkeeper, buying, cost, currency, OCMD_EQUIP)) {
			return 0;
		}
	}
	
	DL_FOREACH2(actor->carrying, obj, next_content) {
		if (!buy_otrig(obj, actor, shopkeeper, buying, cost, currency, OCMD_INVEN)) {
			return 0;
		}
	}

	DL_FOREACH2(ROOM_CONTENTS(IN_ROOM(actor)), obj, next_content) {
		if (!buy_otrig(obj, actor, shopkeeper, buying, cost, currency, OCMD_ROOM)) {
			return 0;
		}
	}

	return 1;
}


/**
* Command trigger (obj) sub-processor.
*
* @param obj_data *obj The item to check.
* @param char_data *actor The person typing a command.
* @param char *cmd The command as-typed (first word).
* @param char *argument Any arguments (remaining text).
* @param int type Location: OCMD_EQUIP, etc.
* @param int mode CMDTRG_EXACT or CMDTRG_ABBREV.
* @return int 1 if a trigger ran (stop); 0 if not (ok to continue).
*/
int cmd_otrig(obj_data *obj, char_data *actor, char *cmd, char *argument, int type, int mode) {
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];

	if (obj && SCRIPT_CHECK(obj, OTRIG_COMMAND)) {
		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
			// not a command trigger
			if (!TRIGGER_CHECK(t, OTRIG_COMMAND)) {
				continue;
			}
			
			// bad location
			if (!IS_SET(GET_TRIG_NARG(t), type)) {
				continue;
			}

			if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
				syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: O-Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
				continue;
			}

			if (match_command_trig(cmd, GET_TRIG_ARG(t), mode)) {
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				skip_spaces(&argument);
				add_var(&GET_TRIG_VARS(t), "arg", argument, 0);
				skip_spaces(&cmd);
				add_var(&GET_TRIG_VARS(t), "cmd", cmd, 0);

				union script_driver_data_u sdd;
				sdd.o = obj;
				if (script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW)) {
					return 1;
				}
			}
		}
	}

	return 0;
}


/**
* Command trigger (obj).
*
* @param char_data *actor The person typing a command.
* @param char *cmd The command as-typed (first word).
* @param char *argument Any arguments (remaining text).
* @param int mode CMDTRG_EXACT or CMDTRG_ABBREV.
* @return int 1 if a trigger ran (stop); 0 if not (ok to continue).
*/
int command_otrigger(char_data *actor, char *cmd, char *argument, int mode) {
	obj_data *obj;
	int i;

	/* prevent people we like from becoming trapped :P */
	if (!valid_dg_target(actor, 0)) {
		return 0;
	}

	for (i = 0; i < NUM_WEARS; i++) {
		if (cmd_otrig(GET_EQ(actor, i), actor, cmd, argument, OCMD_EQUIP, mode)) {
			return 1;
		}
	}
	
	DL_FOREACH2(actor->carrying, obj, next_content) {
		if (cmd_otrig(obj, actor, cmd, argument, OCMD_INVEN, mode)) {
			return 1;
		}
	}
	
	DL_FOREACH2(ROOM_CONTENTS(IN_ROOM(actor)), obj, next_content) {
		if (cmd_otrig(obj, actor, cmd, argument, OCMD_ROOM, mode)) {
			return 1;
		}
	}

	return 0;
}


/**
* Runs kill triggers on a single object, then on its next contents
* (recursively). This function is called by run_kill_triggers, which has
* already validated that the owner of the object qualifies as either the killer
* or an ally of the killer.
*
* @param obj_data *obj The object possibly running triggers.
* @param char_data *dying The person who has died.
* @param char_data *killer Optional: Person who killed them.
* @return int The return value of a script (1 is normal, 0 suppresses the death cry).
*/
int kill_otrigger(obj_data *obj, char_data *dying, char_data *killer) {
	obj_data *next_contains, *next_inside;
	union script_driver_data_u sdd;
	trig_data *trig, *next_trig;
	int val = 1;	// default value
	
	if (!obj) {
		return val;	// often called with no obj
	}
	
	// save for later
	next_contains = obj->contains;
	next_inside = obj->next_content;
	
	// run script if possible...
	if (SCRIPT_CHECK(obj, OTRIG_KILL)) {
		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), trig, next_trig) {
			if (!TRIGGER_CHECK(trig, OTRIG_KILL) || (number(1, 100) > GET_TRIG_NARG(trig))) {
				continue;	// wrong trig or failed random percent
			}
			
			// ok:
			memset((char *) &sdd, 0, sizeof(union script_driver_data_u));
			ADD_UID_VAR(buf, trig, char_script_id(dying), "actor", 0);
			if (killer) {
				ADD_UID_VAR(buf, trig, char_script_id(killer), "killer", 0);
			}
			else {
				add_var(&GET_TRIG_VARS(trig), "killer", "", 0);
			}
			sdd.o = obj;
			
			// run it -- any script returning 0 guarantees we will return 0
			val &= script_driver(&sdd, trig, OBJ_TRIGGER, TRIG_NEW);
			
			// ensure obj is safe
			obj = sdd.o;
			if (!obj) {
				break;	// cannot run more triggers -- obj is gone
			}
		}
	}
	
	// run recursively
	if (next_contains) {
		val &= kill_otrigger(next_contains, dying, killer);
	}
	if (next_inside) {
		val &= kill_otrigger(next_inside, dying, killer);
	}
	
	return val;
}


/**
* Called when an actor tries to start a quest.
*
* @param obj_data *obj The object to check start-quests on.
* @param char_data *actor The person trying to start a quest.
* @param quest_data *quest Which quest they're trying to start.
* @param struct instance_data *inst What instance the quest is associated with, if any.
* @return int 0 to prevent it; 1 to allow it.
*/
int start_quest_otrigger_one(obj_data *obj, char_data *actor, quest_data *quest, struct instance_data *inst) {
	char buf[MAX_INPUT_LENGTH];
	int ret_val = TRUE;
	trig_data *t, *next_t;
	
	if (!SCRIPT_CHECK(obj, OTRIG_START_QUEST)) {
		return TRUE;
	}
	
	// store instance globally to allow %instance.xxx% in scripts
	quest_instance_global = inst;

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (TRIGGER_CHECK(t, OTRIG_START_QUEST)) {
			union script_driver_data_u sdd;
			if (quest) {
				snprintf(buf, sizeof(buf), "%d", QUEST_VNUM(quest));
				add_var(&GET_TRIG_VARS(t), "questvnum", buf, 0);
				add_var(&GET_TRIG_VARS(t), "questname", QUEST_NAME(quest), 0);
			}
			else {	// no quest?
				add_var(&GET_TRIG_VARS(t), "questvnum", "0", 0);
				add_var(&GET_TRIG_VARS(t), "questname", "Unknown", 0);
			}
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sdd.o = obj;
			ret_val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			if (!ret_val) {
				break;
			}
		}
	}
	
	quest_instance_global = NULL;	// un-set this
	return ret_val;
}


/**
* Start quest trigger (obj).
*
* @param char_data *actor The person trying to get a quest.
* @param quest_data *quest The quest to try to start
* @param struct instance_data *inst The instance associated with the quest, if any.
* @return int 0/FALSE to stop the quest, 1/TRUE to allow it to continue.
*/
int start_quest_otrigger(char_data *actor, quest_data *quest, struct instance_data *inst) {
	obj_data *obj;
	int i;

	/* prevent people we like from becoming trapped :P */
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return 1;
	}

	for (i = 0; i < NUM_WEARS; i++) {
		if (GET_EQ(actor, i) && !start_quest_otrigger_one(GET_EQ(actor, i), actor, quest, inst)) {
			return 0;
		}
	}
	
	DL_FOREACH2(actor->carrying, obj, next_content) {
		if (!start_quest_otrigger_one(obj, actor, quest, inst)) {
			return 0;
		}
	}
	
	DL_FOREACH2(ROOM_CONTENTS(IN_ROOM(actor)), obj, next_content) {
		if (!start_quest_otrigger_one(obj, actor, quest, inst)) {
			return 0;
		}
	}

	return 1;
}


/**
* Called when a person tries to wear/equip an item.
*
* @param obj_data *obj The item they want to equip.
* @param char_data *actor The person trying to equip it.
* @param int where Which WEAR_ pos.
* @return int 1 to allow the item, 0 to prevent it.
*/
int wear_otrigger(obj_data *obj, char_data *actor, int where) {
	trig_data *t, *next_t;
	bool multi = FALSE;
	char buf[MAX_INPUT_LENGTH];
	int ret_val = 1;

	if (!SCRIPT_CHECK(obj, OTRIG_WEAR)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, OTRIG_WEAR)) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sdd.o = obj;
			ret_val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			/* don't allow a wear to take place, if
			* the object is purged.
			*/
			if (!obj) {
				return 0;
			}
			else if (!ret_val || !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
				return ret_val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return ret_val;
}


/**
* Called when a player tries to stop wearing an item.
*
* @param obj_data *obj The item the player wants to remove.
* @param char_data *actor The person trying to do it.
* @return int 0 to prevent the person from removing the item; 1 to allow it.
*/
int remove_otrigger(obj_data *obj, char_data *actor) {
	trig_data *t, *next_t;
	bool multi = FALSE;
	char buf[MAX_INPUT_LENGTH];
	int ret_val = 1;

	if (!SCRIPT_CHECK(obj, OTRIG_REMOVE) || NOHASSLE(actor)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, OTRIG_REMOVE)) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sdd.o = obj;
			ret_val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			/* don't allow a remove to take place, if
			* the object is purged.
			*/
			if (!obj) {
				return 0;
			}
			else if (!ret_val || !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
				return ret_val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return ret_val;
}


/**
* Called when the player attempts to drop/put an item somewhere.
*
* @param obj_data *obj The object being dropped/put/etc.
* @param char_data *actor The person doing.
* @param int mode Any DROP_TRIG_ type.
* @return int 0 to prevent the item being dropped; 1 to allow it.
*/
int drop_otrigger(obj_data *obj, char_data *actor, int mode) {
	trig_data *t, *next_t;
	bool multi = FALSE;
	char buf[MAX_INPUT_LENGTH];
	int ret_val = 1;

	if (!SCRIPT_CHECK(obj, OTRIG_DROP) || NOHASSLE(actor)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, OTRIG_DROP) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			
			switch (mode) {
				case DROP_TRIG_DROP: {
					add_var(&GET_TRIG_VARS(t), "command", "drop", 0);
					break;
				}
				case DROP_TRIG_JUNK: {
					add_var(&GET_TRIG_VARS(t), "command", "junk", 0);
					break;
				}
				case DROP_TRIG_PUT: {
					add_var(&GET_TRIG_VARS(t), "command", "put", 0);
					break;
				}
				case DROP_TRIG_SACRIFICE: {
					add_var(&GET_TRIG_VARS(t), "command", "sacrifice", 0);
					break;
				}
				default: {
					add_var(&GET_TRIG_VARS(t), "command", "unknown", 0);
					break;
				}
			}
			
			sdd.o = obj;
			ret_val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			/* don't allow a drop to take place, if
			* the object is purged (nothing to drop).
			*/
			if (!obj) {
				return 0;
			}
			else if (!ret_val || !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
				return ret_val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return ret_val;
}


/**
* Called when the actor tries to give away an item.
*
* @param obj_data *obj The item being given away.
* @param char_data *actor The person trying to give it.
* @param char_data *victim The person to receive the item.
* @return int 0 to prevent giving the item; 1 to allow it.
*/
int give_otrigger(obj_data *obj, char_data *actor, char_data *victim) {
	trig_data *t, *next_t;
	bool multi = FALSE;
	char buf[MAX_INPUT_LENGTH];
	int ret_val = 1;

	if (!SCRIPT_CHECK(obj, OTRIG_GIVE) || NOHASSLE(actor)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, OTRIG_GIVE) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			ADD_UID_VAR(buf, t, char_script_id(victim), "victim", 0);
			sdd.o = obj;
			ret_val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			/* don't allow a give to take place, if
			* a) the object is purged.
			* b) the object is not carried by the giver.
			*/
			if (!obj || obj->carried_by != actor) {
				return 0;
			}
			else if (!ret_val || !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
				return ret_val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return ret_val;
}


/**
* Called when an item is first loaded.
*
* @param obj_data *obj The item.
* @return int 0 if the obj was purged; 1 otherwise.
*/
int load_otrigger(obj_data *obj) {
	trig_data *trig, *next_trig;
	bool multi = FALSE;
	int return_val = 1;	// default to ok

	if (!SCRIPT_CHECK(obj, OTRIG_LOAD)) {
		return return_val;
	}
	
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), trig, next_trig) {
		if (multi && !IS_SET(GET_TRIG_TYPE(trig), OTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(trig, OTRIG_LOAD) && (number(1, 100) <= GET_TRIG_NARG(trig))) {
			union script_driver_data_u sdd;
			sdd.o = obj;
			return_val = script_driver(&sdd, trig, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			if (!obj) {
				// purged!
				return_val = 0;
			}
			
			if (!return_val || !IS_SET(GET_TRIG_TYPE(trig), OTRIG_ALLOW_MULTIPLE)) {
				break;
			}
			else {
				multi = TRUE;
			}
		}
	}
	
	return return_val;
}


/**
* Called when an ability targets an object.
*
* @param char_data *actor The person performing the ability.
* @param obj_data *obj The object being targeted by the ability.
* @param any_vnum abil The vnum of the ability.
* @return int 0 to prevent the ability; 1 to allow it.
*/
int ability_otrigger(char_data *actor, obj_data *obj, any_vnum abil) {
	trig_data *t, *next_t;
	bool multi = FALSE;
	char buf[MAX_INPUT_LENGTH];
	int val = 1;
	ability_data *ab;

	if (obj == NULL || !(ab = find_ability_by_vnum(abil))) {
		return 1;
	}

	if (!SCRIPT_CHECK(obj, OTRIG_ABILITY)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, OTRIG_ABILITY) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;

			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sprintf(buf, "%d", abil);
			add_var(&GET_TRIG_VARS(t), "ability", buf, 0);
			add_var(&GET_TRIG_VARS(t), "abilityname", ABIL_NAME(ab), 0);
			sdd.o = obj;
			val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			
			if (!val || !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
				return val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return 1;
}


/**
* Called as a person tries to leave the room the object is in.
*
* @param room_data *room The room the person is trying to leave.
* @param char_data *actor The person trying to leave.
* @param int dir The direction they are trying to go (passed through to %direction%).
* @param char *custom_dir Optional: A different value for %direction% (may be NULL).
* @param char *method Optional: The method by which they moved (may be NULL).
* @return int 0 = block the leave, 1 = pass
*/
int leave_otrigger(room_data *room, char_data *actor, int dir, char *custom_dir, char *method) {
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int temp, final = 1, any_in_room = -1;
	obj_data *obj, *obj_next;
	
	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return 1;
	}
	
	DL_FOREACH_SAFE2(ROOM_CONTENTS(room), obj, obj_next, next_content) {
		if (!SCRIPT_CHECK(obj, OTRIG_LEAVE)) {
			continue;
		}

		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
			if (!TRIGGER_CHECK(t, OTRIG_LEAVE)) {
				continue;
			}
			if (TRIG_IS_LOCAL(t)) {
				if (any_in_room == -1) {
					any_in_room = any_players_in_room(room);
				}
				if (!any_in_room) {
					continue;	// requires a player
				}
			}
			if (number(1, 100) <= GET_TRIG_NARG(t)) {
				union script_driver_data_u sdd;
				if (custom_dir) {
					add_var(&GET_TRIG_VARS(t), "direction", custom_dir, 0);
				}
				else {
					add_var(&GET_TRIG_VARS(t), "direction", (dir >= 0 && dir < NUM_OF_DIRS) ? ((char *)dirs[dir]) : "none", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
				sdd.o = obj;
				temp = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
				obj = sdd.o;
				if (temp == 0 || !obj) {
					final = 0;
					break;
				}
			}
		}
	}

	return final;
}


/**
* A trigger that fires when a character tries to 'consume' an object. Most
* consume triggers can block the action by returning 0, but a few (poisons and
* shooting) cannot block.
*
* @param obj_data *obj The item to test for triggers.
* @param char_data *actor The player consuming the object.
* @param int cmd The command that's consuming the item (OCMD_*).
* @param char_data *target Optional: If the consume is targeted (e.g. poisons), the target (may be NULL).
* @return int 0 to block consume, 1 to continue.
*/
int consume_otrigger(obj_data *obj, char_data *actor, int cmd, char_data *target) {
	trig_data *t, *next_t;
	bool multi = FALSE;
	char buf[MAX_INPUT_LENGTH];
	int ret_val = 1;

	if (!SCRIPT_CHECK(obj, OTRIG_CONSUME)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, OTRIG_CONSUME)) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			if (target) {
				ADD_UID_VAR(buf, t, char_script_id(target), "target", 0);
			}
			else {
				add_var(&GET_TRIG_VARS(t), "target", "", 0);
			}
			
			switch (cmd) {
				case OCMD_EAT: {
					add_var(&GET_TRIG_VARS(t), "command", "eat", 0);
					break;
				}
				case OCMD_TASTE: {
					add_var(&GET_TRIG_VARS(t), "command", "taste", 0);
					break;
				}
				case OCMD_DRINK: {
					add_var(&GET_TRIG_VARS(t), "command", "drink", 0);
					break;
				}
				case OCMD_SIP: {
					add_var(&GET_TRIG_VARS(t), "command", "sip", 0);
					break;
				}
				case OCMD_QUAFF: {
					add_var(&GET_TRIG_VARS(t), "command", "quaff", 0);
					break;
				}
				case OCMD_READ: {
					add_var(&GET_TRIG_VARS(t), "command", "read", 0);
					break;
				}
				case OCMD_BUILD: {
					add_var(&GET_TRIG_VARS(t), "command", "build", 0);
					break;
				}
				case OCMD_CRAFT: {
					add_var(&GET_TRIG_VARS(t), "command", "craft", 0);
					break;
				}
				case OCMD_SHOOT: {
					add_var(&GET_TRIG_VARS(t), "command", "shoot", 0);
					break;
				}
				case OCMD_POISON: {
					add_var(&GET_TRIG_VARS(t), "command", "poison", 0);
					break;
				}
				case OCMD_PAINT: {
					add_var(&GET_TRIG_VARS(t), "command", "paint", 0);
					break;
				}
				case OCMD_LIGHT: {
					add_var(&GET_TRIG_VARS(t), "command", "light", 0);
					break;
				}
			}
			sdd.o = obj;
			ret_val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			// ensure object wasn't purged
			if (!obj) {
				return 0;
			}
			else if (!ret_val || !IS_SET(GET_TRIG_TYPE(t), OTRIG_ALLOW_MULTIPLE)) {
				return ret_val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return ret_val;
}


/**
* Called when a character is finished with an object, e.g. reading a book.
*
* @param obj_data *obj The object being finished.
* @param char_data *actor The person finishing it.
* @return int 1 to proceed, 0 if a script returned 0 or the obj was purged.
*/
int finish_otrigger(obj_data *obj, char_data *actor) {
	char buf[MAX_INPUT_LENGTH];
	int val = TRUE;
	trig_data *t, *next_t;

	if (!SCRIPT_CHECK(obj, OTRIG_FINISH)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (TRIGGER_CHECK(t, OTRIG_FINISH) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			sdd.o = obj;
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			if (!val || !obj) {
				break;
			}
		}
	}
	
	return (val && obj) ? 1 : 0;
}


/**
* Called on an object after the mud starts up.
*
* @param obj_data *obj The item.
*/
void reboot_otrigger(obj_data *obj) {
	trig_data *t, *next_t;
	int val;

	if (!SCRIPT_CHECK(obj, OTRIG_REBOOT)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(obj)), t, next_t) {
		if (TRIGGER_CHECK(t, OTRIG_REBOOT)) {
			union script_driver_data_u sdd;
			sdd.o = obj;
			val = script_driver(&sdd, t, OBJ_TRIGGER, TRIG_NEW);
			obj = sdd.o;
			if (!val) {
				break;
			}
		}
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// WORLD TRIGGERS //////////////////////////////////////////////////////////

/**
* Called on adventure tile/room as the adventure instance is being removed.
*
* @param room_data *room The adventure's location.
*/
void adventure_cleanup_wtrigger(room_data *room) {
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;

	if (!SCRIPT_CHECK(room, WTRIG_ADVENTURE_CLEANUP)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (TRIGGER_CHECK(t, WTRIG_ADVENTURE_CLEANUP) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			sdd.r = room;
			if (!script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW)) {
				break;
			}
		}
	}
}


/**
* Called when a building is completed.
*
* @param room_data *room The building's location.
*/
void complete_wtrigger(room_data *room) {
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;

	if (!SCRIPT_CHECK(room, WTRIG_COMPLETE)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (TRIGGER_CHECK(t, WTRIG_COMPLETE) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			sdd.r = room;
			if (!script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW)) {
				break;
			}
		}
	}
}


/**
* Called when a player attempts to dismantle or redesignate a room. Returning
* a 0 from the script will prevent the dismantle if possible.
*
* NOT preventable if a player is dismantling a building, but this trigger is
* firing on some interior room (e.g. a study).
*
* It's also possible for there to be NO actor if a building is being destroyed
* by something other than a player.
*
* @param room_data *room The room attempting to dismantle.
* @param char_data *actor Optional: The player attempting the dismantle (may be NULL).
* @param bool preventable If TRUE, returning 0 will prevent the dismantle.
* @return int The exit code from the script.
*/
int dismantle_wtrigger(room_data *room, char_data *actor, bool preventable) {
	char buf[MAX_INPUT_LENGTH];
	trig_data *trig;
	int one, value = 1;
	
	if (!SCRIPT_CHECK(room, WTRIG_DISMANTLE)) {
		return 1;
	}
	
	LL_FOREACH(TRIGGERS(SCRIPT(room)), trig) {
		if (TRIGGER_CHECK(trig, WTRIG_DISMANTLE)) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, trig, room_script_id(room), "room", 0);
			add_var(&GET_TRIG_VARS(trig), "preventable", preventable ? "1" : "0", 0);
			if (actor) {
				ADD_UID_VAR(buf, trig, char_script_id(actor), "actor", 0);
			}
			else {
				add_var(&GET_TRIG_VARS(trig), "actor", "", 0);
			}
			sdd.r = room;
			one = script_driver(&sdd, trig, WLD_TRIGGER, TRIG_NEW);
			value = MIN(value, one);	// can be set to 0
			if (!one) {
				break;	// stop on first zero
			}
		}
	}

	return value;
}


/**
* Called when a person walks into a room.
*
* @param room_data *room The room being entered.
* @param char_data *actor The person who entered.
* @param int dir Which direction they moved.
* @param char *method The way the person entered.
* @return int 1 to allow the entry; 0 to try to send them back.
*/
int enter_wtrigger(room_data *room, char_data *actor, int dir, char *method) {
	union script_driver_data_u sdd;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int any_in_room = -1, val = 1, this;
	bool multi = FALSE;
	
	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return TRUE;
	}

	if (!SCRIPT_CHECK(room, WTRIG_ENTER)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
			continue;	// alread did an allow-multi
		}
		if (!TRIGGER_CHECK(t, WTRIG_ENTER) || (number(1, 100) > GET_TRIG_NARG(t))) {
			continue;
		}
		if (TRIG_IS_LOCAL(t)) {
			if (any_in_room == -1) {
				any_in_room = any_players_in_room(room);
			}
			if (!any_in_room) {
				continue;	// requires a player
			}
		}
		
		// ok:
		ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
		if (dir>=0 && dir < NUM_OF_DIRS) {
			add_var(&GET_TRIG_VARS(t), "direction", (char *)dirs[rev_dir[dir]], 0);
		}
		else {
			add_var(&GET_TRIG_VARS(t), "direction", "none", 0);
		}
		ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
		add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
		sdd.r = room;
		this = script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW);
		val = MIN(val, this);
		
		if (!val || !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
			return val;
		}
		else {
			multi = TRUE;
		}
	}

	return val;
}


/**
* Called when an actor tries to turn in a quest.
*
* @param room_data *room The room the person is trying top do this in.
* @param char_data *actor The person trying to finish a quest.
* @param quest_data *quest Which quest they're trying to finish.
* @param struct instance_data *inst What instance the quest is associated with, if any.
* @return int 0 to prevent it; 1 to allow it.
*/
int finish_quest_wtrigger(room_data *room, char_data *actor, quest_data *quest, struct instance_data *inst) {
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;

	if (!SCRIPT_CHECK(room, WTRIG_FINISH_QUEST)) {
		return 1;
	}
	
	// store instance globally to allow %instance.xxx% in scripts
	quest_instance_global = inst;

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (TRIGGER_CHECK(t, WTRIG_FINISH_QUEST)) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			if (quest) {
				snprintf(buf, sizeof(buf), "%d", QUEST_VNUM(quest));
				add_var(&GET_TRIG_VARS(t), "questvnum", buf, 0);
				add_var(&GET_TRIG_VARS(t), "questname", QUEST_NAME(quest), 0);
			}
			else {	// no quest?
				add_var(&GET_TRIG_VARS(t), "questvnum", "0", 0);
				add_var(&GET_TRIG_VARS(t), "questname", "Unknown", 0);
			}
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sdd.r = room;
			if (!script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW)) {
				quest_instance_global = NULL;	// un-set this
				return FALSE;
			}
		}
	}
	
	quest_instance_global = NULL;	// un-set this
	return TRUE;
}


/**
* Called when a building first loads -- often before it's complete.
*
* @param room_data *room The room.
*/
void load_wtrigger(room_data *room) {
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;
	int val = 0;
	bool multi = FALSE;

	if (!SCRIPT_CHECK(room, WTRIG_LOAD)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, WTRIG_LOAD) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			sdd.r = room;
			val = script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW);
			
			if (!val || !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
				break;
			}
			else {
				multi = TRUE;
			}
		}
	}
}


/**
* Called on rooms in an instance when the room resets. Or, out on the map,
* reset triggers are called every few minutes.
*
* @param room_data *room The room being reset.
*/
void reset_wtrigger(room_data *room) {
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;
	bool multi = FALSE;

	if (!SCRIPT_CHECK(room, WTRIG_RESET)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, WTRIG_RESET) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			sdd.r = room;
			script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW);
			
			if (IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
				multi = TRUE;
			}
			else {
				break;
			}
		}
	}
}


/**
* Buy trigger (room): fires when someone tries to buy
*
* @param char_data *actor The person trying to buy.
* @param char_data *shopkeeper The mob shopkeeper, if any (many shops have none).
* @param obj_data *buying The item being bought.
* @param int cost The amount to be charged.
* @param any_vnum currency The currency type (NOTHING for coins).
* @return int 0 if a trigger blocked the buy (stop); 1 if not (ok to continue).
*/
int buy_wtrigger(char_data *actor, char_data *shopkeeper, obj_data *buying, int cost, any_vnum currency) {
	room_data *room = IN_ROOM(actor);
	union script_driver_data_u sdd;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t;

	if (!SCRIPT_CHECK(IN_ROOM(actor), WTRIG_BUY)) {
		return 1;
	}
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return 1;	// gods not affected
	}
	
	LL_FOREACH(TRIGGERS(SCRIPT(room)), t) {
		if (!TRIGGER_CHECK(t, WTRIG_BUY)) {
			continue;
		}
		
		// vars
		ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
		ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
		ADD_UID_VAR(buf, t, obj_script_id(buying), "obj", 0);
		if (shopkeeper) {
			ADD_UID_VAR(buf, t, char_script_id(shopkeeper), "shopkeeper", 0);
		}
			else {
				add_var(&GET_TRIG_VARS(t), "shopkeeper", "", 0);
			}
		
		snprintf(buf, sizeof(buf), "%d", cost);
		add_var(&GET_TRIG_VARS(t), "cost", buf, 0);
		
		if (currency == NOTHING) {
			strcpy(buf, "coins");
		}
		else {
			snprintf(buf, sizeof(buf), "%d", currency);
		}
		add_var(&GET_TRIG_VARS(t), "currency", buf, 0);
		
		// run it
		sdd.r = room;
		if (!script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW)) {
			return 0;
		}
	}
	
	return 1;
}


/**
* Command trigger (room).
*
* @param char_data *actor The person typing a command.
* @param char *cmd The command as-typed (first word).
* @param char *argument Any arguments (remaining text).
* @param int mode CMDTRG_EXACT or CMDTRG_ABBREV.
* @return int 1 if a trigger ran (stop); 0 if not (ok to continue).
*/
int command_wtrigger(char_data *actor, char *cmd, char *argument, int mode) {
	room_data *room;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];

	if (!actor || !SCRIPT_CHECK(IN_ROOM(actor), WTRIG_COMMAND)) {
		return 0;
	}

	/* prevent people we like from becoming trapped :P */
	if (!valid_dg_target(actor, 0)) {
		return 0;
	}

	room = IN_ROOM(actor);
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (!TRIGGER_CHECK(t, WTRIG_COMMAND)) {
			continue;
		}

		if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
			syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: W-Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
			continue;
		}

		if (match_command_trig(cmd, GET_TRIG_ARG(t), mode)) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			skip_spaces(&argument);
			add_var(&GET_TRIG_VARS(t), "arg", argument, 0);
			skip_spaces(&cmd);
			add_var(&GET_TRIG_VARS(t), "cmd", cmd, 0);

			sdd.r = room;
			if (script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW)) {
				return 1;
			}
		}
	}

	return 0;
}


/**
* Called when someone utters some words in the room. Does not catch private
* messages such as whisper, nor out-of-character messages.
*
* @param char_data *actor The person speaking.
* @param char *str What was said.
* @param generic_data *language The language it was spoken in.
*/
void speech_wtrigger(char_data *actor, char *str, generic_data *language) {
	room_data *room;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int any_in_room = -1;
	bool multi = FALSE;

	if (!actor || !SCRIPT_CHECK(IN_ROOM(actor), WTRIG_SPEECH)) {
		return;
	}

	room = IN_ROOM(actor);
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (!TRIGGER_CHECK(t, WTRIG_SPEECH)) {
			continue;
		}
		if (TRIG_IS_LOCAL(t)) {
			if (any_in_room == -1) {
				any_in_room = any_players_in_room(room);
			}
			if (!any_in_room) {
				continue;	// requires a player
			}
		}

		if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
			syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: W-Speech Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
			continue;
		}

		if (*GET_TRIG_ARG(t)=='*' || (GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) || (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str))) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			add_var(&GET_TRIG_VARS(t), "speech", strip_color(str), 0);
			add_var(&GET_TRIG_VARS(t), "lang", language ? GEN_NAME(language) : "", 0);
			sprintf(buf, "%d", language ? GEN_VNUM(language) : NOTHING);
			add_var(&GET_TRIG_VARS(t), "lang_vnum", buf, 0);
			sdd.r = room;
			script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW);
			
			if (IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
				multi = TRUE;
			}
			else {
				break;
			}
		}
	}
}


/**
* Called when an actor tries to start a quest.
*
* @param room_data *room The room where the person is trying to start it.
* @param char_data *actor The person trying to start a quest.
* @param quest_data *quest Which quest they're trying to start.
* @param struct instance_data *inst What instance the quest is associated with, if any.
* @return int 0 to prevent it; 1 to allow it.
*/
int start_quest_wtrigger(room_data *room, char_data *actor, quest_data *quest, struct instance_data *inst) {
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;

	if (!SCRIPT_CHECK(room, WTRIG_START_QUEST)) {
		return 1;
	}
	
	// store instance globally to allow %instance.xxx% in scripts
	quest_instance_global = inst;
	
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (TRIGGER_CHECK(t, WTRIG_START_QUEST)) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			if (quest) {
				snprintf(buf, sizeof(buf), "%d", QUEST_VNUM(quest));
				add_var(&GET_TRIG_VARS(t), "questvnum", buf, 0);
				add_var(&GET_TRIG_VARS(t), "questname", QUEST_NAME(quest), 0);
			}
			else {	// no quest?
				add_var(&GET_TRIG_VARS(t), "questvnum", "0", 0);
				add_var(&GET_TRIG_VARS(t), "questname", "Unknown", 0);
			}
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sdd.r = room;
			if (!script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW)) {
				quest_instance_global = NULL;	// un-set this
				return FALSE;
			}
		}
	}
	
	quest_instance_global = NULL;	// un-set this
	return TRUE;
}


/**
* Called when someone tries to drop an item in the room.
* 
* @param obj_data *obj The object being dropped/put/etc.
* @param char_data *actor The person doing.
* @param int mode Any DROP_TRIG_ type.
* @return int 0 to prevent the drop; 1 to allow it.
*/
int drop_wtrigger(obj_data *obj, char_data *actor, int mode) {
	room_data *room;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int ret_val;
	bool multi = FALSE;

	if (!actor || !SCRIPT_CHECK(IN_ROOM(actor), WTRIG_DROP)) {
		return 1;
	}

	room = IN_ROOM(actor);
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, WTRIG_DROP) && (number(1, 100) <= GET_TRIG_NARG(t))) {	
			union script_driver_data_u sdd;

			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			ADD_UID_VAR(buf, t, obj_script_id(obj), "object", 0);
			
			switch (mode) {
				case DROP_TRIG_DROP: {
					add_var(&GET_TRIG_VARS(t), "command", "drop", 0);
					break;
				}
				case DROP_TRIG_JUNK: {
					add_var(&GET_TRIG_VARS(t), "command", "junk", 0);
					break;
				}
				case DROP_TRIG_PUT: {
					add_var(&GET_TRIG_VARS(t), "command", "put", 0);
					break;
				}
				case DROP_TRIG_SACRIFICE: {
					add_var(&GET_TRIG_VARS(t), "command", "sacrifice", 0);
					break;
				}
				default: {
					add_var(&GET_TRIG_VARS(t), "command", "unknown", 0);
					break;
				}
			}
			
			sdd.r = room;
			ret_val = script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW);
			if (!ret_val || obj->carried_by != actor) {
				return 0;
			}
			else if (IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
				multi = TRUE;
			}
			else {
				return ret_val;
			}
		}
	}

	return 1;
}


/**
* Called when someone tries to use an ability in the room, no matter what the
* target.
*
* @param char_data *actor The person doing the ability.
* @param char_data *vict The character target, if any.
* @param obj_data *obj The object target, if any.
* @param vehicle_data *veh The vehicle target, if any.
* @param any_vnum abil The ability being used.
* @return int 0 to prevent the ability; 1 to allow it.
*/
int ability_wtrigger(char_data *actor, char_data *vict, obj_data *obj, vehicle_data *veh, any_vnum abil) {
	room_data *room;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	ability_data *ab;
	int val;
	bool multi = FALSE;

	if (!actor || !(ab = find_ability_by_vnum(abil)) || !SCRIPT_CHECK(IN_ROOM(actor), WTRIG_ABILITY)) {
		return 1;
	}

	room = IN_ROOM(actor);
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, WTRIG_ABILITY) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;

			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			if (vict) {
				ADD_UID_VAR(buf, t, char_script_id(vict), "victim", 0);
			}
			if (obj) {
				ADD_UID_VAR(buf, t, obj_script_id(obj), "object", 0);
			}
			if (veh) {
				ADD_UID_VAR(buf, t, veh_script_id(veh), "vehicle", 0);
			}
			sprintf(buf, "%d", abil);
			add_var(&GET_TRIG_VARS(t), "ability", buf, 0);
			add_var(&GET_TRIG_VARS(t), "abilityname", ABIL_NAME(ab), 0);
			sdd.r = room;
			val = script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW);
			if (!val || !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
				return val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return 1;
}


/**
* Called as a character tries to leave the room.
*
* @param room_data *room The room the person is trying to leave.
* @param char_data *actor The person trying to leave.
* @param int dir The direction they are trying to go (passed through to %direction%).
* @param char *custom_dir Optional: A different value for %direction% (may be NULL).
* @param char *method Optional: The method by which they moved (may be NULL).
* @return int 0 = block the leave; 1 = pass and allow.
*/
int leave_wtrigger(room_data *room, char_data *actor, int dir, char *custom_dir, char *method) {
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int any_in_room = -1, val;
	bool multi = FALSE;

	if (!SCRIPT_CHECK(room, WTRIG_LEAVE)) {
		return 1;
	}
	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return 1;
	}
	
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (!TRIGGER_CHECK(t, WTRIG_LEAVE)) {
			continue;
		}
		if (TRIG_IS_LOCAL(t)) {
			if (any_in_room == -1) {
				any_in_room = any_players_in_room(room);
			}
			if (!any_in_room) {
				continue;	// requires a player
			}
		}
		if (number(1, 100) <= GET_TRIG_NARG(t)) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			if (custom_dir) {
				add_var(&GET_TRIG_VARS(t), "direction", custom_dir, 0);
			}
			else {
				add_var(&GET_TRIG_VARS(t), "direction", (dir >= 0 && dir < NUM_OF_DIRS) ? ((char *)dirs[dir]) : "none", 0);
			}
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
			sdd.r = room;
			val = script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW);
			if (!val || !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
				return val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return 1;
}


/**
* Called when someone in the room tries to operate a door.
*
* @param char_data *actor The person trying to work the door.
* @param int subcmd Which door command was used.
* @param int dir Which direction the door is in.
* @return int 0 to prevent the door action; 1 to allow it.
*/
int door_wtrigger(char_data *actor, int subcmd, int dir) {
	room_data *room;
	trig_data *t, *next_t;
	char buf[MAX_INPUT_LENGTH];
	int val = 1;
	bool multi = FALSE;

	if (!actor || !SCRIPT_CHECK(IN_ROOM(actor), WTRIG_DOOR)) {
		return 1;
	}

	room = IN_ROOM(actor);
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, WTRIG_DOOR) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			add_var(&GET_TRIG_VARS(t), "cmd", (char *)cmd_door[subcmd], 0);
			if (dir >= 0 && dir < NUM_OF_DIRS) {
				add_var(&GET_TRIG_VARS(t), "direction", (char *)dirs[dir], 0);
			}
			else {
				add_var(&GET_TRIG_VARS(t), "direction", "none", 0);
			}
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			sdd.r = room;
			val = script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW);
			if (!val || !IS_SET(GET_TRIG_TYPE(t), WTRIG_ALLOW_MULTIPLE)) {
				return val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return val;
}


/**
* Called on all rooms after the game starts up.
*
* @param room_data *room The room.
*/
void reboot_wtrigger(room_data *room) {
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;
	int val;

	if (!SCRIPT_CHECK(room, WTRIG_REBOOT)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(room)), t, next_t) {
		if (TRIGGER_CHECK(t, WTRIG_REBOOT)) {
			union script_driver_data_u sdd;
			ADD_UID_VAR(buf, t, room_script_id(room), "room", 0);
			sdd.r = room;
			val = script_driver(&sdd, t, WLD_TRIGGER, TRIG_NEW);
			if (!val) {
				break;
			}
		}
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// VEHICLE TRIGGERS ////////////////////////////////////////////////////////

/**
* Ability trigger for an ability targeting the vehicle.
*
* @param char_data *actor Which actor using the ability.
* @param vehicle_data *veh Which vehicle it's targeting
* @param any_vnum abil Which ability.
* @return int 1 to proceed; 0 to prevent it the ability.
*/
int ability_vtrigger(char_data *actor, vehicle_data *veh, any_vnum abil) {
	bool multi = FALSE;
	char buf[MAX_INPUT_LENGTH];
	int val;
	ability_data *ab;
	trig_data *trig, *next_trig;

	if (veh == NULL || VEH_IS_EXTRACTED(veh) || !(ab = find_ability_by_vnum(abil))) {
		return 1;
	}

	if (!SCRIPT_CHECK(veh, VTRIG_ABILITY)) {
		return 1;
	}
	
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), trig, next_trig) {
		if (multi && !IS_SET(GET_TRIG_TYPE(trig), VTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(trig, VTRIG_ABILITY) && (number(1, 100) <= GET_TRIG_NARG(trig))) {
			union script_driver_data_u sdd;
			
			ADD_UID_VAR(buf, trig, char_script_id(actor), "actor", 0);
			sprintf(buf, "%d", abil);
			add_var(&GET_TRIG_VARS(trig), "ability", buf, 0);
			add_var(&GET_TRIG_VARS(trig), "abilityname", ABIL_NAME(ab), 0);
			sdd.v = veh;
			val = script_driver(&sdd, trig, VEH_TRIGGER, TRIG_NEW);
			
			if (!val || !IS_SET(GET_TRIG_TYPE(trig), VTRIG_ALLOW_MULTIPLE)) {
				return val;
			}
			else {
				multi = TRUE;
			}
		}
	}

	return 1;
}


/**
* Buy trigger (vehicle): fires when someone tries to buy
*
* @param char_data *actor The person trying to buy.
* @param char_data *shopkeeper The mob shopkeeper, if any (many shops have none).
* @param obj_data *buying The item being bought.
* @param int cost The amount to be charged.
* @param any_vnum currency The currency type (NOTHING for coins).
* @return int 0 if a trigger blocked the buy (stop); 1 if not (ok to continue).
*/
int buy_vtrigger(char_data *actor, char_data *shopkeeper, obj_data *buying, int cost, any_vnum currency) {
	union script_driver_data_u sdd;
	vehicle_data *veh, *next_veh;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t;

	// gods not affected
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return 1;
	}
	
	DL_FOREACH_SAFE2(ROOM_VEHICLES(IN_ROOM(actor)), veh, next_veh, next_in_room) {
		if (VEH_IS_EXTRACTED(veh) || !SCRIPT_CHECK(veh, VTRIG_BUY)) {
			continue;
		}
		
		LL_FOREACH(TRIGGERS(SCRIPT(veh)), t) {
			if (!TRIGGER_CHECK(t, VTRIG_BUY)) {
				continue;
			}
			
			// vars
			ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
			ADD_UID_VAR(buf, t, obj_script_id(buying), "obj", 0);
			if (shopkeeper) {
				ADD_UID_VAR(buf, t, char_script_id(shopkeeper), "shopkeeper", 0);
			}
			else {
				add_var(&GET_TRIG_VARS(t), "shopkeeper", "", 0);
			}
			
			snprintf(buf, sizeof(buf), "%d", cost);
			add_var(&GET_TRIG_VARS(t), "cost", buf, 0);
			
			if (currency == NOTHING) {
				strcpy(buf, "coins");
			}
			else {
				snprintf(buf, sizeof(buf), "%d", currency);
			}
			add_var(&GET_TRIG_VARS(t), "currency", buf, 0);
			
			// run it
			sdd.v = veh;
			if (!script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW)) {
				return 0;
			}
		}
	}

	return 1;
}


/**
* Command trigger (vehicle).
*
* @param char_data *actor The person typing a command.
* @param char *cmd The command as-typed (first word).
* @param char *argument Any arguments (remaining text).
* @param int mode CMDTRG_EXACT or CMDTRG_ABBREV.
* @return int 1 if a trigger ran (stop); 0 if not (ok to continue).
*/
int command_vtrigger(char_data *actor, char *cmd, char *argument, int mode) {
	vehicle_data *veh, *next_veh;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;

	/* prevent people we like from becoming trapped :P */
	if (!valid_dg_target(actor, 0)) {
		return 0;
	}
	
	DL_FOREACH_SAFE2(ROOM_VEHICLES(IN_ROOM(actor)), veh, next_veh, next_in_room) {
		if (!VEH_IS_EXTRACTED(veh) && SCRIPT_CHECK(veh, VTRIG_COMMAND)) {
			LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
				if (!TRIGGER_CHECK(t, VTRIG_COMMAND)) {
					continue;
				}

				if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
					syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: Command Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
					continue;
				}

				if (match_command_trig(cmd, GET_TRIG_ARG(t), mode)) {
					union script_driver_data_u sdd;
					ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
					skip_spaces(&argument);
					add_var(&GET_TRIG_VARS(t), "arg", argument, 0);
					skip_spaces(&cmd);
					add_var(&GET_TRIG_VARS(t), "cmd", cmd, 0);
					sdd.v = veh;

					if (script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW)) {
						return 1;
					}
				}
			}
		}
	}

	return 0;
}


/**
* Called when a vehicle is destroyed.
*
* @param vehicle_data *veh The vehicle being destroyed.
* @param char *method What destroyed it ("burning", etc).
* @return int 0 will prevent destruction of the vehicle; 1 is a normal result
*/
int destroy_vtrigger(vehicle_data *veh, char *method) {
	bool multi = FALSE;
	int val = 1;
	trig_data *t, *next_t;

	if (!SCRIPT_CHECK(veh, VTRIG_DESTROY)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, VTRIG_DESTROY) && (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
			sdd.v = veh;
			val = script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW);
			if (!val || !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
				break;
			}
			else {
				multi = TRUE;
			}

		}
	}

	return val;
}


/**
* Called when a player attempts to dismantle a vehicle. Returning a 0 from the
* script will prevent the dismantle if possible.
*
* It's also possible for there to be NO actor if a vehicle is being destroyed
* by something other than a player.
*
* @param char_data *actor Optional: The player attempting the dismantle (may be NULL).
* @param vehicle_data *veh The vehicle attemping to be dismantled.
* @param bool preventable If TRUE, returning 0 will prevent the dismantle.
* @return int The exit code from the script: 1 to continue, 0 to prevent if possible.
*/
int dismantle_vtrigger(char_data *actor, vehicle_data *veh, bool preventable) {
	char buf[MAX_INPUT_LENGTH];
	trig_data *trig, *next_trig;
	int one, value = 1;
	
	if (veh == NULL || VEH_IS_EXTRACTED(veh)) {
		return 1;
	}

	if (!SCRIPT_CHECK(veh, VTRIG_DISMANTLE)) {
		return 1;
	}
	
	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), trig, next_trig) {
		if (TRIGGER_CHECK(trig, VTRIG_DISMANTLE)) {
			union script_driver_data_u sdd;
			
			add_var(&GET_TRIG_VARS(trig), "preventable", preventable ? "1" : "0", 0);
			if (actor) {
				ADD_UID_VAR(buf, trig, char_script_id(actor), "actor", 0);
			}
			else {
				add_var(&GET_TRIG_VARS(trig), "actor", "", 0);
			}
			sdd.v = veh;
			one = script_driver(&sdd, trig, VEH_TRIGGER, TRIG_NEW);
			value = MIN(value, one);	// can be set to 0
			if (!one) {
				break;	// stop on first zero
			}
		}
	}

	return value;
}


/**
* Called when a vehicle enters a new room.
*
* @param vehicle_data *veh The vehicle that moved.
* @param char *method How it moved.
* @return int 0 to send it back; 1 to let it stay.
*/
int entry_vtrigger(vehicle_data *veh, char *method) {
	union script_driver_data_u sdd;
	trig_data *t, *next_t;
	bool multi = FALSE;
	int any_in_room = -1, ret_val = 1;

	if (!SCRIPT_CHECK(veh, VTRIG_ENTRY)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (!TRIGGER_CHECK(t, VTRIG_ENTRY) || (number(1, 100) > GET_TRIG_NARG(t))) {
			continue;
		}
		if (TRIG_IS_LOCAL(t)) {
			if (any_in_room == -1) {
				any_in_room = any_players_in_room(IN_ROOM(veh));
			}
			if (!any_in_room) {
				continue;	// requires a player
			}
		}
		
		// ok:
		add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
		sdd.v = veh;
		ret_val = script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW);
		if (!ret_val || !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
			return ret_val;
		}
		else {
			multi = TRUE;
		}
	}

	return ret_val;
}


/**
* Called when an actor tries to turn in a quest.
*
* @param char_data *actor The person trying to finish a quest.
* @param quest_data *quest Which quest they're trying to finish.
* @param struct instance_data *inst What instance the quest is associated with, if any.
* @return int 0 to prevent it; 1 to allow it.
*/
int finish_quest_vtrigger(char_data *actor, quest_data *quest, struct instance_data *inst) {
	vehicle_data *veh, *next_veh;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;

	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return TRUE;
	}
	
	// store instance globally to allow %instance.xxx% in scripts
	quest_instance_global = inst;
	
	DL_FOREACH_SAFE2(ROOM_VEHICLES(IN_ROOM(actor)), veh, next_veh, next_in_room) {
		if (VEH_IS_EXTRACTED(veh) || !SCRIPT_CHECK(veh, VTRIG_FINISH_QUEST)) {
			continue;
		}

		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
			if (TRIGGER_CHECK(t, VTRIG_FINISH_QUEST)) {
				union script_driver_data_u sdd;
				if (quest) {
					snprintf(buf, sizeof(buf), "%d", QUEST_VNUM(quest));
					add_var(&GET_TRIG_VARS(t), "questvnum", buf, 0);
					add_var(&GET_TRIG_VARS(t), "questname", QUEST_NAME(quest), 0);
				}
				else {	// no quest?
					add_var(&GET_TRIG_VARS(t), "questvnum", "0", 0);
					add_var(&GET_TRIG_VARS(t), "questname", "Unknown", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				sdd.v = veh;
				if (!script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW)) {
					quest_instance_global = NULL;	// un-set this
					return FALSE;
				}
			}
		}
	}
	
	quest_instance_global = NULL;	// un-set this
	return TRUE;
}


/**
* Called when a person walks into a room to run triggers on vehicles.
*
* @param char_data *ch The person who moved.
* @param int dir Which direction they moved.
* @param char *method The way the person entered.
* @return int 1 to allow the entry; 0 to try to send them back.
*/
int greet_vtrigger(char_data *actor, int dir, char *method) {
	int intermediate, final = TRUE, any_in_room = -1;
	vehicle_data *veh, *next_veh;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;

	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return 1;
	}
	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return TRUE;
	}
	
	DL_FOREACH_SAFE2(ROOM_VEHICLES(IN_ROOM(actor)), veh, next_veh, next_in_room) {
		if (VEH_IS_EXTRACTED(veh) || !SCRIPT_CHECK(veh, VTRIG_GREET)) {
			continue;
		}

		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
			if (!TRIGGER_CHECK(t, VTRIG_GREET)) {
				continue;
			}
			if (TRIG_IS_LOCAL(t)) {
				if (any_in_room == -1) {
					any_in_room = any_players_in_room(IN_ROOM(veh));
				}
				if (!any_in_room) {
					continue;	// requires a player
				}
			}
			if (number(1, 100) <= GET_TRIG_NARG(t)) {
				union script_driver_data_u sdd;
				if (dir >= 0 && dir < NUM_OF_DIRS) {
					add_var(&GET_TRIG_VARS(t), "direction", (char *)dirs[rev_dir[dir]], 0);
				}
				else {
					add_var(&GET_TRIG_VARS(t), "direction", "none", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
				sdd.v = veh;
				intermediate = script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW);
				if (!intermediate) {
					final = FALSE;
				}
				continue;
			}
		}
	}
	return final;
}


/**
* @param char_data *actor The person trying to leave.
* @param int dir The direction they are trying to go (passed through to %direction%).
* @param char *custom_dir Optional: A different value for %direction% (may be NULL).
* @param char *method Optional: The method by which they moved (may be NULL).
* @return int 0 = block the leave, 1 = pass
*/
int leave_vtrigger(char_data *actor, int dir, char *custom_dir, char *method) {
	vehicle_data *veh, *next_veh;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;
	bool multi = FALSE;
	int any_in_room = -1, val = 1;
	
	if (IS_IMMORTAL(actor) && (GET_INVIS_LEV(actor) > LVL_MORTAL || PRF_FLAGGED(actor, PRF_WIZHIDE))) {
		return 1;
	}
	
	DL_FOREACH_SAFE2(ROOM_VEHICLES(IN_ROOM(actor)), veh, next_veh, next_in_room) {
		if (VEH_IS_EXTRACTED(veh) || !SCRIPT_CHECK(veh, VTRIG_LEAVE)) {
			continue;
		}

		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
			if (!TRIGGER_CHECK(t, VTRIG_LEAVE)) {
				continue;
			}
			if (multi && !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
				continue;	// already did an allow-multi
			}
			
			if (TRIG_IS_LOCAL(t)) {
				if (any_in_room == -1) {
					any_in_room = any_players_in_room(IN_ROOM(actor));
				}
				if (!any_in_room) {
					continue;	// requires a player
				}
			}
			if (number(1, 100) <= GET_TRIG_NARG(t)) {
				union script_driver_data_u sdd;
				if (custom_dir) {
					add_var(&GET_TRIG_VARS(t), "direction", custom_dir, 0);
				}
				else {
					add_var(&GET_TRIG_VARS(t), "direction", (dir >= 0 && dir < NUM_OF_DIRS) ? ((char *)dirs[dir]) : "none", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				add_var(&GET_TRIG_VARS(t), "method", method ? method : "none", 0);
				sdd.v = veh;
				val = script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW);
				if (!val || !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
					return val;
				}
				else {
					multi = TRUE;
				}
			}
		}
	}
	return val;
}


/**
* Called when a vehicle is first loaded, sometimes before it's been built.
*
* @param vehicle_data *veh The vehicle that was just loaded.
* @return int 0 to indicate the vehicle is gone; 1 if it's ok.
*/
int load_vtrigger(vehicle_data *veh) {
	trig_data *t, *next_t;
	bool multi = FALSE;
	int val = 1;

	if (!SCRIPT_CHECK(veh, VTRIG_LOAD)) {
		return 1;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
		if (multi && !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
			continue;	// already did an allow-multi
		}
		if (TRIGGER_CHECK(t, VTRIG_LOAD) &&  (number(1, 100) <= GET_TRIG_NARG(t))) {
			union script_driver_data_u sdd;
			sdd.v = veh;
			val = script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW);
			if (!sdd.v) {
				val = 0;
			}
			if (!val || !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
				break;
			}
			else {
				multi = TRUE;
			}
		}
	}
	
	return val;
}


/**
* This is called on all vehicles after a reboot.
*
* @param vehicle_data *veh The vehicle.
*/
void reboot_vtrigger(vehicle_data *veh) {
	trig_data *t, *next_t;
	int val;

	if (!SCRIPT_CHECK(veh, VTRIG_REBOOT)) {
		return;
	}

	LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
		if (TRIGGER_CHECK(t, VTRIG_REBOOT)) {
			union script_driver_data_u sdd;
			sdd.v = veh;
			val = script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW);
			if (!val) {
				break;
			}
		}
	}
}


/**
* Called when someone speaks in a room with a vehicle in it.
*
* @param char_data *actor The person speaking.
* @param char *str The string being spoken.
* @param generic_data *language The language being spoken.
*/
void speech_vtrigger(char_data *actor, char *str, generic_data *language) {
	vehicle_data *veh, *next_veh;
	bool multi = FALSE;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;
	int any_in_room = -1, val;
	
	DL_FOREACH_SAFE2(ROOM_VEHICLES(IN_ROOM(actor)), veh, next_veh, next_in_room) {
		if (!VEH_IS_EXTRACTED(veh) && SCRIPT_CHECK(veh, VTRIG_SPEECH)) {
			multi = FALSE;
			LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
				if (multi && !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
					continue;	// already did an allow-multi
				}
				if (!TRIGGER_CHECK(t, VTRIG_SPEECH)) {
					continue;
				}
				if (TRIG_IS_LOCAL(t)) {
					if (any_in_room == -1) {
						any_in_room = any_players_in_room(IN_ROOM(actor));
					}
					if (!any_in_room) {
						continue;	// requires a player
					}
				}

				if (!GET_TRIG_ARG(t) || !*GET_TRIG_ARG(t)) {
					syslog(SYS_ERROR, LVL_BUILDER, TRUE, "SYSERR: Speech Trigger #%d has no text argument!", GET_TRIG_VNUM(t));
					continue;
				}

				if (*GET_TRIG_ARG(t) == '*' || ((GET_TRIG_NARG(t) && word_check(str, GET_TRIG_ARG(t))) || (!GET_TRIG_NARG(t) && is_substring(GET_TRIG_ARG(t), str)))) {
					union script_driver_data_u sdd;
					ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
					add_var(&GET_TRIG_VARS(t), "speech", strip_color(str), 0);
					add_var(&GET_TRIG_VARS(t), "lang", language ? GEN_NAME(language) : "", 0);
					sprintf(buf, "%d", language ? GEN_VNUM(language) : NOTHING);
					add_var(&GET_TRIG_VARS(t), "lang_vnum", buf, 0);
					sdd.v = veh;
					val = script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW);
					if (!val || !IS_SET(GET_TRIG_TYPE(t), VTRIG_ALLOW_MULTIPLE)) {
						break;
					}
					else {
						multi = TRUE;
					}
				}
			}
		}
	}
}


/**
* Called when an actor tries to start a quest.
*
* @param char_data *actor The person trying to start a quest.
* @param quest_data *quest Which quest they're trying to start.
* @param struct instance_data *inst What instance the quest is associated with, if any.
* @return int 0 to prevent it; 1 to allow it.
*/
int start_quest_vtrigger(char_data *actor, quest_data *quest, struct instance_data *inst) {
	vehicle_data *veh, *next_veh;
	char buf[MAX_INPUT_LENGTH];
	trig_data *t, *next_t;

	if (!valid_dg_target(actor, DG_ALLOW_GODS)) {
		return TRUE;
	}
	
	// store instance globally to allow %instance.xxx% in scripts
	quest_instance_global = inst;
	
	DL_FOREACH_SAFE2(ROOM_VEHICLES(IN_ROOM(actor)), veh, next_veh, next_in_room) {
		if (VEH_IS_EXTRACTED(veh) || !SCRIPT_CHECK(veh, VTRIG_START_QUEST)) {
			continue;
		}

		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh)), t, next_t) {
			if (TRIGGER_CHECK(t, VTRIG_START_QUEST)) {
				union script_driver_data_u sdd;
				if (quest) {
					snprintf(buf, sizeof(buf), "%d", QUEST_VNUM(quest));
					add_var(&GET_TRIG_VARS(t), "questvnum", buf, 0);
					add_var(&GET_TRIG_VARS(t), "questname", QUEST_NAME(quest), 0);
				}
				else {	// no quest?
					add_var(&GET_TRIG_VARS(t), "questvnum", "0", 0);
					add_var(&GET_TRIG_VARS(t), "questname", "Unknown", 0);
				}
				ADD_UID_VAR(buf, t, char_script_id(actor), "actor", 0);
				sdd.v = veh;
				if (!script_driver(&sdd, t, VEH_TRIGGER, TRIG_NEW)) {
					quest_instance_global = NULL;	// un-set this
					return FALSE;
				}
			}
		}
	}
	
	quest_instance_global = NULL;	// un-set this
	return TRUE;
}


 //////////////////////////////////////////////////////////////////////////////
//// COMBO TRIGGERS //////////////////////////////////////////////////////////

/**
* Checks all triggers for something that would prevent a 'buy'.
*
* @param char_data *actor The person trying to buy.
* @param char_data *shopkeeper The mob shopkeeper, if any (many shops have none).
* @param obj_data *buying The item being bought.
* @param int cost The amount to be charged.
* @param any_vnum currency The currency type (NOTHING for coins).
* @return bool FALSE means hit-trigger/stop; TRUE means continue buying.
*/
bool check_buy_trigger(char_data *actor, char_data *shopkeeper, obj_data *buying, int cost, any_vnum currency) {
	int cont = 1;
	
	cont = buy_wtrigger(actor, shopkeeper, buying, cost, currency);	// world trigs
	if (cont) {
		cont = buy_mtrigger(actor, shopkeeper, buying, cost, currency);	// mob trigs
	}
	if (cont) {
		cont = buy_otrigger(actor, shopkeeper, buying, cost, currency);	// obj trigs
	}
	if (cont) {
		cont = buy_vtrigger(actor, shopkeeper, buying, cost, currency);	// vehicles
	}
	return cont;
}


/**
* Checks all triggers for a command match.
*
* @param char_data *actor The person typing a command.
* @param char *cmd The command as-typed (first word).
* @param char *argument Any arguments (remaining text).
* @param int mode CMDTRG_EXACT or CMDTRG_ABBREV.
* @return bool TRUE means hit-trigger/stop; FALSE means continue execution
*/
bool check_command_trigger(char_data *actor, char *cmd, char *argument, int mode) {
	int cont = 0;
	
	if (!IS_NPC(actor) && ACCOUNT_FLAGGED(actor, ACCT_FROZEN)) {
		return cont;	// frozen players
	}
	if (IS_DEAD(actor)) {
		return cont;	// dead people
	}
	
	// never override the toggle command for immortals
	if (IS_IMMORTAL(actor) && is_abbrev(cmd, "toggles")) {
		return cont;
	}

	cont = command_wtrigger(actor, cmd, argument, mode);	// world trigs
	if (!cont) {
		cont = command_mtrigger(actor, cmd, argument, mode);	// mob trigs
	}
	if (!cont) {
		cont = command_otrigger(actor, cmd, argument, mode);	// obj trigs
	}
	if (!cont) {
		cont = command_vtrigger(actor, cmd, argument, mode);	// vehicles
	}
	return cont;
}


/**
* Checks all quest triggers
*
* @param char_data *actor The person trying to finish a quest.
* @param quest_data *quest The quest.
* @param struct instance_data *inst The associated instance, if any.
* @return int 0 means stop execution (block quest), 1 means continue
*/
int check_finish_quest_trigger(char_data *actor, quest_data *quest, struct instance_data *inst) {
	room_data *room = IN_ROOM(actor);
	struct trig_proto_list *tpl;
	trig_data *proto, *trig;
	int val = 1;

	if (val) {
		val = finish_quest_mtrigger(actor, quest, inst);	// mob trigs
	}
	if (val) {
		val = finish_quest_otrigger(actor, quest, inst);	// obj trigs
	}
	if (val) {
		val = finish_quest_vtrigger(actor, quest, inst);	// vehicles
	}
	if (val) {
		// still here? world triggers require additional work because we add
		// the trigs from the quest itself
		LL_FOREACH(QUEST_SCRIPTS(quest), tpl) {
			if (!(proto = real_trigger(tpl->vnum))) {
				continue;
			}
			if (!TRIGGER_CHECK(proto, WTRIG_FINISH_QUEST)) {
				continue;
			}
			
			// attach this trigger
			if (!(trig = read_trigger(tpl->vnum))) {
				continue;
			}
			if (!SCRIPT(room)) {
				create_script_data(room, WLD_TRIGGER);
			}
			add_trigger(SCRIPT(room), trig, -1);
		}
		
		val = finish_quest_wtrigger(room, actor, quest, inst);	// world trigs
		
		// now remove those triggers again
		LL_FOREACH(QUEST_SCRIPTS(quest), tpl) {
			if (!(proto = real_trigger(tpl->vnum))) {
				continue;
			}
			if (!TRIGGER_CHECK(proto, WTRIG_FINISH_QUEST)) {
				continue;
			}
			
			// find and remove
			if (SCRIPT(room)) {
				remove_live_script_by_vnum(SCRIPT(room), tpl->vnum);
			}
		}
	}
	return val;
}


/**
* Checks all quest triggers
*
* @param char_data *actor The person trying to start a quest.
* @param quest_data *quest The quest.
* @param struct instance_data *inst An associated instance, if there is one.
* @return int 0 means stop execution (block quest), 1 means continue
*/
int check_start_quest_trigger(char_data *actor, quest_data *quest, struct instance_data *inst) {
	room_data *room = IN_ROOM(actor);
	struct trig_proto_list *tpl;
	trig_data *proto, *trig;
	int val = 1;

	if (val) {
		val = start_quest_mtrigger(actor, quest, inst);	// mob trigs
	}
	if (val) {
		val = start_quest_otrigger(actor, quest, inst);	// obj trigs
	}
	if (val) {
		val = start_quest_vtrigger(actor, quest, inst);	// vehicles
	}
	if (val) {
		// still here? world triggers require additional work because we add
		// the trigs from the quest itself
		LL_FOREACH(QUEST_SCRIPTS(quest), tpl) {
			if (!(proto = real_trigger(tpl->vnum))) {
				continue;
			}
			if (!TRIGGER_CHECK(proto, WTRIG_START_QUEST)) {
				continue;
			}
			
			// attach this trigger
			if (!(trig = read_trigger(tpl->vnum))) {
				continue;
			}
			if (!SCRIPT(room)) {
				create_script_data(room, WLD_TRIGGER);
			}
			add_trigger(SCRIPT(room), trig, -1);
		}
		
		val = start_quest_wtrigger(room, actor, quest, inst);	// world trigs
		
		// now remove those triggers again
		LL_FOREACH(QUEST_SCRIPTS(quest), tpl) {
			if (!(proto = real_trigger(tpl->vnum))) {
				continue;
			}
			if (!TRIGGER_CHECK(proto, WTRIG_START_QUEST)) {
				continue;
			}
			
			// find and remove
			if (SCRIPT(room)) {
				remove_live_script_by_vnum(SCRIPT(room), tpl->vnum);
			}
		}
	}
	return val;
}


/**
* Runs entry and enter triggers on everything in the room. If the movement is
* preventable, it stops on any trigger that prevents it. If it's not, it runs
* them all.
*
* Enter triggers run after moving, before looking, and before greet triggers.
*
* @param char_data *actor The person who has entered the room.
* @param int dir Which direction the character came from.
* @param char *method Method of movement.
* @param bool preventable TRUE if the character can be sent back; FALSE if not.
* @return int 1 to allow the character here, 0 to attempt to send them back.
*/
int enter_triggers(char_data *ch, int dir, char *method, bool preventable) {
	if (!entry_mtrigger(ch, method) && preventable) {
		return 0;
	}
	else if (!enter_wtrigger(IN_ROOM(ch), ch, dir, method) && preventable) {
		return 0;
	}

	return 1;
}


/**
* Runs greet and memory triggers on everything in the room. If the movement is
* preventable, it stops on any trigger that prevents it. If it's not, it runs
* them all.
*
* Greet triggers run after moving and looking, and are the last movement trigs
* to run.
*
* @param char_data *actor The person who has entered the room.
* @param int dir Which direction the character came from.
* @param char *method Method of movement.
* @param bool preventable TRUE if the character can be sent back; FALSE if not.
* @return int 1 to allow the character here, 0 to attempt to send them back.
*/
int greet_triggers(char_data *ch, int dir, char *method, bool preventable) {
	if (!greet_mtrigger(ch, dir, method) && preventable) {
		return 0;
	}
	else if (!greet_vtrigger(ch, dir, method) && preventable) {
		return 0;
	}
	else if (!greet_otrigger(ch, dir, method) && preventable) {
		return 0;
	}
	
	// if we got this far, we're staying
	
	// people we remember when we enter
	entry_memory_mtrigger(ch);
	
	// people we remember when they enter
	greet_memory_mtrigger(ch);

	return 1;
}


/**
* Runs kill triggers for everyone involved in the kill -- the killer, their
* allies, and all items possessed by those people. Additionally, vehicles also
* fire kill triggers if they cause the kill, even if they are not in the same
* room.
*
* Kill triggers will not fire if a person kills himself.
*
* @param char_data *dying The person who has died.
* @param char_data *killer Optional: Person who killed them.
* @param vehicle_data *veh_killer Optional: Vehicle who killed them.
* @return int The return value of a script (1 is normal, 0 suppresses the death cry).
*/
int run_kill_triggers(char_data *dying, char_data *killer, vehicle_data *veh_killer) {
	union script_driver_data_u sdd;
	char_data *ch_iter, *next_ch;
	trig_data *trig, *next_trig;
	char buf[MAX_INPUT_LENGTH];
	room_data *room;
	int pos;
	
	int val = 1;	// default return value
	
	if (!dying) {
		return val;	// somehow
	}
	
	// store this, in case it changes during any script
	room = IN_ROOM(dying);
	
	if (killer && killer != dying) {
		// check characters first:
		DL_FOREACH_SAFE2(ROOM_PEOPLE(room), ch_iter, next_ch, next_in_room) {
			if (EXTRACTED(ch_iter) || IS_DEAD(ch_iter) || !SCRIPT_CHECK(ch_iter, MTRIG_KILL)) {
				continue;
			}
			if (ch_iter == dying) {
				continue;	// cannot fire if killing self
			}
			if (ch_iter != killer && !is_fight_ally(ch_iter, killer)) {
				continue;	// is not on the killing team
			}
			LL_FOREACH_SAFE(TRIGGERS(SCRIPT(ch_iter)), trig, next_trig) {
				if (AFF_FLAGGED(ch_iter, AFF_CHARM) && !TRIGGER_CHECK(trig, MTRIG_CHARMED)) {
					continue;	// cannot do while charmed
				}
				if (!TRIGGER_CHECK(trig, MTRIG_KILL) || (number(1, 100) > GET_TRIG_NARG(trig))) {
					continue;	// wrong trig or failed random percent
				}
			
				// ok:
				memset((char *) &sdd, 0, sizeof(union script_driver_data_u));
				ADD_UID_VAR(buf, trig, char_script_id(dying), "actor", 0);
				if (killer) {
					ADD_UID_VAR(buf, trig, char_script_id(killer), "killer", 0);
				}
				else {
					add_var(&GET_TRIG_VARS(trig), "killer", "", 0);
				}
				sdd.c = ch_iter;
			
				// run it -- any script returning 0 guarantees we will return 0
				val &= script_driver(&sdd, trig, MOB_TRIGGER, TRIG_NEW);
			}
		}
		
		// check gear on characters present, IF they are on the killing team:
		DL_FOREACH_SAFE2(ROOM_PEOPLE(room), ch_iter, next_ch, next_in_room) {
			if (EXTRACTED(ch_iter) || IS_DEAD(ch_iter)) {
				continue;	// cannot benefit if dead
			}
			if (ch_iter != killer && !is_fight_ally(ch_iter, killer)) {
				continue;	// is not on the killing team
			}
			
			// equipped
			for (pos = 0; pos < NUM_WEARS; ++pos) {
				if (!GET_EQ(ch_iter, pos)) {
					continue;	// no item
				}
				
				// ok:
				val &= kill_otrigger(GET_EQ(ch_iter, pos), dying, killer);
			}
			
			// inventory:
			val &= kill_otrigger(ch_iter->carrying, dying, killer);
		}
	}
	
	// and the vehicle
	if (veh_killer && SCRIPT_CHECK(veh_killer, VTRIG_KILL)) {
		LL_FOREACH_SAFE(TRIGGERS(SCRIPT(veh_killer)), trig, next_trig) {
			if (!TRIGGER_CHECK(trig, VTRIG_KILL) || (number(1, 100) > GET_TRIG_NARG(trig))) {
				continue;	// wrong trig or failed random percent
			}
			
			// ok:
			memset((char *) &sdd, 0, sizeof(union script_driver_data_u));
			ADD_UID_VAR(buf, trig, char_script_id(dying), "actor", 0);
			ADD_UID_VAR(buf, trig, veh_script_id(veh_killer), "killer", 0);
			sdd.v = veh_killer;
		
			// run it -- any script returning 0 guarantees we will return 0
			val &= script_driver(&sdd, trig, VEH_TRIGGER, TRIG_NEW);
		}
	}
	
	return val;
}


 //////////////////////////////////////////////////////////////////////////////
//// RESET TRIGGER HELPER ////////////////////////////////////////////////////

// runs room reset triggers on a loop
EVENTFUNC(run_reset_triggers) {
	struct room_event_data *data = (struct room_event_data *)event_obj;
	room_data *room;
	
	// grab data but don't free (we usually reschedule this)
	room = data->room;
	
	// still have any?
	if (IS_ADVENTURE_ROOM(room) || !SCRIPT_CHECK(room, WTRIG_RESET)) {
		delete_stored_event_room(room, SEV_RESET_TRIGGER);
		free(data);
		return 0;
	}
	
	reset_wtrigger(room);
	return (7.5 * 60) RL_SEC;	// reenqueue for the original time
}


/**
* Checks if a room has reset triggers. If so, sets up the data for it and
* schedules the event. If not, it clears that data.
*
* @param room_data *room The room to check for reset triggers.
* @param bool random_offest If TRUE, throws in some random in the 1st timer.
*/
void check_reset_trigger_event(room_data *room, bool random_offset) {
	struct room_event_data *data;
	struct dg_event *ev;
	int mins;
	
	if (!IS_ADVENTURE_ROOM(room) && SCRIPT_CHECK(room, WTRIG_RESET)) {
		if (!find_stored_event_room(room, SEV_RESET_TRIGGER)) {
			CREATE(data, struct room_event_data, 1);
			data->room = room;
			
			// schedule every 7.5 minutes
			mins = 7.5 - (random_offset ? number(0,6) : 0);
			ev = dg_event_create(run_reset_triggers, (void*)data, (mins * 60) RL_SEC);
			add_stored_event_room(room, SEV_RESET_TRIGGER, ev);
		}
	}
	else {
		cancel_stored_event_room(room, SEV_RESET_TRIGGER);
	}
}

