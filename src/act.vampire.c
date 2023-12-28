/* ************************************************************************
*   File: act.vampire.c                                   EmpireMUD 2.0b5 *
*  Usage: Code related to the Vampire ability and its commands            *
*                                                                         *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2024                      *
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
#include "vnums.h"
#include "dg_event.h"
#include "dg_scripts.h"
#include "constants.h"

/**
* Contents:
*   Helpers
*   Commands
*/

// external functions
ACMD(do_say);
ACMD(do_stand);
ACMD(do_wake);

// locals
ACMD(do_bite);
EVENTFUNC(vampire_feeding_event);


 //////////////////////////////////////////////////////////////////////////////
//// HELPERS /////////////////////////////////////////////////////////////////

/**
* Cancels vampire feeding, if happening.
*
* @param char_data *ch The vampire who's biting someone.
* @param bool preventable If TRUE, may be prevented. If FALSE, it will definitely stop.
* @return bool TRUE if biting was canceled; FALSE if nobody was biting.
*/
bool cancel_biting(char_data *ch, bool preventable) {
	char_data *vict;
	
	if (ch && (vict = GET_FEEDING_FROM(ch))) {
		if (preventable && (AFF_FLAGGED(ch, AFF_STUNNED | AFF_HARD_STUNNED) || IS_BLOOD_STARVED(ch))) {
			msg_to_char(ch, "You can't seem to stop%s!\r\n", (IS_BLOOD_STARVED(ch) ? " yourself" : ""));
			return TRUE;
		}
		
		act("You stop feeding from $N.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n stops feeding from you.", FALSE, ch, NULL, vict, TO_VICT);
		act("$n stops feeding from $N.", FALSE, ch, NULL, vict, TO_NOTVICT);
		GET_FED_ON_BY(vict) = NULL;
		GET_FEEDING_FROM(ch) = NULL;
		cancel_stored_event(&GET_STORED_EVENTS(ch), SEV_VAMPIRE_FEEDING);
		return TRUE;
	}
	
	return FALSE;
}


/**
* Attempts to cancel the vampire powers that require upkeeps, e.g. upon
* death or when out of blood.
*
* @param char_data *ch The vampire paying upkeeps.
*/
void cancel_blood_upkeeps(char_data *ch) {
	char buf[MAX_STRING_LENGTH];
	struct affected_type *aff;
	struct obj_apply *app;
	obj_data *obj;
	int iter;
	bool any, messaged;
	
	if (IS_NPC(ch) || !IS_VAMPIRE(ch)) {
		return;
	}
	
	// we'll only send the preface message if they're not dead
	messaged = (GET_POS(ch) < POS_SLEEPING);
	
	// affs: loop because removing multiple affects makes iterating over affects hard
	do {
		any = FALSE;
		LL_FOREACH(ch->affected, aff) {
			if (aff->location == APPLY_BLOOD_UPKEEP && aff->modifier > 0) {
				if (!messaged && SHOW_STATUS_MESSAGES(ch, SM_LOW_BLOOD)) {
					msg_to_char(ch, "You're too low on blood...\r\n");
					messaged = TRUE;
				}
				
				// special case: morphs
				if (aff->type == ATYPE_MORPH) {
					perform_morph(ch, NULL);
					any = TRUE;
					break;
				}
				
				msg_to_char(ch, "Your %s effect fades.\r\n", get_generic_name_by_vnum(aff->type));
				snprintf(buf, sizeof(buf), "$n's %s effect fades.", get_generic_name_by_vnum(aff->type));
				act(buf, TRUE, ch, NULL, NULL, TO_ROOM);
				
				affect_from_char(ch, aff->type, FALSE);
				any = TRUE;
				break;	// this removes multiple affs so it's not safe to continue on the list
			}
		}
	} while (any);
	
	// gear:
	for (iter = 0; iter < NUM_WEARS; ++iter) {
		if (!(obj = GET_EQ(ch, iter))) {
			continue;
		}
		
		LL_FOREACH(obj->applies, app) {
			if (app->location == APPLY_BLOOD_UPKEEP && app->modifier > 0) {
				if (!messaged && SHOW_STATUS_MESSAGES(ch, SM_LOW_BLOOD)) {
					msg_to_char(ch, "You're too low on blood...\r\n");
					messaged = TRUE;
				}
				
				// char message
				if (obj_has_custom_message(obj, OBJ_CUSTOM_REMOVE_TO_CHAR)) {
					act(obj_get_custom_message(obj, OBJ_CUSTOM_REMOVE_TO_CHAR), FALSE, ch, obj, NULL, TO_CHAR);
				}
				else {
					act("You can no longer use $p.", FALSE, ch, obj, NULL, TO_CHAR);
				}
	
				// room message
				if (obj_has_custom_message(obj, OBJ_CUSTOM_REMOVE_TO_ROOM)) {
					act(obj_get_custom_message(obj, OBJ_CUSTOM_REMOVE_TO_ROOM), TRUE, ch, obj, NULL, TO_ROOM);
				}
				else {
					act("$n stops using $p.", TRUE, ch, obj, NULL, TO_ROOM);
				}
				
				// this may extract it
				unequip_char_to_inventory(ch, iter);
				determine_gear_level(ch);
				break;	// only need 1 matching apply
			}
		}
	}
}


/**
* Called when the player cancels ACT_SIRING for some reason.
*
* @param char_data *ch The acting player
*/
void cancel_siring(char_data *ch) {
	cancel_biting(ch, FALSE);
}


// returns TRUE if the character is a vampire and has the ability; sends its own error
bool check_vampire_ability(char_data *ch, any_vnum ability, int cost_pool, int cost_amount, int cooldown_type) {
	if (!IS_VAMPIRE(ch)) {
		send_config_msg(ch, "must_be_vampire");
		return FALSE;
	}
	if (!can_use_ability(ch, ability, cost_pool, cost_amount, cooldown_type)) {
		return FALSE;
	}
	
	return TRUE;
}


/**
* @param char_data *ch the vampire
* @param bool message if TRUE, sends the you-can't-do-that-in-sun
* @return bool TRUE if the ability can proceed, FALSE if sunny
*/
bool check_vampire_sun(char_data *ch, bool message) {
	if (IS_NPC(ch) || has_player_tech(ch, PTECH_VAMPIRE_SUN_IMMUNITY) || IS_GOD(ch) || IS_IMMORTAL(ch) || AFF_FLAGGED(ch, AFF_EARTHMELD) || !check_sunny(IN_ROOM(ch))) {
		// ok -- not sunny
		return TRUE;
	}
	else {
		// sunny
		if (message) {
			msg_to_char(ch, "Your powers don't work in direct sunlight.\r\n");
		}
		return FALSE;
	}
}


/**
* Ends all boost affects on a character.
*
* @param char_data *ch The boosted character.
*/
void end_boost(char_data *ch) {
	if (affected_by_spell(ch, ATYPE_BOOST)) {
		msg_to_char(ch, "Your boost fades.\r\n");
		affect_from_char(ch, ATYPE_BOOST, FALSE);
	}
}


