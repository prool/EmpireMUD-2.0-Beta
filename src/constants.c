/* ************************************************************************
*   File: constants.c                                     EmpireMUD 2.0b5 *
*  Usage: Numeric and string contants used by the MUD                     *
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
#include "skills.h"
#include "interpreter.h"	/* alias_data */
#include "olc.h"
#include "constants.h"

/**
* Contents:
*   EmpireMUD Constants
*   Ability Constants
*   Adventure Constants
*   Archetype Constants
*   Attack Message Constants
*   Augment Constants
*   Class Constants
*   Player Constants
*   Direction And Room Constants
*   Character Constants
*   Craft Recipe Constants
*   Empire Constants
*   Event Constants
*   Faction Constants
*   Generic Constants
*   Mob Constants
*   Moon Constants
*   Item Contants
*   OLC Constants
*   Progress Constants
*   Quest Constants
*   Room/World Constants
*   Shop Constants
*   Skill Constants
*   Social Constants
*   Trigger Constants
*   Misc Constants
*/

// external funcs
void afk_notify(char_data *ch);
void tog_informative(char_data *ch);
void tog_mapcolor(char_data *ch);
void tog_political(char_data *ch);
void tog_pvp(char_data *ch);


 //////////////////////////////////////////////////////////////////////////////
//// EMPIREMUD CONSTANTS /////////////////////////////////////////////////////

// Shown on the "version" command and sent over MSSP
const char *version = "EmpireMUD 2.0 beta 5.179";
const char *DG_SCRIPT_VERSION = "DG Scripts 1.0.12 e5.1.3";


// data for the built-in game levels -- this adapts itself if you reduce the number of immortal levels
const char *level_names[][2] = {
		{ "N/A", "Not Started" },
		{ "MORT", "Mortal" },
		{ "GOD", "God" },
	#if (LVL_START_IMM < LVL_TOP)
		{ "IMM", "Immortal" },
		#if (LVL_ASST < LVL_TOP && LVL_ASST > LVL_START_IMM)
			{ "ASST", "Assistant" },
			#if (LVL_CIMPL < LVL_TOP && LVL_CIMPL > LVL_ASST)
				{ "CIMPL", "Co-Implementor" },
			#endif
		#endif
	#endif
		{ "IMPL", "Implementor" }
};


// reboot messages information
// TODO could auto-detect number of strings (consider adding a count_strings function)
// TODO could move to a file
const int num_of_reboot_strings = 3;
const char *reboot_strings[] = {
	"   EmpireMUD is performing a reboot. This process generally takes one to\r\n"
	"two minutes and will not disconnect you. Most character actions are not\r\n"
	"affected, although fighting will stop.\r\n",

	"Q. What is a reboot?\r\n"
	"A. A reboot allows the mud to reload code without disconnecting players.\r\n"
	"This means that you won't have to go through the hassle of logging in again.\r\n",

	"Q. What if I lose something in the reboot?\r\n"
	"A. Nothing is lost because your character and equipment are saved when the\r\n"
	"reboot happens. The world is also saved so if you were building or chopping\r\n"
	"you won't have to start over.\r\n",

	"\n"
};


// REBOOT_x: reboot modes
const char *reboot_types[] = {
	"none",
	"reboot",
	"shutdown",
	"\n"
};


