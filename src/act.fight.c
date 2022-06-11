/* ************************************************************************
*   File: act.fight.c                                     EmpireMUD 2.0b5 *
*  Usage: non-skill commands and functions related to the fight system    *
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
#include "constants.h"

/**
* Contents:
*   Helpers
*   Commands
*/


 //////////////////////////////////////////////////////////////////////////////
//// HELPERS ////////////////////////////////////////////////////////////////

/**
* For the "use" command -- switches your preferred arrows.
*
* @param char_data *ch The player.
* @param obj_data *obj The ammo to prefer.
*/
void use_ammo(char_data *ch, obj_data *obj) {
	if (!IS_AMMO(obj)) {
		// ??? shouldn't ever get here
		act("$p isn't something you can use like this.", FALSE, ch, obj, NULL, TO_CHAR);
		return;
	}
	
	USING_AMMO(ch) = GET_OBJ_VNUM(obj);
	act("You are now using $p as your preferred ammunition.", FALSE, ch, obj, NULL, TO_CHAR);
}


 //////////////////////////////////////////////////////////////////////////////
//// COMMANDS ////////////////////////////////////////////////////////////////

ACMD(do_approach) {
	if (!FIGHTING(ch)) {
		msg_to_char(ch, "You aren't even fighting.\r\n");
	}
	else if (FIGHT_MODE(ch) == FMODE_MELEE) {
		msg_to_char(ch, "You're already in melee combat.\r\n");
	}
	else if (FIGHT_MODE(ch) == FMODE_WAITING) {
		msg_to_char(ch, "You're already trying to approach!\r\n");
	}
	else if (AFF_FLAGGED(ch, AFF_STUNNED | AFF_HARD_STUNNED | AFF_ENTANGLED)) {
		msg_to_char(ch, "You can't try to approach right now!\r\n");
	}
	else {
		FIGHT_MODE(ch) = FMODE_WAITING;
		FIGHT_WAIT(ch) = 4;
		msg_to_char(ch, "You start to approach!\r\n");
		act("$n starts to approach!", FALSE, ch, NULL, NULL, TO_ROOM);
	}
}


ACMD(do_assist) {
	char_data *helpee, *opponent;

	if (FIGHTING(ch)) {
		send_to_char("You're already fighting! How can you assist someone else?\r\n", ch);
		return;
	}
	one_argument(argument, arg);

	if (!*arg)
		send_to_char("Whom do you wish to assist?\r\n", ch);
	else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if (helpee == ch)
		send_to_char("You can't help yourself any more than this!\r\n", ch);
	else {
		/*
		 * Hit the same enemy the person you're helping is.
		 */
		if (FIGHTING(helpee)){
			opponent = FIGHTING(helpee);
		}
		else {
			DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(ch)), opponent, next_in_room) {
				if (FIGHTING(opponent) == helpee) {
					break;	// found!
				}
			}
		}

		if (!opponent)
			act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (!CAN_SEE(ch, opponent))
			act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (FIGHT_MODE(opponent) != FMODE_MELEE)
			msg_to_char(ch, "You can't attack until they've engaged in melee combat.\r\n");
		else if (can_fight(ch, opponent)) {
			send_to_char("You join the fight!\r\n", ch);
			act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
			act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
			hit(ch, opponent, GET_EQ(ch, WEAR_WIELD), TRUE);
		}
		else {
			act("You can't attack $N!", FALSE, ch, 0, opponent, TO_CHAR);
		}
	}
}


ACMD(do_clearmeters) {
	if (!IS_NPC(ch)) {
		reset_combat_meters(ch);
		GET_COMBAT_METERS(ch).over = TRUE;
	}
	send_config_msg(ch, "ok_string");
}