// max blood is set in mobfile for npc, but computed for player
int GET_MAX_BLOOD(char_data *ch) {
	int base = base_player_pools[BLOOD];
	
	if (IS_NPC(ch)) {
		base = ch->points.max_pools[BLOOD];
	}
	else {
		base += GET_EXTRA_BLOOD(ch);

		if (IS_VAMPIRE(ch)) {
			if (HAS_BONUS_TRAIT(ch, BONUS_BLOOD)) {
				base += config_get_int("pool_bonus_amount") * (1 + (get_approximate_level(ch) / 25));
			}
		}
	}

	return base;
}


/**
* This function sets all the important data for people who are becoming
* vampires. This requires a skill tree with the VAMPIRE flag on it. You can
* specify which skill or let the system detect it.
*
* @param char_data *ch becomes a vampire
* @param bool lore if TRUE, also adds lore
* @param any_vnum skill_vnum If you provide a VAMPIRE skill vnum here, gives the player that skill at level 1 (if they don't already have it). Pass NOTHING here to auto-detect -- or to ignore it if the player already has a vampire skill.
*/
void make_vampire(char_data *ch, bool lore, any_vnum skill_vnum) {
	skill_data *skl, *next_skl;
	bool already_vampire = IS_VAMPIRE(ch);
	
	if (!already_vampire && skill_vnum == NOTHING) {		// auto-detect skill:
		skill_data *found = NULL;
		int num_found = 0;
		bool basic = FALSE;
		
		HASH_ITER(hh, skill_table, skl, next_skl) {
			if (SKILL_FLAGGED(skl, SKILLF_IN_DEVELOPMENT) || !SKILL_FLAGGED(skl, SKILLF_VAMPIRE)) {
				continue;	// bad flags
			}
			if (!SKILL_FLAGGED(skl, SKILLF_BASIC) && basic) {
				continue;	// already have a basic one
			}
			
			// ok, we should save this one:
			if (SKILL_FLAGGED(skl, SKILLF_BASIC) && !basic) {
				// first basic one: override num_found
				num_found = 1;
				found = skl;
				basic = TRUE;
			}
			else if (!number(0, num_found++)) {
				// rolled random number if more than 1 match to choose them at random
				found = skl;
			}
		}
		
		if (found) {
			skill_vnum = SKILL_VNUM(found);
		}
		else {
			log("SYSERR: make_vampire called with no available skills flagged VAMPIRE");
			return;
		}
	}
	
	if (skill_vnum != NOTHING && !noskill_ok(ch, skill_vnum)) {
		return;
	}
	
	if (!IS_NPC(ch)) {
		/* set BEFORE set as a vampire! */
		GET_APPARENT_AGE(ch) = GET_REAL_AGE(ch);
		
		if (skill_vnum != NOTHING && get_skill_level(ch, skill_vnum) < 1) {
			gain_skill(ch, find_skill_by_vnum(skill_vnum), 1, NULL);
		}

		set_blood(ch, config_get_int("blood_starvation_level") * 1.5);

		remove_lore(ch, LORE_START_VAMPIRE);
		remove_lore(ch, LORE_SIRE_VAMPIRE);
		remove_lore(ch, LORE_PURIFY);
		if (lore && IS_VAMPIRE(ch)) {
			add_lore(ch, LORE_START_VAMPIRE, "Sired");
		}
		
		queue_delayed_update(ch, CDU_SAVE);
	}
}


// for do_sire
void sire_char(char_data *ch, char_data *victim) {
	struct player_skill_data *plsk, *next_plsk;
	any_vnum vamp_skill = NOTHING;
	int num_found = 0;
	bool basic = FALSE;
	
	GET_FEEDING_FROM(ch) = NULL;
	GET_FED_ON_BY(victim) = NULL;

	act("$N drops limply from your fangs...", FALSE, ch, 0, victim, TO_CHAR);
	act("$N drops limply from $n's fangs...", FALSE, ch, 0, victim, TO_NOTVICT);
	act("You fall to the ground. In the distance, you think you see a light...", FALSE, ch, 0, victim, TO_VICT);
	
	// determine if ch has a VAMPIRE-flagged skill they can 'give' to the victim (preferring a basic one)
	HASH_ITER(hh, GET_SKILL_HASH(ch), plsk, next_plsk) {
		if (!plsk->ptr || SKILL_FLAGGED(plsk->ptr, SKILLF_IN_DEVELOPMENT) || !SKILL_FLAGGED(plsk->ptr, SKILLF_VAMPIRE)) {
			continue;	// missing proto or not a valid VAMPIRE skill
		}
		if (!SKILL_FLAGGED(plsk->ptr, SKILLF_BASIC) && basic) {
			continue;	// already found a basic one
		}
		if (!noskill_ok(victim, plsk->vnum)) {
			continue;	// victim has noskilled it
			// TODO: should probably REQUIRE a basic one if it exists; this will skip over SOME basic ones if noskilled
		}
		
		// ok, we should save this one:
		if (SKILL_FLAGGED(plsk->ptr, SKILLF_BASIC) && !basic) {
			// first basic one: override num_found
			num_found = 1;
			vamp_skill = plsk->vnum;
			basic = TRUE;
		}
		else if (!number(0, num_found++)) {
			// rolled random number if more than 1 match to choose them at random
			vamp_skill = plsk->vnum;
		}
	}
	
	// did we find a valid vamp skill
	if (vamp_skill != NOTHING && CAN_GAIN_NEW_SKILLS(victim) && noskill_ok(victim, vamp_skill)) {
		make_vampire(victim, FALSE, vamp_skill);
		set_blood(ch, GET_BLOOD(ch) - 10);

		act("You tear open your wrist with your fangs and drip blood into $N's mouth!", FALSE, ch, 0, victim, TO_CHAR);
		act("$n tears open $s wrist with $s teeth and drips blood into $N's mouth!", FALSE, ch, 0, victim, TO_NOTVICT);
		msg_to_char(victim, "&rSuddenly, a warm sensation touches your lips and a stream of blood flows down your throat...&0\r\n");
		msg_to_char(victim, "&rAs the blood fills you, a strange sensation covers your body... The light in the distance turns blood-red and a hunger builds within you!&0\r\n");

		msg_to_char(victim, "You sit up quickly, nearly knocking over your sire!\r\n");
		act("$n sits up quickly!", FALSE, victim, 0, 0, TO_ROOM);

		remove_lore(victim, LORE_SIRE_VAMPIRE);
		remove_lore(victim, LORE_PURIFY);
		remove_lore(victim, LORE_START_VAMPIRE);
		add_lore(victim, LORE_SIRE_VAMPIRE, "Sired by %s", PERS(ch, ch, TRUE));
		add_lore(ch, LORE_MAKE_VAMPIRE, "Sired %s", PERS(victim, victim, TRUE));

		/* Turn off that SIRING action */
		end_action(ch);

		queue_delayed_update(ch, CDU_SAVE);
		queue_delayed_update(victim, CDU_SAVE);
	}
	else {
		// can't gain a vampire skills: guess I'll just die then
		if (!IS_NPC(victim)) {
			death_log(victim, ch, TYPE_SUFFERING);
		}
		die(victim, ch);	// returns a corpse but we don't need it
	}
}


