/* ************************************************************************
*   File: act.battle.c                                    EmpireMUD 2.0b5 *
*  Usage: commands and functions related to the Battle skill              *
*                                                                         *
*  EmpireMUD code base by Paul Clarke, (C) 2000-2024                      *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  EmpireMUD based upon CircleMUD 3.0, bpl 17, by Jeremy Elson.           *
*  CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include <math.h>

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
*   Battle Helpers
*   Battle Commands
*/

 //////////////////////////////////////////////////////////////////////////////
//// BATTLE HELPERS //////////////////////////////////////////////////////////

/**
* Performs a rescue and ensures everyone is fighting the right thing.
*
* @param char_data *ch The person who is rescuing...
* @param char_data *vict The person in need of rescue.
* @param char_data *from The attacker, who will now hit ch.
* @param int msg Which RESCUE_ message type to send.
*/
void perform_rescue(char_data *ch, char_data *vict, char_data *from, int msg) {
	switch (msg) {
		case RESCUE_RESCUE: {
			send_to_char("Banzai! To the rescue...\r\n", ch);
			act("You are rescued by $N!", FALSE, vict, 0, ch, TO_CHAR);
			act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);
			break;
		}
		case RESCUE_FOCUS: {
			act("$N changes $S focus to you!", FALSE, ch, NULL, from, TO_CHAR);
			act("You change your focus to $n!", FALSE, ch, NULL, from, TO_VICT);
			act("$N changes $S focus to $n!", FALSE, ch, NULL, from, TO_NOTVICT);
			break;
		}
		default: {	// and RESCUE_NO_MSG
			// no message
			break;
		}
	}
	
	// switch ch to fighting from
	if (FIGHTING(ch) && FIGHTING(ch) != from) {
		FIGHTING(ch) = from;
		if (FIGHT_MODE(from) != FMODE_MELEE && FIGHT_MODE(ch) == FMODE_MELEE) {
			FIGHT_MODE(ch) = FMODE_MISSILE;
		}
	}
	else if (!FIGHTING(ch)) {
		set_fighting(ch, from, (FIGHTING(from) && FIGHT_MODE(from) != FMODE_MELEE) ? FMODE_MISSILE : FMODE_MELEE);
	}
	
	// switch from to fighting ch
	if (FIGHTING(from) && FIGHTING(from) != ch) {
		FIGHTING(from) = ch;
		if (FIGHT_MODE(ch) != FMODE_MELEE && FIGHT_MODE(from) == FMODE_MELEE) {
			FIGHT_MODE(from) = FMODE_MISSILE;
		}
	}
	else if (!FIGHTING(from)) {
		set_fighting(from, ch, (FIGHTING(ch) && FIGHT_MODE(ch) != FMODE_MELEE) ? FMODE_MISSILE : FMODE_MELEE);
	}
	
	// cancel vict's fight
	if (FIGHTING(vict) == from) {
		stop_fighting(vict);
	}
}


 //////////////////////////////////////////////////////////////////////////////
//// BATTLE COMMANDS /////////////////////////////////////////////////////////

