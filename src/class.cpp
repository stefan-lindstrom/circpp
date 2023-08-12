/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */
#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "class.h"
#include "config.h"
#include "act.h"
#include "utils.h"

/* Names first */
const char *class_abbrevs[] = {
  "Mu",
  "Cl",
  "Th",
  "Wa",
  "\n"
};


const char *pc_class_types[] = {
  "Magic User",
  "Cleric",
  "Thief",
  "Warrior",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n"
"Select a class:\r\n"
"  [C]leric\r\n"
"  [T]hief\r\n"
"  [W]arrior\r\n"
"  [M]agic-user\r\n";



/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char arg)
{
  arg = LOWER(arg);

  switch (arg) {
  case 'm': return CLASS_MAGIC_USER;
  case 'c': return CLASS_CLERIC;
  case 'w': return CLASS_WARRIOR;
  case 't': return CLASS_THIEF;
  default:  return CLASS_UNDEFINED;
  }
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.) up to the limit of your bitvector_t, typically 0-31.
 */
bitvector_t find_class_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_class(arg[rpos]));

  return (ret);
}


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
  /* MAG	CLE	THE	WAR */
  { 95,		95,	85,	80	},	/* learned level */
  { 100,	100,	12,	12	},	/* max per practice */
  { 25,		25,	0,	0	},	/* min per practice */
  { SPELL,	SPELL,	SKILL,	SKILL	},	/* prac name */
};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
struct guild_info_type guild_info[] = {

/* Midgaard */
  { CLASS_MAGIC_USER,	3017,	SCMD_SOUTH	},
  { CLASS_CLERIC,	3004,	SCMD_NORTH	},
  { CLASS_THIEF,	3027,	SCMD_EAST	},
  { CLASS_WARRIOR,	3021,	SCMD_EAST	},

/* Brass Dragon */
  { -999 /* all */ ,	5065,	SCMD_WEST	},

/* this must go last -- add new guards above! */
  { -1, NOWHERE, -1}
};



/*
 * Saving throws for:
 * MCTW
 *   PARA, ROD, PETRI, BREATH, SPELL
 *     Levels 0-40
 *
 * Do not forget to change extern declaration in magic.c if you add to this.
 */