// SHUTDOWN_x
const char *shutdown_types[] = {
	"normal",
	"pause",
	"die",
	"complete",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// ABILITY CONSTANTS ///////////////////////////////////////////////////////

// ABIL_ACTION_x: ability actions
const char *ability_actions[] = {
	"detect hide",	// 0
	"detect earthmeld",
	"detect players around",
	"detect adventures around",
	"devastate area",
	"magic growth",	// 5
	"close portal",
	"apply poison",
	"remove physical DoTs",
	"remove magical DoTs",
	"remove fire DoTs",	// 10
	"remove poison DoTs",
	"remove all DoTs",
	"remove debuffs",
	"remove drunk",
	"taunt",	// 15
	"rescue one",
	"rescue all",
	"hide",
	"put to sleep",
	"disenchant obj",	// 20
	"purify",
	"close to melee",
	"set fighting target",
	"push back to ranged combat",
	"look at room",		// 25
	"\n"
};


// ABILF_x (1/2): ability flags
const char *ability_flags[] = {
	"VIOLENT",	// 0
	"COUNTERSPELLABLE",
	"TOGGLE",
	"INVISIBLE",
	"!ENGAGE",
	"RANGED",	// 5
	"!ANIMAL",
	"!INVULNERABLE",
	"CASTER-ROLE",
	"HEALER-ROLE",
	"MELEE-ROLE",	// 10
	"TANK-ROLE",
	"RANGED-ONLY",
	"IGNORE-SUN",
	"UNSCALED-BUFF",
	"LIMIT-CROWD-CONTROL",	// 15
	"NOT-IN-COMBAT",
	"ONE-AT-A-TIME",
	"OVER-TIME",
	"SPOKEN",
	"REPEAT-OVER-TIME",	// 20
	"CUMULATIVE-BUFF",
	"CUMULATIVE-DURATION",
	"WEAPON-HIT",
	"DIFFICULT-ANYWAY",
	"NOT-IN-DARK",	// 25
	"UNSCALED-PENALTY",
	"STOP-ON-MISS",
	"REDUCED-ON-EXTRA-TARGETS",
	"USE-SKILL-BELOW-MAX",
	"UNREMOVABLE-BUFF",	// 30
	"BUFF-SELF-NOT-TARGET",
	"STAY-HIDDEN",
	"BUFFS-COMMAND",
	"\n"
};


// ABILF_x (2/2): ability flags shown to players as notes
const char *ability_flag_notes[] = {
	"violent",	// 0
	"counterspellable",
	"toggles on/off",
	"",	// INVISIBLE
	"",	// !ENGAGE
	"ranged",	// 5
	"can't be used in animal form",
	"can't be used while invulnerable",
	"stronger in caster role",
	"stronger in healer role",
	"stronger in melee role",	// 10
	"stronger in tank role",
	"can only be used at range",
	"unaffected by sun",
	"",	// UNSCALED-BUFF
	"limited crowd control",	// 15
	"",	// NOT-IN-COMBAT
	"",	// ONE-AT-A-TIME
	"long action",
	"spoken aloud",
	"auto-repeating",	// 20
	"cumulative",
	"extendable",
	"hits with weapon",
	"",	// ABILF_DIFFICULT_ANYWAY
	"",	// 25 - ABILF_NOT_IN_DARK
	"",	// ABILF_UNSCALED_PENALTY
	"",	// ABILF_STOP_ON_MISS
	"reduced on extra targets",
	"uses skill level if below max",
	"unremovable",	// 30
	"buffs self not target",
	"won't cancel hide",
	"",	// ABILF_BUFFS_COMMAND
	"\n"
};


// ABILT_x (1/2): ability type flags
const char *ability_type_flags[] = {
	"CRAFT",	// 0
	"BUFF",
	"DAMAGE",
	"DOT",
	"PTECH",
	"PASSIVE-BUFF",	// 5
	"READY-WEAPONS",
	"COMPANION",
	"SUMMON-ANY",
	"SUMMON-RANDOM",
	"MORPH",	// 10
	"AUGMENT",
	"CUSTOM",
	"CONJURE-OBJECT",
	"CONJURE-LIQUID",
	"CONJURE-VEHICLE",	// 15
	"ROOM-AFFECT",
	"PAINT-BUILDING",
	"ACTION",
	"BUILDING-DAMAGE",
	"TELEPORT",		// 20
	"RESURRECT",
	"RESOURCE",
	"ATTACK",
	"RESTORE",
	"MASTERY",	// 25
	"LINK",
	"MOVE",
	"\n"
};


// ABILT_x (2/2): ability types as shown to players
const char *ability_type_notes[] = {
	"crafting",	// 0
	"buff",
	"damage",
	"DoT",
	"player tech",
	"passive buff",	// 5
	"ready weapon",
	"companion",
	"summon",
	"summon",
	"morphing",	// 10
	"augment",
	"custom",
	"conjure",	// actually conjure object
	"conjure liquid",
	"conjure",	// 15, actually conjure vehicle (shown to players as conjure as it can conjure furniture and buildings too)
	"room affect",
	"paint",
	"action",
	"building damage",
	"teleportation",	// 20
	"resurrection",
	"resource gathering",
	"attack",
	"restoration",
	"mastery",	// 25
	"links abilities",
	"movement",
	"\n"
};


// ATAR_x: ability targeting flags
const char *ability_target_flags[] = {
	"IGNORE",	// 0
	"CHAR-ROOM",
	"CHAR-WORLD",
	"CHAR-CLOSEST",
	"FIGHT-SELF",
	"FIGHT-VICTIM",	// 5
	"SELF-ONLY",
	"NOT-SELF",
	"OBJ-INV",
	"OBJ-ROOM",
	"OBJ-WORLD",	// 10
	"OBJ-EQUIP",
	"VEH-ROOM",
	"VEH-WORLD",
	"ROOM-HERE",
	"ROOM-ADJACENT",	// 15
	"ROOM-EXIT",
	"ROOM-HOME",
	"ROOM-RANDOM",
	"ROOM-CITY",
	"ROOM-COORDS",	// 20
	"ROOM-NOT-HERE",
	"STRING",
	"ALLIES-MULTI",
	"GROUP-MULTI",
	"ANY-MULTI",	// 25
	"ENEMIES-MULTI",
	"MULTI-CAN-SEE",
	"NOT-ALLY",
	"NOT-ENEMY",
	"DEAD-OK",	// 30
	"ROOM-RANDOM-CAN-USE",
	"\n"
};


// ABIL_CUSTOM_x (1/2): custom types for abilities
const char *ability_custom_types[] = {
	"self-to-char",	// 0
	"self-to-room",
	"targ-to-char",
	"targ-to-vict",
	"targ-to-room",
	"counter-to-char",	// 5
	"counter-to-vict",
	"counter-to-room",
	"fail-self-to-char",
	"fail-self-to-room",
	"fail-targ-to-char",	// 10
	"fail-targ-to-vict",
	"fail-targ-to-room",
	"pre-self-to-char",
	"pre-self-to-room",
	"pre-targ-to-char",	// 15
	"pre-targ-to-vict",
	"pre-targ-to-room",
	"per-veh-to-char",
	"per-veh-to-room",
	"per-item-to-char",	// 20
	"per-item-to-room",
	"over-time-longdesc",
	"toggle-to-char",
	"toggle-to-room",
	"per-char-to-char",	// 25
	"per-char-to-vict",
	"per-char-to-room",
	"spec-to-char",
	"spec-to-vict",
	"spec-to-room",	// 30
	"no-argument",
	"ot-self-to-char",
	"ot-self-to-room",
	"ot-targ-to-char",
	"ot-targ-to-vict",	// 35
	"ot-targ-to-room",
	"immune-self-to-char",
	"immune-targ-to-char",
	"no-target",
	"self-one-at-a-time",	// 40
	"targ-one-at-a-time",
	"\n"
};


// ABIL_CUSTOM_x (2/2): how they're shown in olc
const char *ability_custom_type_help =
"Types of messages:\r\n"
"&w Ability that targets yourself or no one: self-to-char self-to-room&0\r\n"
" Targets a character: targ-to-char targ-to-vict targ-to-room\r\n"
"&w Shown before activation, targeting self: pre-self-to-char pre-self-to-room&0\r\n"
" Shown before activation, targeting character: pre-targ-to-char pre-targ-to-vict pre-targ-to-room\r\n"
"&w Fails when targeting self or no one: fail-self-to-char fail-self-to-room&0\r\n"
" Fails targeting a character: fail-targ-to-char fail-targ-to-vict fail-targ-to-room\r\n"
"&w Is counterspelled: counter-to-char counter-to-vict counter-to-room&0\r\n"
" For each vehicle on some types: per-veh-to-char per-veh-to-room\r\n"
"&w For each item on some types: per-item-to-char per-item-to-room&0\r\n"
" For each target/creature for some abilities: per-char-to-char per-char-to-vict per-char-to-room\r\n"
"&w Special messaging for some abilities: spec-to-char spec-to-vict spec-to-room&0\r\n"
" When turning off a 'toggle' ability: toggle-to-char toggle-to-room\r\n"
"&w Long description when performing an over-tiem ability: over-time-longdesc&0\r\n"
" Over time messages with no target: ot-self-to-char ot-self-to-room\r\n"
"&w Over time messsages with target: ot-targ-to-char ot-targ-to-vict ot-targ-to-room&0\r\n"
" Message shown to the user when the target is immune: immune-self-to-char, immune-targ-to-char\r\n"
"&w Shown when one-at-a-time flag prevents the ability: self-one-at-a-time, targ-one-at-a-time&0\r\n"
" Other messages: no-argument, no-target\r\n";



// ADL_x: for adding to ability_data_list
const char *ability_data_types[] = {
	"PTECH",	// 0
	"EFFECT",
	"READY-WEAPON",
	"SUMMON-MOB",
	"LIMITATION",
	"PAINT-COLOR",	// 5
	"ACTION",
	"RANGE",
	"PARENT",
	"SUPERCEDED-BY",
	"\n"
};


// ABIL_EFFECT_x: things that happen when an ability is used
const char *ability_effects[] = {
	"dismount",	// 0
	"distrust from hostile",
	"\n"
};


// AGH_x: ability gain hooks
const char *ability_gain_hooks[] = {
	"ONLY-WHEN-AFFECTED",	// 0
	"MELEE",
	"RANGED",
	"DODGE",
	"BLOCK",
	"TAKE-DAMAGE",	// 5
	"PASSIVE-FREQUENT",
	"PASSIVE-HOURLY",
	"ONLY-DARK",
	"ONLY-LIGHT",
	"ONLY-VS-ANIMAL",	// 10
	"VAMPIRE-FEEDING",
	"MOVING",
	"ONLY-USING-READY-WEAPON",
	"ONLY-USING-COMPANIONS",
	"NOT-WHILE-ASLEEP",	// 15
	"DYING",
	"DO-HEAL",
	"ONLY-INDOORS",
	"ONLY-OUTDOORS",
	"\n"
};


// AHOOK_x: Ability hooks (things that cause an ability to run itself)
const char *ability_hook_types[] = {
	"Ability",	// 0
	"Attack",
	"Attack-Type",
	"Damage-Type",
	"Kill",
	"Melee-Attack",	// 5
	"Ranged-Attack",
	"Weapon-Type",
	"Dying",
	"Respawn",
	"Resurrect",	// 10
	"Damage-Any",
	"Attack-Mage",
	"Attack-Vampire",
	"\n"
};


// ABIL_MOVE_x: Move ability types
const char *ability_move_types[] = {
	"normal",	// 0
	"earthmeld",
	"\n"
};


// ABIL_LIMIT_x (1/2): Limitations when trying to use an ability
const char *ability_limitations[] = {
	"on a barrier tile", 	// 0
	"own the tile",
	"can use as guest",
	"can use as ally",
	"can use as member",
	"on a road",		// 5
	"paintable building",
	"in a city",
	"have an empire",
	"indoors",
	"outdoors",		// 10
	"on the map",
	"terraform approval",
	"valid siege target",
	"not distracted",
	"not immobilized",	// 15
	"can teleport here",
	"within range",
	"not god target",
	"guest permission at target",
	"ally permission at target",	// 20
	"member permission at target",
	"can teleport target",
	"target not foreign empire npc",
	"not here",
	"check city found time",	// 25
	"item type",
	"wield any weapon",
	"wield attack type",
	"wield weapon type",
	"not being attacked",	// 30
	"disarmable target",
	"target has mana",
	"using any poison",
	"target has DoT type",
	"target has any DoT",	// 35
	"target being attacked",
	"in role",
	"no witnesses",
	"no witnesses hide",
	"check obj binding",	// 40
	"obj flagged",
	"obj not flagged",
	"target is vampire",
	"can purify target",
	"in combat",	// 45
	"not being attacked melee",
	"not being attacked mobile melee",
	"not affected by",
	"target not affeced by",
	"not leading mob",	// 50
	"not leading vehicle",
	"is affected by",
	"target is affected by",
	"target is human",
	"\n"
};


// ABIL_LIMIT_x (2/2): Type needed for "misc" data
const int ability_limitation_misc[] = {
	ABLIM_NOTHING,	// ABIL_LIMIT_ON_BARRIER
	ABLIM_NOTHING,	// ABIL_LIMIT_OWN_TILE
	ABLIM_NOTHING,	// ABIL_LIMIT_CAN_USE_GUEST
	ABLIM_NOTHING,	// ABIL_LIMIT_CAN_USE_ALLY
	ABLIM_NOTHING,	// ABIL_LIMIT_CAN_USE_MEMBER
	ABLIM_NOTHING,	// ABIL_LIMIT_ON_ROAD
	ABLIM_NOTHING,	// ABIL_LIMIT_PAINTABLE_BUILDING
	ABLIM_NOTHING,	// ABIL_LIMIT_IN_CITY
	ABLIM_NOTHING,	// ABIL_LIMIT_HAVE_EMPIRE
	ABLIM_NOTHING,	// ABIL_LIMIT_INDOORS
	ABLIM_NOTHING,	// ABIL_LIMIT_OUTDOORS
	ABLIM_NOTHING,	// ABIL_LIMIT_ON_MAP
	ABLIM_NOTHING,	// ABIL_LIMIT_TERRAFORM_APPROVAL
	ABLIM_NOTHING,	// ABIL_LIMIT_VALID_SIEGE_TARGET
	ABLIM_NOTHING,	// ABIL_LIMIT_NOT_DISTRACTED
	ABLIM_NOTHING,	// ABIL_LIMIT_NOT_IMMOBILIZED
	ABLIM_NOTHING,	// ABIL_LIMIT_CAN_TELEPORT_HERE
	ABLIM_NOTHING,	// ABIL_LIMIT_WITHIN_RANGE
	ABLIM_NOTHING,	// ABIL_LIMIT_NOT_GOD_TARGET
	ABLIM_NOTHING,	// ABIL_LIMIT_GUEST_PERMISSION_AT_TARGET
	ABLIM_NOTHING,	// ABIL_LIMIT_ALLY_PERMISSION_AT_TARGET
	ABLIM_NOTHING,	// ABIL_LIMIT_MEMBER_PERMISSION_AT_TARGET
	ABLIM_NOTHING,	// ABIL_LIMIT_CAN_TELEPORT_TARGET
	ABLIM_NOTHING,	// ABIL_LIMIT_TARGET_NOT_FOREIGN_EMPIRE_NPC
	ABLIM_NOTHING,	// ABIL_LIMIT_NOT_HERE
	ABLIM_NOTHING,	// ABIL_LIMIT_CHECK_CITY_FOUND_TIME
	ABLIM_ITEM_TYPE,	// ABIL_LIMIT_ITEM_TYPE
	ABLIM_NOTHING,	// ABIL_LIMIT_WIELD_ANY_WEAPON
	ABLIM_ATTACK_TYPE,	// ABIL_LIMIT_WIELD_ATTACK_TYPE
	ABLIM_WEAPON_TYPE,	// ABIL_LIMIT_WIELD_WEAPON_TYPE
	ABLIM_NOTHING,	// ABIL_LIMIT_NOT_BEING_ATTACKED
	ABLIM_NOTHING,	// ABIL_LIMIT_DISARMABLE_TARGET
	ABLIM_NOTHING,	// ABIL_LIMIT_TARGET_HAS_MANA
	ABLIM_NOTHING,	// ABIL_LIMIT_USING_ANY_POISON
	ABLIM_DAMAGE_TYPE,	// ABIL_LIMIT_TARGET_HAS_DOT_TYPE
	ABLIM_NOTHING,	// ABIL_LIMIT_TARGET_HAS_ANY_DOT
	ABLIM_NOTHING,	// ABIL_LIMIT_TARGET_BEING_ATTACKED
	ABLIM_ROLE,	// ABIL_LIMIT_IN_ROLE
	ABLIM_NOTHING,	// ABIL_LIMIT_NO_WITNESSES
	ABLIM_NOTHING,	// ABIL_LIMIT_NO_WITNESSES_HIDE
	ABLIM_NOTHING,	// ABIL_LIMIT_CHECK_OBJ_BINDING
	ABLIM_OBJ_FLAG,	// ABIL_LIMIT_OBJ_FLAGGED
	ABLIM_OBJ_FLAG,	// ABIL_LIMIT_OBJ_NOT_FLAGGED
	ABLIM_NOTHING,	// ABIL_LIMIT_TARGET_IS_VAMPIRE
	ABLIM_NOTHING,	// ABIL_LIMIT_CAN_PURIFY_TARGET
	ABLIM_NOTHING,	// ABIL_LIMIT_IN_COMBAT
	ABLIM_NOTHING,	// ABIL_LIMIT_NOT_BEING_ATTACKED_MELEE
	ABLIM_NOTHING,	// ABIL_LIMIT_NOT_BEING_ATTACKED_MOBILE_MELEE
	ABLIM_AFF_FLAG,	// ABIL_LIMIT_NOT_AFFECTED_BY
	ABLIM_AFF_FLAG,	// ABIL_LIMIT_TARGET_NOT_AFFECTED_BY
	ABLIM_NOTHING,	// ABIL_LIMIT_NOT_LEADING_MOB
	ABLIM_NOTHING,	// ABIL_LIMIT_NOT_LEADING_VEHICLE
	ABLIM_AFF_FLAG,	// ABIL_LIMIT_IS_AFFECTED_BY
	ABLIM_AFF_FLAG,	// ABIL_LIMIT_TARGET_IS_AFFECTED_BY
	ABLIM_NOTHING,	// ABIL_LIMIT_TARGET_IS_HUMAN
};


// used to filter words out of conjure/summon names, e.g. "Conjure Food" lists as "Food" in do_conjure.
const char *conjure_words[] = {
	"conjure",
	"create",
	"summon",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// ADVENTURE CONSTANTS /////////////////////////////////////////////////////

// ADV_x
const char *adventure_flags[] = {
	"IN-DEVELOPMENT",	// 0
	"LOCK-LEVEL-ON-ENTER",
	"LOCK-LEVEL-ON-COMBAT",
	"!NEARBY",
	"ROTATABLE",
	"CONFUSING-RANDOMS",	// 5
	"!NEWBIE",
	"NEWBIE-ONLY",
	"NO-MOB-CLEANUP",
	"EMPTY-RESET-ONLY",
	"CAN-DELAY-LOAD",	// 10
	"IGNORE-WORLD-SIZE",
	"IGNORE-ISLAND-LEVELS",
	"CHECK-OUTSIDE-FIGHTS",
	"GLOBAL-NEARBY",
	"DETECTABLE",	// 15
	"\n"
};


// ADV_LINKF_x
const char *adventure_link_flags[] = {
	"CLAIMED-OK",	// 0
	"CITY-ONLY",
	"!CITY",
	"CLAIMED-ONLY",
	"CONTINENT-ONLY",
	"!CONTINENT",	// 5
	"\n"
};


// ADV_LINK_x (1/2): link types
const char *adventure_link_types[] = {
	"BDG-EXISTING",	// 0
	"BDG-NEW",
	"PORTAL-WORLD",
	"PORTAL-BDG-EXISTING",
	"PORTAL-BDG-NEW",
	"TIME-LIMIT",	// 5
	"NOT-NEAR-SELF",
	"PORTAL-CROP",
	"EVENT-RUNNING",
	"PORTAL-VEHICLE-EXISTING",
	"PORTAL-VEHICLE-NEW-BDG-EXISTING",	// 10
	"PORTAL-VEHICLE-NEW-BDG-NEW",
	"PORTAL-VEHICLE-NEW-CROP",
	"PORTAL-VEHICLE-NEW-WORLD",
	"IN-VEHICLE-EXISTING",
	"IN-VEHICLE-NEW-BDG-EXISTING",	// 15
	"IN-VEHICLE-NEW-BDG-NEW",
	"IN-VEHICLE-NEW-CROP",
	"IN-VEHICLE-NEW-WORLD",
	"\n"
};


// ADV_LINK_x (2/2): whether or not a rule specifies a possible location (other types are for limits)
const bool adventure_link_is_location_rule[] = {
	TRUE,	// ADV_LINK_BUILDING_EXISTING
	TRUE,	// ADV_LINK_BUILDING_NEW
	TRUE,	// ADV_LINK_PORTAL_WORLD
	TRUE,	// ADV_LINK_PORTAL_BUILDING_EXISTING
	TRUE,	// ADV_LINK_PORTAL_BUILDING_NEW
	FALSE,	// ADV_LINK_TIME_LIMIT
	FALSE,	// ADV_LINK_NOT_NEAR_SELF
	TRUE,	// ADV_LINK_PORTAL_CROP
	FALSE,	// ADV_LINK_EVENT_RUNNING
	TRUE,	// ADV_LINK_PORTAL_VEH_EXISTING
	TRUE,	// ADV_LINK_PORTAL_VEH_NEW_BUILDING_EXISTING
	TRUE,	// ADV_LINK_PORTAL_VEH_NEW_BUILDING_NEW
	TRUE,	// ADV_LINK_PORTAL_VEH_NEW_CROP
	TRUE,	// ADV_LINK_PORTAL_VEH_NEW_WORLD
	TRUE,	// ADV_LINK_IN_VEH_EXISTING
	TRUE,	// ADV_LINK_IN_VEH_NEW_BUILDING_EXISTING
	TRUE,	// ADV_LINK_IN_VEH_NEW_BUILDING_NEW
	TRUE,	// ADV_LINK_IN_VEH_NEW_CROP
	TRUE,	// ADV_LINK_IN_VEH_NEW_WORLD
};


// ADV_SPAWN_x
const char *adventure_spawn_types[] = {
	"MOB",
	"OBJ",
	"VEH",
	"\n"
};


// INST_x
const char *instance_flags[] = {
	"COMPLETED",
	"NEEDS-LOAD",
	"\n"
};


// RMT_x
const char *room_template_flags[] = {
	"OUTDOOR",	// 0
	"DARK",
	"LIGHT",
	"!MOB",
	"PEACEFUL",
	"NEED-BOAT",	// 5
	"!TELEPORT",
	"LOOK-OUT",
	"!LOCATION",
	"*",
	"*",	// 10
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// ARCHETYPE CONSTANTS /////////////////////////////////////////////////////

// ARCH_x: archetype flags
const char *archetype_flags[] = {
	"IN-DEVELOPMENT",	// 0
	"BASIC",
	"LOCKED",
	"\n"
};


// ARCHT_x (1/2): archetype types
const char *archetype_types[] = {
	"ORIGIN",
	"HOBBY",
	"\n"
};


// ARCHT_x (2/2): The order and contents of this array determine what players see during creation.
const struct archetype_menu_type archetype_menu[] = {
	{ ARCHT_ORIGIN,
		"Origin",
		"Your character's origins, be they noble or meager, determine your attributes\r\n"
		"and starting skills. But remember, this is only the beginning of your path.\r\n"
	},
	{ ARCHT_HOBBY,
		"Hobby",
		"Round out your character by selecting a hobby. You will receive 5 extra skill\r\n"
		"points in whichever skill you choose.\r\n"
	},
	
	{ NOTHING, "\n", "\n" }	// this goes last
};


 //////////////////////////////////////////////////////////////////////////////
//// ATTACK MESSAGE CONSTANTS ////////////////////////////////////////////////

// AMDF_x: Attack message flags
const char *attack_message_flags[] = {
	"WEAPON",	// 0
	"MOBILE",
	"DISARMABLE",
	"APPLY-POISON",
	"IGNORE-MISSING",
	"\n"
};


// SPD_x: speeds for attacks
const char *attack_speed_types[] = {
	"Fast",
	"Normal",
	"Slow",
	"\n"
};


// WEAPON_x: Weapon types
const char *weapon_types[] = {
	"blunt",	// 0
	"sharp",
	"magic",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// AUGMENT CONSTANTS ///////////////////////////////////////////////////////

// AUGMENT_x (1/2): augment types
const char *augment_types[] = {
	"None",
	"Enchantment",
	"Hone",
	"\n"
};


// AUGMENT_x (1/2): augment type data
const struct augment_type_data augment_info[] = {
	// noun, verb, apply-type, default-flags, use-obj-flag
	{ "augment", "augment", APPLY_TYPE_NATURAL, NOBITS, NOBITS },
	{ "enchantment", "enchant", APPLY_TYPE_ENCHANTMENT, NOBITS, OBJ_ENCHANTED },
	{ "hone", "hone", APPLY_TYPE_HONED, AUG_SELF_ONLY, NOBITS },
	
	{ "\n", "\n", 0, 0 }	// last
};


// AUG_x: augment flags
const char *augment_flags[] = {
	"IN-DEVELOPMENT",
	"SELF-ONLY",
	"ARMOR",
	"SHIELD",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// CLASS CONSTANTS /////////////////////////////////////////////////////////

// CLASSF_x: class flags
const char *class_flags[] = {
	"IN-DEVELOPMENT",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// PLAYER CONSTANTS ////////////////////////////////////////////////////////

// ACCT_x: Account flags
const char *account_flags[] = {
	"FROZEN",	// 0
	"MUTED",
	"SITEOK",
	"!TITLE",
	"MULTI-IP",
	"MULTI-CHAR",	// 5
	"APPR",
	"!CUSTOMIZE",
	"\n"
};


// BONUS_x (1/2): bonus traits
const char *bonus_bits[] = {
	"STRENGTH",		// 0
	"DEXTERITY",
	"CHARISMA",
	"GREATNESS",
	"INTELLIGENCE",
	"WITS",	// 5
	"HEALTH",
	"MOVES",
	"MANA",
	"LIGHT-RADIUS",
	"MOVE-REGEN",	// 10
	"MANA-REGEN",
	"FAST-CHORES",
	"EXTRA-DAILY-SKILLS",
	"INVENTORY",
	"FASTER",	// 15
	"BLOOD",
	"CLOCK",
	"NO-THIRST",
	"NO-HUNGER",
	"VIEW-HEIGHT",	// 20
	"WARM-RESIST",
	"COLD-RESIST",
	"\n"
};


// BONUS_x (2/2): bonus traits as shown to players
const char *bonus_bit_descriptions[] = {
	"Big boned (+1 Strength)",	// 0
	"Double-jointed (+1 Dexterity)",
	"Friendly (+1 Charisma)",
	"Upper class (+1 Greatness)",
	"Literate upbringing (+1 Intelligence)",
	"Quick-witted (+1 Wits)",	// 5
	"Thick skinned (extra health)",
	"Traveller (extra moves)",
	"Mana vessel (extra mana)",
	"Twilight sight (larger light radius)",
	"Unusual stamina (faster move regeneration)",	// 10
	"Channeler (faster mana regeneration)",
	"Work ethic (faster chores like chopping)",
	"Quick learner (extra daily bonus exp)",
	"Pack mule (larger inventory)",
	"Nimble (shorter walking delay)",	// 15
	"Rich veins (extra blood for vampires)",
	"Sense of time (don't need a clock)",
	"Salt blooded (never thirsty)",
	"Tenacious waif (never hungry)",
	"Surveyor (+1 view height)",	// 20
	"Fireborn (tolerates warm climates)",
	"Frostborn (tolerates cold climates)",
	"\n"
};


// COND_x: player conditions
const char *condition_types[] = {
	"Drunk",
	"Full",
	"Thirst",
	"\n"
};


// CUSTOM_COLOR_x
const char *custom_color_types[] = {
	"emote",	// 0
	"esay",
	"gsay",
	"oocsay",
	"say",
	"slash-channels",	// 5
	"tell",
	"status",
	"sun",
	"temperature",
	"weather",	// 10
	"\n"
};


// ATT_x: extra attributes
const char *extra_attribute_types[] = {
	"Bonus-Inventory",	// 0
	"Resist-Physical",
	"Block",
	"To-Hit",
	"Dodge",
	"Extra-Blood",	// 5
	"Bonus-Physical",
	"Bonus-Magical",
	"Bonus-Healing",
	"Heal-Over-Time",
	"Resist-Magical",	// 10
	"Crafting-Bonus",
	"Blood-Upkeep",
	"Age",
	"Night-Vision",
	"Nearby-Range",	// 15
	"Where-Range",
	"Warmth",
	"Cooling",
	"\n"
};


// FM_x: combat messages
const char *combat_message_types[] = {
	"my hits",	// 0
	"my misses",
	"hits against me",
	"misses against me",
	"ally hits",
	"ally misses",		// 5
	"hits against allies",
	"misses against allies",
	"hits against target",
	"misses against target",
	"hits against tank",	// 10
	"misses against tank",
	"other hits",
	"other misses",
	"autodiagnose",
	"my buffs in combat",	// 15
	"ally buffs in combat",
	"other buffs in combat",
	"damage numbers",
	"my affects in combat",
	"ally affects in combat",	// 20
	"other affects in combat",
	"my abilities",
	"ally abilities",
	"other abilities",
	"abilities against me",	// 25
	"abilities against allies",
	"abilities against target",
	"abilities against tank",
	"my heals",
	"heals on me",	// 30
	"heals on allies",
	"heals on target",
	"heals on other",
	"\n"
};


// FRIEND_x: status on the friends list
const char *friend_status_types[] = {
	"not friends",	// 0
	"request sent",
	"requesting friendship",
	"friends",
	"\n"
};


// GRANT_x
const char *grant_bits[] = {
	"advance",	// 0
	"ban",
	"clearabilities",
	"dc",
	"echo",
	"editnotes",	// 5
	"empires",
	"force",
	"freeze",
	"gecho",
	"instance",	// 10
	"load",
	"mute",
	"olc",
	"olc-controls",
	"page",	// 15
	"purge",
	"reboot",
	"reload",
	"restore",
	"send",	// 20
	"set",
	"shutdown",
	"snoop",
	"switch",
	"tedit",	// 25
	"transfer",
	"unbind",
	"users",
	"wizlock",
	"rescale",	// 30
	"approve",
	"forgive",
	"hostile",
	"slay",
	"island",	// 35
	"oset",
	"playerdelete",
	"unquest",
	"automessage",
	"peace",	// 40
	"unprogress",
	"events",
	"triggers",
	"\n"
};


// INFORMATIVE_x: For players' informative views
const char *informative_view_bits[] = {
	"building-status",	// 0
	"disrepair",
	"mine-status",
	"public",
	"no-work",
	"no-abandon",	// 5
	"no-dismantle",
	"\n"
};


// MOUNT_x: mount flags
const char *mount_flags[] = {
	"riding",
	"aquatic",
	"flying",
	"waterwalking",
	"\n"
};


/* PLR_x */
const char *player_bits[] = {
	"APPR",	// 0
	"TR",
	"MAILING",
	"DONTSET",
		"UNUSED",
		"UNUSED",	// 5
		"UNUSED",
		"UNUSED",
	"LOADRM",
	"!WIZL",
	"!DEL",	// 10
	"INVST",
	"IPMASK",
	"DISGUISED",
		"UNUSED",
		"UNUSED",	// 15
	"NEEDS-NEWBIE-SETUP",
	"!RESTICT",
	"KEEP-LOGIN",
	"EXTRACTED",
	"ADV-SUMMON",	// 20
	"\n"
};


/* PRF_x */
const char *preference_bits[] = {
	"AFK",	// 0
	"COMPACT",
	"DEAF",
	"!TELL",
	"POLIT",
	"RP",	// 5
		"*",	// formerly MORTLOG
	"!REP",
	"LIGHT",
	"INCOGNITO",
	"!WIZ",	// 10
	"!MCOL",
	"!HASSLE",
	"!IDLE-OUT",
	"ROOMFLAGS",
	"	*",	// 15, formerly !CHANNEL-JOINS
	"AUTOKILL",
	"SCRL",
	"NO-ROOM-DESCS",
	"BOTHER",
	"AUTORECALL",	// 20
	"!GOD",
	"PVP",
	"INFORMATIVE",
	"!SPAM",
	"SCREENREADER",	// 25
	"STEALTHABLE",
	"WIZHIDE",
	"AUTONOTES",
	"AUTODISMOUNT",
	"!EMPIRE",	// 30
	"CLEARMETERS",
	"!TUTORIALS",
	"!PAINT",
	"EXTRA-SPACING",
		"*",	// 35, formerly TRAVEL-LOOK
	"AUTOCLIMB",
	"AUTOSWIM",
	"ITEM-QUALITY",
	"ITEM-DETAILS",
	"!EXITS",	// 40
	"SHORT-EXITS",
	"NO-FRIENDS",
	"\n"
};


// ROLE_x (1/2): role names
const char *class_role[] = {
	"none",	// 0
	"Tank",
	"Melee",
	"Caster",
	"Healer",
	"Utility",	// 5
	"Solo",
	"\n"
};


// ROLE_x (2/2): role colors for who list
const char *class_role_color[] = {
	"\t0",	// 0
	"\ty",	// tank
	"\tr",	// melee
	"\ta",	// caster
	"\tj",	// healer
	"\tm",	// 5, utility
	"\tw",	// solo
	"\n"
};


// PRF_x: for do_toggle, this controls the "toggle" command and these are displayed in rows of 3	
const struct toggle_data_type toggle_data[] = {
	// these are shown in rows of 3
	// name, type, prf, level, func
	
	{ "autoclimb", TOG_ONOFF, PRF_AUTOCLIMB, 0, NULL },
	{ "screen-reader", TOG_ONOFF, PRF_SCREEN_READER, 0, NULL },
	{ "afk", TOG_ONOFF, PRF_AFK, 0, afk_notify },
	
	{ "autodismount", TOG_ONOFF, PRF_AUTODISMOUNT, 0, NULL },
	{ "scrolling", TOG_ONOFF, PRF_SCROLLING, 0, NULL },
	{ "bother", TOG_ONOFF, PRF_BOTHERABLE, 0, NULL },
	
	{ "autokill", TOG_ONOFF, PRF_AUTOKILL, 0, NULL },
	{ "action-spam", TOG_OFFON, PRF_NOSPAM, 0, NULL },
	{ "clearmeters", TOG_ONOFF, PRF_CLEARMETERS, 0, NULL },
	
	{ "autorecall", TOG_ONOFF, PRF_AUTORECALL, 0, NULL },
	{ "room-descs", TOG_OFFON, PRF_NO_ROOM_DESCS, 0, NULL },
	{ "rp", TOG_ONOFF, PRF_RP, 0, NULL },
	
	{ "autoswim", TOG_ONOFF, PRF_AUTOSWIM, 0, NULL },
	{ "compact", TOG_ONOFF, PRF_COMPACT, 0, NULL },
	{ "pvp", TOG_ONOFF, PRF_ALLOW_PVP, 0, tog_pvp },
	
	{ "tutorials",	TOG_OFFON, PRF_NO_TUTORIALS, 0, NULL },
	{ "extra-spacing",	TOG_ONOFF, PRF_EXTRA_SPACING, 0, NULL },
	{ "stealthable", TOG_ONOFF, PRF_STEALTHABLE, 0, NULL },
	
	{ "informative", TOG_ONOFF, PRF_INFORMATIVE, 0, tog_informative },
	{ "no-repeat", TOG_ONOFF, PRF_NOREPEAT, 0, NULL },
	{ "tell", TOG_OFFON, PRF_NOTELL, 0, NULL },
	
	{ "political", TOG_ONOFF, PRF_POLITICAL, 0, tog_political },
	{ "no-paint", TOG_ONOFF, PRF_NO_PAINT, 0, NULL },
	{ "shout", TOG_OFFON, PRF_DEAF, 0, NULL },
	
	{ "map-color", TOG_OFFON, PRF_NOMAPCOL, 0, tog_mapcolor },
	{ "item-details", TOG_ONOFF, PRF_ITEM_DETAILS, 0, NULL },
	{ "item-quality", TOG_ONOFF, PRF_ITEM_QUALITY, 0, NULL },
	
	{ "no-empire", TOG_ONOFF, PRF_NOEMPIRE, 0, NULL },
	{ "exits", TOG_OFFON, PRF_NO_EXITS, 0, NULL },
	{ "short-exits", TOG_ONOFF, PRF_SHORT_EXITS, 0, NULL },
	
	{ "no-friends", TOG_ONOFF, PRF_NO_FRIENDS, 0, NULL },
	
	// imm section
	{ "wiznet", TOG_OFFON, PRF_NOWIZ, LVL_START_IMM, NULL },
	{ "holylight", TOG_ONOFF, PRF_HOLYLIGHT, LVL_START_IMM, NULL },
	{ "roomflags", TOG_ONOFF, PRF_ROOMFLAGS, LVL_START_IMM, NULL },
	
	{ "hassle", TOG_OFFON, PRF_NOHASSLE, LVL_START_IMM, NULL },
	{ "idle-out", TOG_OFFON, PRF_NO_IDLE_OUT, LVL_START_IMM, NULL },
	{ "incognito", TOG_ONOFF, PRF_INCOGNITO, LVL_START_IMM, NULL },
	
	{ "wizhide", TOG_ONOFF, PRF_WIZHIDE, LVL_START_IMM, NULL },
	{ "autonotes", TOG_ONOFF, PRF_AUTONOTES, LVL_START_IMM, NULL },
	
	// this goes last
	{ "\n", 0, NOBITS, 0, NULL }
};


/* CON_x */
const char *connected_types[] = {
	"Playing",	// 0
	"Disconnecting",
	"Get name",
	"Confirm name",
	"Get password",
	"Get new PW",	// 5
	"Confirm new PW",
	"Select sex",
		"UNUSED 1",
	"Reading MOTD",
	"Disconnecting",	// 10
	"Referral?",
	"Screen reader?",
	"Last name?",
	"Get last name",
	"Cnf last name",	// 15
	"Cnf archetype",
	"Have alt?",
	"Alt name",
	"Alt password",
	"Finish Creation",	// 20
	"Archetype",
	"Goodbye",
	"Bonus reset",
	"Bonus trait",
	"Promo code?",	// 25
	"Confirm promo",
	"\n"
};


// PTECH_x: player techs
const char *player_tech_types[] = {
	"RESERVED",	// 0
	"Armor-Heavy",
	"Armor-Light",
	"Armor-Mage",
	"Armor-Medium",
	"Block",	// 5
	"Block-Ranged",
	"Block-Magical",
	"Bonus-vs-Animals",
	"Butcher-Upgrade",
	"Customize-Building",	// 10
	"Deep-Mines",
	"Dual-Wield",
	"Fast-Wood-Processing",
	"Fastcasting",
	"Fast-Find",	// 15
	"Fish-Command",
	"Forage-Command",
	"Harvest-Upgrade",
	"*",
	"Hide-Upgrade",	// 20
	"Infiltrate",
	"Infiltrate-Upgrade",
	"Larger-Light-Radius",
	"Light-Fire",
	"Map-Invis",	// 25
	"Mill-Upgrade",
	"Navigation",
	"!Hunger",
	"!Poison",
	"!Thirst",	// 30
	"!Track-City",
	"!Track-Wild",
	"Pickpocket",
	"Poison",
	"Poison-Upgrade",	// 35
	"Portal",
	"Portal-Upgrade",
	"Ranged-Combat",
	"Riding",
	"Riding-Flying",	// 40
	"Riding-Upgrade",
	"Rough-Terrain",
	"See-Chars-In-Dark",
	"See-Objs-In-Dark",
	"See-Inventory",	// 45
	"Shear-Upgrade",
	"Steal-Upgrade",
	"Swimming",
	"Search-Command",
	"Two-Handed-Mastery",	// 50
	"Where-Upgrade",
	"Dodge-Cap",
	"Skinning-Upgrade",
	"Barde",
	"Herd-Command",	// 55
	"Milk-Command",
	"Shear-Command",
	"Tame-Command",
	"Bite-Melee-Upgrade",
	"Bite-Tank-Upgrade",	// 60
	"Bite-Steal-Blood",
	"See-In-Dark-Outdoors",
	"Hunt-Animals",
	"Clock",
	"Calendar",	// 65
	"Mint-Command",
	"Tan-Command",
	"No-Purify",
	"Vampire-Sun-Immunity",
	"Gather-Command",	// 70
	"Chop-Command",
	"Dig-Command",
	"Harvest-Command",
	"Pick-Command",
	"Quarry-Command",	// 75
	"Drink-Blood-Faster",
	"Summon-Materials",
	"Customize-Vehicle",
	"Plant-Crops",
	"Chip-Command",	// 80
	"Saw-Command",
	"Scrape-Command",
	"Map-Memory",
	"See-In-Magic-Darkness",
	"Track-Command",	// 85
	"Resist-Poison",
	"Vampire-Bite",
	"Enemy-Buff-Details",
	"Conceal-Equipment",
	"Conceal-Inventory",	// 90
	"Flee-Upgrade",
	"Faster-Melee-Combat",
	"Faster-Ranged-Combat",
	"Riding-Swap-Anywhere",
	"Riding-Release-Mount",	// 95
	"Rework-Command",
	"Bite-Regeneration",
	"Steal-Command",
	"Use-Honed-Gear",
	"Redirect-Magical-Damage-to-Mana",	// 100
	"More-Blood-From-Humans",
	"\n"
};


// SM_x: status messages
const char *status_message_types[] = {
	"animal movement",	// 0
	"channel joins",
	"cooldowns",
	"empire logs",
	"hunger",
	"thirst",	// 5
	"low blood",
	"mortlog",
	"prompt",
	"skill gains",
	"sun",	// 10
	"sun auto look",
	"temperature",
	"extreme temperature",
	"travel auto look",
	"vehicle movement",	// 15
	"weather",
	"fight prompt",
	"\n"
};


// SYS_x syslog types
const char *syslog_types[] = {
	"config",	// 0
	"death",
	"error",
	"gc",
	"info",
	"level",	// 5
	"login",
	"olc",
	"script",
	"system",
	"validation",	// 10
	"empire",
	"event",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// DIRECTION AND ROOM CONSTANTS ////////////////////////////////////////////

// cardinal directions for both display and detecting arguments -- NUM_OF_DIRS
const char *dirs[] = {
	"north",	// 0
	"east",
	"south",
	"west",
	"northwest",
	"northeast",	// 5
	"southwest",
	"southeast",
	"up",
	"down",
	"fore",	// 10
	"starboard",
	"port",
	"aft",
	"random",
	"\n"
};


// alternate direction names, to allow certain abbrevs in argument parsing -- NUM_OF_DIRS
const char *alt_dirs[] = {
	"n",	// 0
	"e",
	"s",
	"w",
	"nw",
	"ne",	// 5
	"sw",
	"se",
	"u",
	"d",
	"fo",	// 10
	"st",
	"po",
	"af",
	"random",
	"\n"
};


// the direction I'm coming from, for walking messages -- NUM_OF_DIRS
const char *from_dir[] = {
	"the south",	// 0
	"the west",
	"the north",
	"the east",
	"the southeast",
	"the southwest",	// 5
	"the northeast",
	"the northwest",
	"below",
	"above",
	"the aft",	// 10
	"port",
	"starboard",
	"the front",
	"random",
	"\n"
};


// these are the arguments to real_shift() to shift one tile in a direction, e.g. real_shift(room, shift_dir[dir][0], shift_dir[dir][1]) -- NUM_OF_DIRS
const int shift_dir[][2] = {
	{ 0, 1 },	// 0, north
	{ 1, 0 },	// east
	{ 0, -1},	// south
	{-1, 0 },	// west
	{-1, 1 },	// nw
	{ 1, 1 },	// 5, ne
	{-1, -1},	// sw
	{ 1, -1},	// se
	{ 0, 0 },	// up
	{ 0, 0 },	// down
	{ 0, 0 },	// 10, fore
	{ 0, 0 },	// starboard
	{ 0, 0 },	// port
	{ 0, 0 },	// aft
	{ 0, 0 }	// random
};


// whether or not a direction can be used by designate, building version
const bool can_designate_dir[NUM_OF_DIRS] = {
	TRUE,	// 0, north
	TRUE,	// east
	TRUE,	// south
	TRUE,	// west
	TRUE,	// nw
	TRUE,	// 5, ne
	TRUE,	// sw
	TRUE,	// se
	TRUE,	// up
	TRUE,	// down
	FALSE,	// 10, fore
	FALSE,	// starboard
	FALSE,	// port
	FALSE,	// aft
	FALSE	// random
};


// whether or not a direction can be used by designate, vehicle version
const bool can_designate_dir_vehicle[NUM_OF_DIRS] = {
	FALSE,	// 0, north
	FALSE,	// east
	FALSE,	// south
	FALSE,	// west
	FALSE,	// nw
	FALSE,	// 5, ne
	FALSE,	// sw
	FALSE,	// se
	TRUE,	// up
	TRUE,	// down
	TRUE,	// 10, fore
	TRUE,	// starboard
	TRUE,	// port
	TRUE,	// aft
	FALSE	// random
};


// whether or not you can flee in a given direction
const bool can_flee_dir[NUM_OF_DIRS] = {
	TRUE,	// 0, north
	TRUE,	// east
	TRUE,	// south
	TRUE,	// west
	TRUE,	// nw
	TRUE,	// 5, ne
	TRUE,	// sw
	TRUE,	// se
	FALSE,	// up
	FALSE,	// down
	TRUE,	// 10, fore
	TRUE,	// starboard
	TRUE,	// port
	TRUE,	// aft
	FALSE	// random
};


// whether or not a direction is "flat" (2D)
const bool is_flat_dir[NUM_OF_DIRS] = {
	TRUE,	// 0, north
	TRUE,	// east
	TRUE,	// south
	TRUE,	// west
	TRUE,	// nw
	TRUE,	// 5, ne
	TRUE,	// sw
	TRUE,	// se
	FALSE,	// up
	FALSE,	// down
	TRUE,	// 10, fore
	TRUE,	// starboard
	TRUE,	// port
	TRUE,	// aft
	FALSE	// random
};


/* EX_x */
const char *exit_bits[] = {
	"DOOR",
	"CLOSED",
	"\n"
};


// indicates the opposite of each direction
const int rev_dir[NUM_OF_DIRS] = {
	SOUTH,	// 0
	WEST,
	NORTH,
	EAST,
	SOUTHEAST,
	SOUTHWEST,	// 5
	NORTHEAST,
	NORTHWEST,
	DOWN,
	UP,
	AFT,	// 10
	PORT,
	STARBOARD,
	FORE,
	DIR_RANDOM
};


// for PTECH_NAVIGATION: confused_dir[which dir is north][reverse][which dir to translate]
// reverse=0 is for moving
// reverse=1 is for which way directions are displayed (this was very confusing to figure out)
const int confused_dirs[NUM_2D_DIRS][2][NUM_OF_DIRS] = {
	// NORTH (normal)
	{{ NORTH, EAST, SOUTH, WEST, NORTHWEST, NORTHEAST, SOUTHWEST, SOUTHEAST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM },
	{ NORTH, EAST, SOUTH, WEST, NORTHWEST, NORTHEAST, SOUTHWEST, SOUTHEAST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM }},
	
	// EAST (rotate left)
	{{ EAST, SOUTH, WEST, NORTH, NORTHEAST, SOUTHEAST, NORTHWEST, SOUTHWEST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM },
	{ WEST, NORTH, EAST, SOUTH, SOUTHWEST, NORTHWEST, SOUTHEAST, NORTHEAST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM }},
	
	// SOUTH (rotate 180)
	{{ SOUTH, WEST, NORTH, EAST, SOUTHEAST, SOUTHWEST, NORTHEAST, NORTHWEST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM },
	{ SOUTH, WEST, NORTH, EAST, SOUTHEAST, SOUTHWEST, NORTHEAST, NORTHWEST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM }},
	
	// WEST (rotate right)
	{{ WEST, NORTH, EAST, SOUTH, SOUTHWEST, NORTHWEST, SOUTHEAST, NORTHEAST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM },
	{ EAST, SOUTH, WEST, NORTH, NORTHEAST, SOUTHEAST, NORTHWEST, SOUTHWEST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM }},
	
	// these rotations only work for adventures, not for the whole map:
	
	// NORTHWEST
	{{ NORTHWEST, NORTHEAST, SOUTHEAST, SOUTHWEST, WEST, NORTH, SOUTH, EAST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM },
	{ SOUTHEAST, SOUTHWEST, NORTHWEST, NORTHEAST, EAST, SOUTH, NORTH, WEST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM }},
	
	// NORTHEAST
	{{ NORTHEAST, SOUTHEAST, SOUTHWEST, NORTHWEST, NORTH, EAST, WEST, SOUTH, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM },
	{ SOUTHWEST, NORTHWEST, NORTHEAST, SOUTHEAST, SOUTH, WEST, EAST, NORTH, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM }},
	
	// SOUTHWEST
	{{ SOUTHWEST, NORTHWEST, NORTHEAST, SOUTHEAST, SOUTH, WEST, EAST, NORTH, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM },
	{ NORTHEAST, SOUTHEAST, SOUTHWEST, NORTHWEST, NORTH, EAST, WEST, SOUTH, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM }},
	
	// SOUTHEAST
	{{ SOUTHEAST, SOUTHWEST, NORTHWEST, NORTHEAST, EAST, SOUTH, NORTH, WEST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM },
	{ NORTHWEST, NORTHEAST, SOUTHEAST, SOUTHWEST, WEST, NORTH, SOUTH, EAST, UP, DOWN, FORE, STARBOARD, PORT, AFT, DIR_RANDOM }}
};


// for PTECH_NAVIGATION: how_to_show_map[dir which is north for char][x=0,y=1]
// for each direction, whether the x/y coord goes from positive to negative (1) or negative to positive (-1)
const int how_to_show_map[NUM_SIMPLE_DIRS][2] = {
	{ -1, 1 },	// north
	{ -1, -1 },	// east
	{ 1, -1 },	// south
	{ 1, 1 }	// west
};


// for PTECH_NAVIGATION: show_map_y_first[dir which is north for char]
// 1 = show y coordinate vertically, 0 = show x coord vertically
const int show_map_y_first[NUM_SIMPLE_DIRS] = {
	1,	// N
	0,	// E
	1,	// S
	0	// W
};


// used for get_partial_direction_to() etc
const char *partial_dirs[][2] = {
	// counter-clockwise from ENE, ending with E
	{ "east-northeast", "ene" },
	{ "northeast", "ne" },
	{ "north-northeast", "nne" },
	{ "north", "n" },
	{ "north-northwest", "nnw" },
	{ "northwest", "nw" },
	{ "west-northwest", "wnw" },
	{ "west", "w" },
	{ "west-southwest", "wsw" },
	{ "southwest", "sw" },
	{ "south-southwest", "ssw" },
	{ "south", "s" },
	{ "south-southeast", "sse" },
	{ "southeast", "se" },
	{ "east-southeast", "ese" },
	{ "east", "e" },
	
	{ "\n", "\n" }		// must be last for the iterator to work
};


 //////////////////////////////////////////////////////////////////////////////
//// CHARACTER CONSTANTS /////////////////////////////////////////////////////

// AFF_x (1/3) - flags
const char *affected_bits[] = {
	"BLIND",	// 0
	"MAJESTY",
	"INFRA",
	"SNEAK",
	"HIDDEN",
	"*CHARM",	// 5
	"INVIS",
	"IMMUNE-PHYSICAL-DEBUFFS",
	"SENSE-HIDDEN",
	"!PHYSICAL",
	"!TARGET",	// 10
	"!SEE",
	"FLYING",
	"!ATTACK",
	"IMMUNE-MAGICAL-DEBUFFS",
	"DISARMED",	// 15
	"HASTE",
	"IMMOBILIZED",
	"SLOW",
	"STUNNED",
	"STONED",	// 20
	"!BLOOD",
	"*",	// formerly CLAWS
	"DEATHSHROUDED",
	"EARTHMELDED",
	"MUMMIFIED",	// 25
	"SOULMASK",
	"NO-TRACKS",
	"IMMUNE-POISON-DEBUFFS",
	"IMMUNE-MENTAL-DEBUFFS",
	"!STUN",	// 30
	"*ORDERED",
	"!DRINK-BLOOD",
	"DISTRACTED",
	"HARD-STUNNED",
	"IMMUNE-DAMAGE",	// 35
	"!WHERE",
	"WATERWALKING",
	"LIGHT",
	"POOR-REGENS",
	"SLOWER-ACTIONS",	// 40
	"HUNGRIER",
	"THIRSTIER",
	"IMMUNE-TEMPERATURE",
	"AUTO-RESURRECT",
	"COUNTERSPELL",		// 45
	"!DISARM",
	"\n"
};

// AFF_x (2/3) - strings shown when you consider someone (empty for no-show)
const char *affected_bits_consider[] = {
	"",	// 0 - blind
	"$E has a majestic aura!",	// majesty
	"",	// infra
	"",	// sneak
	"",	// hide
	"",	// 5 - charm
	"",	// invis
	"$E is immune to physical debuffs.",	// immune-phyical-debuffs
	"",	// sense hide
	"$E is immune to physical damage.",	// !physical
	"",	// 10 - no-target-in-room
	"",	// no-see-in-room
	"",	// flying
	"$E cannot be attacked.",	// !attack
	"$E is immune to magical debuffs.",	// immune-magical-debuffs
	"",	// 15 - disarmed
	"",	// haste
	"",	// immobilized
	"",	// slow
	"",	// stunned
	"",	// 20 - stoned
	"",	// can't spend blood
	"",	// unused
	"",	// deathshrouded
	"",	// earthmeld
	"",	// 25 - mummified
	"$E is soulmasked.",	// soulmask
	"",	// no-tracks
	"$E is immune to poison debuffs.",	// immune-poison-debuffs
	"$E is immune to mental debuffs.",	// immune-mental-debuffs
	"$E is immune to stuns.",	// 30 - !stun
	"",	// ordred
	"",	// !drink-blood
	"",	// distracted
	"",	// hard-stunned
	"",	// 35 - immune-damage
	"",	// !where
	"",	// waterwalking
	"",	// light
	"",	// poor-regens
	"",	// 40 - slower-actions
	"",	// hungrier
	"",	// thirstier
	"",	// immune-temperature
	"",	// auto-resurrect
	"",	// 45 - counterspell
	"$E cannot be disarmed",	// !disarm
	"\n"
};

// AFF_x (3/3) - determines if an aff flag is "bad" for the bearer
const bool aff_is_bad[] = {
	TRUE,	// 0 - blind
	FALSE,	// majesty
	FALSE,	// infravision
	FALSE,	// sneak
	FALSE,	// hidden
	FALSE,	// 5 - charm
	FALSE,	// invisible
	FALSE,	// immune-physical-debuffs
	FALSE,	// sense-hidden
	FALSE,	// immune-physical (damage)
	FALSE,	// 10 - no-target-in-room
	FALSE,	// no-see-in-room / !SEE
	FALSE,	// flying
	FALSE,	// !attack
	FALSE,	// immune-magical-debuffs
	TRUE,	// 15 - disarmed
	FALSE,	// haste
	TRUE,	// immobilized
	TRUE,	// slow
	TRUE,	// stunned
	TRUE,	// 20 - stoned
	TRUE,	// !blood
	FALSE,	// * unused
	FALSE,	// deathshrouded
	FALSE,	// earthmelded
	FALSE,	// 25 - mummified
	FALSE,	// soulmask
	FALSE,	// no-tracks
	FALSE,	// immune-poison-debuffs
	FALSE,	// immune-mental-debuffs
	FALSE,	// 30 - !stun
	FALSE,	// ordered
	FALSE,	// no-drink-blood
	TRUE,	// distracted
	FALSE,	// hard-stunned (not 'bad' because it's uncleansable)
	FALSE,	// 35 - immune-damage
	FALSE,	// no-where
	FALSE,	// waterwalking
	FALSE,	// light
	TRUE,	// poor-regens
	TRUE,	// 40 - slower-actions
	TRUE,	// hungrier
	TRUE,	// thirstier
	FALSE,	// immune-temperature
	FALSE,	// auto-resurrect
	FALSE,	// 45 - counterspell
	FALSE,	// !disarm
};


// for prompt and diagnosis -- 0-10 relates to 0-100% health
const char *health_levels[] = {
	"nearly dead",	// 0
	"crippled",
	"mauled",
	"wounded",
	"wounded",
	"injured",	// 5
	"injured",
	"hurt",
	"hurt",
	"bruised",
	"healthy",	// 10
	"\n"
};


// for prompt -- 0-10 relates to 0-100% move
const char *move_levels[] = {
	"exhausted",	// 0
	"exhausted",
	"tired",
	"slowing",
	"tiring",
	"tiring",		// 5
	"going strong",
	"feeling good",
	"rested",
	"rested",
	"fully rested",	// 10
	"\n"
};


// for prompt -- 0-10 relates to 0-100% mana
const char *mana_levels[] = {
	"no mana",	// 0
	"very low mana",
	"low mana",
	"low mana",
	"half mana",
	"half mana",	// 5
	"half mana",
	"lots of mana",
	"high mana",
	"high mana",
	"full mana",	// 10
	"\n"
};


// for prompt -- 0-10 relates to 0-100% blood
const char *blood_levels[] = {
	"no blood",	// 0
	"extremely blood-starved",
	"blood-starved",
	"low blood",
	"half blood",
	"half blood",	// 5
	"half blood",
	"lots of blood",
	"high blood",
	"high blood",
	"satiated",	// 10
	"\n"
};


/* SEX_x */
const char *genders[] = {
	"neutral",
	"male",
	"female",
	"\n"
};


/* POS_x (1/2) */
const char *position_types[] = {
	"Dead",	// 0
	"Mortally wounded",
	"Incapacitated",
	"Stunned",
	"Sleeping",
	"Resting",	// 5
	"Sitting",
	"Fighting",
	"Standing",
	"\n"
};


// POS_x (2/2): command to reach that state
const char *position_commands[] = {
	"",	// 0, "Dead",
	"",	// "Mortally wounded",
	"",	// "Incapacitated",
	"",	// "Stunned",
	"sleep",
	"rest",	// 5
	"sit",
	"",	// "Fighting",
	"stand",
	"\n"
};


// POS_x
const int regen_by_pos[] = {
	0,	// 0, dead
	0,	// mortally wounded
	0,	// incapacitated
	1,	// stunned
	4,	// sleep
	3,	// 5, rest
	2,	// sit
	1,	// fighting
	1	// standing
};


/* INJ_x */
const char *injury_bits[] = {
	"TIED",
	"STAKED",
	"\n"
};


// APPLY_TYPE_x (1/2): source of obj apply
const char *apply_type_names[] = {
	"natural",	// 0
	"enchantment",
	"honed",
	"superior",
	"hard-drop",
	"group-drop",	// 5
	"boss-drop",
	"\n"
};


// APPLY_TYPE_x (2/2): preserve ones set by players through fresh_copy_obj
const bool apply_type_from_player[] = {
	FALSE,	// 0, natural
	TRUE,	// enchantment
	TRUE,	// honed
	FALSE,	// superior -- will be applied by the scaler
	FALSE,	// hard-drop
	FALSE,	// 5, group-drop
	FALSE,	// boss-drop
};


/* APPLY_x (1/4) */
const char *apply_types[] = {
	"NONE",	// 0
	"STRENGTH",
	"DEXTERITY",
	"HEALTH-REGEN",
	"CHARISMA",
	"GREATNESS",	// 5
	"MOVE-REGEN",
	"MANA-REGEN",
	"INTELLIGENCE",
	"WITS",
	"AGE",	// 10
	"MAX-MOVE",
	"RESIST-PHYSICAL",
	"BLOCK",
	"HEAL-OVER-TIME",
	"MAX-HEALTH",	// 15
	"MAX-MANA",
	"TO-HIT",
	"DODGE",
	"INVENTORY",
	"MAX-BLOOD",	// 20
	"BONUS-PHYSICAL",
	"BONUS-MAGICAL",
	"BONUS-HEALING",
	"RESIST-MAGICAL",
	"CRAFTING",	// 25
	"BLOOD-UPKEEP",
	"NIGHT-VISION",
	"NEARBY-RANGE",
	"WHERE-RANGE",
	"WARMTH",	// 30
	"COOLING",
	"\n"
};


// APPLY_x (2/4) -- for rate_item (amount multiplied by the apply modifier to make each of these equal to 1)
const double apply_values[] = {
	0.01,	// 0 - NONE
	1,		// STRENGTH
	1,		// DEXTERITY
	0.5,	// HEALTH-REGEN
	1,		// CHARISMA
	1,		// 5 - GREATNESS
	0.5,	// MOVE-REGEN
	0.5,	// MANA-REGEN
	1,		// INTELLIGENCE
	1,		// WITS
	0.1,	// 10 - AGE
	0.025,	// MAX-MOVE
	0.3,	// RESIST-PHYSICAL
	0.25,	// BLOCK
	0.25,	// HEAL-OVER-TIME
	0.020,	// 15, HEALTH
	0.025,	// MAX-MANA
	0.15,	// TO-HIT
	0.10,	// DODGE
	0.15,	// INVENTORY
	0.02,	// 20, BLOOD
	1,		// BONUS-PHYSICAL
	1,		// BONUS-MAGICAL
	1,		// BONUS-HEALING
	0.3,	// RESIST-MAGICAL
	0.01,	// 25, CRAFTING
	1,		// BLOOD-UPKEEP
	1,		// NIGHT-VISION
	1,		// NEARBY-RANGE
	1,		// WHERE-RANGE
	1,		// 30, WARMTH
	1,		// COOLING
};


// APPLY_x (3/4) applies that are directly tied to attributes
const int apply_attribute[] = {
	NOTHING,	// 0
	STRENGTH,
	DEXTERITY,
	NOTHING,	// health-regen
	CHARISMA,
	GREATNESS,	// 5
	NOTHING,	// move-regen
	NOTHING,	// mana-regen
	INTELLIGENCE,
	WITS,
	NOTHING,	// 10, age
	NOTHING,	// max-move
	NOTHING,	// resist-physical
	NOTHING,	// block
	NOTHING,	// heal-over-time
	NOTHING,	// 15, max-health
	NOTHING,	// max-mana
	NOTHING,	// to-hit
	NOTHING,	// dodge
	NOTHING,	// inv
	NOTHING,	// 20, blood
	NOTHING,	// bonus-phys
	NOTHING,	// bonus-mag
	NOTHING,	// bonus-heal
	NOTHING,	// resist-magical
	NOTHING,	// 25,crafting
	NOTHING,	// blood-upkeep
	NOTHING,	// night-vision
	NOTHING,	// nearby-range
	NOTHING,	// where-range
	NOTHING,	// 30, warmth
	NOTHING,	// cooling
};


// APPLY_x (4/4) if TRUE, this apply is not scaled (applied as-is)
const bool apply_never_scales[] = {
	FALSE,	// 0, NONE
	FALSE,	// STRENGTH
	FALSE,	// DEXTERITY
	FALSE,	// HEALTH-REGEN
	FALSE,	// CHARISMA
	TRUE,	// 5, GREATNESS
	FALSE,	// MOVE-REGEN
	FALSE,	// MANA-REGEN
	FALSE,	// INTELLIGENCE
	FALSE,	// WITS
	TRUE,	// 10, AGE
	FALSE,	// MAX-MOVE
	FALSE,	// ESIST-PHYSICAL
	FALSE,	// BLOCK
	FALSE,	// HEAL-OVER-TIME
	FALSE,	// 15, HEALTH
	FALSE,	// MAX-MANA
	FALSE,	// TO-HIT
	FALSE,	// DODGE
	FALSE,	// INVENTORY
	FALSE,	// 20, BLOOD
	FALSE,	// BONUS-PHYSICAL
	FALSE,	// BONUS-MAGICAL
	FALSE,	// BONUS-HEALING
	FALSE,	// RESIST-MAGICAL
	TRUE,	// 25, CRAFTING
	TRUE,	// BLOOD-UPKEEP
	TRUE,	// NIGHT-VISION
	TRUE,	// NEARBY-RANGE
	TRUE,	// WHERE-RANGE
	TRUE,	// 30, WARMTH
	TRUE,	// COOLING
};


// STRENGTH, etc (part 1)
const struct attribute_data_type attributes[NUM_ATTRIBUTES] = {
	// Label, Description, low-stat error
	{ "Strength", "Strength improves your melee damage and lets you chop trees faster", "too weak" },
	{ "Dexterity", "Dexterity helps you hit opponents and dodge hits", "not agile enough" },
	{ "Charisma", "Charisma improves your success with Stealth abilities", "not charming enough" },
	{ "Greatness", "Greatness determines how much territory your empire can claim", "not great enough" },
	{ "Intelligence", "Intelligence improves your magical damage and healing", "not clever enough" },
	{ "Wits", "Wits improves your speed and effectiveness in combat", "too slow" }
};

// STRENGTH, etc (part 2)
const int attribute_display_order[NUM_ATTRIBUTES] = {
	STRENGTH, CHARISMA, INTELLIGENCE,
	DEXTERITY, GREATNESS, WITS
};


// NUM_POOLS, HEALTH, MOVES, MANA, BLOOD (any other search terms?)
const char *pool_types[] = {
	"health",
	"move",
	"mana",
	"blood",
	"\n"
};


// NUM_POOLS, HEALTH, MOVES, MANA, BLOOD (any other search terms?)
const char *pool_abbrevs[] = {
	"H",	// health
	"V",	// move
	"M",	// mana
	"B",	// blood
	"\n"
};


// SIZE_x (1/2): character size, strings
const char *size_types[] = {
	"negligible",	// 0
	"tiny",
	"small",
	"normal",
	"large",
	"huge",	// 5
	"enormous",
	"\n"
};


// SIZE_x (1/2): character size, data
const struct character_size_data size_data[] = {
	// max_blood, corpse_flags, can_take_corpse, show_on_map, corpse kws, corpse longdesc, body longdesc, show on look
	/* negligible */{ 100, NOBITS, TRUE, FALSE, "", "%s's corpse is festering on the ground.", "%s's body is lying here.", NULL },
	/* tiny */		{ 10, NOBITS, TRUE, FALSE, "tiny", "The tiny corpse of %s is rotting on the ground.", "%s's tiny body is lying here.", "$E is tiny!" },
	/* small */		{ 25, NOBITS, TRUE, FALSE, "", "%s's corpse is festering on the ground.", "%s's body is lying here.", "$E is small." },
	/* normal/human */	{ 100, OBJ_LARGE, TRUE, FALSE, "", "%s's corpse is festering on the ground.", "%s's body is lying here.", NULL },
	/* large */		{ 150, OBJ_LARGE, TRUE, FALSE, "large", "The large corpse of %s is rotting on the ground.", "%s's large body is lying here, rotting.", "$E is large." },
	/* huge */		{ 200, OBJ_LARGE, FALSE, FALSE, "huge", "The huge corpse of %s is festering here.", "The huge body of %s body is festering here.", "$E is huge!" },
	/* enormous */	{ 300, OBJ_LARGE, FALSE, TRUE, "enormous", "The enormous corpse of %s is rotting here.", "%s's enormous body is rotting here.", "$E is enormous!" },
};


 //////////////////////////////////////////////////////////////////////////////
//// CRAFT RECIPE CONSTANTS //////////////////////////////////////////////////

// CRAFT_x (1/2): flag names
const char *craft_flags[] = {
	"POTTERY",	// 0
	"BUILDING",
	"SKILLED-LABOR",
	"SKIP-CONSUMES-TO",
	"DARK-OK",
	"*",	// 5: formerly alchemy (identical to FIRE)
	"*",	// formerly sharp-tool
	"FIRE",
	"SOUP",
	"IN-DEVELOPMENT",
	"UPGRADE",	// 10
	"DISMANTLE-ONLY",
	"IN-CITY-ONLY",
	"VEHICLE",
	"*",	// formerly shipyard (now uses a function)
	"*",	// 15: formerly bld-upgraded (now uses a function)
	"LEARNED",
	"BY-RIVER",
	"REMOVE-PRODUCTION",
	"TAKE-REQUIRED-OBJ",
	"DISMANTLE-WITHOUT-ABILITY",	// 20
	"TOOL-OR-FUNCTION",
	"UNDAMAGED-DISMANTLE-REFUND",
	"FULL-DISMANTLE-REFUND",
	"NO-BUILDCHECK",
	"\n"
};


// CRAFT_x (2/2): how flags that show up on "craft info"
const char *craft_flag_for_info[] = {
	"pottery",	// 0
	"",	// building
	"",	// skilled labor
	"",	// skip-consumes-to
	"can be made in the dark",
	"",	// 5
	"",
	"requires fire",
	"",	// soup
	"",	// in-dev
	"is an upgrade",	// 10: upgrade
	"",	// dismantle-only
	"in-city only",
	"",	// vehicle
	"",
	"",	// 15
	"",	// learned
	"must be by a river",
	"",	// remove-production
	"",	// take-required-obj
	"", // 20: dismantle-without-ability
	"",	// tool-or-function
	"",	// undamaged-dismantle-refund
	"",	// full-dismantle-refund
	"",	// no-buildcheck
	"\n"
};


// CRAFT_TYPE_x
const char *craft_types[] = {
	"UNDEFINED",	// 0
	"FORGE",
	"CRAFT",
	"COOK",
	"SEW",
	"MILL",	// 5
	"BREW",
	"MIX",
	"BUILD",
	"WEAVE",
	"WORKFORCE",	// 10
	"MANUFACTURE",
	"SMELT",
	"PRESS",
	"BAKE",
	"MAKE",	// 15
	"PROCESS",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// EMPIRE CONSTANTS ////////////////////////////////////////////////////////

// name, icon, radius, show-to-others, is-capital
const struct city_metadata_type city_type[] = {
	{ "outpost", "&0-&?C1&0-", 5, FALSE, FALSE },
	{ "village", "&0-&?C2&0-", 10, TRUE, FALSE },
	{ "city", "&0-&?C3&0-", 15, TRUE, FALSE },
	{ "capital", "&0-&?C4&0-", 25, TRUE, TRUE },

	// this must go last
	{ "\n", "\n", 0, FALSE, FALSE }
};


// DIPL_x (1/2): Diplomacy types
const char *diplomacy_flags[] = {
	"peace",	// 0
	"war",
	"thievery",
	"allied",
	"nonaggression",
	"trade",	// 5
	"distrust",
	"truce",
	"\n"
};


// DIPL_x (2/2): Types considered better than a given status (for REQ_DIPLOMACY_OVER)
// These must start with the flag itself (it is considered greater than or equal to itself).
// These should include anything considered an upgraded version of the flag.
const bitvector_t diplomacy_better_list[] = {
	DIPL_PEACE | DIPL_NONAGGR | DIPL_ALLIED,	// 0 - PEACE
	DIPL_WAR,	// WAR
	DIPL_THIEVERY | DIPL_WAR,	// THIEVERY
	DIPL_ALLIED,	// ALLIED
	DIPL_NONAGGR | DIPL_ALLIED,	// NONAGGR
	DIPL_TRADE,	// 5 - TRADE
	DIPL_DISTRUST | DIPL_WAR,	// DISTRUST
	DIPL_TRUCE | DIPL_PEACE | DIPL_NONAGGR | DIPL_ALLIED,	// TRUCE
};


// OFFENSE_x: offense definitions
// note: weights are in relation to the offense_min_to_war and offenses_for_free_war
const struct offense_info_type offense_info[NUM_OFFENSES] = {
	// name, weight
	{ "stealing", 15 },	// 0
	{ "attacked player", 5 },
	{ "guard tower", 1 },
	{ "killed player", 15 },
	{ "infiltrated", 2 },
	{ "attacked npc", 1 },	// 5
	{ "sieged building", 15 },
	{ "sieged vehicle", 15 },
	{ "burned building", 5 },
	{ "burned vehicle", 5 },
	{ "pickpocketed", 5 },	// 10
	{ "reclaimed a tile", 2 },
	{ "burned tile", 5 }
};


// ELOG_x (1/3)
const char *empire_log_types[] = {
	"None",	// 0
	"Admin",
	"Diplomacy",
	"Hostility",
	"Members",
	"Territory",	// 5
	"Trade",
	"Logins",
	"Shipping",
	"Workforce",
	"Progress",	// 10
	"Alert",
	"\n"
};


// ELOG_x (2/3): Whether or not logs are shown to players online
const bool show_empire_log_type[] = {
	TRUE,	// 0, none
	TRUE,	// admin
	TRUE,	// diplo
	TRUE,	// hostility
	TRUE,	// members
	TRUE,	// 5, territory
	FALSE,	// trade
	TRUE,	// logins
	FALSE,	// shipments
	FALSE,	// workforce
	TRUE,	// 10, progress
	TRUE,	// alert
};


// ELOG_x (3/3): Whether or not logs are shown on the base 'elog' command
const bool empire_log_request_only[] = {
	FALSE,	// 0, none
	FALSE,	// admin
	FALSE,	// diplo
	FALSE,	// hostility
	FALSE,	// members
	FALSE,	// 5, territory
	FALSE,	// trade
	TRUE,	// logins
	FALSE,	// shipments
	TRUE,	// workforce
	FALSE,	// 10, progress
	FALSE,	// alert
};


// EADM_x: empire admin flags
const char *empire_admin_flags[] = {
	"!WAR",
	"!STEAL",
	"CITY-CLAIMS-ONLY",
	"!RENAME",
	"DID-NEWBIE-MOVE",
	"\n"
};


// EATT_x: empire attributes
const char *empire_attributes[] = {
	"Progress Pool",	// 0
	"Bonus City Points",
	"Max City Size",
	"Tty per 100 Wealth",
	"Tty per Greatness",
	"Workforce Cap",	// 5
	"Bonus Territory",
	"Default Keep",
	"\n"
};


// ENEED_x: empire need types
const char *empire_needs_types[] = {
	"food for workforce",
	"\n"
};


// ENEED_STATUS_x: empire statuses
const char *empire_needs_status[] = {
	"UNSUPPLIED",
	"\n"
};


// EUS_x -- lowercase
const char *unique_storage_flags[] = {
	"vault",
	"\n"
};


// OFF_x: offense flags
const char *offense_flags[] = {
	"SEEN",
	"WAR",
	"AVENGED",
	"\n"
};


// TECH_x: empire techs
const char *empire_tech_types[] = {
	"*",	// 0
	"City Lights",
	"Locks",
	"*",
	"Seaport",
	"Workforce",	// 5
	"Prominence",
	"Citizens",
	"Portals",
	"Master Portals",
	"Skilled Labor",	// 10
	"Trade Routes",
	"Workforce Prospecting",
	"Deep Mines",
	"Rare Metals",
	"Bonus Experience",	// 15
	"Tunnels",
	"Fast Prospect",
	"Fast Excavate",
	"Hidden Progress",
	"\n"
};


// ETRAIT_x
const char *empire_trait_types[] = {
	"Distrustful",
	"\n"
};


// words that can be filtered out of the beginning of empire names
const char *empire_words[] = {
	"alliance",
	"association",
	"coalition",
	"commune",
	"communes",
	"community",
	"communities",
	"confederation",
	"empire",
	"empires",
	"federation",
	"fiefdom",
	"fiefdoms",
	"kingdom",
	"kingdoms",
	"nation",
	"nations",
	"people",
	"peoples",
	"province",
	"provinces",
	"republic",
	"society",
	"societies",
	"state",
	"states",
	"territory",
	"territories",
	"tribal",
	"tribe",
	"tribes",
	"union",
	"unions",
	"united",
	"village",
	"villages",
	"\n"	// must have terminator
};


// PRIV_x
const char *priv[] = {
	"claim",	// 0
	"build",
	"harvest",
	"promote",
	"chop",
	"cede",	// 5
	"enroll",
	"withdraw",
	"diplomacy",
	"customize",
	"workforce",	// 10
	"stealth",
	"cities",
	"trade",
	"logs",
	"shipping",	// 15
	"homes",
	"storage",
	"warehouse",
	"progress",
	"dismantle",	// 20
	"\n"
};


// SCORE_x -- score types
const char *score_type[] = {
	"Community",	// 0
	"Defense",
	"Greatness",
	"Industry",
	"Inventory",
	"Members",	// 5
	"Playtime",
	"Prestige",
	"Territory",
	"Wealth",
	"\n"
};


// TRADE_x -- all lowercase
const char *trade_type[] = {
	"export",
	"import",
	"\n"
};

// TRADE_x
const char *trade_mostleast[] = {
	"",	// export
	"at most ",	// import
	"\n"
};

// TRADE_x
const char *trade_overunder[] = {
	"over",	// export
	"under",	// import
	"\n"
};


// WF_PROB_x: Workforce problem logging
const char *wf_problem_types[] = {
	"no workers",	// 0
	"over limit",
	"depleted",
	"no resources",
	"already sheared",
	"delayed",	// 5
	"out of city",
	"adventure present",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// EVENT CONSTANTS /////////////////////////////////////////////////////////

// EVT_x: event types
const char *event_types[] = {
};


// EVTF_x: event flags
const char *event_flags[] = {
	"IN-DEVELOPMENT",	// 0
	"CONTINUES",
	"\n"
};


// EVTS_x: event status
const char *event_status[] = {
	"not started",
	"running",
	"complete",
	"collected",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// FACTION CONSTANTS ///////////////////////////////////////////////////////

// FCT_x: faction flags
const char *faction_flags[] = {
	"IN-DEVELOPMENT",
	"REP-FROM-KILLS",
	"HIDE-IN-LIST",
	"HIDE-ON-MOB",
	"\n"
};


// FCTR_x (1/2): relationship flags
const char *relationship_flags[] = {
	"SHARED-GAINS",
	"INVERSE-GAINS",
	"MUTUALLY-EXCLUSIVE",
	"UNLISTED",
	"\n"
};


// FCTR_x (2/2): relationship descriptions (shown to players)
const char *relationship_descs[] = {
	"Allied",
	"Enemies",
	"Mutually Exclusive",
	"",	// unlisted
	"\n"
};


// REP_x: faction reputation levels
const struct faction_reputation_type reputation_levels[] = {
	// { type const, name, by/to, color, points to achieve this level } -> ASCENDING ORDER
	// note: you achieve the level when you reach the absolute value of its
	// points (-99 < x < 99 is neutral, +/-100 are the cutoffs for the first rank)
	
	{ REP_DESPISED, "Despised", "by", "\tr", -1000 },
	{ REP_HATED, "Hated", "by", "\tr", -750 },
	{ REP_LOATHED, "Loathed", "by", "\to", -300 },
	{ REP_DISLIKED, "Disliked", "by", "\ty", -100 },
	{ REP_NEUTRAL, "Neutral", "to", "\tt", 0 },
	{ REP_LIKED, "Liked", "by", "\tc", 100 },
	{ REP_ESTEEMED, "Esteemed", "by", "\ta", 300 },
	{ REP_VENERATED, "Venerated", "by", "\tg", 750 },
	{ REP_REVERED, "Revered", "by", "\tG", 1000 },
	
	{ -1, "\n", "\t0", 0 },	// last
};


 //////////////////////////////////////////////////////////////////////////////
//// GENERIC CONSTANTS ///////////////////////////////////////////////////////

// GENERIC_x (1/2): generic types
const char *generic_types[] = {
	"UNKNOWN",	// 0
	"LIQUID",
	"ACTION",
	"COOLDOWN",
	"AFFECT",
	"CURRENCY",	// 5
	"COMPONENT",
	"MOON",
	"LANGUAGE",
	"\n"
};


// GENERIC_x (2/2): generic types that are affected by in-development
const bool generic_types_uses_in_dev[] = {
	FALSE,	// UNKNOWN	// 0
	FALSE,	// LIQUID
	FALSE,	// ACTION
	FALSE,	// COOLDOWN
	FALSE,	// AFFECT
	FALSE,	// CURRENCY	// 5
	FALSE,	// COMPONENT
	TRUE,	// MOON
	TRUE,	// LANGUAGE
};


// GEN_x: generic flags
const char *generic_flags[] = {
	"BASIC",	// 0
	"IN-DEVELOPMENT",
	"SHOW-ADVENTURE",
	"\n"
};


// LANG_x: how well someone speaks a language
const char *language_types[] = {
	"unknown",	// 0
	"recognizes",
	"speaks",
	"\n"
};


// LIQF_x: liquid flags for generics
const char *liquid_flags[] = {
	"WATER",	// 0
	"COOLING",
	"WARMING",
	"BLOOD",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// MOB CONSTANTS ///////////////////////////////////////////////////////////

/* MOB_x */
const char *action_bits[] = {
	"BRING-A-FRIEND",	// 0
	"SENTINEL",
	"AGGR",
	"ISNPC",
	"MOUNTABLE",
	"MILKABLE",	// 5
	"SCAVENGER",
	"NO-CORPSE",
	"TIED",
	"ANIMAL",
	"MOUNTAIN-WALK",	// 10
	"AQUATIC",
	"*PLURAL",
	"NO-ATTACK",
	"SPAWNED",
	"CHAMPION",	// 15
	"EMPIRE",
	"*",
	"*PICKPOCKETED",
	"CITYGUARD",
	"PURSUE",	// 20
	"HUMAN",
	"VAMPIRE",
	"CASTER",
	"TANK",
	"DPS",	// 25
	"HARD",
	"GROUP",
	"*EXTRACTED",
	"!LOOT",
	"!TELEPORT",	// 30
	"!EXP",
	"!RESCALE",
	"SILENT",
	"COINS",
	"NO-COMMAND",	// 35
	"NO-UNCONSCIOUS",
	"IMPORTANT",
	"AVOID-BUILDINGS",
	"\n"
};


// MOB_CUSTOM_x (1/2): type names
const char *mob_custom_types[] = {
	"echo",	// 0
	"say",
	"say-day",
	"say-night",
	"echo-day",
	"echo-night",	// 5
	"long-desc",
	"script1",
	"script2",
	"script3",
	"script4",	// 10
	"script5",
	"scavenge-corpse",
	"\n"
};


// MOB_CUSTOM_x (2/2): how they're shown in olc
const char *mob_custom_type_help =
"Types of messages:\r\n"
"&w Echo messages: echo echo-day echo-night&0\r\n"
" Speak text: say say-day say-night\r\n"
"&w Varying lond descriptions: long-desc&0\r\n"
" Shown when a scavenger mob eats a corpse: scavenge-corpse\r\n"
"&w Available for scripts: script1 script2 script3 script4 script5&0\r\n";


// MOB_MOVE_x: mob/vehicle move types
const char *mob_move_types[] = {
	"walks",
	"climbs",	"flies",	"paddles",	"rides",	"slithers",	// 1 - 5
	"swims",	"scurries",	"skitters",	"creeps",	"oozes",	// 6 - 10
	"runs",	"gallops",	"shambles",	"trots",	"hops",			// 11 - 15
	"waddles",	"crawls",	"flutters",	"drives",	"sails",	// 16 - 20
	"rolls",	"rattles",	"skis",	"slides",	"soars",		// 21 - 25
	"lumbers",	"floats",	"lopes",	"blows",	"drifts",	// 26 - 30
	"bounces",	"flows",	"leaves",	"shuffles",	"marches",	// 31 - 35
	"sweeps",	"barges",	"bolts",	"charges",	"clambers",	// 36 - 40
	"coasts",	"darts",	"dashes",	"draws",	"flits",	// 41 - 45
	"glides",	"goes",	"hikes",	"hobbles",	"hurries",		// 46 - 50
	"inches",	"jogs",	"journeys",	"jumps",	"leaps",		// 51 - 55
	"limps",	"lurches",	"meanders",	"moseys",	"parades",	// 56 - 60
	"plods",	"prances",	"prowls",	"races",	"roams",	// 61 - 65
	"romps",	"roves",	"rushes",	"sashays",	"saunters",	// 66 - 70
	"scampers",	"scoots",	"scrambles",	"scutters",	"sidles",	// 71 - 75
	"skips",	"skulks",	"sleepwalks",	"slinks",	"slogs",	// 76 - 80
	"sneaks",	"staggers",	"stomps",	"streaks",	"strides",	// 81 - 85
	"strolls",	"struts",	"stumbles",	"swims",	"tacks",	// 86 - 90
	"tears",	"tiptoes",	"toddles",	"totters",	"traipses",	// 91 - 95
	"tramps",	"travels",	"treks",	"trudges",	"vaults",	// 96 - 100
	"wades",	"wanders",	"whizzes",	"zigzags",	"zooms",	// 101 - 105
	"\n"
};


// NAMES_x
const char *name_sets[] = {
	"Citizens",	// 0
	"Country-folk",
	"Roman",
	"Northern",
	"Primitive-Short",
	"Descriptive",	// 5
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// MOON CONSTANTS //////////////////////////////////////////////////////////

// PHASE_x (1/3): moon phases (short names)
const char *moon_phases[] = {
	"new",	// 0
	"waxing crescent",
	"first quarter",
	"waxing gibbous",
	"full",
	"waning gibbous",	// 5
	"third quarter",
	"waning crescent",
	"\n"
};


// PHASE_x (2/3): moon phases	-- shown as "<moon name> is <moon_phases_long>, <moon_positions>."
const char *moon_phases_long[] = {
	"a new moon",	// 0
	"a waxing crescent",
	"in the first quarter",
	"a waxing gibbous",
	"full",
	"a waning gibbous",	// 5
	"in the third quarter",
	"a waning crescent",
	"\n"
};


// PHASE_x (2/3): moon phase brightness (base distance you can see in the dark)
const int moon_phase_brightness[NUM_PHASES] = {
	1,	// 0, new
	2,	// crescent
	3,	// quarter
	3,	// gibbous
	4,	// full
	3,	// 5, gibbous
	3,	// quarter
	2	// crescent
};


// MOON_POS_x: moon position in the sky	-- shown as "<moon name> is <moon_phases_long>, <moon_positions>."
const char *moon_positions[] = {
	"down",	// 0
	"low in the east",
	"high in the east",
	"overhead",
	"high in the west",
	"low in the west",	// 5
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// ITEM CONSTANTS //////////////////////////////////////////////////////////

// WEAR_x -- keyword to target a pos (for search_block); also see wear_data (below)
const char *wear_keywords[] = {
	"head",	// 0
	"ears",
	"neck",
	"\r!",	// neck 2
	"clothes",
	"armor",	// 5
	"about",
	"arms",
	"wrists",
	"hands",
	"finger",	// 10
	"\r!",	// finger 2
	"waist",
	"legs",
	"feet",
	"pack",	// 15
	"saddle",
	"\r!",	// sheathed 1
	"\r!",	// sheathed 2
	"\r!",	// wield
	"\r!",	// 20, ranged
	"\r!",	// hold
	"\r!",	// tool
	"\r!",	// share
	"\n"
};


// WEAR_x -- data for each wear slot
const struct wear_data_type wear_data[NUM_WEARS] = {
	// eq tag,				 name, wear bit,       count-stats, gear-level-mod, cascade-pos, already-wearing, wear-message-to-room, wear-message-to-char, allow-custom-msgs, save-to-eq-set
	{ "    <worn on head> ", "head", ITEM_WEAR_HEAD, TRUE, 1.0, NO_WEAR, "You're already wearing $p on your head.", "$n wears $p on $s head.", "You wear $p on your head.", TRUE, TRUE },
	{ "    <worn on ears> ", "ears", ITEM_WEAR_EARS, TRUE, 1.0, NO_WEAR, "You're already wearing $p on your ears.", "$n pins $p onto $s ears.", "You pin $p onto your ears.", TRUE, TRUE },
	{ "<worn around neck> ", "neck", ITEM_WEAR_NECK, TRUE, 1.0, WEAR_NECK_2, "YOU SHOULD NEVER SEE THIS MESSAGE. PLEASE REPORT.", "$n wears $p around $s neck.", "You wear $p around your neck.", TRUE, TRUE },
	{ "<worn around neck> ", "neck", ITEM_WEAR_NECK, TRUE, 1.0, NO_WEAR, "You're already wearing enough around your neck.", "$n wears $p around $s neck.", "You wear $p around your neck.", TRUE, TRUE },
	{ " <worn as clothes> ", "clothes", ITEM_WEAR_CLOTHES, TRUE, 0, NO_WEAR, "You're already wearing $p as clothes.", "$n wears $p as clothing.", "You wear $p as clothing.", TRUE, TRUE },
	{ "   <worn as armor> ", "armor", ITEM_WEAR_ARMOR, TRUE, 2.0, NO_WEAR, "You're already wearing $p as armor.", "$n wears $p as armor.", "You wear $p as armor.", TRUE, TRUE },
	{ " <worn about body> ", "about", ITEM_WEAR_ABOUT, TRUE, 1.0, NO_WEAR, "You're already wearing $p about your body.", "$n wears $p about $s body.", "You wear $p around your body.", TRUE, TRUE },
	{ "    <worn on arms> ", "arms", ITEM_WEAR_ARMS, TRUE, 1.0, NO_WEAR, "You're already wearing $p on your arms.", "$n wears $p on $s arms.", "You wear $p on your arms.", TRUE, TRUE },
	{ "  <worn on wrists> ", "wrists", ITEM_WEAR_WRISTS, TRUE, 1.0, NO_WEAR, "You're already wearing $p on your wrists.", "$n wears $p on $s wrists.", "You wear $p on your wrists.", TRUE, TRUE },
	{ "   <worn on hands> ", "hands", ITEM_WEAR_HANDS, TRUE, 1.0, NO_WEAR, "You're already wearing $p on your hands.", "$n puts $p on $s hands.", "You put $p on your hands.", TRUE, TRUE },
	{ "  <worn on finger> ", "finger", ITEM_WEAR_FINGER, TRUE, 1.0, WEAR_FINGER_L, "YOU SHOULD NEVER SEE THIS MESSAGE. PLEASE REPORT.", "$n slides $p on to $s right ring finger.", "You slide $p on to your right ring finger.", TRUE, TRUE },
	{ "  <worn on finger> ", "finger", ITEM_WEAR_FINGER, TRUE, 1.0, NO_WEAR, "You're already wearing something on both of your ring fingers.", "$n slides $p on to $s left ring finger.", "You slide $p on to your left ring finger.", TRUE, TRUE },
	{ "<worn about waist> ", "waist", ITEM_WEAR_WAIST, TRUE, 1.0, NO_WEAR, "You already have $p around your waist.", "$n wears $p around $s waist.", "You wear $p around your waist.", TRUE, TRUE },
	{ "    <worn on legs> ", "legs", ITEM_WEAR_LEGS, TRUE, 1.0, NO_WEAR, "You're already wearing $p on your legs.", "$n puts $p on $s legs.", "You put $p on your legs.", TRUE, TRUE },
	{ "    <worn on feet> ", "feet", ITEM_WEAR_FEET, TRUE, 1.0, NO_WEAR, "You're already wearing $p on your feet.", "$n wears $p on $s feet.", "You wear $p on your feet.", TRUE, TRUE },
	{ " <carried as pack> ", "pack", ITEM_WEAR_PACK, TRUE, 0.5, NO_WEAR, "You're already using $p.", "$n starts using $p.", "You start using $p.", TRUE, TRUE },
	{ "  <used as saddle> ", "saddle", ITEM_WEAR_SADDLE, TRUE, 0, NO_WEAR, "You're already using $p.", "$n start using $p.", "You start using $p.", TRUE, FALSE },
	{ "          (sheath) ", "sheath", ITEM_WEAR_WIELD, FALSE, 0, WEAR_SHEATH_2, "You've already got something sheathed.", "$n sheathes $p.", "You sheathe $p.", FALSE, TRUE },
	{ "          (sheath) ", "sheath", ITEM_WEAR_WIELD, FALSE, 0, NO_WEAR, "You've already got something sheathed.", "$n sheathes $p.", "You sheathe $p.", FALSE, TRUE },
	{ "         <wielded> ", "wield", ITEM_WEAR_WIELD, 	TRUE, 2.0, NO_WEAR, "You're already wielding $p.", "$n wields $p.", "You wield $p.", TRUE, TRUE },
	{ "          <ranged> ", "ranged", ITEM_WEAR_RANGED, TRUE, 0, NO_WEAR, "You're already using $p.", "$n uses $p.", "You use $p.", TRUE, TRUE },
	{ "            <held> ", "hold", ITEM_WEAR_HOLD, TRUE, 1.0, NO_WEAR, "You're already holding $p.", "$n grabs $p.", "You grab $p.", TRUE, TRUE },
	{ "            <tool> ", "tool", ITEM_WEAR_TAKE, FALSE, 0, NO_WEAR, "You're already using $p.", "$n equips $p.", "You equip $p.", TRUE, FALSE },
	{ "          (shared) ", "shared", ITEM_WEAR_TAKE, FALSE, 0, NO_WEAR, "You're already sharing $p.", "$n shares $p.", "You share $p.", FALSE, FALSE }
};


/* ITEM_x (ordinal object types) */
const char *item_types[] = {
	"UNDEFINED",	// 0
	"WEAPON",
	"WORN",
	"OTHER",
	"CONTAINER",
	"DRINKCON",	// 5
	"FOOD",
	"RECIPE",
	"PORTAL",
	"*BOARD",
	"CORPSE",	// 10
	"COINS",
	"CURRENCY",
	"PAINT",
	"*MAIL",
	"WEALTH",	// 15
	"*",
	"*",
	"LIGHTER",
	"MINIPET",
	"MISSILE-WEAPON",	// 20
	"AMMO",
	"INSTRUMENT",
	"SHIELD",
	"PACK",
	"POTION",	// 25
	"POISON",
	"ARMOR",
	"BOOK",
	"LIGHT",
	"\n"
};


// ITEM_WEAR_x (wear bitvector) -- also see wear_significance
const char *wear_bits[] = {
	"TAKE",	// 0
	"FINGER",
	"NECK",
	"CLOTHES",
	"HEAD",
	"LEGS",	// 5
	"FEET",
	"HANDS",
	"ARMS",
	"WRISTS",
	"ABOUT",	// 10
	"WAIST",
	"EARS",
	"WIELD",
	"HOLD",
	"RANGED",	// 15
	"ARMOR",
	"PACK",
	"SADDLE",
	"\n"
};


// ITEM_WEAR_x, WEAR_POS_x - position importance, for item scaling
const int wear_significance[] = {
	WEAR_POS_MINOR,	// 0, take
	WEAR_POS_MINOR,	// finger
	WEAR_POS_MINOR,	// neck
	WEAR_POS_MINOR,	// clothes
	WEAR_POS_MINOR,	// head
	WEAR_POS_MINOR,	// 5, legs
	WEAR_POS_MINOR,	// feet
	WEAR_POS_MINOR,	// hands
	WEAR_POS_MINOR,	// arms
	WEAR_POS_MINOR,	// wrists
	WEAR_POS_MINOR,	// 10, about
	WEAR_POS_MINOR,	// waist
	WEAR_POS_MINOR,	// ears
	WEAR_POS_MAJOR,	// wield
	WEAR_POS_MAJOR,	// hold
	WEAR_POS_MAJOR,	// 15, ranged
	WEAR_POS_MAJOR,	// armor
	WEAR_POS_MINOR,	// pack
	WEAR_POS_MAJOR	// saddle
};


// ITEM_WEAR_x -- for each wear flag, the first matching eq pos
const int item_wear_to_wear[] = {
	NO_WEAR,	// 0, take
	WEAR_FINGER_R,
	WEAR_NECK_1,
	WEAR_CLOTHES,
	WEAR_HEAD,
	WEAR_LEGS,	// 5
	WEAR_FEET,
	WEAR_HANDS,
	WEAR_ARMS,
	WEAR_WRISTS,
	WEAR_ABOUT,	// 10
	WEAR_WAIST,
	WEAR_EARS,
	WEAR_WIELD,
	WEAR_HOLD,
	WEAR_RANGED,	// 15
	WEAR_ARMOR,
	WEAR_PACK,
	WEAR_SADDLE
};


// OBJ_x (extra bits), part 1
const char *extra_bits[] = {
	"UNIQUE",	// 0
	"PLANTABLE",
	"LIGHT",
	"SUPERIOR",
	"LARGE",
	"*CREATED",	// 5
	"1-USE",
	"SLOW",
	"FAST",
	"ENCHANTED",
	"JUNK",	// 10
	"CREATABLE",
	"SCALABLE",
	"TWO-HANDED",
	"BOE",
	"BOP",	// 15
	"*",	// formerly STAFF
	"UNCOLLECTED-LOOT",
	"*KEEP",
	"*",	// formerly TOOL-PAN
	"*",	// 20, formerly TOOL-SHOVEL
	"!AUTOSTORE",
	"HARD-DROP",
	"GROUP-DROP",
	"GENERIC-DROP",
	"NO-BASIC-STORAGE",	// 25
	"SEEDED",
	"IMPORTANT",
	"LONG-TIMER-IN-STORAGE",
	"NO-WAREHOUSE",
	"\n"
};


// OBJ_x (extra bits), part 2 -- shown in inventory/equipment list as flags
const char *extra_bits_inv_flags[] = {
	"unique",	// 0, unique
	"",	// plantable
	"",	// light (controlled separately)
	"superior",
	"large",
	"",	// 5, created
	"",	// 1-use
	"",	// slow
	"",	// fast
	"enchanted",
	"",	// 10, junk
	"",	// creatable
	"",	// scalable
	"2h",
	"BoE",
	"BoP",	// 15
	"",	// *
	"",	// uncollected
	"keep",
	"",	// *
	"",	// 20, *
	"",	// !autostore
	"",	// hard-drop
	"",	// group-drop
	"",	// generic-drop
	"",	// 25, no-basic-storage
	"",	// seeded
	"",	// important
	"",	// long-timer-in-storage
	"",	// no-warehouse
	"\n"
};


// OBJ_x (extra bits), part 3 -- the amount a flag modifies scale level by (1.0 = no mod)
const double obj_flag_scaling_bonus[] = {
	1.1,	// 0, OBJ_UNIQUE
	1.0,	// OBJ_PLANTABLE
	1.0,	// OBJ_LIGHT
	1.73,	// OBJ_SUPERIOR
	1.0,	// OBJ_LARGE
	1.0,	// 5, OBJ_CREATED
	1.0,	// OBJ_SINGLE_USE
	1.0,	// OBJ_SLOW (weapon attack speed)
	1.0,	// OBJ_FAST (weapon attack speed)
	1.0,	// OBJ_ENCHANTED
	0.5,	// 10, OBJ_JUNK
	1.0,	// OBJ_CREATABLE
	1.0,	// OBJ_SCALABLE
	1.8,	// OBJ_TWO_HANDED
	1.3,	// OBJ_BIND_ON_EQUIP
	1.4,	// 15, OBJ_BIND_ON_PICKUP
	1.0,	// unused
	1.0,	// OBJ_UNCOLLECTED_LOOT
	1.0,	// OBJ_KEEP
	1.0,	// unused
	1.0,	// 20, unused
	1.0,	// OBJ_NO_AUTOSTORE
	1.2,	// OBJ_HARD_DROP
	1.3333,	// OBJ_GROUP_DROP
	1.0,	// OBJ_GENERIC_DROP
	1.0,	// 25, OBJ_NO_BASIC_STORAGE
	1.0,	// OBJ_SEEDED
	1.0,	// OBJ_IMPORTANT
	1.0,	// OBJ_LONG_TIMER_IN_STORAGE
	1.0,	// OBJ_NO_WAREHOUSE
};


// MAT_x -- name, TRUE if it floats
const struct material_data materials[NUM_MATERIALS] = {
	// name, floats, chance-to-get-from-dismantle, decay-on-char, decay-in-room
	{ "WOOD", TRUE, 50.0, "$p rots away in your hands.", "$p rots away to nothing." },
	{ "ROCK", FALSE, 95.0, "$p crumbles in your hands.", "$p crumbles and disintegrates." },
	{ "IRON", FALSE, 90.0, "$p rusts in your hands.", "$p rusts and disintegrates." },
	{ "SILVER", FALSE, 100.0, "$p cracks and disintegrates in your hands.", "$p cracks and disintegrates." },
	{ "GOLD", FALSE, 100.0, "$p cracks and disintegrates in your hands.", "$p cracks and disintegrates." },
	{ "FLINT", FALSE, 95.0, "$p crumbles in your hands.", "$p crumbles and disintegrates." },
	{ "CLAY", FALSE, 75.0, "$p cracks and disintegrates in your hands.", "$p cracks and disintegrates." },
	{ "FLESH", TRUE, 50.0, "$p decays in your hands.", "A quivering horde of maggots consumes $p." },
	{ "GLASS", FALSE, 50.0, "$p cracks and shatters in your hands.", "$p cracks and shatters." },
	{ "WAX", TRUE, 25.0, "$p melts in your hands and is gone.", "$p melts and is gone." },
	{ "MAGIC", TRUE, 100.0, "$p flickers briefly in your hands, then vanishes with a poof.", "$p flickers briefly, then vanishes with a poof." },
	{ "CLOTH", TRUE, 50.0, "$p rots away in your hands.", "$p rots away to nothing." },
	{ "GEM", FALSE, 100.0, "$p cracks and shatters in your hands.", "$p cracks and shatters." },
	{ "COPPER", FALSE, 100.0, "$p cracks and disintegrates in your hands.", "$p cracks and disintegrates." },
	{ "BONE", TRUE, 50.0, "$p rots away in your hands.", "$p rots away to nothing." },
	{ "HAIR", TRUE, 50.0, "$p rots away in your hands.", "$p rots away to nothing." }
};


// ARMOR_x part 1 - names
const char *armor_types[NUM_ARMOR_TYPES+1] = {
	"mage",
	"light",
	"medium",
	"heavy",
	"\n"
};


// ARMOR_x part 2 - scale values
const double armor_scale_bonus[NUM_ARMOR_TYPES] = {
	1.2,	// mage
	1.2,	// light
	1.2,	// medium
	1.2		// heavy
};


/* CONT_x */
const char *container_bits[] = {
	"CLOSEABLE",
	"CLOSED",
	"\n"
};


// CORPSE_x: Corpse flags
const char *corpse_flags[] = {
	"EATEN",	// 0
	"SKINNED",
	"HUMAN",
	"NO-LOOT"
	"\n"
};


/* level of fullness for drink containers */
const char *fullness[] = {
	"less than half ",
	"about half ",
	"more than half ",
	""
};


// LIGHT_FLAG_x (1/2): flags for ITEM_LIGHT, show to immortals
const char *light_flags[] = {
	"LIGHT-FIRE",	// 0
	"CAN-DOUSE",
	"JUNK-WHEN-EXPIRED",
	"COOKING-FIRE",
	"DESTROY-WHEN-DOUSED",
	"\n"
};


// LIGHT_FLAG_x (2/2): flags for ITEM_LIGHT, shown on identify
const char *light_flags_for_identify[] = {
	"can be used as a lighter when lit",	// 0
	"can be doused",
	"",	// LIGHT_FLAG_JUNK_WHEN_EXPIRED
	"can be used for cooking",
	"lost when doused",
	"\n"
};


// MINT_FLAG_x (1/2): flags for wealth items to control minting
const char *mint_flags[] = {
	"AUTOMINT",
	"NO-MINT",
	"\n"
};


// MINT_FLAG_x (2/2): flags as shown to players on identify
const char *mint_flags_for_identify[] = {
	"automatically minted by Workforce",
	"cannot be minted into coins",
	"\n"
};


// house painting (1/2)
const char *paint_colors[] = {
	"&0",	// 0, none/normal
	"&b",	// blue
	"&r",	// red
	"&y",	// yellow
	"&g",	// green
	"&o",	// 5, orange
	"&v",	// violet
	"&a",	// azure
	"&c",	// Cyan
	"&j",	// Jade
	"&l",	// 10, Lime
	"&m",	// Magenta
	"&p",	// Pink
	"&t",	// Tan
	"&w",	// White
	"\n"
};


// house painting (2/2)
const char *paint_names[] = {
	"none",	// 0
	"Blue",
	"Red",
	"Yellow",
	"Green",
	"Orange",	// 5
	"Violet",
	"Azure",
	"Cyan",
	"Jade",
	"Lime",	// 10
	"Magenta",
	"Pink",
	"Tan",
	"White",
	"\n"
};


// RES_x: resource requirement types
const char *resource_types[] = {
	"object",	// 0
	"component",
	"liquid",
	"coins",
	"pool",
	"action",	// 5
	"currency",
	"tool",
	"\n"
};


// STORAGE_x
const char *storage_bits[] = {
	"WITHDRAW",
	"\n"
};


// TOOL_x: tool flags for objects
const char *tool_flags[] = {
	"axe",	// 0
	"fishing tool",
	"hammer",
	"harvesting tool",
	"knapper",
	"knife",	// 5
	"loom",
	"mining tool",
	"pan",
	"grinding stone",
	"*quarrying tools",	// 10
	"saw",
	"sewing kit",
	"*shears",
	"shovel",
	"staff",	// 15
	"\n"
};


// OBJ_CUSTOM_x (1/2): message types
const char *obj_custom_types[] = {
	"build-to-char",	// 0
	"build-to-room",
	"instrument-to-char",
	"instrument-to-room",
	"consume-to-char",
	"consume-to-room",	// 5
	"craft-to-char",
	"craft-to-room",
	"wear-to-char",
	"wear-to-room",
	"remove-to-char",	// 10
	"remove-to-room",
	"longdesc",
	"longdesc-female",
	"longdesc-male",
	"fish-to-char",	// 15
	"fish-to-room",
	"decays-on-char",
	"decays-in-room",
	"resource-to-char",
	"resource-to-room",	// 20
	"script1",
	"script2",
	"script3",
	"script4",
	"script5",	// 25
	"mine-to-char",
	"mine-to-room",
	"chop-to-char",
	"chop-to-room",
	"enter-portal-to-char",	// 30
	"enter-portal-to-room",
	"exit-portal-to-room",
	"\n"
};


// OBJ_CUSTOM_x (2/2): how they're shown in olc
const char *obj_custom_type_help =
"Types of messages:\r\n"
"&w Changes the player's long description: longdesc longdesc-female longdesc-male&0\r\n"
" Shown when you wear it as equipment: wear-to-char wear-to-room\r\n"
"&w Shown when you remove it from equipment: remove-to-char remove-to-room&0\r\n"
" Shown when using it to build a building: build-to-char build-to-room\r\n"
"&w Shown when using it to build a vehicle: craft-to-char craft-to-room&0\r\n"
" Shown when you eat/drink/quaff/light it: consume-to-char consume-to-room\r\n"
"&w Shown when it decays in your inventory: decays-on-char&0\r\n"
" Shown when it decays on the ground: decays-in-room\r\n"
"&w Shown when gained as a resource (chopped): resource-to-char resource-to-room&0\r\n"
" Shown when using the item to fish: fish-to-char fish-to-room\r\n"
"&w Shown when you use it to mine ore: mine-to-char mine-to-room&0\r\n"
" Shown when you use it to chop trees: chop-to-char chop-to-room\r\n"
"&w Shown in order when playing it: instrument-to-char instrument-to-room&0\r\n"
" Shown when using portals: enter-portal-to-char, enter-portal-to-room,\r\n"
"   exit-portal-to-room\r\n"
"&w Available for scripts: script1 script2 script3 script4 script5&0\r\n";


// basic speed is the theoretical average weapon speed without wits/haste,
// and is used to apply bonus-physical/magical evenly by adjusting for speed
const double basic_speed = 4.0;	// seconds between attacks


// rework: if the first keyword of an item is in this list, it's safe to rename
const char *item_rename_keywords[] = {
	"aegis", "aklys", "amulet", "antlers", "apparatus", "armband", "armbands",
	"armguards", "armor", "axe", "backpack", "bag", "bags", "band", "bands",
	"bangles", "barbette", "battlegear", "baubles", "beads", "bedknob",
	"bells", "belt", "bindings", "blouse", "bodysuit", "bone", "bonnet",
	"book", "boots", "bow", "bracelet", "bracelets", "bracer", "bracers",
	"braces", "breastplate", "breeches", "broom", "buckler", "bulwark",
	"candlestick", "cannon", "cannons", "cap", "cape", "caps", "carapace",
	"cauldron", "censer", "chain", "chains", "chaps", "charm", "chisel",
	"choker", "chopper", "chrysanthemum", "cincture", "circlet", "claws",
	"claymore", "cloak", "clothes", "clothing", "club", "coat", "coif", "coil",
	"collar", "counterweight", "cowl", "crown", "cuffs", "cuirass",
	"cumberbund", "dagger", "doublet", "dress", "earcuff", "earcuffs",
	"earplates", "earring", "earrings", "edge", "effigy", "ensemble", "eye",
	"eyepiece", "falchion", "fang", "fetish", "fiddle", "flail", "fleece",
	"flute", "focus", "footwraps", "garb", "garments", "garter", "gauntlet",
	"gauntlets", "girdle", "glaive", "gloves", "goggles", "gourd", "gown",
	"greatbow", "greatstaff", "greaves", "grimoire", "guard", "guards",
	"hammer", "hand", "handaxe", "handguards", "handwraps", "hat", "hatchet",
	"hauberk", "headdress", "helm", "helmet", "hides", "hole", "hood", "hoop",
	"horn", "hose", "idol", "jacket", "jerkin", "kilt", "knapsack", "kneecap",
	"knife", "kris", "lantern", "leathers", "leggings", "legguards",
	"legplates", "locket", "loincloth", "longsword", "lyre", "mace", "mantle",
	"mask", "mattock", "maul", "moccasins", "necklace", "net", "orb", "outfit",
	"pack", "pan", "pantaloons", "pants", "pauldrons", "pearls", "pelts",
	"pendant", "pendulum", "pestle", "pick", "pickaxe", "pike", "pipes",
	"plate", "plates", "pocket", "post", "pouch", "raiment", "raiments",
	"regalia", "ring", "robe", "robes", "rucksack", "sabatons", "sack",
	"saddle", "sandals", "sash", "satchel", "scepter", "sceptre", "scythe",
	"shackles", "shawl", "shield", "shinguards", "shirt", "shiv", "shoe",
	"shoes", "shoestring", "shovel", "sickle", "sigil", "skean", "skin",
	"skirt", "skull", "skullcap", "sleeves", "slippers", "spear", "spellbook",
	"spike", "staff", "stake", "statuette", "stick", "stiletto", "stockings",
	"strands", "strap", "stud", "studs", "suit", "surcoat", "sweater", "sword",
	"talisman", "talon", "tambourine", "timepiece", "toga", "tome", "torch",
	"totem", "trappings", "trident", "trousers", "tuba", "tunic", "tusk",
	"vambraces", "vest", "waistband", "waistcoat", "wand", "warhammer",
	"warstaff", "watch", "weapon", "whip", "wimple", "wings", "wraps",
	"wristband", "wristbands", "wristblade", "wristcuff", "wristguard",
	"wristguards", "wristplate", "wristplates", "xylophone",
	"\n"	// this must be last
};


 //////////////////////////////////////////////////////////////////////////////
//// OLC CONSTANTS ///////////////////////////////////////////////////////////

// OLC_FLAG_x
const char *olc_flag_bits[] = {
	"ALL-VNUMS",	// 0
	"MAP-EDIT",
	"CLEAR-IN-DEV",
	"!CRAFT",
	"!MOBILE",
	"!OBJECT",	// 5
	"!BUILDING",
	"!SECTORS",
	"!CROP",
	"!TRIGGER",
	"!ADVENTURE",	// 10
	"!ROOMTEMPLATE",
	"!GLOBAL",
	"!AUGMENT",
	"!ARCHETYPE",
	"!ABILITIES",	// 15
	"!CLASSES",
	"!SKILLS",
	"!VEHICLES",
	"!MORPHS",
	"!QUESTS",	// 20
	"!SOCIALS",
	"!FACTIONS",
	"!GENERICS",
	"!SHOPS",
	"!PROGRESS",	// 25
	"!EVENTS",
	"!ATTACKS",
	"REFRESH-COMPANIONS",
	"\n"
};


// OLC_x types
const char *olc_type_bits[] = {
	"craft",	// 0
	"mobile",
	"object",
	"map",
	"building",
	"trigger",	// 5
	"crop",
	"sector",
	"adventure",
	"roomtemplate",
	"global",	// 10
	"book",
	"augment",
	"archetype",
	"ability",
	"class",	// 15
	"skill",
	"vehicle",
	"morph",
	"quest",
	"social",	// 20
	"faction",
	"generic",
	"shop",
	"progression",
	"event",	// 25
	"attackmessage",
	"\n"
};


// auditors: keywords that can be safely ignored if missing
const char *ignore_missing_keywords[] = {
	"and",
	"for",
	"in",
	"one",
	"your",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// PROGRESS CONSTANTS //////////////////////////////////////////////////////

// PROGRESS_x: progress types
const char *progress_types[] = {
	"UNDEFINED",
	"Community",
	"Industry",
	"Defense",
	"Prestige",
	"\n"
};


// PRG_x: progress flags
const char *progress_flags[] = {
	"IN-DEVELOPMENT",	// 0
	"PURCHASABLE",
	"NO-AUTOSTART",
	"HIDDEN",
	"NO-ANNOUNCE",
	"NO-PREVIEW",	// 5
	"NO-TRACKER",
	"\n"
};


// PRG_PERK_x: progress perk types (should be all 1 word)
const char *progress_perk_types[] = {
	"Technology",	// 0
	"City-points",
	"Craft",
	"Max-city-size",
	"Wealth-territory-per-100",
	"Greatness-territory",	// 5
	"Workforce-cap",
	"Territory",
	"Speak-language",
	"Recognize-language",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// QUEST CONSTANTS /////////////////////////////////////////////////////////

// QST_x: quest flags
const char *quest_flags[] = {
	"IN-DEVELOPMENT",	// 0
	"REPEAT-PER-INSTANCE",
	"EXPIRES-AFTER-INSTANCE",
	"EXTRACT-TASK-OBJECTS",
	"DAILY",
	"EMPIRE-ONLY",	// 5
	"NO-GUESTS",
	"TUTORIAL",
	"GROUP-COMPLETION",
	"EVENT",
	"IN-CITY-ONLY",	// 10
	"\n"
};


// QG_x: quest giver types
const char *quest_giver_types[] = {
	"BUILDING",	// 0
	"MOBILE",
	"OBJECT",
	"ROOM-TEMPLATE",
	"TRIGGER",
	"QUEST",	// 5
	"VEHICLE",
	"\n"
};


// QR_x: quest reward types
const char *quest_reward_types[] = {
	"BONUS-EXP",	// 0
	"COINS",
	"OBJECT",
	"SET-SKILL",
	"SKILL-EXP",
	"SKILL-LEVELS",	// 5
	"QUEST-CHAIN",
	"REPUTATION",
	"CURRENCY",
	"EVENT-POINTS",
	"SPEAK-LANGUAGE",	// 10
	"RECOGNIZE-LANGUAGE",
	"GRANT-PROGRESS",
	"START-PROGRESS",
	"UNLOCK-ARCHETYPE",
	"\n",
};


 //////////////////////////////////////////////////////////////////////////////
//// ROOM/WORLD CONSTANTS ////////////////////////////////////////////////////

// BLD_ON_x (1/2): names of build-on flags
const char *bld_on_flags[] = {
	"water",	// 0
	"plains",
	"mountain",
	"full-forest",
	"desert",
	"river",	// 5
	"jungle",
	"not-player-made",
	"ocean",
	"oasis",
	"crops",	// 10
	"grove",
	"swamp",
	"any-forest",
	"open-building",
	"flat-terrain",	// 15
	"shallow-sea",
	"coast",
	"riverbank",
	"estuary",
	"lake",	// 20
	"base-terrain-allowed",
	"giant-tree",
	"road",
	"\n"
};


// BLD_ON_x (2/2): order to display build-in flags
const bitvector_t bld_on_flags_order[] = {
	BLD_ON_PLAINS,
	BLD_ON_MOUNTAIN,
	BLD_ON_FOREST,
	BLD_ANY_FOREST,
	BLD_ON_GIANT_TREE,
	
	// desert types
	BLD_ON_DESERT,
	BLD_ON_OASIS,
	BLD_ON_GROVE,
	
	// jungle types
	BLD_ON_JUNGLE,
	BLD_ON_SWAMP,
	
	// water types
	BLD_ON_WATER,
	BLD_ON_OCEAN,
	BLD_ON_SHALLOW_SEA,
	BLD_ON_COAST,
	BLD_ON_ESTUARY,
	BLD_ON_RIVER,
	BLD_ON_RIVERBANK,
	BLD_ON_LAKE,
	
	// end modifiers
	BLD_ON_ROAD,
	BLD_ON_FLAT_TERRAIN,
	BLD_FACING_OPEN_BUILDING,
	BLD_ON_BASE_TERRAIN_ALLOWED,
	BLD_FACING_CROP,
	BLD_ON_NOT_PLAYER_MADE,
	NOBITS	// end
};


// BLD_x: building flags -- * flags are removed flags
const char *bld_flags[] = {
	"ROOM",	// 0
	"ALLOW-MOUNTS",
	"TWO-ENTRANCES",
	"OPEN",
	"CLOSED",
	"INTERLINK",	// 5
	"HERD",
	"DEDICATE",
	"IS-RUINS",
	"!NPC",
	"BARRIER",	// 10
	"IN-CITY-ONLY",
	"LARGE-CITY-RADIUS",
	"!PAINT",
	"ATTACH-ROAD",
	"BURNABLE",	// 15
	"EXIT",
	"OBSCURE-VISION",
	"ROAD-ICON",
	"ROAD-ICON-WIDE",
	"ATTACH-BARRIER",	// 20
	"NO-CUSTOMIZE",
	"NO-ABANDON-WHEN-RUINED",
	"SHOW-VEHICLES",
	"*",
	"*",	// 25
	"*",
	"*",
	"*",
	"SAIL",
	"*",	// 30
	"*",
	"*",
	"ITEM-LIMIT",
	"LONG-AUTOSTORE",
	"*",	// 35
	"*",
	"HIGH-DEPLETION",
	"*",
	"*",
	"!DELETE",	// 40
	"*",
	"NEED-BOAT",
	"LOOK-OUT",
	"2ND-TERRITORY",
	"*",	// 45
	"*",
	"*",
	"\n"
};


// BLD_REL_x (1/2): relationships with other buildings
const char *bld_relationship_types[] = {
	"UPGRADES-TO-BLD",	// 0
	"STORES-LIKE-BLD",
	"STORES-LIKE-VEH",
	"UPGRADES-TO-VEH",
	"FORCE-UPGRADE-BLD",
	"FORCE-UPGRADE-VEH",	// 5
	"\n"
};


// BLD_REL_x (2/2): vnum types
const int bld_relationship_vnum_types[] = {
	TYPE_BLD,	// 0, UPGRADES-TO-BLD
	TYPE_BLD,	// STORES-LIKE-BLD
	TYPE_VEH,	// STORES-LIKE-VEH
	TYPE_VEH,	// UPGRADES-TO-VEH
	TYPE_BLD,	// FORCE-UPGRADE-BLD
	TYPE_VEH,	// 5, FORCE-UPGRADE-VEH
};


// CLIM_x (1/4): climate flags
const char *climate_flags[] = {
	"*",	// 0
	"*",
	"*",
	"*",
	"hot",
	"cold",	// 5
	"high",
	"low",
	"magical",
	"temperate",
	"arid",	// 10
	"tropical",
	"mountain",
	"river",
	"fresh water",
	"salt water",	// 15
	"forest",
	"grassland",
	"coastal",
	"ocean",
	"lake",	// 20
	"waterside",
	"mild",
	"harsh",
	"frozen water",
	"\n"
};


// CLIM_x (2/4): modifiers for temperature (see also: season_temperature, sun_temperature)
const struct climate_temperature_t climate_temperature[] = {
	// { base-add, sun-weight (1.0 or NO_TEMP_MOD), season-weight (1.0 or NO_TEMP_MOD), cold-mod (1.0), heat-mod (1.0) }
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 1.0, 1.0 },	// 0
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 1.0, 1.0 },	// unused climates
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 1.0, 1.0 },
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 1.0, 1.0 },
	
	{ 15, NO_TEMP_MOD, NO_TEMP_MOD, 0.75, 1.25 },	// CLIM_HOT
	{ -15, NO_TEMP_MOD, NO_TEMP_MOD, 1.25, 0.75 },	// 5: CLIM_COLD
	{ -7, 0.5, 0.5, 1.0, 1.0 },	// CLIM_HIGH
	{ 7, 0.5, 0.5, 1.0, 1.0 },	// CLIM_LOW
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 1.0, 1.0 },	// CLIM_MAGICAL
	{ -8, 0.4, 3.0, 1.0, 1.0 },	// CLIM_TEMPERATE
	{ 13, 1.75, 3.25, 1.0, 1.0 },	// 10: CLIM_ARID
	{ 15, 0.25, 0.5, 1.0, 1.0 },	// CLIM_TROPICAL
	{ -5, 1.25, NO_TEMP_MOD, 1.0, 1.0 },	// CLIM_MOUNTAIN
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 1.0, 1.0 },	// CLIM_RIVER
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 1.0, 1.0 },	// CLIM_FRESH_WATER
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 1.0, 1.0 },	// 15: CLIM_SALT_WATER
	{ 0, 0.25, 1.5, 1.0, 1.0 },	// CLIM_FOREST
	{ 0, 0.75, 1.0, 1.0, 1.0 },	// CLIM_GRASSLAND
	{ 0, 0.5, 0.75, 1.0, 1.0 },	// CLIM_COASTAL
	{ 5, 1.0, 3.5, 1.0, 1.0 },	// CLIM_OCEAN
	{ 0, 0.75, NO_TEMP_MOD, 1.0, 1.0 },	// 20: CLIM_LAKE
	{ 0, 0.75, NO_TEMP_MOD, 1.0, 1.0 },	// CLIM_WATERSIDE
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 0.5, 0.5 },	// CLIM_MILD
	{ 0, NO_TEMP_MOD, NO_TEMP_MOD, 1.5, 1.5 },	// CLIM_HARSH
	{ -12, NO_TEMP_MOD, NO_TEMP_MOD, 1.25, 1.0 },	// CLIM_FROZEN_WATER
};


// CLIM_x (3/4): order to display climate flags
const bitvector_t climate_flags_order[] = {
	CLIM_MILD, CLIM_HARSH,	// modifiers
	CLIM_HOT, CLIM_COLD,	// temperatures first
	CLIM_HIGH, CLIM_LOW,	// relative elevation
	CLIM_MAGICAL,			// special attribute
	
	CLIM_TEMPERATE, CLIM_ARID, CLIM_TROPICAL,	// latitude adjustments
	
	CLIM_COASTAL, CLIM_FRESH_WATER, CLIM_SALT_WATER, CLIM_FROZEN_WATER,	// water prefixes
	CLIM_RIVER, CLIM_OCEAN, CLIM_LAKE,	// water types
	
	CLIM_WATERSIDE,	// before land types
	
	CLIM_MOUNTAIN,		// land terrain that could be prefix or whole name
	CLIM_FOREST, CLIM_GRASSLAND,	// land terrains
	
	CLIM_UNUSED1, CLIM_UNUSED2, CLIM_UNUSED3, CLIM_UNUSED4,	// move these when used
	NOBITS	// last
};


// CLIM_x (4/4): whether or not vehicles can ruin slowly over time when they have an invalid climate
const bool climate_ruins_vehicle_slowly[][2] = {
	// { when gaining climate, when losing climate }
	{ FALSE, FALSE },	// *	// 0
	{ FALSE, FALSE },	// *
	{ FALSE, FALSE },	// *
	{ FALSE, FALSE },	// *
	{ TRUE, TRUE },	// hot
	{ TRUE, TRUE },	// cold	// 5
	{ TRUE, TRUE },	// high
	{ TRUE, TRUE },	// low
	{ FALSE, FALSE },	// magical
	{ TRUE, TRUE },	// temperate
	{ TRUE, TRUE },	// arid	// 10
	{ TRUE, TRUE },	// tropical
	{ FALSE, FALSE },	// mountain
	{ FALSE, TRUE },	// river
	{ FALSE, TRUE },	// fresh water
	{ FALSE, TRUE },	// salt water	// 15
	{ TRUE, FALSE },	// forest
	{ TRUE, TRUE },	// grassland
	{ TRUE, TRUE },	// coastal
	{ FALSE, TRUE },	// ocean
	{ FALSE, TRUE },	// lake
	{ TRUE, TRUE },	// waterside
	{ TRUE, TRUE },	// mild
	{ TRUE, TRUE },	// harsh
	{ FALSE, TRUE },	// frozen water
};


// CROPF_x
const char *crop_flags[] = {
	"REQUIRES-WATER",	// 0
	"ORCHARD",
	"!WILD",
	"NEWBIE-ONLY",
	"!NEWBIE",
	"ANY-LISTED-CLIMATE",	// 5
	"NO-GLOBAL-SPAWNS",
	"LOCK-ICON",
	"\n"
};


// CROP_CUSTOM_x: custom messages
const char *crop_custom_types[] = {
	"magic-growth",	// 0
	"\n"
};


// DPLTN_x (1/2): basic depletion names
const char *depletion_types[] = {
	"dig",		// 0
	"forage",
	"gather",
	"pick",
	"fish",
	"quarry",	// 5
	"pan",
	"trapping",
	"chop",
	"hunt",
	"production",	// 10
	"secondary",
	"tertiary",
	"\n"
};


// DPLTN_x (2/2): depletion strings for survey; may be empty; use $$ as a placeholder for the depletion_levels[] amount
const char *depletion_strings[] = {
	"dug up $$ ground",	// 0
	"foraged $$ food in the area",
	"gathered $$ material on the ground",
	"picked $$ things here",
	"caught $$ fish",
	"quarried $$ stone",	// 5
	"panned $$ gold",
	"trapped $$ animals",
	"chopped $$ trees",
	"hunted $$ animals",	// unlikely to be able to show this one
	"",	// 10, production
	"",	// secondary production
	"",	// tertiary production
	"\n"
};


// for depletion displays, take the percent depleted, divide by 25:
const char *depletion_levels[] = {
	"some",				// 0-24
	"a bunch of the",	// 25-49
	"a lot of",			// 50-74
	"most of the",		// 75-99
	"all the",			// 100%
	"\n"
};


// DES_x
const char *designate_flags[] = {
	"CRYPT",	// 0
	"VAULT",
	"FORGE",
	"TUNNEL",
	"HALL",
	"SKYBRIDGE",	// 5
	"THRONE",
	"ARMORY",
	"GREAT-HALL",
	"BATHS",
	"LABORATORY",	// 10
	"TOP-OF-TOWER",
	"HOUSEHOLD",
	"HAVEN",
	"SHIP-MAIN",
	"SHIP-LARGE",	// 15
	"SHIP-EXTRA",
	"LAND-VEHICLE",
	"\n"
};


// EVO_x 1/3: world evolution names
const char *evo_types[] = {
	"CHOPPED-DOWN",	// 0
	"CROP-GROWS",
	"ADJACENT-ONE",
	"ADJACENT-MANY",
	"RANDOM",
	"TRENCH-START",	// 5
	"TRENCH-FULL",
	"NEAR-SECTOR",
	"PLANTS-TO",
	"MAGIC-GROWTH",
	"NOT-ADJACENT",	// 10
	"NOT-NEAR-SECTOR",
	"SPRING",
	"SUMMER",
	"AUTUMN",
	"WINTER",	// 15
	"BURNS-TO",
	"SPREADS-TO",
	"HARVEST-TO",
	"DEFAULT-HARVEST-TO",
	"TIMED",	// 20
	"OWNED",
	"UNOWNED",
	"BURN-STUMPS",
	"ADJACENT-SECTOR-FLAG",
	"NOT-ADJACENT-SECTOR-FLAG",	// 25
	"NEAR-SECTOR-FLAG",
	"NOT-NEAR-SECTOR-FLAG",
	"\n"
};


// EVO_x 2/3 and EVO_VAL_x: what type of data the evolution.value uses
const int evo_val_types[NUM_EVOS] = {
	EVO_VAL_NONE,	// 0, chopped-down
	EVO_VAL_NONE,	// crop-grows
	EVO_VAL_SECTOR,	// adjacent-one
	EVO_VAL_SECTOR,	// adjacent-many
	EVO_VAL_NONE,	// random
	EVO_VAL_NONE,	// 5, trench-start
	EVO_VAL_NONE,	// trench-full
	EVO_VAL_SECTOR,	// near-sector
	EVO_VAL_NONE,	// plants-to
	EVO_VAL_NONE,	// magic-growth
	EVO_VAL_SECTOR,	// 10, not-adjacent
	EVO_VAL_SECTOR,	// not-near-sector
	EVO_VAL_NONE,	// spring
	EVO_VAL_NONE,	// summer
	EVO_VAL_NONE,	// autumn
	EVO_VAL_NONE,	// 15, winter
	EVO_VAL_NONE,	// burns-to
	EVO_VAL_SECTOR,	// spreads-to
	EVO_VAL_NONE,	// harvest-to
	EVO_VAL_NONE,	// default-harvest-to
	EVO_VAL_NUMBER,	// 20, timed (minutes)
	EVO_VAL_NONE,	// owned
	EVO_VAL_NONE,	// unowned
	EVO_VAL_NONE,	// burn-stumps
	EVO_VAL_SECTOR_FLAG,	// adjacent-sector-flag
	EVO_VAL_SECTOR_FLAG,	// 25, not-adjacent-sector-flag
	EVO_VAL_SECTOR_FLAG,	// near-sector-flag
	EVO_VAL_SECTOR_FLAG		// not-near-sector-flag
};


// EVO_x 3/3: evolution is over time (as opposed to triggered by an action)
const bool evo_is_over_time[] = {
	FALSE,	// 0, chopped
	FALSE,	// crop grows
	TRUE,	// adjacent-one
	TRUE,	// adjacent-many
	TRUE,	// random
	FALSE,	// 5, trench-start
	FALSE,	// trench-full
	TRUE,	// near-sect
	FALSE,	// plants-to
	FALSE,	// magic-growth
	TRUE,	// 10, not-adjacent
	TRUE,	// not-near-sector
	TRUE,	// spring
	TRUE,	// summer
	TRUE,	// autumn
	TRUE,	// 15, winter
	FALSE,	// burns-to
	TRUE,	// spreads-to
	FALSE,	// harvest-to
	FALSE,	// default-harvest-to
	TRUE,	// 20, timed
	TRUE,	// owned
	TRUE,	// unowned
	FALSE,	// burn-stumps
	TRUE,	// adjacent-sector-flag
	TRUE,	// 25, not-adjacent-sector-flag
	TRUE,	// near-sector-flag
	TRUE,	// not-near-sector-flag
};


// FNC_x (1/2): function flags (for buildings)
const char *function_flags[] = {
	"ALCHEMIST",	// 0
	"UPGRADED",
	"*",
	"BEDROOM",
	"CARPENTER",
	"*",	// 5
	"DOCKS",
	"FORGE",
	"GLASSBLOWER",
	"GUARD-TOWER",
	"*",	// 10
	"LIBRARY",
	"MAIL",
	"MILL",
	"MINE",
	"MINT",	// 15
	"PORTAL",
	"POTTER",
	"PRESS",
	"SAW",
	"SHIPYARD",	// 20
	"SMELT",
	"STABLE",
	"SUMMON-PLAYER",
	"TAILOR",
	"TANNERY",	// 25
	"TAVERN",
	"TOMB",
	"TRADING-POST",
	"VAULT",
	"WAREHOUSE",	// 30
	"DRINK-WATER",
	"COOKING-FIRE",
	"LARGER-NEARBY",
	"FISHING",
	"STORE-ALL", // 35
	"IN-CITY-ONLY",
	"OVEN",
	"MAGIC-WORKSHOP",
	"APOTHECARY",
	"\n"
};


// FNC_x (2/2): explainers, usually shown as "You must be %s to craft that."
const char *function_flags_long[] = {
	"at an alchemist",	// 0
	"an upgraded building",
	"",
	"in a bedroom",
	"at a carpenter",
	"",	// 5
	"at the docks",
	"at a forge",
	"at a glassblower",
	"at a guard tower",
	"",	// 10
	"in a library",
	"at a post box",
	"at a mill",
	"in a mine",
	"at a mint",	// 15
	"at a portal",
	"at a potter",
	"at a press",
	"somewhere that saws",
	"at a shipyard",	// 20
	"at a foundry",
	"in a stable",
	"somewhere you can summon players",
	"at a tailor",
	"at a tannery",	// 25
	"in a tavern",
	"in a tomb",
	"at a trading post",
	"in a vault",
	"in a warehouse",	// 30
	"somewhere with drinking water",
	"somewhere with a cooking fire",
	"somewhere that extends nearby",
	"in a fishery",
	"at a depository", // 35
	"",	// in-city-only?
	"somewhere with an oven",
	"in a magic workshop",
	"at an apothecary",
	"\n"
};


// ISLE_x -- island flags
const char *island_bits[] = {
	"NEWBIE",	// 0
	"!AGGRO",
	"!CUSTOMIZE",
	"CONTINENT",
	"*",	// has-custom-desc (internal use only)
	"!CHART",	// 5
	"!TEMPERATURE-PENALTIES",
	"ALWAYS-LIGHT",
	"\n"
};


// these must match up to mapout_color_tokens -- do not insert or change the order
const char *mapout_color_names[] = {
	"Starting Location",	// 0
	"Neutral",
	"Bright White",
	"Bright Red",
	"Bright Green",
	"Bright Yellow",	// 5
	"Bright Blue",
	"Bright Magenta",
	"Bright Cyan",
	"Dark Red",
	"Pale Green",	// 10
	"Yellow-Green",
	"Sea Green",
	"Medium Green",
	"Dark Green",
	"Olive Green",	// 15
	"Ice Blue",
	"Light Blue",
	"Medium Blue",
	"Deep Blue",
	"Light Tan",	// 20
	"Pale Yellow",
	"Peach",
	"Orange",
	"Yellow Brown",
	"Brown",	// 25
	"Medium Gray",
	"Dark Gray",
	"Dark Blue",
	"Dark Azure Blue",
	"Dark Magenta",	// 30
	"Dark Cyan",
	"Lime Green",
	"Dark Lime Green",
	"Dark Orange",
	"Pink",	// 35
	"Dark Pink",
	"Tan",
	"Violet",
	"Deep Violet",
	"Dark Dark Green",	// 40
	"Dark Brown",
	"Deep Yellow",
	"\n"
};


// these must match up to mapout_color_names -- do not insert or change the order
// these must also match up to the map.php generator
const char mapout_color_tokens[] = {
	'*',	// "Starting Location",	// 0
	'?',	// "Neutral",
	'0',	// "Bright White",
	'1',	// "Bright Red",
	'2',	// "Bright Green",
	'3',	// "Bright Yellow",	// 5
	'4',	// "Bright Blue",
	'5',	// "Bright Magenta",
	'6',	// "Bright Cyan",
	'a',	// "Dark Red",
	'b',	// "Pale Green",	// 10
	'c',	// "Yellow-Green",
	'd',	// "Sea Green",
	'e',	// "Medium Green",
	'f',	// "Dark Green",
	'g',	// "Olive Green",	// 15
	'h',	// "Ice Blue",
	'i',	// "Light Blue",
	'j',	// "Medium Blue",
	'k',	// "Deep Blue",
	'l',	// "Light Tan",	// 20
	'm',	// "Pale Yellow",
	'n',	// "Peach",
	'o',	// "Orange",
	'p',	// "Yellow Brown",
	'q',	// "Brown",	// 25
	'r',	// "Medium Gray",
	's',	// "Dark Gray",
	't',	// "Dark Blue",
	'u',	// "Dark Azure Blue",
	'v',	// "Dark Magenta",	// 30
	'w',	// "Dark Cyan",
	'x',	// "Lime Green",
	'y',	// "Dark Lime Green",
	'z',	// "Dark Orange",
	'A',	// "Pink",	// 35
	'B',	// "Dark Pink",
	'C',	// "Tan",
	'D',	// "Violet",
	'E',	// "Deep Violet",
	'F',	// "Dark Dark Green",	// 40
	'G',	// "Dark Brown",
	'H',	// "Deep Yellow",
};


// this maps a banner color (the 'r' in "&r") to a mapout_color_token character ('1')
const char banner_to_mapout_token[][2] = {
	{ '0', '0' },
	{ 'n', '0' },
	// non-bright colors:
	{ 'r', 'a' },
	{ 'g', 'f' },
	{ 'b', 't' },
	{ 'y', 'p' },
	{ 'm', 'v' },
	{ 'c', 'w' },
	{ 'w', 'r' },
	{ 'a', 'u' },
	{ 'j', 'b' },
	{ 'l', 'y' },
	{ 'o', 'z' },
	{ 'p', 'B' },
	{ 't', 'q' },
	{ 'v', 'E' },
	// bright colors:
	{ 'R', '1' },
	{ 'G', '2' },
	{ 'B', '4' },
	{ 'Y', '3' },
	{ 'M', '5' },
	{ 'C', '6' },
	{ 'W', '0' },
	{ 'A', 'j' },
	{ 'J', 'd' },
	{ 'L', 'x' },
	{ 'O', 'o' },
	{ 'P', 'A' },
	{ 'T', 'C' },
	{ 'V', 'D' },
	
	// last
	{ '\n', '\n' }
};


// ROAD_x
const char *road_types[] = {
	"ROAD",
	"BRIDGE",
	"SWAMPWALK",
	"\n"
};


/* ROOM_AFF_x: */
const char *room_aff_bits[] = {
	"MAGIC-DARKNESS",	// 0
	"SILENT",
	"*HAS-INSTANCE",
	"CHAMELEON",
	"*TEMPORARY",
	"!EVOLVE",	// 5
	"*UNCLAIMABLE",
	"*PUBLIC",
	"*DISMANTLING",
	"!FLY",
	"!WEATHER",	// 10
	"*IN-VEHICLE",
	"*!WORK",
	"!DISREPAIR",
	"*!DISMANTLE",
	"*INCOMPLETE",	// 15
	"!TELEPORT",
	"BRIGHT-PAINT",
	"*FAKE-INSTANCE",
	"!ABANDON",
	"REPEL-NPCS",	// 20
	"REPEL-ANIMALS",
	"NO-WORKFORCE-EVOS",
	"HIDE-REAL-NAME",
	"MAPOUT-BUILDING",
	"NO-TRACKS",	// 25
	"PERMANENT-PAINT",
	"\n"
};


// ROOM_EXTRA_x
const char *room_extra_types[] = {
	"prospect empire",	// 0
	"mine amount",
	"fire remaining",
	"seed time",
	"tavern type",	// unused
	"tavern brewing time",	// 5		// unused
	"tavern available time",	// unused
	"ruins icon",	// unused
	"chop progress",
	"trench progress",
	"harvest progress",	// 10
	"paint color",
	"dedicate id",
	"build recipe",
	"found time",
	"redesignate time",	// 15
	"ceded",
	"mine global vnum",
	"trench fill time",
	"trench original sector",
	"original builder",	// 20
	"sector time",
	"workforce prospect",
	"\n"
};


// SECTF_x
const char *sector_flags[] = {
	"LOCK-ICON",	// 0
	"IS-ADVENTURE",
	"NON-ISLAND",
	"CHORE",
	"!CLAIM",
	"START-LOCATION",	// 5
	"FRESH-WATER",
	"OCEAN",
	"DRINK",
	"HAS-CROP-DATA",
	"CROP",	// 10
	"LAY-ROAD",
	"IS-ROAD",
	"CAN-MINE",
	"SHOW-ON-POLITICAL-MAPOUT",
	"MAP-BUILDING",	// 15
	"INSIDE-ROOM",
	"LARGE-CITY-RADIUS",
	"OBSCURE-VISION",
	"IS-TRENCH",
	"NO-GLOBAL-SPAWNS",	// 20
	"ROUGH",
	"SHALLOW-WATER",
	"NEEDS-HEIGHT",
	"KEEPS-HEIGHT",
	"SEPARATE-NOT-ADJACENTS",	// 25
	"SEPARATE-NOT-NEARS",
	"INHERIT-BASE-CLIMATE",
	"IRRIGATES-AREA",
	"\n"
};


// SECT_CUSTOM_x: custom messages
const char *sect_custom_types[] = {
	"magic-growth",	// 0
	"\n"
};


// SPAWN_x (1/2) -- full length spawn flags
const char *spawn_flags[] = {
	"NOCTURNAL",	// 0
	"DIURNAL",
	"CLAIMED",
	"UNCLAIMED",
	"CITY",
	"OUT-OF-CITY",	// 5
	"NORTHERN",
	"SOUTHERN",
	"EASTERN",
	"WESTERN",
	"CONTINENT-ONLY",	// 10
	"NO-CONTINENT",
	"SPRING-ONLY",
	"SUMMER-ONLY",
	"AUTUMN-ONLY",
	"WINTER-ONLY",	// 15
	"\n"
};


// SPAWN_x (2/2) -- short form for compressed display
const char *spawn_flags_short[] = {
	"NOCT",	// 0
	"DIA",
	"CLM",
	"!CLM",
	"CITY",
	"!CITY",	// 5
	"NORTH",
	"SOUTH",
	"EAST",
	"WEST",
	"CONT",	// 10
	"!CONT",
	"SPRING",
	"SUMMER",
	"AUTUMN",
	"WINTER",	// 15
	"\n"
};


// TILESET_x (1/3): season names
const char *seasons[] = {
	"UNKNOWN",	// TILESET_ANY (should never hit this case)
	"springtime",
	"summertime",
	"autumn",
	"wintertime",
	"\n"
};


// TILESET_x (2/3): icon type / season name
const char *icon_types[] = {
	"Any",
	"Spring",
	"Summer",
	"Autumn",
	"Winter",
	"\n"
};


// TILESET_x (3/3): seasonal temperature base (see also: climate_temperature, sun_temperature)
const int season_temperature[] = {
	0,	// TILESET_ANY
	-5,	// spring
	8,	// summer
	0,	// autumn
	-10,	// winter
};


// SUN_x (1/2): sun states (anything other than 'dark' is light)
const char *sun_types[] = {
	"dark",
	"rising",
	"light",
	"setting",
	"\n"
};


// SUN_x (2/2): temperature modifiers for sun (see also: climate_temperature, season_temperature)
const int sun_temperature[] = {
	-10,	// SUN_DARK
	0,	// SUN_RISE
	8,	// SUN_LIGHT
	0,	// SUN_SET
};


// TEMPERATURE_x: Temperature flags for adventures, buildings, and room templates
const char *temperature_types[] = {
	"use climate",	// 0
	"always comfortable",
	"milder",
	"harsher",
	"freezing",
	"cold",	// 5
	"cool",
	"cooler",
	"cooler when hot",
	"neutral",
	"warm",	// 10
	"warmer",
	"warmer when cold",
	"hot",
	"sweltering",
	"\n"
};


// SKY_x -- mainly used by scripting
// TODO replace these with a new weather system because raining/lightning are
// displayed to the player as "snowing" in cold rooms as of b5.162
const char *weather_types[] = {
	"sunny",
	"cloudy",
	"raining",
	"lightning",
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// SHOP CONSTANTS //////////////////////////////////////////////////////////

// SHOP_x: shop flags
const char *shop_flags[] = {
	"IN-DEVELOPMENT",	// 0
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// SKILL CONSTANTS /////////////////////////////////////////////////////////

// DAM_x (1/2): damage types
const char *damage_types[] = {
	"physical",	// 0
	"magical",
	"fire",
	"poison",
	"direct",
	"\n"
};


// DAM_x (2/2): damage type to DoT attack type
const int damage_type_to_dot_attack[] = {
	ATTACK_PHYSICAL_DOT,	// 0
	ATTACK_MAGICAL_DOT,
	ATTACK_FIRE_DOT,
	ATTACK_POISON_DOT,
	ATTACK_PHYSICAL_DOT,	// DAM_DIRECT uses physical dot
};


// DIFF_x (1/2): OLC labels for how difficult a roll is
const char *skill_check_difficulty[] = {
	"trivial (always passes)",
	"easy (always passes after 50 skill)",
	"medium (always passes at 100 skill)",
	"hard (can still fail at 100)",
	"rare (passes 10% of the time at 100)",
	"\n"
};


// DIFF_x (2/2): modifiers to your skill level before a skill check
double skill_check_difficulty_modifier[NUM_DIFF_TYPES] = {
	100,	// trivial (always passes)
	2.0,  // easy
	1.0,  // medium
	0.66,  // hard
	0.1  // rarely
};


// SKILLF_x: skill flags
const char *skill_flags[] = {
	"IN-DEVELOPMENT",	// 0
	"BASIC",
	"NO-SPECIALIZE",
	"VAMPIRE",
	"CASTER",
	"REMOVED-BY-PURIFY",	// 5
	"\n"
};


 //////////////////////////////////////////////////////////////////////////////
//// SOCIAL CONSTANTS ////////////////////////////////////////////////////////

// SOC_x: Social flags
const char *social_flags[] = {
	"IN-DEVELOPMENT",
	"HIDE-IF-INVIS",
	"\n"
};


// SOCM_x: social message string  { "Label", "command" }
const char *social_message_types[NUM_SOCM_MESSAGES][2] = {
	{ "No-arg to character", "n2char" },	// 0
	{ "No-arg to others", "n2other" },
	{ "Targeted to character", "t2char" },
	{ "Targeted to others", "t2other" },
	{ "Targeted to victim", "t2vict" },
	{ "Target not found", "tnotfound" },	// 5
	{ "Target-self to character", "s2char" },
	{ "Target-self to others", "s2other" }
};


 //////////////////////////////////////////////////////////////////////////////
//// TRIGGER CONSTANTS ///////////////////////////////////////////////////////

// MTRIG_x -- mob trigger types
const char *trig_types[] = {
	"Global",	// 0
	"Random",
	"Command",
	"Speech",
	"Act",
	"Death",	// 5
	"Greet",
	"Greet-All",
	"Entry",
	"Receive",
	"Fight",	// 10
	"HitPrcnt",
	"Bribe",
	"Load",
	"Memory",
	"Ability",	// 15
	"Leave",
	"Door",
	"Leave-All",
	"Charmed",
	"Start-Quest",	// 20
	"Finish-Quest",
	"Player-in-Room",
	"Reboot",
	"Buy",
	"Kill",		// 25
	"Allow-Multiple",
	"Can-Fight",
	"Pre-Greet-All",
	"\n"
};

// MTRIG_x -- mob trigger argument types
const bitvector_t mtrig_argument_types[] = {
	NOBITS,	// 0, global
	TRIG_ARG_PERCENT,	// random
	TRIG_ARG_COMMAND,	// command
	TRIG_ARG_PHRASE_OR_WORDLIST,	// speech
	TRIG_ARG_PHRASE_OR_WORDLIST,	// act
	TRIG_ARG_PERCENT,	// 5, death
	TRIG_ARG_PERCENT,	// greet
	TRIG_ARG_PERCENT,	// greet all
	TRIG_ARG_PERCENT,	// entry
	TRIG_ARG_PERCENT,	// receive
	TRIG_ARG_PERCENT,	// 10, fight
	TRIG_ARG_PERCENT,	// hit percent
	TRIG_ARG_COST,	// bribe
	TRIG_ARG_PERCENT,	// load
	TRIG_ARG_PERCENT,	// memory
	TRIG_ARG_PERCENT,	// 15, ability
	TRIG_ARG_PERCENT,	// leave
	TRIG_ARG_PERCENT,	// door
	TRIG_ARG_PERCENT,	// leave-all
	NOBITS,	// charmed modifier
	NOBITS,	// 20, start-quest
	NOBITS,	// finish-quest
	NOBITS,	// player-in-room
	NOBITS,	// reboot
	NOBITS,	// buy
	TRIG_ARG_PERCENT,	// 25, kill
	NOBITS,	// allow-multiple
	NOBITS,	// can-fight
	TRIG_ARG_PERCENT,	// pre-greet-all
};


// OTRIG_x -- obj trigger types
const char *otrig_types[] = {
	"Global",	// 0
	"Random",
	"Command",
	"Greet",
	"*",
	"Timer",	// 5
	"Get",
	"Drop",
	"Give",
	"Wear",
	"*",	// 10
	"Remove",
	"*",
	"Load",
	"*",
	"Ability",	// 15
	"Leave",
	"*",
	"Consume",
	"Finish",
	"Start-Quest",	// 20
	"Finish-Quest",
	"Player-in-Room",
	"Reboot",
	"Buy",
	"Kill",		// 25
	"Allow-Multiple",
	"\n"
};

// OTRIG_x -- obj trigger argument types
const bitvector_t otrig_argument_types[] = {
	NOBITS,	// 0, global
	TRIG_ARG_PERCENT,	// random
	TRIG_ARG_COMMAND | TRIG_ARG_OBJ_WHERE,	// command
	TRIG_ARG_PERCENT,	// greet
	NOBITS,	//
	NOBITS,	// 5, timer
	TRIG_ARG_PERCENT,	// get
	TRIG_ARG_PERCENT,	// drop
	TRIG_ARG_PERCENT,	// give
	NOBITS,	// wear
	NOBITS,	// 10
	NOBITS,	// remove
	NOBITS,	// 
	TRIG_ARG_PERCENT,	// load
	NOBITS,	// 
	TRIG_ARG_PERCENT,	// 15, ability
	TRIG_ARG_PERCENT,	// leave
	NOBITS,	// 
	TRIG_ARG_PERCENT,	// consume
	TRIG_ARG_PERCENT,	// finish
	NOBITS,	// 20, start-quest
	NOBITS,	// finish-quest
	NOBITS,	// player-in-room
	NOBITS,	// reboot
	TRIG_ARG_OBJ_WHERE,	// buy
	TRIG_ARG_PERCENT,	// 25, kill
	NOBITS,	// allow-multiple
};


// VTRIG_x: vehicle trigger types
const char *vtrig_types[] = {
	"Global",	// 0
	"Random",
	"Command",
	"Speech",
	"*",	// 4
	"Destroy",	// 5
	"Greet",
	"*",	// 7
	"Entry",
	"*",	// 9
	"*",	// 10
	"*",	// 11
	"*",	// 12
	"Load",
	"*",	// 14
	"Ability",	// 15
	"Leave",
	"*",	// 17
	"Dismantle",
	"*",	// 19
	"Start-Quest",	// 20
	"Finish-Quest",
	"Player-in-Room",
	"Reboot",
	"Buy",
	"Kill",		// 25
	"Allow-Multiple",
	"\n"
};


// VTRIG_x: argument types for vehicle triggers
const bitvector_t vtrig_argument_types[] = {
	NOBITS,	// 0, global
	TRIG_ARG_PERCENT,	// random
	TRIG_ARG_COMMAND,	// command
	TRIG_ARG_PHRASE_OR_WORDLIST,	// speech
	NOBITS,	// 4
	TRIG_ARG_PERCENT,	// 5, destroy
	TRIG_ARG_PERCENT,	// greet
	NOBITS,	// 7
	TRIG_ARG_PERCENT,	// entry
	NOBITS,	// 9
	NOBITS,	// 10
	NOBITS,	// 11
	NOBITS,	// 12
	TRIG_ARG_PERCENT,	// load
	NOBITS,	// 14
	TRIG_ARG_PERCENT,	// 15, ability
	TRIG_ARG_PERCENT,	// leave
	NOBITS,	// 17
	NOBITS,	// dismantle
	NOBITS,	// 19
	NOBITS,	// 20, start-quest
	NOBITS,	// finish-quest
	NOBITS,	// player-in-room
	NOBITS,	// reboot
	NOBITS,	// buy
	TRIG_ARG_PERCENT,	// 25, kill
	NOBITS,	// allow-multiple
};


// WTRIG_x: wld trigger types
const char *wtrig_types[] = {
	"Global",	// 0
	"Random",
	"Command",
	"Speech",
	"Adventure Cleanup",
	"Zone Reset",	// 5
	"Enter",
	"Drop",
	"*",
	"*",
	"*",	// 10
	"*",
	"*",
	"Load",
	"Complete",
	"Ability",	// 15
	"Leave",
	"Door",
	"Dismantle",
	"*",
	"Start-Quest",	// 20
	"Finish-Quest",
	"Player-in-Room",
	"Reboot",
	"Buy",
	"*",	// 25
	"Allow-Multiple",
	"\n"
};

// WTRIG_x -- world trigger argument types
const bitvector_t wtrig_argument_types[] = {
	NOBITS,	// 0, global
	TRIG_ARG_PERCENT,	// random
	TRIG_ARG_COMMAND,	// command
	TRIG_ARG_PHRASE_OR_WORDLIST,	// speech
	TRIG_ARG_PERCENT,	// adventure cleanup
	TRIG_ARG_PERCENT,	// 5, zone reset
	TRIG_ARG_PERCENT,	// enter
	TRIG_ARG_PERCENT,	// drop
	NOBITS,	// 
	NOBITS,	// 
	NOBITS,	// 10
	NOBITS,	// 
	NOBITS,	// 
	TRIG_ARG_PERCENT,	// load
	TRIG_ARG_PERCENT,	// complete
	TRIG_ARG_PERCENT,	// 15, ability
	TRIG_ARG_PERCENT,	// leave
	TRIG_ARG_PERCENT,	// door
	NOBITS,	// dismantle
	NOBITS,	// 19
	NOBITS,	// 20, start-quest
	NOBITS,	// finish-quest
	NOBITS,	// player-in-room
	NOBITS,	// reboot
	NOBITS,	// buy
	NOBITS,	// 25
	NOBITS,	// allow-multiple
};


// x_TRIGGER
const char *trig_attach_types[] = {
	"Mobile",	// 0
	"Object",
	"Room",
	"*RMT (use Room)",	// rmt_trigger -- never set on an actual trigger
	"*ADV (use Room)",	// adv_trigger -- never set on an actual trigger
	"Vehicle",	// 5
	"*BDG (use Room)",	// bdg_trigger -- actually just uses room triggers
	"*EMP",	// emp_trigger -- empires only store scripts, not triggers
	"\n"
};


// x_TRIGGER -- get typenames by attach type
const char **trig_attach_type_list[] = {
	trig_types,	// 0, MOB_TRIGGER
	otrig_types,	// OBJ_TRIGGER
	wtrig_types,	// WLD_TRIGGER
	wtrig_types,	// RMT_TRIGGER (not really used)
	wtrig_types,	// ADV_TRIGGER (not really used)
	vtrig_types,	// 5, VEH_TRIGGER
	wtrig_types,	// BLD_TRIGGER (not really used)
	wtrig_types,	// EMP_TRIGGER (not really used)
};


// x_TRIGGER -- argument types by attach type
const bitvector_t *trig_argument_type_list[] = {
	mtrig_argument_types,	// 0, MOB_TRIGGER
	otrig_argument_types,	// OBJ_TRIGGER
	wtrig_argument_types,	// WLD_TRIGGER
	wtrig_argument_types,	// RMT_TRIGGER (not really used)
	wtrig_argument_types,	// ADV_TRIGGER (not really used)
	vtrig_argument_types,	// 5, VEH_TRIGGER
	wtrig_argument_types,	// BLD_TRIGGER (not really used)
	wtrig_argument_types,	// EMP_TRIGGER (not really used)
};


 //////////////////////////////////////////////////////////////////////////////
//// MISC CONSTANTS //////////////////////////////////////////////////////////

// AUTOMSG_x: automessage types
const char *automessage_types[] = {
	"one-time",
	"login",
	"repeating",
	"\n"
};


// for command-interpreting
const char *fill_words[] = {
	"in",
	"from",
	"with",
	"the",
	"on",
	"at",
	"to",
	"into",
	"of",
	"\n"
};


// GLOBAL_x types
const char *global_types[] = {
	"Mob Interactions",
	"Mine Data",
	"Newbie Gear",
	"Map Spawns",
	"Obj Interactions",
	"\n"
};


// GLB_FLAG_X global flags
const char *global_flags[] = {
	"IN-DEVELOPMENT",
	"ADVENTURE-ONLY",
	"CUMULATIVE-PRC",
	"CHOOSE-LAST",
	"RARE",
	"\n"
};


// INTERACT_x (1/2): names of interactions
const char *interact_types[] = {
	"BUTCHER",	// 0
	"SKIN",
	"SHEAR",
	"BARDE",
	"LOOT",
	"DIG",	// 5
	"FORAGE",
	"PICK",
	"HARVEST",
	"GATHER",
	"ENCOUNTER",	// 10
	"LIGHT",
	"PICKPOCKET",
	"MINE",
	"COMBINE",
	"SEPARATE",	// 15
	"SCRAPE",
	"SAW",
	"TAN",
	"CHIP",
	"CHOP",	// 20
	"FISH",
	"PAN",
	"QUARRY",
	"TAME",
	"SEED",	// 25
	"DECAYS-TO",
	"CONSUMES-TO",
	"IDENTIFIES-TO",
	"RUINS-TO-BLD",
	"RUINS-TO-VEH",	// 30
	"PRODUCTION",
	"SKILLED-LABOR",
	"LIQUID-CONJURE",
	"OBJECT-CONJURE",
	"VEHICLE-CONJURE",	// 35
	"DISENCHANT",
	"\n"
};


// INTERACT_x: (2/2): additional data for each interaction
const struct interact_data_t interact_data[NUM_INTERACTS] = {
	// attach-to, vnum-type, one-at-a-time, depletion
	{ TYPE_MOB,	TYPE_OBJ, FALSE, NOTHING }, // 0, INTERACT_BUTCHER
	{ TYPE_MOB,	TYPE_OBJ, FALSE, NOTHING }, // INTERACT_SKIN
	{ TYPE_MOB,	TYPE_OBJ, FALSE, NOTHING }, // INTERACT_SHEAR
	{ TYPE_MOB,	TYPE_MOB, FALSE, NOTHING }, // INTERACT_BARDE
	{ TYPE_MOB,	TYPE_OBJ, FALSE, NOTHING }, // INTERACT_LOOT
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_DIG }, // 5, INTERACT_DIG
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_FORAGE }, // INTERACT_FORAGE
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_PICK }, // INTERACT_PICK
	{ TYPE_ROOM, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_HARVEST
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_GATHER }, // INTERACT_GATHER
	{ TYPE_ROOM, TYPE_MOB, FALSE, NOTHING }, // 10, INTERACT_ENCOUNTER
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_LIGHT
	{ TYPE_MOB, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_PICKPOCKET
	{ TYPE_MINE_DATA, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_MINE
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_COMBINE
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // 15, INTERACT_SEPARATE
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_SCRAPE
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_SAW
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_TAN
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_CHIP
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_CHOP }, // 20, INTERACT_CHOP
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_FISH }, // INTERACT_FISH
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_PAN }, // INTERACT_PAN
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_QUARRY }, // INTERACT_QUARRY
	{ TYPE_MOB, TYPE_MOB, FALSE, NOTHING }, // INTERACT_TAME
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // 25, INTERACT_SEED
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_DECAYS_TO
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_CONSUMES_TO
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_IDENTIFIES_TO
	{ TYPE_ROOM, TYPE_BLD, FALSE, NOTHING }, // INTERACT_RUINS_TO_BLD
	{ TYPE_ROOM, TYPE_VEH, FALSE, NOTHING }, // 30, INTERACT_RUINS_TO_VEH
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_PRODUCTION }, // INTERACT_PRODUCTION
	{ TYPE_ROOM, TYPE_OBJ, TRUE, DPLTN_PRODUCTION }, // INTERACT_SKILLED_LABOR
	{ TYPE_ABIL, TYPE_LIQUID, FALSE, NOTHING }, // INTERACT_LIQUID_CONJURE
	{ TYPE_ABIL, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_OBJECT_CONJURE
	{ TYPE_ABIL, TYPE_VEH, FALSE, NOTHING }, // 35, INTERACT_VEHICLE_CONJURE
	{ TYPE_OBJ, TYPE_OBJ, FALSE, NOTHING }, // INTERACT_DISENCHANT
	
	// WARNING: Currently, only actions performed through do_gen_interact_room() or workforce support one-at-a-time.
	// Conceptually, this will ONLY work for room/vehicle interactions unless you put depletions/counters on objects and mobs.
};


// INTERACT_RESTRICT_x: types of interaction restrictions
const char *interact_restriction_types[] = {
	"ability",	// 0
	"ptech",
	"tech",
	"normal",
	"hard",
	"group",	// 5
	"boss",
	"depletion",
	"tool",
	"\n"
};


// MORPHF_x
const char *morph_flags[] = {
	"IN-DEVELOPMENT",	// 0
	"SCRIPT-ONLY",
	"ANIMAL",
	"VAMPIRE-ONLY",
	"TEMPERATE-AFFINITY",
	"ARID-AFFINITY",	// 5
	"TROPICAL-AFFINITY",
	"CHECK-SOLO",
	"!SLEEP",
	"GENDER-NEUTRAL",
	"CONSUME-OBJ",	// 10
	"!FASTMORPH",
	"!MORPH-MESSAGE",
	"HIDE-REAL-NAME",
	"\n"
};


// REQ_x (1/3): types of requirements (e.g. quest tasks)
const char *requirement_types[] = {
	"COMPLETED-QUEST",	// 0
	"GET-COMPONENT",
	"GET-OBJECT",
	"KILL-MOB",
	"KILL-MOB-FLAGGED",
	"NOT-COMPLETED-QUEST",	// 5
	"NOT-ON-QUEST",
	"OWN-BUILDING",
	"OWN-VEHICLE",
	"SKILL-LEVEL-OVER",
	"SKILL-LEVEL-UNDER",	// 10
	"TRIGGERED",
	"VISIT-BUILDING",
	"VISIT-ROOM-TEMPLATE",
	"VISIT-SECTOR",
	"HAVE-ABILITY",	// 15
	"REP-OVER",
	"REP-UNDER",
	"WEARING",
	"WEARING-OR-HAS",
	"GET-CURRENCY",	// 20
	"GET-COINS",
	"CAN-GAIN-SKILL",
	"CROP-VARIETY",
	"OWN-HOMES",
	"OWN-SECTOR",	// 25
	"OWN-BUILDING-FUNCTION",
	"OWN-VEHICLE-FLAGGED",
	"EMPIRE-WEALTH",
	"EMPIRE-FAME",
	"EMPIRE-GREATNESS",	// 30
	"DIPLOMACY",
	"HAVE-CITY",
	"EMPIRE-MILITARY",
	"EMPIRE-PRODUCED-OBJECT",
	"EMPIRE-PRODUCED-COMPONENT",	// 35
	"EVENT-RUNNING",
	"EVENT-NOT-RUNNING",
	"LEVEL-UNDER",
	"LEVEL-OVER",
	"OWN-VEHICLE-FUNCTION",	// 40
	"SPEAK-LANGUAGE",
	"RECOGNIZE-LANGUAGE",
	"COMPLETED-QUEST-EVER",
	"DAYTIME",
	"NIGHTTIME",	// 45
	"DIPLOMACY-OVER",
	"OWN-ROADS",
	"\n",
};


// REQ_x (2/3): requirement types that take a numeric arg
const bool requirement_amt_type[] = {
	REQ_AMT_NONE,	// 0, completed quest
	REQ_AMT_NUMBER,	// get component
	REQ_AMT_NUMBER,	// get object
	REQ_AMT_NUMBER,	// kill mob
	REQ_AMT_NUMBER,	// kill mob flagged
	REQ_AMT_NONE,	// 5, not completed quest
	REQ_AMT_NONE,	// not on quest
	REQ_AMT_NUMBER,	// own building
	REQ_AMT_NUMBER,	// own vehicle
	REQ_AMT_THRESHOLD,	// skill over
	REQ_AMT_THRESHOLD,	// 10, skill under
	REQ_AMT_NUMBER,	// triggered
	REQ_AMT_NONE,	// visit building
	REQ_AMT_NONE,	// visit rmt
	REQ_AMT_NONE,	// visit sect
	REQ_AMT_NONE,	// 15, have ability
	REQ_AMT_REPUTATION,	// faction-over
	REQ_AMT_REPUTATION,	// faction-under
	REQ_AMT_NONE,	// wearing
	REQ_AMT_NONE,	// wearing-or-has
	REQ_AMT_NUMBER,	// 20, get currency
	REQ_AMT_NUMBER,	// get coins
	REQ_AMT_NONE,	// can gain skill
	REQ_AMT_NUMBER,	// crop variety
	REQ_AMT_NUMBER,	// own homes
	REQ_AMT_NUMBER,	// 25, own sector
	REQ_AMT_NUMBER,	// own building function
	REQ_AMT_NUMBER,	// own vehicle flagged
	REQ_AMT_NUMBER,	// empire wealth
	REQ_AMT_NUMBER,	// empire fame
	REQ_AMT_NUMBER,	// 30, empire greatness
	REQ_AMT_NUMBER,	// diplomacy
	REQ_AMT_NUMBER,	// have city
	REQ_AMT_NUMBER,	// empire military
	REQ_AMT_NUMBER,	// empire produced object
	REQ_AMT_NUMBER,	// 35, empire produced component
	REQ_AMT_NONE,	// event running
	REQ_AMT_NONE,	// event not running
	REQ_AMT_THRESHOLD,	// level under
	REQ_AMT_THRESHOLD,	// level over
	REQ_AMT_NUMBER,	// 40, own-vehicle-function
	REQ_AMT_NONE,	// speak-language
	REQ_AMT_NONE,	// recognize-language
	REQ_AMT_NONE,	// completed-quest-ever
	REQ_AMT_NONE,	// daytime
	REQ_AMT_NONE,	// 45, nighttime
	REQ_AMT_NUMBER,	// diplomacy-over
	REQ_AMT_NUMBER,	// own-roads
};


// REQ_x (3/3): types that require a quest tracker (can't be determined in realtime)
const bool requirement_needs_tracker[] = {
	FALSE,	// 0, completed quest
	FALSE,	// get component
	FALSE,	// get object
	TRUE,	// kill mob
	TRUE,	// kill mob flagged
	FALSE,	// 5, not completed quest
	FALSE,	// not on quest
	FALSE,	// own building
	FALSE,	// own vehicle
	FALSE,	// skill over
	FALSE,	// 10, skill under
	TRUE,	// triggered
	TRUE,	// visit building
	TRUE,	// visit rmt
	TRUE,	// visit sect
	FALSE,	// 15, have ability
	FALSE,	// faction-over
	FALSE,	// faction-under
	FALSE,	// wearing
	FALSE,	// wearing-or-has
	FALSE,	// 20, get currency
	FALSE,	// get coins
	FALSE,	// can gain skill
	FALSE,	// crop variety
	FALSE,	// own homes
	FALSE,	// 25, own sector
	FALSE,	// own building function
	FALSE,	// own vehicle flagged
	FALSE,	// empire wealth
	FALSE,	// empire fame
	FALSE,	// 30, empire greatness
	FALSE,	// diplomacy
	FALSE,	// have city
	FALSE,	// empire military
	FALSE,	// empire produced object
	FALSE,	// 35, empire produced component
	FALSE,	// event running
	FALSE,	// event not running
	FALSE,	// level under
	FALSE,	// level over
	FALSE,	// 40, own vehicle function
	FALSE,	// speak-language
	FALSE,	// recognize-language
	FALSE,	// completed quest ever
	FALSE,	// daytime
	FALSE,	// 45, nighttime
	FALSE,	// diplomacy-over
	FALSE,	// own-roads
};


// for command-interpreting
const char *reserved_words[] = {
	"a",
	"an",
	"some",
	"self",
	"me",
	"all",
	"room",
	"of",
	"someone",
	"something",
	"misc",
	"miscellaneous",
	"other",
	"\n"
};


// You can use any names you want for weekdays, but there are always 7
const char *weekdays[] = {
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	"Sunday",
	"\n"
};


// You can use any names you want for the months, so long as there are 12
const char *month_name[] = {
	"January",				"February",					"March",
	"April",				"May",						"June",
	"July",					"August",					"September",
	"October",				"November",					"December",
	"\n"
};


// used for olc parsers
const char *offon_types[] = {
	"off",
	"on",
	"\n"
};


// VEH_x (1/2): Vehicle flags
const char *vehicle_flags[] = {
	"*INCOMPLETE",	// 0
	"DRIVING",
	"SAILING",
	"FLYING",
	"ALLOW-ROUGH",
	"SIT",	// 5
	"IN",
	"BURNABLE",
	"CONTAINER",
	"SHIPPING",
	"CUSTOMIZABLE",	// 10
	"DRAGGABLE",
	"!BUILDING",
	"CAN-PORTAL",
	"LEADABLE",
	"CARRY-VEHICLES",	// 15
	"CARRY-MOBS",
	"SIEGE-WEAPONS",
	"ON-FIRE",
	"!LOAD-ONTO-VEHICLE",
	"VISIBLE-IN-DARK",	// 20
	"!CLAIM",
	"BUILDING",
	"NEVER-DISMANTLE",
	"*PLAYER-NO-DISMANTLE",
	"*DISMANTLING",	// 25
	"*PLAYER-NO-WORK",
	"CHAMELEON",
	"INTERLINK",
	"IS-RUINS",
	"SLEEP",	// 30
	"!PAINT",
	"*BRIGHT-PAINT",
	"DEDICATE",
	"*EXTRACTED",
	"RUIN-SLOWLY-FROM-CLIMATE",	// 35
	"RUIN-QUICKLY-FROM-CLIMATE",
	"OBSCURE-VISION",
	"*INSTANCE",
	"*TEMPORARY",
	"TINY",	// 40
	"\n"
};


// VEH_x (2/2): "identify <vehicle>" text for flags
const char *identify_vehicle_flags[] = {
	"",	// *INCOMPLETE	// 0
	"can drive",
	"can sail",
	"can pilot (flying)",
	"rough terrain (mountainwalk)",
	"",	// SIT (has special handling)	// 5
	"",	// IN (has sepcial meaning)
	"burnable",
	"container (get/put/look in)",
	"used for shipping",
	"customizable",	// 10
	"draggable",
	"large (cannot enter buildings)",
	"can use portals",
	"can be led",
	"can carry vehicles",	// 15
	"can carry NPCs",
	"has siege weapons",
	"",	// ON-FIRE
	"cannot be loaded onto vehicles",
	"",	// VISIBLE-IN-DARK	// 20
	"",	// !CLAIM
	"",	// BUILDING
	"",	// NEVER-DISMANTLE
	"is set no-dismantle",
	"being dismantled",	// 25
	"is set no-work",
	"chameleon",
	"can interlink",
	"is ruined",
	"",	// SLEEP (special handling)	// 30
	"",	// NO-PAINT
	"",	// BRIGHT-PAINT
	"can dedicate",
	"",	// *EXTRACTED
	"",	// RUIN-SLOWLY-FROM-CLIMATE	// 35
	"",	// RUIN-QUICKLY-FROM-CLIMATE
	"",	// OBSCURE-VISION
	"",	// *INSTANCE
	"",	// *TEMPORARY
	"",	// TINY
	"\n"
};


// VEH_CUSTOM_x: custom message types
const char *veh_custom_types[] = {
	"ruins-to-room",	// 0
	"climate-change-to-room",
	"enter-to-inside",
	"enter-to-outside",
	"exit-to-inside",
	"exit-to-outside",	// 5
	"\n"
};


// VSPEED_x: Vehicle speed classes
const char *vehicle_speed_types[] = {
	"Very Slow",
	"Slow",
	"Normal",
	"Fast",
	"Very Fast",
	"\n"
};


// WAIT_x: Wait types for the command_lag() function.
const char *wait_types[] = {
	"NONE",	// 0
	"ABILITY",
	"COMBAT-ABILITY",
	"COMBAT-SPELL",
	"MOVEMENT",
	"SPELL",	// 5
	"OTHER",
	"LONG",
	"VERY-LONG",
	"\n"
};