/**
* Initiates drinking blood. Use cancel_biting() to stop.
*
* @param char_data *ch The person doing the biting.
* @param char_data *victim The person being bitten.
*/
void start_drinking_blood(char_data *ch, char_data *victim) {
	struct char_event_data *data;
	struct dg_event *ev;
	
	// safety first
	if (GET_FEEDING_FROM(ch)) {
		cancel_biting(ch, FALSE);
	}
	if (GET_FED_ON_BY(victim)) {
		cancel_biting(GET_FED_ON_BY(victim), FALSE);
	}
	
	GET_FEEDING_FROM(ch) = victim;
	GET_FED_ON_BY(victim) = ch;

	stop_fighting(ch);
	stop_fighting(victim);
	
	// ensure no existing event
	cancel_stored_event(&GET_STORED_EVENTS(ch), SEV_VAMPIRE_FEEDING);
	
	// create new event
	CREATE(data, struct char_event_data, 1);
	data->character = ch;
	ev = dg_event_create(vampire_feeding_event, data, 5 RL_SEC);
	add_stored_event(&GET_STORED_EVENTS(ch), SEV_VAMPIRE_FEEDING, ev);
	
	// messaging last
	if (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_BOTHERABLE)) {
		act("You grasp $N's wrist and bite into it.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n grasps $N's wrist and bites into it.", FALSE, ch, 0, victim, TO_NOTVICT);
		act("$n grasps your wrist and bites into it.", FALSE, ch, 0, victim, TO_VICT);
	}
	else if (GET_HEALTH(victim) > 0 || !AWAKE(victim)) {
		act("You lunge at $N, grasping onto $S neck and biting into it vigorously!", FALSE, ch, 0, victim, TO_CHAR);
		act("$n lunges at $N, grasping onto $S neck and biting into it vigorously!", FALSE, ch, 0, victim, TO_NOTVICT);
		act("$n lunges at you... $e grasps onto your neck and bites into it!", FALSE, ch, 0, victim, TO_VICT | TO_SLEEP);
	}
	else {	// probably dead or asleep
		act("You grasp onto $N and bite deep into $S neck!", FALSE, ch, 0, victim, TO_CHAR);
		act("$n grasps onto $N and bites deep into $S neck!", FALSE, ch, 0, victim, TO_NOTVICT);
		act("$n grasps onto you and bites deep into your neck!", FALSE, ch, 0, victim, TO_VICT | TO_SLEEP);
	}
}


/**
* Attempts to auto-bite either the character's current target, or a random
* valid target in the room. Call only when the vampire is already starving.
*
* @param char_data *ch The person who is trying to bite.
* @return bool TRUE if the vampire aggroed something, FALSE if not.
*/
bool starving_vampire_aggro(char_data *ch) {
	char_data *ch_iter, *backup = NULL, *victim = FIGHTING(ch);
	int backup_found = 0, vict_found = 0;
	char arg[MAX_INPUT_LENGTH];
	struct affected_type *af;
	
	if (IS_IMMORTAL(ch) || GET_FEEDING_FROM(ch) || IS_DEAD(ch) || GET_POS(ch) < POS_RESTING || AFF_FLAGGED(ch, AFF_STUNNED | AFF_HARD_STUNNED) || IS_INJURED(ch, INJ_TIED | INJ_STAKED) || !IS_VAMPIRE(ch)) {
		return FALSE;	// conditions which will block bite
	}
	if (get_cooldown_time(ch, COOLDOWN_BITE) > 0) {
		return FALSE;	// can't bite right now
	}
	
	if (!victim) {	// not fighting anyone? pick a target
		DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(ch)), ch_iter, next_in_room) {
			if (ch_iter == ch) {
				continue;	// self
			}
			if (AFF_FLAGGED(ch_iter, AFF_NO_DRINK_BLOOD)) {
				continue;	// nothing to drink
			}
			if (!CAN_SEE(ch, ch_iter) || IS_IMMORTAL(ch_iter) || !can_fight(ch, ch_iter)) {
				continue;	// cannot attack
			}
			
			// appears valid...
			if (is_fight_ally(ch, ch_iter)) {
				// only a backup if it's an ally
				if (!number(0, backup_found++) || !backup) {
					backup = ch_iter;
				}
			}
			else {	// not an ally
				// randomly choose if we already found one
				if (!number(0, vict_found++) || !victim) {
					victim = ch_iter;
				}
			}
		}
		
		// did we find someone valid?
		if (!victim) {
			victim = backup;
		}
	}
	
	if (!victim) {
		return FALSE;	// nobody to aggro
	}
	
	// get ready
	if (GET_POS(ch) < POS_FIGHTING) {
		if (GET_POS(ch) == POS_SLEEPING) {
			do_wake(ch, "", 0, 0);
		}
		if (GET_POS(ch) == POS_RESTING || GET_POS(ch) == POS_SITTING) {
			do_stand(ch, "", 0, 0);
		}
		if (GET_POS(ch) < POS_FIGHTING) {
			return FALSE;	// failed to stand
		}
	}
	
	// cancel timed action
	if (GET_ACTION(ch) != ACT_NONE) {
		cancel_action(ch);
	}
	
	// message only if not already fighting
	if (!FIGHTING(ch)) {
		act("You lunge toward $N as the sound of $S heartbeat overwhelms your senses...", FALSE, ch, NULL, victim, TO_CHAR);
		act("$n lunges toward you with fiery red eyes and bare fangs...", FALSE, ch, NULL, victim, TO_VICT);
		act("$n lunges toward $N with fiery red eyes and bare fangs...", FALSE, ch, NULL, victim, TO_NOTVICT);
	}
	
	sprintf(arg, "%c%d", UID_CHAR, char_script_id(victim));
	do_bite(ch, arg, 0, 0);
	
	// stun to keep them from stopping
	if (GET_FEEDING_FROM(ch)) {
		af = create_flag_aff(ATYPE_CANT_STOP, 30, AFF_HARD_STUNNED, ch);
		affect_join(ch, af, 0);
	}
	
	return TRUE;
}


// sends the message that a sunlight effect is happening
void sun_message(char_data *ch) {
	if (AWAKE(ch)) {
		msg_to_char(ch, "You wince and recoil from the sunlight!\r\n");
		act("$n winces and recoils from the sunlight!", TRUE, ch, 0, 0, TO_ROOM);
	}
	else {
		msg_to_char(ch, "The sun shines down on you.\r\n");
		act("The sun shines down on $n.", TRUE, ch, 0, 0, TO_ROOM);
	}
}


/**
* Checks if a person is not a vampire and, if not, clears certain vampire
* traits and restores the player's blood (since non-vampires cannot drink
* blood).
*
* This function can also remove vampire skills, on-request.
*
* @param char_data *ch The player to check/remove vampire skills on.
* @param bool remove_vampire_skills If TRUE, will set all VAMPIRE-flagged skills to 0 (forcefully un-vampires the player).
*/
void check_un_vampire(char_data *ch, bool remove_vampire_skills) {
	if (IS_NPC(ch)) {
		if (remove_vampire_skills) {
			remove_mob_flags(ch, MOB_VAMPIRE);
		}
		return;	// no further work
	}
	
	if (remove_vampire_skills) {
		remove_skills_by_flag(ch, SKILLF_VAMPIRE);
	}
	
	// only if not a vampire:
	if (!IS_VAMPIRE(ch)) {
		remove_lore(ch, LORE_PURIFY);
		add_lore(ch, LORE_PURIFY, "Purified");
		set_blood(ch, GET_MAX_BLOOD(ch));
		GET_APPARENT_AGE(ch) = 0;
	}
}