namespace {
  byte save_throws[NUM_CLASSES][NUM_SAVES][52] 
  {
    // Magic user
    {
      { 90, 70, 69, 68, 67, 66, 65, 63, 61, 60, 59, 57, 55, 54, 53, 53, 52, 51, 50, 48, 46, 45, 44, 42, 40, 38, 36, 34, 32, 30, 28, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 55, 53, 51, 49, 47, 45, 43, 41, 40, 39, 37, 35, 33, 31, 30, 29, 27, 25, 23, 21, 20, 19, 17, 15, 14, 13, 12, 11, 10, 9 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 65, 63, 61, 59, 57, 55, 53, 51, 50, 49, 47, 45, 43, 41, 40, 39, 37, 35, 33, 31, 30, 29, 27, 25, 23, 21, 19, 17, 15, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 75, 73, 71, 69, 67, 65, 63, 61, 60, 59, 57, 55, 53, 51, 50, 49, 47, 45, 43, 41, 40, 39, 37, 35, 33, 31, 29, 27, 25, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 60, 58, 56, 54, 52, 50, 48, 46, 45, 44, 42, 40, 38, 36, 35, 34, 32, 30, 28, 26, 25, 24, 22, 20, 18, 16, 14, 12, 10,  8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    // Cleric
    {
      { 90, 60, 59, 48, 46, 45, 43, 40, 37, 35, 34, 33, 31, 30, 29, 27, 26, 25, 24, 23, 22, 21, 20, 18, 15, 14, 12, 10,  9,  8, 7 , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 70, 69, 68, 66, 65, 63, 60, 57, 55, 54, 53, 51, 50, 49, 47, 46, 45, 44, 43, 42, 41, 40, 38, 35, 34, 32, 30, 29, 28, 27, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 65, 64, 63, 61, 60, 58, 55, 53, 50, 49, 48, 46, 45, 44, 43, 41, 40, 39, 38, 37, 36, 35, 33, 31, 29, 27, 25, 24, 23, 22, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 80, 79, 78, 76, 75, 73, 70, 67, 65, 64, 63, 61, 60, 59, 57, 56, 55, 54, 53, 52, 51, 50, 48, 45, 44, 42, 40, 39, 38, 37, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 75, 74, 73, 71, 70, 68, 65, 63, 60, 59, 58, 56, 55, 54, 53, 51, 50, 49, 48, 47, 46, 45, 43, 41, 39, 37, 35, 34, 33, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    // Thief
    {
      { 90, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 70, 68, 66, 64, 62, 60, 58, 56, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 60, 59, 58, 58, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 75, 73, 71, 69, 67, 65, 63, 61, 59, 57, 55, 53, 51, 49, 47, 45, 43, 41, 39, 37, 35, 33, 31, 29, 27, 25, 23, 21, 19, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    },
    // Fighter
    {
      { 90, 70, 68, 67, 65, 62, 58, 55, 53, 52, 50, 47, 43, 40, 38, 37, 35, 32, 28, 25, 24, 23, 22, 20, 19, 17, 16, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6, 5,  4,  3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 80, 78, 77, 75, 72, 68, 65, 63, 62, 60, 57, 53, 50, 48, 47, 45, 42, 38, 35, 34, 33, 32, 30, 29, 27, 26, 25, 24, 23, 22, 20, 18, 16, 14, 12, 10, 8,  6,  5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 75, 73, 72, 70, 67, 63, 60, 58, 57, 55, 52, 48, 45, 43, 42, 40, 37, 33, 30, 29, 28, 26, 25, 24, 23, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9,  8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0 },
      { 90, 85, 83, 82, 80, 75, 70, 65, 63, 62, 60, 55, 50, 45, 43, 42, 40, 37, 33, 30, 29, 28, 26, 25, 24, 23, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9,  8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0 },      
      { 90, 85, 83, 82, 80, 77, 73, 70, 68, 67, 65, 62, 58, 55, 53, 52, 50, 47, 43, 40, 39, 38, 36, 35, 34, 33, 31, 30, 29, 28, 27, 25, 23, 21, 19, 17, 15, 13, 11, 9, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0 }
    }
  };

  int thac0[NUM_CLASSES][35]  
  { 
    // mage
    { 100, 20, 20, 20, 19, 19, 19, 18, 18, 18, 17, 17, 17, 16, 16, 16, 15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 10, 9 },

    // cleric
    { 100, 20, 20, 20, 18, 18, 18, 16, 16, 16, 14, 14, 14, 12, 12, 12, 10, 10, 10,  8,  8,  8,  6,  6,  6,  4,  4,  4,  2,  2,  2,  1,  1,  1, 1 },

    // thief
    { 100, 20, 20, 19, 19, 18, 18, 17, 17, 16, 16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, 10,  9,  9,  8,  8,  7,  7,  6,  6,  5,  5,  4, 4 },

    // warrior
    { 100, 20, 19, 18, 17, 16, 15, 14, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, 1 }
  };
}

byte saving_throws(int class_num, int type, int level)
{
  if (not_in_range(class_num, 0, NUM_CLASSES - 1) || not_in_range(type, 0, NUM_SAVES -1) || not_in_range(level, 0, 50)) {
    basic_mud_log("SYSERR: Invalid saving throw inputs, class: %d, type %d, level %d.", class_num, type, level);
    return 100;
  }
  return save_throws[class_num][type][level];
}

/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(int class_num, int level)
{
  if (not_in_range(class_num, 0, NUM_CLASSES - 1) || not_in_range(level, 0, 34)) { 
    basic_mud_log("SYSERR: Invalid THAC0 inputs, class: %d, evel %d.", class_num, level);
    return 100;
  }
  return thac0[class_num][level];
}


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(struct char_data *ch)
{
  int i, j, k, temp;
  ubyte table[6];
  ubyte rolls[4];

  for (i = 0; i < 6; i++)
    table[i] = 0;

  for (i = 0; i < 6; i++) {

    for (j = 0; j < 4; j++)
      rolls[j] = rand_number(1, 6);

    temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
      MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

    for (k = 0; k < 6; k++)
      if (table[k] < temp) {
	temp ^= table[k];
	table[k] ^= temp;
	temp ^= table[k];
      }
  }

  ch->real_abils.str_add = 0;

  switch (GET_CLASS(ch)) {
  case CLASS_MAGIC_USER:
    ch->real_abils.intel = table[0];
    ch->real_abils.wis = table[1];
    ch->real_abils.dex = table[2];
    ch->real_abils.str = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_CLERIC:
    ch->real_abils.wis = table[0];
    ch->real_abils.intel = table[1];
    ch->real_abils.str = table[2];
    ch->real_abils.dex = table[3];
    ch->real_abils.con = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_THIEF:
    ch->real_abils.dex = table[0];
    ch->real_abils.str = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.intel = table[3];
    ch->real_abils.wis = table[4];
    ch->real_abils.cha = table[5];
    break;
  case CLASS_WARRIOR:
    ch->real_abils.str = table[0];
    ch->real_abils.dex = table[1];
    ch->real_abils.con = table[2];
    ch->real_abils.wis = table[3];
    ch->real_abils.intel = table[4];
    ch->real_abils.cha = table[5];
    if (ch->real_abils.str == 18)
      ch->real_abils.str_add = rand_number(0, 100);
    break;
  }
  ch->aff_abils = ch->real_abils;
}


/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch)
{
  GET_LEVEL(ch) = 1;
  GET_EXP(ch) = 1;

  set_title(ch, NULL);
  roll_real_abils(ch);

  GET_MAX_HIT(ch)  = 10;
  GET_MAX_MANA(ch) = 100;
  GET_MAX_MOVE(ch) = 82;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    break;

  case CLASS_CLERIC:
    break;

  case CLASS_THIEF:
    SET_SKILL(ch, SKILL_SNEAK, 10);
    SET_SKILL(ch, SKILL_HIDE, 5);
    SET_SKILL(ch, SKILL_STEAL, 15);
    SET_SKILL(ch, SKILL_BACKSTAB, 10);
    SET_SKILL(ch, SKILL_PICK_LOCK, 10);
    SET_SKILL(ch, SKILL_TRACK, 10);
    break;

  case CLASS_WARRIOR:
    break;
  }

  advance_level(ch);
  mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));

  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  if (siteok_everyone)
    SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data *ch)
{
  int add_hp, add_mana = 0, add_move = 0, i;

  add_hp = con_app[GET_CON(ch)].hitp;

  switch (GET_CLASS(ch)) {

  case CLASS_MAGIC_USER:
    add_hp += rand_number(3, 8);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(0, 2);
    break;

  case CLASS_CLERIC:
    add_hp += rand_number(5, 10);
    add_mana = rand_number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = rand_number(0, 2);
    break;

  case CLASS_THIEF:
    add_hp += rand_number(7, 13);
    add_mana = 0;
    add_move = rand_number(1, 3);
    break;

  case CLASS_WARRIOR:
    add_hp += rand_number(10, 15);
    add_mana = 0;
    add_move = rand_number(1, 3);
    break;
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
    GET_PRACTICES(ch) += MAX(2, wis_app[GET_WIS(ch)].bonus);
  else
    GET_PRACTICES(ch) += MIN(2, MAX(1, wis_app[GET_WIS(ch)].bonus));

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char) -1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  }