ACMD(do_consider) {
	char buf[MAX_STRING_LENGTH];
	bitvector_t bits;
	int diff, pos, hitch;
	char_data *vict;
	bool any = FALSE;
	
	one_argument(argument, arg);
	
	if (!*arg) {
		msg_to_char(ch, "Consider whom?\r\n");
	}
	else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
	}
	else if (vict == ch) {
		msg_to_char(ch, "You look pretty wimpy.\r\n");
	}
	else if (AFF_FLAGGED(vict, AFF_NO_ATTACK)) {
		act("$N cannot be attacked.", FALSE, ch, NULL, vict, TO_CHAR);
	}
	else {
		// scale first
		if (!IS_IMMORTAL(ch)) {
			check_scaling(vict, ch);
		}
		diff = get_approximate_level(ch) - determine_best_scale_level(vict, FALSE);
				
		act("You consider your chances against $N.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n considers $s chances against $N.", FALSE, ch, NULL, vict, TO_NOTVICT);
		act("$n considers $s chances against you.", FALSE, ch, NULL, vict, TO_VICT);
		
		if (diff != 0) {
			snprintf(buf, sizeof(buf), "$E is %d level%s %s you.", ABSOLUTE(diff), PLURAL(ABSOLUTE(diff)), diff > 0 ? "below" : "above");
			act(buf, FALSE, ch, NULL, vict, TO_CHAR);
			any = TRUE;
		}
		
		// difficulty
		if (MOB_FLAGGED(vict, MOB_HARD) && MOB_FLAGGED(vict, MOB_GROUP)) {
			act("$E is a boss fight (requires 4 players of proper level).", FALSE, ch, NULL, vict, TO_CHAR);
			any = TRUE;
		}
		else if (MOB_FLAGGED(vict, MOB_GROUP)) {
			act("$E is a group fight (requires 3 players of proper level).", FALSE, ch, NULL, vict, TO_CHAR);
			any = TRUE;
		}
		else if (MOB_FLAGGED(vict, MOB_HARD)) {
			act("$E is a hard fight (may require 2 players of proper level).", FALSE, ch, NULL, vict, TO_CHAR);
			any = TRUE;
		}

		// hit/dodge
		hitch = get_to_hit(ch, vict, FALSE, FALSE) - get_dodge_modifier(vict, ch, FALSE);
		if (hitch < 50) {
			act("You would have trouble hitting $M.", FALSE, ch, NULL, vict, TO_CHAR);
			any = TRUE;
		}
		hitch = get_to_hit(vict, ch, FALSE, FALSE) - get_dodge_modifier(ch, vict, FALSE);
		if (hitch > 50) {
			act("You would have trouble dodging $S attacks.", FALSE, ch, NULL, vict, TO_CHAR);
			any = TRUE;
		}

		// flags (with overflow protection on affected_bits_consider[])
		for (bits = AFF_FLAGS(vict), pos = 0; bits && *affected_bits_consider[pos] != '\n'; bits >>= 1, ++pos) {
			if (IS_SET(bits, BIT(0)) && *affected_bits_consider[pos]) {
				act(affected_bits_consider[pos], FALSE, ch, NULL, vict, TO_CHAR);
				any = TRUE;
			}
		}
		
		// no message sent?
		if (!any) {
			msg_to_char(ch, "You seem to be an even match.\r\n");
		}
	}
}


ACMD(do_execute) {
	char_data *victim;

	one_argument(argument, arg);

	if (!*arg)
		msg_to_char(ch, "Execute whom?\r\n");
	else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if (victim == ch)
		msg_to_char(ch, "Seek professional help.\r\n");
	else if (GET_POS(victim) >= POS_SLEEPING && !IS_INJURED(victim, INJ_TIED | INJ_STAKED))
		act("You need to knock $M out or tie $M up.", FALSE, ch, 0, victim, TO_CHAR);
	else if (IS_DEAD(victim)) {
		msg_to_char(ch, "Your victim is already dead!\r\n");
	}
	else if (!can_fight(ch, victim)) {
		act("You can't execute $N!", FALSE, ch, NULL, victim, TO_CHAR);
	}
	else {
		perform_execute(ch, victim, TYPE_UNDEFINED, DAM_PHYSICAL);
	}
}