/**
* Updates the "biting" action, per real update (usually 5 seconds).
*
* @param char_data *ch The person who might be biting, to update.
*/
void update_biting_char(char_data *ch) {
	char_data *victim;
	obj_data *corpse;
	int amount, hamt;

	if (!(victim = GET_FEEDING_FROM(ch)))
		return;
	
	check_scaling(victim, ch);

	// permanent cancels
	if (GET_POS(ch) < POS_STANDING || IN_ROOM(victim) != IN_ROOM(ch) || GET_BLOOD(victim) <= 0 || IS_DEAD(ch) || IS_DEAD(victim) || EXTRACTED(ch) || EXTRACTED(victim)) {
		GET_FEEDING_FROM(ch) = NULL;
		GET_FED_ON_BY(victim) = NULL;
		return;
	}
	
	// Transfuse blood -- 10-25 points (pints?) at a time
	amount = MIN(number(10, 25), GET_BLOOD(victim));
	set_blood(victim, GET_BLOOD(victim) - amount);
	
	// can gain more
	if (has_player_tech(ch, PTECH_DRINK_BLOOD_FASTER)) {
		amount *= 2;
	}
	set_blood(ch, GET_BLOOD(ch) + amount);
	
	// bite regeneration ptech: 10% heal to h/m/v per drink when biting humans
	if ((!IS_NPC(victim) || MOB_FLAGGED(victim, MOB_HUMAN)) && has_player_tech(ch, PTECH_BITE_REGENERATION)) {
		hamt = GET_MAX_HEALTH(ch) / 10;
		heal(ch, ch, hamt);
		
		hamt = GET_MAX_MANA(ch) / 10;
		set_mana(ch, GET_MANA(ch) + hamt);
		
		hamt = GET_MAX_MOVE(ch) / 10;
		set_move(ch, GET_MOVE(ch) + hamt);
	}

	if (GET_BLOOD(victim) <= 0 && GET_ACTION(ch) != ACT_SIRING) {
		set_blood(victim, 0);

		if (!IS_NPC(victim) && !PRF_FLAGGED(ch, PRF_AUTOKILL)) {
			// give back a little blood
			set_blood(victim, 1);
			set_blood(ch, GET_BLOOD(ch) - 1);
			cancel_biting(ch, TRUE);
			if (GET_FEEDING_FROM(ch) != victim) {
				return;
			}
		}

		act("You pull the last of $N's blood from $S veins, and $E falls limply to the ground!", FALSE, ch, 0, victim, TO_CHAR);
		act("$N falls limply from $n's arms!", FALSE, ch, 0, victim, TO_NOTVICT);
		act("You feel faint as the last of your blood is pulled from your body!", FALSE, ch, 0, victim, TO_VICT);
		
		// cancel a can't-stop effect, if present
		affect_from_char(ch, ATYPE_CANT_STOP, FALSE);

		msg_to_char(victim, "You are dead! Sorry...\r\n");
		if (!IS_NPC(victim)) {
			death_log(victim, ch, ATTACK_EXECUTE);
			add_lore(ch, LORE_PLAYER_KILL, "Killed %s", PERS(victim, victim, TRUE));
			add_lore(victim, LORE_PLAYER_DEATH, "Slain by %s", PERS(ch, ch, TRUE));
		}

		check_scaling(victim, ch);	// ensure scaling
		tag_mob(victim, ch);	// ensures loot binding if applicable
		corpse = die(victim, ch);
		
		GET_FED_ON_BY(victim) = NULL;
		GET_FEEDING_FROM(ch) = NULL;
		
		// tag corpse
		if (corpse && !IS_NPC(ch)) {
			corpse->last_owner_id = GET_IDNUM(ch);
			corpse->last_empire_id = GET_LOYALTY(ch) ? EMPIRE_VNUM(GET_LOYALTY(ch)) : NOTHING;
		}
	}
	else {
		act("You shudder with ecstasy as you feed from $N!", FALSE, ch, 0, victim, TO_CHAR);
		act("$n shudders with ecstasy as $e feeds from $N!", FALSE, ch, 0, victim, TO_NOTVICT);
		act("A surge of ecstasy fills your body as $n feeds from your veins!", FALSE, ch, 0, victim, TO_VICT);
		
		if ((GET_BLOOD(victim) * 100 / GET_MAX_BLOOD(victim)) < 20) {
			act("...$e sways from lack of blood.", FALSE, victim, NULL, NULL, TO_ROOM);
		}
	}
	
	gain_player_tech_exp(ch, PTECH_VAMPIRE_BITE, 5);
	gain_player_tech_exp(ch, PTECH_BITE_REGENERATION, 5);
	run_ability_gain_hooks(ch, victim, AGH_VAMPIRE_FEEDING);
	run_ability_hooks_by_player_tech(ch, PTECH_VAMPIRE_BITE, victim, NULL, NULL, NULL);
	run_ability_hooks_by_player_tech(ch, PTECH_BITE_REGENERATION, ch, NULL, NULL, NULL);
}


void update_vampire_sun(char_data *ch) {
	bool repeat, found = FALSE;
	int iter;
	obj_data *obj;
	struct affected_type *af;
	struct obj_apply *app;
	
	// only applies if vampire and not an NPC
	if (IS_NPC(ch) || !IS_VAMPIRE(ch) || check_vampire_sun(ch, FALSE)) {
		return;
	}
	
	// look for things that cause blood upkeep and remove them
	do {
		repeat = FALSE;
		
		LL_FOREACH(ch->affected, af) {
			if (af->cast_by == CAST_BY_ID(ch) && af->location == APPLY_BLOOD_UPKEEP && af->modifier > 0) {
				if (!found) {
					sun_message(ch);
					found = TRUE;
				}
				repeat = TRUE;	// gotta try again
				affect_from_char(ch, af->type, TRUE);
				break;	// this removes multiple affs so it's not safe to continue on the list
			}
		}
	} while (repeat);
	
	// gear
	for (iter = 0; iter < NUM_WEARS; ++iter) {
		if (!wear_data[iter].count_stats || !(obj = GET_EQ(ch, iter))) {
			continue;	// skippable
		}

		LL_FOREACH(obj->applies, app) {
			if (app->location == APPLY_BLOOD_UPKEEP && app->modifier > 0) {
				if (!found) {
					sun_message(ch);
					found = TRUE;
				}

				// char message
				if (obj_has_custom_message(obj, OBJ_CUSTOM_REMOVE_TO_CHAR)) {
					act(obj_get_custom_message(obj, OBJ_CUSTOM_REMOVE_TO_CHAR), FALSE, ch, obj, NULL, TO_CHAR);
				}
				else {
					act("You can no longer use $p.", FALSE, ch, obj, NULL, TO_CHAR);
				}
	
				// room message
				if (obj_has_custom_message(obj, OBJ_CUSTOM_REMOVE_TO_ROOM)) {
					act(obj_get_custom_message(obj, OBJ_CUSTOM_REMOVE_TO_ROOM), TRUE, ch, obj, NULL, TO_ROOM);
				}
				else {
					act("$n stops using $p.", TRUE, ch, obj, NULL, TO_ROOM);
				}
				
				// this may extract it
				unequip_char_to_inventory(ch, iter);
				determine_gear_level(ch);
			}
		}
	}
	
	
	// revert vampire morphs
	if (IS_MORPHED(ch) && CHAR_MORPH_FLAGGED(ch, MORPHF_VAMPIRE_ONLY)) {
		if (!found) {
			sun_message(ch);
			found = TRUE;
		}
		
		// store morph name
		sprintf(buf, "%s lurches and reverts into $n!", PERS(ch, ch, 0));

		perform_morph(ch, NULL);

		act(buf, TRUE, ch, 0, 0, TO_ROOM);
		msg_to_char(ch, "You revert to your natural form!\r\n");
	}
	
	// lastly
	if (found) {
		command_lag(ch, WAIT_ABILITY);
	}
}