ACMD(do_charge) {
	struct affected_type *af;
	int res, cost = 50;
	char_data *vict;
	
	one_argument(argument, arg);

	if (!can_use_ability(ch, ABIL_CHARGE, MOVE, cost, COOLDOWN_CHARGE)) {
		// nope
	}
	else if (*arg && !(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
		send_config_msg(ch, "no_person");
	}
	else if (!*arg && !(vict = FIGHTING(ch))) {
		msg_to_char(ch, "You aren't fighting anybody.\r\n");
	}
	else if (FIGHTING(ch) == vict && FIGHT_MODE(ch) == FMODE_MELEE) {
		msg_to_char(ch, "You're already in melee range.\r\n");
	}
	else if (FIGHTING(ch) && !NOT_MELEE_RANGE(ch, vict)) {
		msg_to_char(ch, "You're already in melee range.\r\n");
	}
	else if (AFF_FLAGGED(ch, AFF_STUNNED | AFF_HARD_STUNNED | AFF_IMMOBILIZED)) {
		msg_to_char(ch, "You can't charge right now!\r\n");
	}
	else if (!can_fight(ch, vict)) {
		act("You can't attack $N!", FALSE, ch, NULL, vict, TO_CHAR);
	}
	else {	// ok
		if (SHOULD_APPEAR(ch)) {
			appear(ch);
		}
		
		// 'charge' ability cost :D
		charge_ability_cost(ch, MOVE, cost, COOLDOWN_CHARGE, 3 * SECS_PER_REAL_MIN, WAIT_COMBAT_ABILITY);
		act("You charge at $N!", FALSE, ch, NULL, vict, TO_CHAR | ACT_ABILITY);
		act("$n charges at you!", FALSE, ch, NULL, vict, TO_VICT | ACT_ABILITY);
		act("$n charges at $N!", FALSE, ch, NULL, vict, TO_NOTVICT | ACT_ABILITY);
		
		// apply temporary hit/damage boosts
		af = create_mod_aff(ATYPE_CHARGE, 5, APPLY_TO_HIT, 100, ch);
		affect_join(ch, af, 0);
		af = create_mod_aff(ATYPE_CHARGE, 5, APPLY_BONUS_PHYSICAL, GET_STRENGTH(ch), ch);
		affect_join(ch, af, 0);
		af = create_mod_aff(ATYPE_CHARGE, 5, APPLY_BONUS_MAGICAL, GET_INTELLIGENCE(ch), ch);
		affect_join(ch, af, 0);
		
		res = hit(ch, vict, GET_EQ(ch, WEAR_WIELD), TRUE);
		
		if (res >= 0 && FIGHTING(ch) && FIGHTING(ch) != vict) {
			// ensure ch is hitting the right person
			FIGHTING(ch) = vict;
		}
		if (FIGHTING(ch) == vict) {	// ensure melee
			FIGHT_MODE(ch) = FMODE_MELEE;
			FIGHT_WAIT(ch) = 0;
		}
		if (FIGHTING(vict) == ch) {	// reciprocate melee
			FIGHT_MODE(vict) = FMODE_MELEE;
			FIGHT_WAIT(vict) = 0;
		}
		
		if (can_gain_exp_from(ch, vict)) {
			gain_ability_exp(ch, ABIL_CHARGE, 15);
		}
		run_ability_hooks(ch, AHOOK_ABILITY, ABIL_CHARGE, 0, vict, NULL, NULL, NULL, NOBITS);
	}
}


ACMD(do_kite) {
	int kitable = 0, cost = 50;
	char_data *vict;
	
	if (!can_use_ability(ch, ABIL_KITE, MOVE, cost, COOLDOWN_KITE)) {
		return;
	}
	if (!FIGHTING(ch)) {
		msg_to_char(ch, "You're not even in combat!\r\n");
		return;
	}
	
	// look for people hitting me
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(ch)), vict, next_in_room) {
		if (vict == ch || FIGHTING(vict) != ch) {
			continue;	// not hitting me
		}
		if (AFF_FLAGGED(vict, AFF_STUNNED | AFF_HARD_STUNNED | AFF_IMMOBILIZED)) {
			++kitable;	// can kite
			continue;
		}
		if (FIGHT_MODE(vict) != FMODE_MELEE) {
			++kitable;	// can kite
			continue;
		}
		
		// seems to be hitting me
		act("You can't kite right now because $N is attacking you.", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}
	
	// seems ok... check triggers
	if (ABILITY_TRIGGERS(ch, NULL, NULL, ABIL_KITE)) {
		return;
	}
	
	// ok!
	charge_ability_cost(ch, MOVE, cost, COOLDOWN_KITE, 30, WAIT_COMBAT_ABILITY);
	act("You move back and attempt to kite...", FALSE, ch, NULL, NULL, TO_CHAR);
	act("$n moves back and attempts to kite...", FALSE, ch, NULL, NULL, TO_ROOM);
	
	// move ch out
	FIGHT_MODE(ch) = FMODE_MISSILE;
	FIGHT_WAIT(ch) = 0;
	
	// move people hitting me out
	DL_FOREACH2(ROOM_PEOPLE(IN_ROOM(ch)), vict, next_in_room) {
		if (vict == ch || FIGHTING(vict) != ch) {
			continue;	// not hitting me
		}
		
		// ok:
		if (FIGHT_MODE(vict) == FMODE_WAITING) {
			FIGHT_WAIT(vict) += 4;
		}
		else if (GET_EQ(vict, WEAR_RANGED) && GET_OBJ_TYPE(GET_EQ(vict, WEAR_RANGED)) == ITEM_MISSILE_WEAPON) {
			FIGHT_MODE(vict) = FMODE_MISSILE;
		}
		else {
			FIGHT_MODE(vict) = FMODE_WAITING;
			FIGHT_WAIT(vict) = 4;
		}
		act("You successfully kite $N!", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n successfully kites you!", FALSE, ch, NULL, vict, TO_VICT);
		run_ability_hooks(ch, AHOOK_ABILITY, ABIL_KITE, 0, vict, NULL, NULL, NULL, NOBITS);
	}
	
	if (kitable > 0) {
		gain_ability_exp(ch, ABIL_KITE, 15);
	}
}