ACMD(do_flee) {
	int i, attempt, try;
	room_data *to_room = NULL;
	char_data *was_fighting;
	bool inside = ROOM_IS_CLOSED(IN_ROOM(ch));
	struct room_direction_data *ex;

	if (GET_POS(ch) < POS_FIGHTING) {
		send_to_char("You are in pretty bad shape, unable to flee!\r\n", ch);
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_ENTANGLED)) {
		msg_to_char(ch, "You are entangled and can't flee.\r\n");
		return;
	}

	// try more times if FLEET
	for (i = 0; i < NUM_2D_DIRS * ((!IS_NPC(ch) && has_ability(ch, ABIL_FLEET)) ? 2 : 1); i++) {
		// chance to fail if not FLEET
		if ((IS_NPC(ch) || !has_ability(ch, ABIL_FLEET)) && number(0, 2) == 0) {
			continue;
		}

		// try 10 times to find a fleeable direction for this try
		for (try = 0, attempt = NO_DIR; try < 10 && (attempt == NO_DIR || !can_flee_dir[attempt]); ++try) {
			attempt = number(0, NUM_OF_DIRS - 1);
		}
		
		// did we find a good dir?
		if (attempt == NO_DIR || !can_flee_dir[attempt]) {
			continue;
		}
		
		// no need for to_room if not inside -- this would be a waste
		if (!inside) {
			to_room = real_shift(IN_ROOM(ch), shift_dir[attempt][0], shift_dir[attempt][1]);
			
			// did we find anything valid that direction?
			if (to_room == IN_ROOM(ch)) {
				continue;
			}
		}
		
		if ((inside && (ex = find_exit(IN_ROOM(ch), attempt)) && CAN_GO(ch, ex)) || (!inside && to_room && (!ROOM_SECT_FLAGGED(to_room, SECTF_ROUGH | SECTF_FRESH_WATER | SECTF_OCEAN) || IS_RIDING(ch)) && !ROOM_IS_CLOSED(to_room))) {
			act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
			was_fighting = FIGHTING(ch);
			if (perform_move(ch, attempt, NULL, NOBITS)) {
				send_to_char("You flee head over heels.\r\n", ch);
				if (was_fighting && can_gain_exp_from(ch, was_fighting)) {
					gain_ability_exp(ch, ABIL_FLEET, 5);
				}
				GET_WAIT_STATE(ch) = 2 RL_SEC;
			}
			else {
				act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
				send_to_char("PANIC! You couldn't escape!\r\n", ch);
				GET_WAIT_STATE(ch) = 2 RL_SEC;
			}
			return;
		}
	}
	send_to_char("PANIC! You couldn't escape!\r\n", ch);
	GET_WAIT_STATE(ch) = 2 RL_SEC;
}


ACMD(do_hit) {
	char_data *vict;

	one_argument(argument, arg);

	if (!*arg)
		send_to_char("Hit whom?\r\n", ch);
	else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_to_char("They don't seem to be here.\r\n", ch);
	else if (vict == ch) {
		send_to_char("You hit yourself...OUCH!.\r\n", ch);
		act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
		}
	else if (AFF_FLAGGED(ch, AFF_CHARM) && (GET_LEADER(ch) == vict))
		act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
	else if (vict == FIGHTING(ch) && FIGHT_MODE(ch) == FMODE_MISSILE) {
		if (FIGHT_MODE(vict) == FMODE_MISSILE) {
			act("You run at $M!", FALSE, ch, 0, vict, TO_CHAR);
			FIGHT_MODE(ch) = FMODE_WAITING;
			FIGHT_WAIT(ch) = 4;
			command_lag(ch, WAIT_OTHER);
		}
		else {
			FIGHT_MODE(ch) = FMODE_MELEE;
		}
	}
	else if (can_fight(ch, vict)) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && GET_LEADER(ch) && !IS_NPC(GET_LEADER(ch)) && !IS_NPC(vict))
			return;

		if (FIGHTING(ch) == vict) {
			msg_to_char(ch, "You do the best that you can!\r\n");
		}
		else if (FIGHTING(vict) && FIGHT_MODE(vict) == FMODE_MISSILE) {
			set_fighting(ch, vict, FMODE_WAITING);
			act("You run at $M!", FALSE, ch, 0, vict, TO_CHAR);
			command_lag(ch, WAIT_OTHER);
		}
		else {
			if (!FIGHTING(ch)) {
				hit(ch, vict, GET_EQ(ch, WEAR_WIELD), FIGHTING(ch) ? FALSE : TRUE);	// count as exp only if not already fighting
				
				// ensure hitting
				if (vict && !EXTRACTED(vict) && !IS_DEAD(vict) && FIGHTING(ch) && FIGHTING(ch) != vict) {
					FIGHTING(ch) = vict;
				}
			}
			else {	// already fighting -- just change targets
				act("You change your focus to $N.", FALSE, ch, NULL, vict, TO_CHAR);
				act("$n changes $s focus to $N.", FALSE, ch, NULL, vict, TO_NOTVICT);
				act("$n changes $s focus to you!", FALSE, ch, NULL, vict, TO_VICT);
				
				FIGHTING(ch) = vict;
				
				if (FIGHTING(vict) == ch && FIGHT_MODE(vict) == FMODE_MELEE) {
					FIGHT_MODE(ch) = FMODE_MELEE;
					FIGHT_WAIT(ch) = 0;
				}
				else if (FIGHT_MODE(vict) == FMODE_MISSILE) {
					if (FIGHT_MODE(ch) != FMODE_MISSILE) {
						FIGHT_MODE(ch) = FMODE_MISSILE;
					}
					FIGHT_WAIT(ch) = 0;
				}
			}
			
			// cancel combat if auto-execute is off and the mob is unconscious after the hit
			if (FIGHTING(ch) == vict && GET_HEALTH(vict) <= 0 && !WOULD_EXECUTE(ch, vict)) {
				stop_fighting(ch);
			}
			
			command_lag(ch, WAIT_OTHER);
		}
	}
	else {
		act("You can't attack $N!", FALSE, ch, 0, vict, TO_CHAR);
	}
}