// handles periodic updates when a vampire is drinking blood
EVENTFUNC(vampire_feeding_event) {
	struct char_event_data *data = (struct char_event_data*)event_obj;
	char_data *ch = data->character;
	
	// always delete first
	delete_stored_event(&GET_STORED_EVENTS(ch), SEV_VAMPIRE_FEEDING);
	
	// biting -- this is usually PC-only, but NPCs could learn to do it
	if (GET_FEEDING_FROM(ch)) {
		update_biting_char(ch);
	}
	
	// check if still feeding (etc)
	if (!GET_FEEDING_FROM(ch) || EXTRACTED(ch) || IS_DEAD(ch)) {
		free(data);
		return 0;	// do not re-enqueue
	}
	
	// if we get here, go ahead and repeat
	add_stored_event(&GET_STORED_EVENTS(ch), SEV_VAMPIRE_FEEDING, the_event);
	return 5 RL_SEC;
}


/**
* Detects and handles cases where the player typed "kill/execute [target]"
* while feeding from the target. It will generally handle any case where the
* player is already feeding.
*
* @param char_data *ch The player/actor who typed kill/execute/etc.
* @param char *argument The argument(s) they typed in.
* @return bool If TRUE, sent a message and handled the command. If FALSE, proceed as normal.
*/
bool vampire_kill_feeding_target(char_data *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	char_data *vict;
	obj_data *corpse;
	
	if (!GET_FEEDING_FROM(ch)) {
		return FALSE;	// not feeding at all
	}
	
	one_argument(argument, arg);
	
	// find and validate target
	if (*arg) {
		if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
			msg_to_char(ch, "They don't seem to be here.\r\n");
			return TRUE;
		}
		if (vict != GET_FEEDING_FROM(ch)) {
			msg_to_char(ch, "You can't do that while feeding!\r\n");
			return TRUE;
		}
	}
	else {
		vict = GET_FEEDING_FROM(ch);
	}
	
	// ok: cancel a can't-stop effect, if present
	affect_from_char(ch, ATYPE_CANT_STOP, FALSE);
	
	act("You bite down and kill $N!", FALSE, ch, NULL, vict, TO_CHAR);
	act("$N falls limply from $n's arms!", FALSE, ch, NULL, vict, TO_NOTVICT);
	act("You feel $n's fangs bite down hard. You are dead! Sorry...", FALSE, ch, NULL, vict, TO_VICT);
	if (!IS_NPC(vict)) {
		death_log(vict, ch, ATTACK_EXECUTE);
		add_lore(ch, LORE_PLAYER_KILL, "Killed %s", PERS(vict, vict, TRUE));
		add_lore(vict, LORE_PLAYER_DEATH, "Slain by %s", PERS(ch, ch, TRUE));
	}
	
	check_scaling(vict, ch);	// ensure scaling
	tag_mob(vict, ch);	// ensures loot binding if applicable
	corpse = die(vict, ch);
	
	GET_FED_ON_BY(vict) = NULL;
	GET_FEEDING_FROM(ch) = NULL;
	
	// tag corpse
	if (corpse && !IS_NPC(ch)) {
		corpse->last_owner_id = GET_IDNUM(ch);
		corpse->last_empire_id = GET_LOYALTY(ch) ? EMPIRE_VNUM(GET_LOYALTY(ch)) : NOTHING;
	}
	return TRUE;
}


 //////////////////////////////////////////////////////////////////////////////
//// COMMANDS ////////////////////////////////////////////////////////////////