  snoop_check(ch);
  save_char(ch);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
  if (level <= 0) {
    return 1;	  /* level 0 */
  }
  else if (level <= 7) {
    return 2;	  /* level 1 - 7 */
  }
  else if (level <= 13) {
    return 3;	  /* level 8 - 13 */
  }
  else if (level <= 20) {
    return 4;	  /* level 14 - 20 */
  }
  else if (level <= 28) {
    return 5;	  /* level 21 - 28 */
  }
  else if (level < LVL_IMMORT) {
    return 6;	  /* all remaining mortal levels */
  }
  else {
    return 20;	  /* immortals */
  }
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */
int invalid_class(struct char_data *ch, struct obj_data *obj)
{
  if (OBJ_FLAGGED(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch)) {
    return TRUE;
  }

  if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch)) {
    return TRUE;
  }

  if (OBJ_FLAGGED(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch)) {
    return TRUE;
  }

  if (OBJ_FLAGGED(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch)) {
    return TRUE;
  }

  return FALSE;
}


/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_spell_levels(void)
{
  /* MAGES */
  spell_level(SPELL_MAGIC_MISSILE, CLASS_MAGIC_USER, 1);
  spell_level(SPELL_DETECT_INVIS, CLASS_MAGIC_USER, 2);
  spell_level(SPELL_DETECT_MAGIC, CLASS_MAGIC_USER, 2);
  spell_level(SPELL_CHILL_TOUCH, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_INFRAVISION, CLASS_MAGIC_USER, 3);
  spell_level(SPELL_INVISIBLE, CLASS_MAGIC_USER, 4);
  spell_level(SPELL_ARMOR, CLASS_MAGIC_USER, 4);
  spell_level(SPELL_BURNING_HANDS, CLASS_MAGIC_USER, 5);
  spell_level(SPELL_LOCATE_OBJECT, CLASS_MAGIC_USER, 6);
  spell_level(SPELL_STRENGTH, CLASS_MAGIC_USER, 6);
  spell_level(SPELL_SHOCKING_GRASP, CLASS_MAGIC_USER, 7);
  spell_level(SPELL_SLEEP, CLASS_MAGIC_USER, 8);
  spell_level(SPELL_LIGHTNING_BOLT, CLASS_MAGIC_USER, 9);
  spell_level(SPELL_BLINDNESS, CLASS_MAGIC_USER, 9);
  spell_level(SPELL_DETECT_POISON, CLASS_MAGIC_USER, 10);
  spell_level(SPELL_COLOR_SPRAY, CLASS_MAGIC_USER, 11);
  spell_level(SPELL_ENERGY_DRAIN, CLASS_MAGIC_USER, 13);
  spell_level(SPELL_CURSE, CLASS_MAGIC_USER, 14);
  spell_level(SPELL_POISON, CLASS_MAGIC_USER, 14);
  spell_level(SPELL_FIREBALL, CLASS_MAGIC_USER, 15);
  spell_level(SPELL_CHARM, CLASS_MAGIC_USER, 16);
  spell_level(SPELL_ENCHANT_WEAPON, CLASS_MAGIC_USER, 26);
  spell_level(SPELL_CLONE, CLASS_MAGIC_USER, 30);

  /* CLERICS */
  spell_level(SPELL_CURE_LIGHT, CLASS_CLERIC, 1);
  spell_level(SPELL_ARMOR, CLASS_CLERIC, 1);
  spell_level(SPELL_CREATE_FOOD, CLASS_CLERIC, 2);
  spell_level(SPELL_CREATE_WATER, CLASS_CLERIC, 2);
  spell_level(SPELL_DETECT_POISON, CLASS_CLERIC, 3);
  spell_level(SPELL_DETECT_ALIGN, CLASS_CLERIC, 4);
  spell_level(SPELL_CURE_BLIND, CLASS_CLERIC, 4);
  spell_level(SPELL_BLESS, CLASS_CLERIC, 5);
  spell_level(SPELL_DETECT_INVIS, CLASS_CLERIC, 6);
  spell_level(SPELL_BLINDNESS, CLASS_CLERIC, 6);
  spell_level(SPELL_INFRAVISION, CLASS_CLERIC, 7);
  spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 8);
  spell_level(SPELL_POISON, CLASS_CLERIC, 8);
  spell_level(SPELL_GROUP_ARMOR, CLASS_CLERIC, 9);
  spell_level(SPELL_CURE_CRITIC, CLASS_CLERIC, 9);
  spell_level(SPELL_SUMMON, CLASS_CLERIC, 10);
  spell_level(SPELL_REMOVE_POISON, CLASS_CLERIC, 10);
  spell_level(SPELL_WORD_OF_RECALL, CLASS_CLERIC, 12);
  spell_level(SPELL_EARTHQUAKE, CLASS_CLERIC, 12);
  spell_level(SPELL_DISPEL_EVIL, CLASS_CLERIC, 14);
  spell_level(SPELL_DISPEL_GOOD, CLASS_CLERIC, 14);
  spell_level(SPELL_SANCTUARY, CLASS_CLERIC, 15);
  spell_level(SPELL_CALL_LIGHTNING, CLASS_CLERIC, 15);
  spell_level(SPELL_HEAL, CLASS_CLERIC, 16);
  spell_level(SPELL_CONTROL_WEATHER, CLASS_CLERIC, 17);
  spell_level(SPELL_SENSE_LIFE, CLASS_CLERIC, 18);
  spell_level(SPELL_HARM, CLASS_CLERIC, 19);
  spell_level(SPELL_GROUP_HEAL, CLASS_CLERIC, 22);
  spell_level(SPELL_REMOVE_CURSE, CLASS_CLERIC, 26);

  /* THIEVES */
  spell_level(SKILL_SNEAK, CLASS_THIEF, 1);
  spell_level(SKILL_PICK_LOCK, CLASS_THIEF, 2);
  spell_level(SKILL_BACKSTAB, CLASS_THIEF, 3);
  spell_level(SKILL_STEAL, CLASS_THIEF, 4);
  spell_level(SKILL_HIDE, CLASS_THIEF, 5);
  spell_level(SKILL_TRACK, CLASS_THIEF, 6);

  /* WARRIORS */
  spell_level(SKILL_KICK, CLASS_WARRIOR, 1);
  spell_level(SKILL_RESCUE, CLASS_WARRIOR, 3);
  spell_level(SKILL_TRACK, CLASS_WARRIOR, 9);
  spell_level(SKILL_BASH, CLASS_WARRIOR, 12);
}