ACMD(do_meters) {
	struct combat_meters *mtr;
	int length = 1;
	double calc;
	
	if (IS_NPC(ch)) {
		msg_to_char(ch, "You have no damage meters.\r\n");
		return;
	}
	
	mtr = &GET_COMBAT_METERS(ch);
	
	msg_to_char(ch, "Damage meters:\r\n");
	
	// raw length
	length = (mtr->over ? mtr->end : time(0)) - mtr->start;
	msg_to_char(ch, "Fight length: %d:%02d (%d second%s)\r\n", (length/60), (length%60), length, PLURAL(length));
	
	// prevent divide-by-zero
	length = MAX(1, length);
	
	calc = mtr->hits + mtr->misses;
	calc = MAX(1.0, calc);	// div/0
	msg_to_char(ch, "Hit percent: %.2f%% (%d/%d)\r\n", mtr->hits * 100.0 / calc, mtr->hits, (mtr->hits + mtr->misses));
	
	calc = mtr->hits_taken + mtr->dodges + mtr->blocks;
	calc = MAX(1.0, calc);	// div/0
	msg_to_char(ch, "Dodge percent: %.2f%% (%d/%d)\r\n", mtr->dodges * 100.0 / calc, mtr->dodges, (mtr->hits_taken + mtr->dodges + mtr->blocks));
	msg_to_char(ch, "Block percent: %.2f%% (%d/%d)\r\n", mtr->blocks * 100.0 / calc, mtr->blocks, (mtr->hits_taken + mtr->dodges + mtr->blocks));
	
	msg_to_char(ch, "Damage dealt: %d (%.2f dps)\r\n", mtr->damage_dealt, (double) mtr->damage_dealt / length);
	
	if (mtr->pet_damage > 0) {
		msg_to_char(ch, "Pet damage: %d (%.2f dps, %.2f total dps)\r\n", mtr->pet_damage, (double) mtr->pet_damage / length, (double) (mtr->damage_dealt + mtr->pet_damage) / length);
	}
	
	msg_to_char(ch, "Damage taken: %d (%.2f dps)\r\n", mtr->damage_taken, (double) mtr->damage_taken / length);
	
	if (mtr->heals_dealt > 0) {
		msg_to_char(ch, "Heals dealt: %d (%.2f hps)\r\n", mtr->heals_dealt, (double) mtr->heals_dealt / length);
	}
	if (mtr->heals_taken > 0) {
		msg_to_char(ch, "Heals taken: %d (%.2f hps)\r\n", mtr->heals_taken, (double) mtr->heals_taken / length);
	}
}


ACMD(do_respawn) {
	if (!IS_DEAD(ch) && !IS_INJURED(ch, INJ_STAKED)) {
		msg_to_char(ch, "You aren't even dead yet!\r\n");
	}
	else if (IS_NPC(ch)) {
		// somehow
		act("$n dies.", FALSE, ch, NULL, NULL, TO_ROOM);
		extract_char(ch);
	}
	else {
		// respawn to starting point
		msg_to_char(ch, "You shuffle off this mortal coil, and die...\r\n");
		act("$n shuffles off $s mortal coil and dies.", FALSE, ch, NULL, NULL, TO_ROOM);
		
		player_death(ch);
		char_to_room(ch, find_load_room(ch));
		GET_LAST_DIR(ch) = NO_DIR;
		qt_visit_room(ch, IN_ROOM(ch));
		
		syslog(SYS_DEATH, GET_INVIS_LEV(ch), TRUE, "%s has respawned at %s", GET_NAME(ch), room_log_identifier(IN_ROOM(ch)));
		act("$n rises from the dead!", TRUE, ch, NULL, NULL, TO_ROOM);
		look_at_room(ch);
		
		affect_total(ch);
		queue_delayed_update(ch, CDU_SAVE);
		greet_mtrigger(ch, NO_DIR);
		greet_memory_mtrigger(ch);
		greet_vtrigger(ch, NO_DIR);
		msdp_update_room(ch);
	}
}