// this is an attack for vampires, and allows them to feed; mortals pass through to the "bite" social
// has subcmd==1 when sent from do_sire
ACMD(do_bite) {
	bool attacked = FALSE, free_bite = FALSE, in_combat = FALSE;
	bool tank, melee;
	char_data *victim = NULL, *ch_iter;
	struct affected_type *af;
	social_data *soc;
	int result, stacks, success;

	one_argument(argument, arg);

	if (!*argument && cancel_biting(ch, TRUE)) {
		// skip this if they typed an arg; cancels if possible; sends own message if so
	}
	else if (GET_FEEDING_FROM(ch)) {
		msg_to_char(ch, "You are already biting someone!\r\n");
	}
	else if (!IS_VAMPIRE(ch) || !has_player_tech(ch, PTECH_VAMPIRE_BITE)) {
		if ((soc = find_social(ch, "bite", TRUE))) {
			// perform a bite social if possible (pass through args)
			perform_social(ch, soc, argument);
		}
		else {	// social not available?
			send_config_msg(ch, "must_be_vampire");
		}
	}
	else if (!char_can_act(ch, POS_FIGHTING, TRUE, FALSE, TRUE)) {
		// do_bite allows positions as low as sleeping so you can cancel biting, but they can't do anything past here
		// sends own message
	}
	else if (IS_NPC(ch)) {
		msg_to_char(ch, "Nope.\r\n");
	}
	else if (GET_ACTION(ch) != ACT_SIRING && GET_ACTION(ch) != ACT_NONE)
		msg_to_char(ch, "You're a bit busy right now.\r\n");
	else if (!*arg && !(victim = FIGHTING(ch))) {
		msg_to_char(ch, "Bite whom?\r\n");
	}
	else if (!victim && (*arg == UID_CHAR ? !(victim = get_char(arg)) : (subcmd ? (!(victim = get_player_vis(ch, arg, FIND_CHAR_ROOM))) : (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))))) {
		// targeting can be by UID for script/aggro use
		send_config_msg(ch, "no_person");
	}
	else if (IN_ROOM(ch) != IN_ROOM(victim)) {
		// uid-targeting can hit people in other rooms
		send_config_msg(ch, "no_person");
	}
	else if (ch == victim) {
		// try to pass through to social if targeting self
		if ((soc = find_social(ch, "bite", TRUE))) {
			// perform a bite social if possible (pass through args)
			perform_social(ch, soc, argument);
		}
		else {	// social not available?
			send_config_msg(ch, "must_be_vampire");
		}
	}
	else if (GET_FED_ON_BY(victim))
		msg_to_char(ch, "Sorry, somebody beat you to it.\r\n");
	else if (IS_GOD(victim) || IS_IMMORTAL(victim))
		msg_to_char(ch, "You can't bite an immortal!\r\n");
	else if (IS_DEAD(victim)) {
		msg_to_char(ch, "Your victim seems to be dead.\r\n");
	}
	else if ((IS_NPC(victim) || !PRF_FLAGGED(victim, PRF_BOTHERABLE)) && !can_fight(ch, victim))
		act("You can't attack $N!", FALSE, ch, 0, victim, TO_CHAR);
	else if (NOT_MELEE_RANGE(ch, victim)) {
		msg_to_char(ch, "You need to be at melee range to do this.\r\n");
	}
	else if (run_ability_triggers_by_player_tech(ch, PTECH_VAMPIRE_BITE, victim, NULL)) {
		return;
	}
	else {
		// more complex checks before we finish...
		check_scaling(victim, ch);
		
		// cases where the player gets a full blood-drinking bite for free
		if (!AFF_FLAGGED(victim, AFF_NO_DRINK_BLOOD) && (!AWAKE(victim) || (!IS_NPC(victim) && PRF_FLAGGED(victim, PRF_BOTHERABLE)) || IS_INJURED(victim, INJ_TIED | INJ_STAKED) || GET_HEALTH(victim) < MAX(5, GET_MAX_HEALTH(victim)/20))) {
			free_bite = TRUE;
		}
		
		// is anybody other than victim fighting me (allows attack bite but not free-bite)
		in_combat = FALSE;
		DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(ch)), ch_iter, next_in_room) {
			if (ch_iter != victim && FIGHTING(ch_iter) == ch) {
				in_combat = TRUE;
				break;
			}
		}
		
		// trying to sire? deny in this case
		if (in_combat && subcmd) {
			msg_to_char(ch, "You can't do that while someone else is attacking you!\r\n");
			return;
		}
		
		// check cooldown late: allow free bites without cooldown
		if ((in_combat || !free_bite) && get_cooldown_time(ch, COOLDOWN_BITE) > 0) {
			msg_to_char(ch, "Bite is still on cooldown.\r\n");
			return;
		}
		
		// SUCCESS
		command_lag(ch, WAIT_COMBAT_ABILITY);
		
		if (SHOULD_APPEAR(ch)) {
			appear(ch);
		}
		
		// attack version
		if (in_combat || !free_bite) {
			melee = (has_player_tech(ch, PTECH_BITE_MELEE_UPGRADE) && (GET_CLASS_ROLE(ch) == ROLE_MELEE || GET_CLASS_ROLE(ch) == ROLE_SOLO) && check_solo_role(ch));
			tank = (has_player_tech(ch, PTECH_BITE_TANK_UPGRADE) && (GET_CLASS_ROLE(ch) == ROLE_TANK || GET_CLASS_ROLE(ch) == ROLE_SOLO) && check_solo_role(ch));
			attacked = TRUE;
			success = player_tech_skill_check_by_ability_difficulty(ch, PTECH_VAMPIRE_BITE) || check_hit_vs_dodge(ch, victim, FALSE);
			
			// only cools down if it's an attack bite
			add_cooldown(ch, COOLDOWN_BITE, melee ? 9 : 12);
			
			if (success) {
				result = damage(ch, victim, (2 * GET_STRENGTH(ch)) + GET_BONUS_PHYSICAL(ch), ATTACK_VAMPIRE_BITE, DAM_PHYSICAL, NULL);
			}
			else {
				result = damage(ch, victim, 0, ATTACK_VAMPIRE_BITE, DAM_PHYSICAL, NULL);
			}
			
			// reduce DODGE
			if (GET_DODGE(ch) > 0 && !tank) {
				af = create_mod_aff(ATYPE_BITE_PENALTY, 5, APPLY_DODGE, -GET_DODGE(ch), ch);
				affect_join(ch, af, 0);
			}
			
			// 33% chance of taunting npcs
			if (!melee && result > 0 && !IS_DEAD(victim) && IS_NPC(victim) && FIGHTING(victim) && FIGHTING(victim) != ch && (tank || !number(0, 2))) {
				perform_rescue(ch, FIGHTING(victim), victim, RESCUE_FOCUS);
			}
			
			// melee DoT effect
			if (melee && result > 0) {
				stacks = get_approximate_level(ch) / 50;
				apply_dot_effect(victim, ATYPE_BITE, 15, DAM_PHYSICAL, 7, MAX(1, stacks), ch);
			}
			
			// steal blood effect
			if (has_player_tech(ch, PTECH_BITE_STEAL_BLOOD) && result > 0 && !AFF_FLAGGED(victim, AFF_NO_DRINK_BLOOD) && !GET_FED_ON_BY(victim)) {
				set_blood(ch, GET_BLOOD(ch) + 2);
				set_blood(victim, MAX(1, GET_BLOOD(victim) - 2));
			}
			
			if (can_gain_exp_from(ch, victim)) {
				gain_player_tech_exp(ch, PTECH_VAMPIRE_BITE, 10);
				if (melee) {
					gain_player_tech_exp(ch, PTECH_BITE_MELEE_UPGRADE, 10);
				}
				if (tank) {
					gain_player_tech_exp(ch, PTECH_BITE_TANK_UPGRADE, 10);
				}
			}
			run_ability_hooks_by_player_tech(ch, PTECH_VAMPIRE_BITE, victim, NULL, NULL, NULL);
			if (melee) {
				run_ability_hooks_by_player_tech(ch, PTECH_BITE_MELEE_UPGRADE, victim, NULL, NULL, NULL);
			}
			if (tank) {
				run_ability_hooks_by_player_tech(ch, PTECH_BITE_TANK_UPGRADE, victim, NULL, NULL, NULL);
			}
		}
		
		// if this attack would kill them, need to go into blood drinking instead
		//	- does that happen in damage() or die() instead?
		
		// actually drink the blood
		if (!attacked && !GET_FEEDING_FROM(ch) && !AFF_FLAGGED(victim, AFF_NO_DRINK_BLOOD) && IN_ROOM(ch) == IN_ROOM(victim) && !IS_DEAD(victim)) {
			start_drinking_blood(ch, victim);
			
			// also clear the bite cooldown in this case (they can do non-attack bites during the cooldown)
			remove_cooldown_by_type(ch, COOLDOWN_BITE);
		}
	}
}