/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */
#define EXP_MAX  10000000
namespace {
  int exp[NUM_CLASSES][31] 
  {
    // mage
    { 0, 1, 2500, 5000, 10000, 20000, 40000, 60000, 90000, 135000, 250000, 375000, 750000, 1125000, 1500000, 1875000, 2250000, 2625000, 
      3000000, 3375000, 3750000, 4000000, 4300000, 4600000, 4900000, 5200000, 5500000, 5950000, 6400000, 6850000, 7400000 },

    // cleric
    { 0, 1, 1500, 3000, 6000, 13000, 27500, 55000, 110000, 225000, 450000, 675000, 900000, 1125000, 1350000, 1575000, 1800000, 2100000, 
      2400000, 2700000, 3000000, 3250000, 3500000, 3800000, 4100000, 4400000, 4800000, 5200000, 5600000, 6000000, 6400000 },

    // thief
    { 0, 1, 1250, 2500, 5000, 10000, 20000, 40000, 70000, 110000, 160000, 220000, 440000, 660000, 880000, 1100000, 1500000, 2000000, 
      2500000, 3000000, 3500000, 3650000, 3800000, 4100000, 4400000, 4700000, 5100000, 5500000, 5900000, 6300000, 6650000 },

    // warrior
    { 0, 1, 2000, 4000, 8000, 16000, 32000, 64000, 125000, 250000, 500000, 750000, 1000000, 1250000, 1500000, 1850000, 2200000, 2550000, 
      2900000, 3250000, 3600000, 3900000, 4200000, 4500000, 4800000, 5150000, 5500000, 5950000, 6400000, 6850000, 7400000 } 
  };
}