ACMD(do_shoot) {
	char_data *vict;

	one_argument(argument, arg);
	
	if (FIGHTING(ch) && FIGHT_MODE(ch) == FMODE_MELEE) {
		msg_to_char(ch, "You can't shoot anyone while you're in melee combat.\r\n");
	}
	else if (!*arg)
		msg_to_char(ch, "Shoot whom?\r\n");
	else if (!(vict= get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if (vict == ch)
		msg_to_char(ch, "Shooting yourself in the foot will do you no good now.\r\n");
	else if (AFF_FLAGGED(ch, AFF_CHARM) && GET_LEADER(ch) == vict)
		act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
	else if (!GET_EQ(ch, WEAR_RANGED)) {
		msg_to_char(ch, "You aren't holding anything you can shoot!\r\n");
	}
	else if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_RANGED)) != ITEM_MISSILE_WEAPON) {
		msg_to_char(ch, "The item in your ranged slot isn't a ranged weapon.\r\n");
	}
	else if (FIGHTING(ch) && vict == FIGHTING(ch)) {
		act("You're already fighting $N!", FALSE, ch, NULL, vict, TO_CHAR);
	}
	else if (FIGHTING(ch)) {
		// switch target (we are either in missile or ranged states
		FIGHTING(ch) = vict;
		if (FIGHT_MODE(ch) == FMODE_WAITING) {
			FIGHT_WAIT(ch) = 0;
		}
		FIGHT_MODE(ch) = FMODE_MISSILE;
		
		act("You change your focus to $N.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n changes $s focus to $N.", FALSE, ch, NULL, vict, TO_NOTVICT);
		act("$n changes $s focus to you!", FALSE, ch, NULL, vict, TO_VICT);
		
		if (!FIGHTING(vict)) {
			set_fighting(vict, ch, FMODE_MISSILE);
		}
	}
	else if (can_fight(ch, vict)) {
		if (AFF_FLAGGED(ch, AFF_CHARM) && GET_LEADER(ch) && !IS_NPC(GET_LEADER(ch)) && !IS_NPC(vict))
			return;

		msg_to_char(ch, "You take aim.\r\n");
		act("$n takes aim.", TRUE, ch, 0, 0, TO_ROOM);
		set_fighting(ch, vict, FMODE_MISSILE);
		if (!FIGHTING(vict) && GET_POS(vict) == POS_STANDING) {
			if (GET_EQ(vict, WEAR_RANGED) && GET_OBJ_TYPE(GET_EQ(vict, WEAR_RANGED)) == ITEM_MISSILE_WEAPON)
				set_fighting(vict, ch, FMODE_MISSILE);
			else
				set_fighting(vict, ch, FMODE_WAITING);
		}
		command_lag(ch, WAIT_OTHER);
	}
	else {
		act("You can't shoot $N!", FALSE, ch, 0, vict, TO_CHAR);
	}
}