ACMD(do_boost) {
	struct affected_type *af;
	int iter, pos = NOTHING, cost = 10;
	bool any;
	
	struct {
		char *name;
		char *alt_name;	// secondary name that can be used optionally (phys instead of bonus-phys)
		int role;	// required ROLE_ or ROLE_NONE
		int main_att;	// like STRENGTH; NOTHING if using a secondary attribute (like an ATT_)
		int apply;	// which APPLY_ const
		int base_amt;	// how much with low-level skill
		int high_amt;	// how much with high skill (TODO: should it just scale?)
		char *msg;	// string shown to the user via act()
	} boost_data[] = {
		{ "charisma", "charisma", ROLE_NONE, CHARISMA, APPLY_CHARISMA, 1, 2, "You focus your blood into your skin and voice, increasing your charisma!" },
		{ "strength", "strength", ROLE_NONE, STRENGTH, APPLY_STRENGTH, 1, 2, "You force blood into your muscles, boosting your strength!" },
		{ "intelligence", "intelligence", ROLE_NONE, INTELLIGENCE, APPLY_INTELLIGENCE, 1, 2, "You focus your blood into your mind, increasing your intelligence!" },
		
		{ "bonus-physical", "physical", ROLE_MELEE, NOTHING, APPLY_BONUS_PHYSICAL, 1, 2, "You focus your blood into a blinding rage, increasing your physical damage!" },
		{ "bonus-magical", "magical", ROLE_CASTER, NOTHING, APPLY_BONUS_MAGICAL, 1, 2, "You turn your blood into pure mental focus, increasing your magical damage!" },
		{ "bonus-healing", "healing", ROLE_HEALER, NOTHING, APPLY_BONUS_HEALING, 1, 2, "You draw the magic from your blood, increasing your magical healing!" },
		{ "dodge", "dodge", ROLE_TANK, NOTHING, APPLY_DODGE, 1, 2, "You focus your blood to increase your speed, boosting your ability to dodge!" },
		
		{ "\n", "\n", ROLE_NONE, NOTHING, NOTHING, 0, 0, "\n" }	// must be last
	};

	one_argument(argument, arg);

	if (!check_vampire_ability(ch, ABIL_BOOST, BLOOD, cost, NOTHING)) {
		return;	// sends own message
	}
	if (!check_vampire_sun(ch, TRUE)) {
		return;	// sends own message
	}
	if (!*arg) {
		msg_to_char(ch, "Which attribute do you wish to boost? Or type 'boost end' to cancel boosts.\r\nAttributes:");
		any = FALSE;
		for (iter = 0; *boost_data[iter].name != '\n'; ++iter) {
			if (boost_data[iter].role == ROLE_NONE || boost_data[iter].role == GET_CLASS_ROLE(ch)) {
				msg_to_char(ch, "%s%s", (any ? ", " : " "), boost_data[iter].name);
				any = TRUE;
			}
		}
		msg_to_char(ch, "\r\n");
		return;
	}
	
	// END boosts
	if (is_abbrev(arg, "end")) {
		if (affected_by_spell(ch, ATYPE_BOOST)) {
			end_boost(ch);
		}
		else {
			msg_to_char(ch, "Your attributes aren't boosted!\r\n");
		}
		return;
	}
	
	// identify which boost
	for (iter = 0; *boost_data[iter].name != '\n'; ++iter) {
		if (is_abbrev(arg, boost_data[iter].name) || is_abbrev(arg, boost_data[iter].alt_name)) {
			pos = iter;	// found!
			break;
		}
	}
	if (pos == NOTHING) {	// no valid choice
		msg_to_char(ch, "You don't know how to boost '%s'.\r\n", arg);
		return;
	}
	
	// final checks:
	if (boost_data[pos].main_att != NOTHING && GET_ATT(ch, boost_data[pos].main_att) >= att_max(ch)) {
		msg_to_char(ch, "Your %s is already at maximum!\r\n", boost_data[pos].name);
		return;
	}
	if (affected_by_spell_and_apply(ch, ATYPE_BOOST, boost_data[pos].apply, NOBITS)) {
		msg_to_char(ch, "Your %s is already boosted!\r\n", boost_data[pos].name);
		return;
	}
	
	// check trigs
	if (ABILITY_TRIGGERS(ch, NULL, NULL, ABIL_BOOST)) {
		return;
	}
	
	// SUCCESS!
	charge_ability_cost(ch, BLOOD, cost, NOTHING, 0, WAIT_ABILITY);
	
	af = create_mod_aff(ATYPE_BOOST, 5 * SECS_PER_REAL_MIN, boost_data[pos].apply, (skill_check(ch, ABIL_BOOST, DIFF_HARD) ? boost_data[pos].high_amt : boost_data[pos].base_amt), ch);
	affect_join(ch, af, AVG_DURATION | ADD_MODIFIER);
	
	af = create_mod_aff(ATYPE_BOOST, 5 * SECS_PER_REAL_MIN, APPLY_BLOOD_UPKEEP, 1, ch);
	affect_to_char(ch, af);
	free(af);
	
	if (boost_data[pos].msg) {
		act(boost_data[pos].msg, FALSE, ch, NULL, NULL, TO_CHAR | ACT_BUFF);
	}
	
	gain_ability_exp(ch, ABIL_BOOST, 20);
	run_ability_hooks(ch, AHOOK_ABILITY, ABIL_BOOST, 0, ch, NULL, NULL, NULL, NOBITS);
}


ACMD(do_command) {
	char_data *victim;
	char *to_do;
	bool un_charm;
	
	int level_threshold = 25;

	argument = two_arguments(argument, arg, buf);

	if (IS_NPC(ch)) {
		msg_to_char(ch, "NPCs cannot use Command.\r\n");
	}
	else if (!check_vampire_ability(ch, ABIL_VAMP_COMMAND, NOTHING, 0, NOTHING)) {
		return;
	}
	else if (!check_vampire_sun(ch, TRUE)) {
		return;
	}
	else if (!*arg || !*buf)
		msg_to_char(ch, "Force whom to do what command?\r\n");
	else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if (ch == victim)
		msg_to_char(ch, "That seems rather pointless, doesn't it?\r\n");
	else if (!IS_NPC(victim)) {
		msg_to_char(ch, "You can only command NPCs.\r\n");
	}
	else if (!MOB_FLAGGED(victim, MOB_HUMAN))
		msg_to_char(ch, "You can only give commands to humans.\r\n");
	else if (MOB_FLAGGED(victim, MOB_HARD | MOB_GROUP | MOB_NO_COMMAND) || AFF_FLAGGED(victim, AFF_NO_ATTACK)) {
		act("You can't command $N.", FALSE, ch, NULL, victim, TO_CHAR);
	}
	else if (IS_VAMPIRE(victim) && (IS_NPC(victim) || has_skill_flagged(victim, SKILLF_VAMPIRE) > has_skill_flagged(ch, SKILLF_VAMPIRE)))
		msg_to_char(ch, "You cannot force your will upon those of more powerful blood.\r\n");
	else if (MAX(has_skill_flagged(ch, SKILLF_VAMPIRE), get_approximate_level(ch)) - get_approximate_level(victim) < level_threshold) {
		msg_to_char(ch, "Your victim is too powerful.\r\n");
	}
	else if (!AWAKE(victim))
		msg_to_char(ch, "Your victim will need to be awake to understand you.\r\n");
	else if (*argument && !str_str(argument, buf))
		msg_to_char(ch, "If you wish to include your orders in a sentence, you must include the word in that sentence.\r\nFormat: command <target> <command> [sentence including command]\r\n");
	else if (!CAN_SEE(victim, ch))
		act("How do you intend to command $M if $E can't even see you?", FALSE, ch, 0, victim, TO_CHAR);
	else if (ABILITY_TRIGGERS(ch, victim, NULL, ABIL_VAMP_COMMAND)) {
		return;
	}
	else {
		/* do_say corrodes buf */
		to_do = str_dup(buf);

		/* The command is spoken.. */
		if (!*argument)
			strcpy(argument, buf);
		do_say(ch, argument, 0, 0);
		
		if (can_gain_exp_from(ch, victim)) {
			gain_ability_exp(ch, ABIL_VAMP_COMMAND, 33.4);
		}

		if (skill_check(ch, ABIL_VAMP_COMMAND, DIFF_MEDIUM) && !AFF_FLAGGED(victim, AFF_IMMUNE_MENTAL_DEBUFFS)) {
			un_charm = AFF_FLAGGED(victim, AFF_CHARM) ? FALSE : TRUE;
			SET_BIT(AFF_FLAGS(victim), AFF_CHARM);
			
			// do
			SET_BIT(AFF_FLAGS(victim), AFF_ORDERED);
			command_interpreter(victim, to_do);
			REMOVE_BIT(AFF_FLAGS(victim), AFF_ORDERED);
			
			if (un_charm && !EXTRACTED(victim)) {
				REMOVE_BIT(AFF_FLAGS(victim), AFF_CHARM);
			}
			
			if (!IS_DEAD(victim) && !EXTRACTED(victim) && IN_ROOM(victim) == IN_ROOM(ch)) {
				run_ability_hooks(ch, AHOOK_ABILITY, ABIL_VAMP_COMMAND, 0, victim, NULL, NULL, NULL, NOBITS);
			}
		}
		
		command_lag(ch, WAIT_ABILITY);
		free(to_do);
	}
}