/* Function to return the exp required for each class/level */
int level_exp(int chclass, int level)
{
  if (level > LVL_IMPL || level < 0) {
    basic_mud_log("SYSERR: Requesting exp for invalid level %d!", level);
    return 0;
  }

  /*
   * Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist.
   */
   if (level > LVL_IMMORT) {
     return EXP_MAX - ((LVL_IMPL - level) * 1000);
   }

   if (level == LVL_IMMORT) {
      return EXP_MAX;
   }

   if (not_in_range(chclass, 0, NUM_CLASSES - 1)) {
     basic_mud_log("SYSERR: Requesting exp for invalid class %d!", chclass);
     return 123456;
   }
   return exp[chclass][level];
}


namespace {
  const char *male_titles[NUM_CLASSES][LVL_IMPL] {
    { "the Mage", "the Apprentice of Magic", "the Spell Student", "the Scholar of Magic", "the Delver in Spells", "the Medium of Magic", "the Scribe of Magic", 
      "the Seer", "the Sage", "the Illusionist", "the Abjurer", "the Invoker", "the Enchanter", "the Conjurer", "the Magician", "the Creator", "the Savant", 
      "the Magus", "the Wizard", "the Warlock", "the Sorcerer", "the Necromancer", "the Thaumaturge", "the Student of the Occult", "the Disciple of the Uncanny", 
      "the Minor Elemental", "the Greater Elemental", "the Crafter of Magics", "the Shaman", "the Keeper of Talismans", "the Archmage", "the Immortal Warlock",
      "the Avatar of Magic","the God of Magic"
    },
    { "the Cleric","the Believer", "the Attendant", "the Acolyte", "the Novice", "the Missionary", "the Adept", "the Deacon", "the Vicar", "the Priest", 
      "the Minister", "the Canon", "the Levite", "the Curate", "the Monk", "the Healer", "the Chaplain", "the Expositor", "the Bishop", "the Arch Bishop", 
      "the Patriarch", "the Cleric","the Cleric", "the Cleric", "the Cleric", "the Cleric", "the Cleric", "the Cleric", "the Cleric", 
      "the Cleric", "the Immortal Cardinal","the Inquisitor", "the God of good and evil", "the Cleric"
    },
    { "the Thief", "the Pilferer", "the Footpad", "the Filcher", "the Pick-Pocket", "the Sneak", "the Pincher", "the Cut-Purse", "the Snatcher", "the Sharper", 
      "the Rogue", "the Robber", "the Magsman", "the Highwayman", "the Burglar", "the Thief", "the Knifer", "the Quick-Blade", "the Killer", "the Brigand", 
      "the Cut-Throat", "the Thief", "the Thief", "the Thief", "the Thief", "the Thief", "the Thief", "the Thief", "the Thief", "the Thief", "the Thief", 
      "the Immortal Assasin", "the Demi God of thieves", "the God of thieves and tradesmen"
    },
    { "the Warrior", "the Swordpupil", "the Recruit", "the Sentry", "the Fighter", "the Soldier", "the Warrior", "the Veteran", "the Swordsman", "the Fencer", 
      "the Combatant", "the Hero", "the Myrmidon", "the Swashbuckler", "the Mercenary", "the Swordmaster", "the Lieutenant", "the Champion", "the Dragoon", 
      "the Cavalier", "the Knight", "the Warrior", "the Warrior", "the Warrior", "the Warrior", "the Warrior", "the Warrior", "the Warrior", "the Warrior", 
      "the Warrior", "the Warrior", "the Immortal Warlord", "the Extirpator", "the God of war"
    }
  };