ACMD(do_stake) {
	char_data *victim;
	obj_data *stake;

	one_argument(argument, arg);

	if (!*arg)
		msg_to_char(ch, "%stake whom?\r\n", subcmd ? "Uns" : "S");
	else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if (IS_GOD(victim) || IS_IMMORTAL(victim))
		msg_to_char(ch, "You can't stake up a god!\r\n");
	else if (!subcmd && IS_INJURED(victim, INJ_STAKED))
		act("$E is already staked!", FALSE, ch, 0, victim, TO_CHAR);
	else if (subcmd && !IS_INJURED(victim, INJ_STAKED))
		act("$E isn't even staked!", FALSE, ch, 0, victim, TO_CHAR);
	else if (IS_INJURED(victim, INJ_STAKED)) {
		act("You unstake $N.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n unstakes you!", FALSE, ch, 0, victim, TO_VICT | TO_SLEEP);
		act("$n unstakes $N.", FALSE, ch, 0, victim, TO_NOTVICT);
		if (GET_HEALTH(victim) > 0)
			GET_POS(victim) = POS_RESTING;
		REMOVE_BIT(INJURY_FLAGS(victim), INJ_STAKED);
		obj_to_char((stake = read_object(o_STAKE, TRUE)), ch);
		scale_item_to_level(stake, 1);	// min scale
		load_otrigger(stake);
	}
	else if (!can_fight(ch, victim))
		act("You can't stake $M!", FALSE, ch, 0, victim, TO_CHAR);
	else if (!(stake = get_obj_in_list_num(o_STAKE, ch->carrying)))
		msg_to_char(ch, "You don't have a stake!\r\n");
	else if (GET_POS(victim) >= POS_SLEEPING) {
		msg_to_char(ch, "You can only stake people who have been incapacitated.\r\n");
	}
	else if (IS_DEAD(victim)) {
		msg_to_char(ch, "You can't stake someone who is already dead.\r\n");
	}
	else {
		command_lag(ch, WAIT_COMBAT_ABILITY);

		act("You jab $p through $N's heart!", FALSE, ch, stake, victim, TO_CHAR);
		act("$n jabs $p through your heart!", FALSE, ch, stake, victim, TO_VICT | TO_SLEEP);
		act("$n jabs $p through $N's heart!", FALSE, ch, stake, victim, TO_NOTVICT);
		
		SET_BIT(INJURY_FLAGS(victim), INJ_STAKED);
		if (GET_HEALTH(victim) <= 0) {
			GET_HEALTH(victim) = 0;
			GET_POS(victim) = POS_STUNNED;
		}
		extract_obj(stake);

		if (!IS_VAMPIRE(victim)) {
			if (!IS_NPC(victim)) {
				death_log(victim, ch, ATTACK_EXECUTE);
				add_lore(ch, LORE_PLAYER_KILL, "Killed %s in battle", PERS(victim, victim, TRUE));
				add_lore(victim, LORE_PLAYER_DEATH, "Slain by %s in battle", PERS(ch, ch, TRUE));
			}
			die(victim, ch);	// returns a corpse but we don't need it
		}
	}
}


ACMD(do_struggle) {
	if (!IS_INJURED(ch, INJ_TIED))
		msg_to_char(ch, "You aren't even bound!\r\n");
	else if (!number(0, MAX(1, GET_STRENGTH(ch)/2))) {
		msg_to_char(ch, "You struggle a bit, but fail to break free.\r\n");
		act("$n struggles a little with $s bindings!", TRUE, ch, 0, 0, TO_ROOM);
		GET_WAIT_STATE(ch) = 30 RL_SEC;
		}
	else {
		msg_to_char(ch, "You break free!\r\n");
		act("$n struggles with $s bindings and breaks free!", TRUE, ch, 0, 0, TO_ROOM);
		REMOVE_BIT(INJURY_FLAGS(ch), INJ_TIED);
	}
}


ACMD(do_summary) {
	char_data *iter;
	bool is_ally, is_enemy, found;
	
	found = FALSE;
	*buf = '\0';
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(ch)), iter, next_in_room) {
		is_ally = in_same_group(ch, iter) || is_fight_ally(ch, iter);
		is_enemy = is_fight_enemy(ch, iter);
		
		// any?
		found |= is_ally | is_enemy;

		if (is_ally) {
			sprintf(buf + strlen(buf), " %s:  %s ", (ch == iter) ? "You" : "Ally", PERS(iter, ch, TRUE));
			if (IS_DEAD(iter)) {
				sprintf(buf + strlen(buf), " &rDEAD&0");
			}
			else {
				sprintf(buf + strlen(buf), " %s%d&0/%d&gh&0", prompt_color_by_prc(GET_HEALTH(iter), GET_MAX_HEALTH(iter)), GET_HEALTH(iter), GET_MAX_HEALTH(iter));
				sprintf(buf + strlen(buf), " %s%d&0/%d&yv&0", prompt_color_by_prc(GET_MOVE(iter), GET_MAX_MOVE(iter)), GET_MOVE(iter), GET_MAX_MOVE(iter));
				sprintf(buf + strlen(buf), " %s%d&0/%d&cm&0", prompt_color_by_prc(GET_MANA(iter), GET_MAX_MANA(iter)), GET_MANA(iter), GET_MAX_MANA(iter));
			}
			
			if (IS_VAMPIRE(iter)) {
				sprintf(buf + strlen(buf), " %s%d&0/%d&rb&0", prompt_color_by_prc(GET_BLOOD(iter), GET_MAX_BLOOD(iter)), GET_BLOOD(iter), GET_MAX_BLOOD(iter));
			}

			if (FIGHTING(iter)) {
				sprintf(buf + strlen(buf), "  vs %s\r\n", PERS(FIGHTING(iter), ch, FALSE));
			}
			else {
				strcat(buf, "\r\n");
			}
		}
		else if (is_enemy) {
			sprintf(buf + strlen(buf), " Enemy:  %s ", PERS(iter, ch, TRUE));
			if (IS_DEAD(iter)) {
				sprintf(buf + strlen(buf), " &rDEAD&0");
			}
			else {
				sprintf(buf + strlen(buf), " %s%d%%&0h", prompt_color_by_prc(GET_HEALTH(iter), GET_MAX_HEALTH(iter)), ((int) GET_HEALTH(iter) * 100 / MAX(1, GET_MAX_HEALTH(iter))));
				sprintf(buf + strlen(buf), " %s%d%%&0v", prompt_color_by_prc(GET_MOVE(iter), GET_MAX_MOVE(iter)), ((int) GET_MOVE(iter) * 100 / MAX(1, GET_MAX_MOVE(iter))));
				sprintf(buf + strlen(buf), " %s%d%%&0m", prompt_color_by_prc(GET_MANA(iter), GET_MAX_MANA(iter)), ((int) GET_MANA(iter) * 100 / MAX(1, GET_MAX_MANA(iter))));
			}

			// no vampire stats on enemy

			if (FIGHTING(iter)) {
				sprintf(buf + strlen(buf), "  vs %s\r\n", PERS(FIGHTING(iter), ch, FALSE));
			}
			else {
				strcat(buf, "\r\n");
			}
		}
	}
	
	if (found) {
		send_to_char("Combat summary:\r\n", ch);
		send_to_char(buf, ch);
	}
	else {
		msg_to_char(ch, "You can't get a combat summary right now.\r\n");
	}
}