ACMD(do_feed) {
	char_data *victim;
	int amt = 0;

	two_arguments(argument, arg, buf);

	if (!IS_VAMPIRE(ch)) {
		msg_to_char(ch, "You need to be a vampire to feed blood to others.\r\n");
		return;
	}
	else if (!*arg || !*buf)
		msg_to_char(ch, "Feed whom how much blood?\r\n");
	else if ((amt = atoi(buf)) <= 0)
		msg_to_char(ch, "Feed how much blood?\r\n");
	else if (amt > GET_BLOOD(ch) - 1)
		msg_to_char(ch, "You can't give THAT much blood.\r\n");
	else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if ( ch == victim )
		msg_to_char(ch, "What would be the point in that?\r\n");
	else if (IS_DEAD(victim)) {
		act("It would do no good for $M now -- $E's dead!", FALSE, ch, NULL, victim, TO_CHAR);
	}
	else if (IS_NPC(victim) || (!PRF_FLAGGED(victim, PRF_BOTHERABLE) && GET_POS(victim) >= POS_SLEEPING)) {
		act("$E refuses your blood.", FALSE, ch, 0, victim, TO_CHAR);
	}
	else {
		act("You slice your wrist open and feed $N some blood!", FALSE, ch, 0, victim, TO_CHAR);
		act("$n slices $s wrist open and feeds $N some blood!", TRUE, ch, 0, victim, TO_NOTVICT);
		act("$n slices $s wrist open and feeds you some blood from the cut!", FALSE, ch, 0, victim, TO_VICT);

		// mve the blood
		set_blood(ch, GET_BLOOD(ch) - amt);
		set_blood(victim, GET_BLOOD(victim) + amt);
	}
}


ACMD(do_sire) {
	char_data *victim;

	one_argument(argument, arg);

	if (!IS_VAMPIRE(ch)) {
		send_config_msg(ch, "must_be_vampire");
	}
	else if (IS_NPC(ch)) {
		msg_to_char(ch, "Nope.\r\n");
	}
	else if (!*arg)
		msg_to_char(ch, "Sire whom?\r\n");
	else if (GET_ACTION(ch) != ACT_NONE)
		msg_to_char(ch, "You're too busy to do that.\r\n");
	else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if (IS_NPC(victim))
		msg_to_char(ch, "You can't sire an NPC.\r\n");
	else if (IS_VAMPIRE(victim))
		msg_to_char(ch, "It looks like someone already beat you to it!\r\n");
	else if (IS_GOD(victim) || IS_IMMORTAL(victim))
		msg_to_char(ch, "You can't sire a deity!\r\n");
	else {
		sprintf(buf, "%s", GET_NAME(victim));
		do_bite(ch, buf, 0, 1);
		
		if (GET_FEEDING_FROM(ch) == victim) {
			start_action(ch, ACT_SIRING, -1);
		}
	}
}


ACMD(do_veintap) {
	obj_data *container;
	int amt = 0;

	argument = two_arguments(argument, buf, buf1);

	/*
	 * buf = amount
	 * buf1 = countainer
	 */

	if (IS_NPC(ch)) {
		msg_to_char(ch, "NPCs cannot veintap.\r\n");
	}
	else if (!check_vampire_ability(ch, ABIL_VEINTAP, NOTHING, 0, NOTHING)) {
		return;
	}
	else if (!*buf || !*buf1)
		msg_to_char(ch, "Usage: veintap <amount> <container>\r\n");
	else if (!IS_VAMPIRE(ch))
		msg_to_char(ch, "Only vampire blood may be stored with blood essence.\r\n");
	else if ((amt = atoi(buf)) <= 0)
		msg_to_char(ch, "Drain how much blood?\r\n");
	else if (GET_BLOOD(ch) < amt + 1)
		msg_to_char(ch, "You can't drain THAT much!\r\n");
	else if (!(container = get_obj_in_list_vis(ch, buf1, NULL, ch->carrying)))
		msg_to_char(ch, "You don't seem to have %s %s.\r\n", AN(buf1), buf1);
	else if (GET_OBJ_TYPE(container) != ITEM_DRINKCON)
		msg_to_char(ch, "You can't drain blood into that!\r\n");
	else if (GET_DRINK_CONTAINER_CONTENTS(container) > 0 && GET_DRINK_CONTAINER_TYPE(container) != LIQ_BLOOD)
		act("$p already contains something else!", FALSE, ch, container, 0, TO_CHAR);
	else if (GET_DRINK_CONTAINER_CONTENTS(container) >= GET_DRINK_CONTAINER_CAPACITY(container))
		act("$p is already full.", FALSE, ch, container, 0, TO_CHAR);
	else if (ABILITY_TRIGGERS(ch, NULL, container, ABIL_VEINTAP)) {
		return;
	}
	else {
		act("You drain some of your blood into $p!", FALSE, ch, container, 0, TO_CHAR);
		act("$n drains some of $s blood into $p!", TRUE, ch, container, 0, TO_ROOM);

		amt = MIN(amt, GET_DRINK_CONTAINER_CAPACITY(container) - GET_DRINK_CONTAINER_CONTENTS(container));

		charge_ability_cost(ch, BLOOD, amt, NOTHING, 0, WAIT_ABILITY);
		set_obj_val(container, VAL_DRINK_CONTAINER_CONTENTS, GET_DRINK_CONTAINER_CONTENTS(container) + amt);
		set_obj_val(container, VAL_DRINK_CONTAINER_TYPE, LIQ_BLOOD);
		GET_OBJ_TIMER(container) = UNLIMITED;
		request_obj_save_in_world(container);
		
		gain_ability_exp(ch, ABIL_VEINTAP, 33.4);
		run_ability_hooks(ch, AHOOK_ABILITY, ABIL_VEINTAP, 0, NULL, container, NULL, NULL, NOBITS);
	}
}