  const char *female_titles[NUM_CLASSES][LVL_IMPL] {
    { "the Witch", "the Apprentice of Magic", "the Spell Student", "the Scholar of Magic", "the Delveress in Spells", "the Medium of Magic", "the Scribess of Magic", 
      "the Seeress", "the Sage", "the Illusionist", "the Abjuress", "the Invoker", "the Enchantress", "the Conjuress", "the Witch", "the Creator", "the Savant", 
      "the Craftess", "the Wizard", "the War Witch", "the Sorceress", "the Necromancress", "the Thaumaturgess", "the Student of the Occult", "the Disciple of the Uncanny", 
      "the Minor Elementress", "the Greater Elementress", "the Crafter of Magics", "Shaman", "the Keeper of Talismans", "Archwitch", "the Immortal Enchantress", 
      "the Empress of Magic", "the Goddess of Magic"
    },
    { "the Cleric", "the Believer", "the Attendant", "the Acolyte", "the Novice", "the Missionary", "the Adept", "the Deaconess", "the Vicaress", "the Priestess", 
      "the Lady Minister", "the Canon", "the Levitess", "the Curess", "the Nunne", "the Healess", "the Chaplain", "the Expositress", "the Bishop", 
      "the Arch Lady of the Church", "the Matriarch", "the Cleric", "the Cleric", "the Cleric", "the Cleric", "the Cleric", "the Cleric", "the Cleric", 
      "the Cleric", "the Cleric", "the Cleric", "the Immortal Priestess", "the Inquisitress", "the Goddess of good and evil"
    },
    { "the Thief", "the Pilferess", "the Footpad", "the Filcheress", "the Pick-Pocket", "the Sneak", "the Pincheress", "the Cut-Purse", "the Snatcheress", 
      "the Sharpress", "the Rogue", "the Robber", "the Magswoman", "the Highwaywoman", "the Burglaress", "the Thief", "the Knifer", "the Quick-Blade", 
      "the Murderess", "the Brigand", "the Cut-Throat", "the Thief", "the Thief", "the Thief", "the Thief", "the Thief", "the Thief", "the Thief", 
      "the Thief", "the Thief", "the Thief", "the Immortal Assasin", "the Demi Goddess of thieves", "the Goddess of thieves and tradesmen"
    },
    { "the Warrior", "the Swordpupil", "the Recruit", "the Sentress", "the Fighter", "the Soldier", "the Warrior", "the Veteran", "the Swordswoman", 
      "the Fenceress", "the Combatess", "the Heroine", "the Myrmidon", "the Swashbuckleress", "the Mercenaress", "the Swordmistress", "the Lieutenant", 
      "the Lady Champion", "the Lady Dragoon", "the Cavalier", "the Lady Knight", "the Warrior", "the Warrior", "the Warrior", "the Warrior", "the Warrior", 
      "the Warrior", "the Warrior", "the Warrior", "the Warrior", "the Warrior", "the Immortal Lady of War", "the Queen of Destruction", "the Goddess of war"
    }
  };
}
/* 
 * Default titles of male characters.
 */
const char *title_male(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL) {
    return "the Man";
  }
  if (level == LVL_IMPL) {
    return "the Implementor";
  } 
  if (not_in_range(chclass, 0, NUM_CLASSES - 1)) {
    return "the Classless";  
  }
  return male_titles[chclass][level];
}


/* 
 * Default titles of female characters.
 */
const char *title_female(int chclass, int level)
{
  if (level <= 0 || level > LVL_IMPL) {
    return "the Woman";
  }
  if (level == LVL_IMPL) {
    return "the Implementress";
  }
  if (not_in_range(chclass, 0, NUM_CLASSES - 1)) {
    return "the Classless";  
  }
  return female_titles[chclass][level];
}