// do_untie -- search hint
ACMD(do_tie) {
	bool kept = FALSE;
	char_data *victim;
	obj_data *rope;

	/* subcmd 0 = tie, 1 = untie */

	one_argument(argument, arg);

	if (!*arg)
		msg_to_char(ch, "%sie whom?\r\n", subcmd ? "Unt" : "T");
	else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_config_msg(ch, "no_person");
	else if (IS_DEAD(victim)) {
		msg_to_char(ch, "You can't do that to someone who is already dead.\r\n");
	}
	else if (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_ANIMAL))
		perform_npc_tie(ch, victim, subcmd);
	else if (IS_GOD(victim) || IS_IMMORTAL(victim))
		msg_to_char(ch, "You can't tie up a god!\r\n");
	else if (!subcmd && IS_INJURED(victim, INJ_TIED))
		act("$E is already tied!", FALSE, ch, 0, victim, TO_CHAR);
	else if (subcmd && !IS_INJURED(victim, INJ_TIED))
		act("$E isn't even tied up!", FALSE, ch, 0, victim, TO_CHAR);
	else if (IS_INJURED(victim, INJ_TIED)) {
		act("You unbind $N.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n unbinds you!", FALSE, ch, 0, victim, TO_VICT | TO_SLEEP);
		act("$n unbinds $N.", FALSE, ch, 0, victim, TO_NOTVICT);
		GET_HEALTH(victim) = MAX(1, GET_HEALTH(victim));
		GET_POS(victim) = POS_RESTING;
		REMOVE_BIT(INJURY_FLAGS(victim), INJ_TIED);
		
		if (GET_ROPE_VNUM(victim) != NOTHING && (rope = read_object(GET_ROPE_VNUM(victim), TRUE))) {
			obj_to_char(rope, ch);
			scale_item_to_level(rope, 1);	// minimum
			load_otrigger(rope);
			act("You receive $p.", FALSE, ch, rope, NULL, TO_CHAR);
		}
		GET_ROPE_VNUM(victim) = NOTHING;
		request_char_save_in_world(victim);
	}
	else if (GET_POS(victim) >= POS_SLEEPING)
		act("You need to knock $M out first.", FALSE, ch, 0, victim, TO_CHAR);
	else if (!(rope = get_component_in_list(COMP_ROPE, ch->carrying, &kept))) {
		msg_to_char(ch, "You don't seem to have any rope%s.\r\n", kept ? " that isn't marked 'keep'" : "");
	}
	else {
		act("You bind and gag $N!", FALSE, ch, 0, victim, TO_CHAR);
		act("$n binds and gags you!", FALSE, ch, 0, victim, TO_VICT | TO_SLEEP);
		act("$n binds and gags $N!", FALSE, ch, 0, victim, TO_NOTVICT);
		SET_BIT(INJURY_FLAGS(victim), INJ_TIED);
		if (GET_HEALTH(victim) <= 1) {
			GET_HEALTH(victim) = 1;
			GET_POS(victim) = POS_RESTING;
		}
		GET_ROPE_VNUM(victim) = GET_OBJ_VNUM(rope);
		extract_obj(rope);
		request_char_save_in_world(victim);
	}
}


ACMD(do_throw) {
	char buf[MAX_STRING_LENGTH];
	int dir = NO_DIR;
	char_data *vict;
	obj_data *obj = NULL;
	room_data *to_room = NULL;
	struct room_direction_data *ex;

	two_arguments(argument, arg, buf);

	if (!*arg || !*buf)
		msg_to_char(ch, "What would you like to throw, and which direction?\r\n");
	else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
		msg_to_char(ch, "You don't have anything like that.\r\n");
	else if ((dir = parse_direction(ch, buf)) == NO_DIR)
		msg_to_char(ch, "Which way did you want to throw it?\r\n");
	else if (ROOM_IS_CLOSED(IN_ROOM(ch))) {
		if (!(ex = find_exit(IN_ROOM(ch), dir)) || !ex->room_ptr)
			msg_to_char(ch, "You can't throw it that way.\r\n");
		else if (EXIT_FLAGGED(ex, EX_CLOSED))
			msg_to_char(ch, "You can't throw it through a closed door!\r\n");
		else
			to_room = ex->room_ptr;
	}
	else {
		if (dir >= NUM_2D_DIRS || !(to_room = real_shift(IN_ROOM(ch), shift_dir[dir][0], shift_dir[dir][1]))) {
			msg_to_char(ch, "You can't throw it that direction.\r\n");
		}
		if (to_room && ROOM_IS_CLOSED(to_room)) {
			if (BUILDING_ENTRANCE(to_room) != dir && (!ROOM_BLD_FLAGGED(to_room, BLD_TWO_ENTRANCES) || BUILDING_ENTRANCE(to_room) != rev_dir[dir])) {
				msg_to_char(ch, "You can only throw it through the entrance.\r\n");
				to_room = NULL;
			}
		}
	}
	
	// safety
	if (!to_room) {
		return;
	}
	
	if (OBJ_BOUND_TO(obj) && ROOM_OWNER(to_room) && ROOM_OWNER(to_room) != GET_LOYALTY(ch)) {
		msg_to_char(ch, "You can't throw bound items there.\r\n");
		return;
	}
	if (GET_OBJ_REQUIRES_QUEST(obj) != NOTHING && !IS_NPC(ch) && !IS_IMMORTAL(ch)) {
		msg_to_char(ch, "You can't throw quest items.\r\n");
		return;
	}
	if (ROOM_BLD_FLAGGED(to_room, BLD_ITEM_LIMIT)) {
		int size = obj_carry_size(obj);
		if ((size + count_objs_in_room(to_room)) > config_get_int("room_item_limit")) {
			msg_to_char(ch, "You can't throw any more items there.\r\n");
			return;
		}
	}

	/* If we came up with a room, lets throw! */

	sprintf(buf, "You throw $p %s as hard as you can!", dirs[get_direction_for_char(ch, dir)]);
	act(buf, FALSE, ch, obj, 0, TO_CHAR);
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(ch)), vict, next_in_room) {
		if (vict != ch && vict->desc) {
			sprintf(buf1, "$n throws $p %s as hard as $e can!", dirs[get_direction_for_char(vict, dir)]);
			act(buf1, TRUE, ch, obj, vict, TO_VICT);
		}
	}

	obj_to_room(obj, to_room);
	
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(obj)), vict, next_in_room) {
		if (vict->desc) {
			sprintf(buf, "$p is hurled in from the %s and falls to the ground at your feet!", dirs[get_direction_for_char(vict, rev_dir[dir])]);
			act(buf, FALSE, vict, obj, 0, TO_CHAR);
		}
	}
	
	// throwing item abuse log
	if (IS_IMMORTAL(ch)) {
		strcpy(buf, room_log_identifier(to_room));	// store one in a buf because it can't show 2 different locations in 1 line
		syslog(SYS_GC, GET_ACCESS_LEVEL(ch), TRUE, "ABUSE: %s threw %s from %s to %s", GET_NAME(ch), GET_OBJ_SHORT_DESC(obj), room_log_identifier(IN_ROOM(ch)), buf);
	}
	
	command_lag(ch, WAIT_COMBAT_ABILITY);
}
