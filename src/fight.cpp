/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External procedures */
char *fread_action(FILE *fl, int nr);
ACMD(do_flee);
int backstab_mult(int level);
int thaco(int ch_class, int level);
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);

/* local functions */
void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type);
void appear(struct char_data *ch);
void load_messages(void);
void free_messages(void);
void free_messages_type(struct msg_type *msg);
void check_killer(struct char_data *ch, struct char_data *vict);
void make_corpse(struct char_data *ch);
void change_alignment(struct char_data *ch, struct char_data *victim);
void death_cry(struct char_data *ch);
void raw_kill(struct char_data *ch);
void die(struct char_data *ch);
void group_gain(struct char_data *ch, struct char_data *victim);
void solo_gain(struct char_data *ch, struct char_data *victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
int compute_armor_class(struct char_data *ch);
int compute_thaco(struct char_data *ch, struct char_data *vict);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},		/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},	/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

void appear(struct char_data *ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);

  if (GET_LEVEL(ch) < LVL_IMMORT)
    act("$n slowly fades into existence.", FALSE, ch, 0, 0, CommTarget::TO_ROOM);
  else
    act("You feel a strange presence as $n appears, seemingly from nowhere.",
	FALSE, ch, 0, 0, CommTarget::TO_ROOM);
}


int compute_armor_class(struct char_data *ch)
{
  int armorclass = GET_AC(ch);

  if (AWAKE(ch))
    armorclass += dex_app[GET_DEX(ch)].defensive * 10;

  return (MAX(-100, armorclass));      /* -100 is lowest */
}


void free_messages_type(struct msg_type *msg)
{
  if (msg->attacker_msg)	free(msg->attacker_msg);
  if (msg->victim_msg)		free(msg->victim_msg);
  if (msg->room_msg)		free(msg->room_msg);
}


void free_messages(void)
{
  int i;

  for (i = 0; i < MAX_MESSAGES; i++)
    while (fight_messages[i].msg) {
      struct message_type *former = fight_messages[i].msg;

      free_messages_type(&former->die_msg);
      free_messages_type(&former->miss_msg);
      free_messages_type(&former->hit_msg);
      free_messages_type(&former->god_msg);

      fight_messages[i].msg = fight_messages[i].msg->next;
      free(former);
    }
}


void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    basic_mud_log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }

  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = NULL;
  }

  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      basic_mud_log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    messages = new message_type;
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}


void update_pos(struct char_data *victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}


void check_killer(struct char_data *ch, struct char_data *vict)
{
  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;

  SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
  send_to_char(ch, "If you want to be a PLAYER KILLER, so be it...\r\n");
  mudlog(BRF, LVL_IMMORT, TRUE, "PC Killer bit set on %s for initiating attack on %s at %s.",
	 GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name.c_str());
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!pk_allowed)
    check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
}



void make_corpse(struct char_data *ch)
{
  char buf2[MAX_NAME_LENGTH + 64];
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  IN_ROOM(corpse) = NOWHERE;
  corpse->name = strdup("corpse");

  snprintf(buf2, sizeof(buf2), "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "the corpse of %s", GET_NAME(ch));
  corpse->short_description = strdup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_WEAR(corpse) = ITEM_WEAR_TAKE;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, 3) = 1;	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time;
  else
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i))
      obj_to_obj(unequip_char(ch, i), corpse);

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /*
     * following 'if' clause added to fix gold duplication loophole
     * The above line apparently refers to the old "partially log in,
     * kill the game character, then finish login sequence" duping
     * bug. The duplication has been fixed (knock on wood) but the
     * test below shall live on, for a while. -gg 3/3/2002
     */
    if (IS_NPC(ch) || ch->desc) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  obj_to_room(corpse, IN_ROOM(ch));
}


/* When ch kills victim */
void change_alignment(struct char_data *ch, struct char_data *victim)
{
  /*
   * new alignment change algorithm: if you kill a monster with alignment A,
   * you move 1/16th of the way to having alignment -A.  Simple and fast.
   */
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}



void death_cry(struct char_data *ch)
{
  int door;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, CommTarget::TO_ROOM);

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (CAN_GO(ch, door))
      send_to_room(std::get<0>(world[IN_ROOM(ch)].dir_option[door]).to_room, "Your blood freezes as you hear someone's death cry.\r\n");
}



void raw_kill(struct char_data *ch)
{
  if (FIGHTING(ch))
    stop_fighting(ch);

  std::for_each(ch->affected.begin(), ch->affected.end(), [&ch](affected_type &a) { affect_remove(ch, a); });

  death_cry(ch);

  make_corpse(ch);
  extract_char(ch);
}



void die(struct char_data *ch)
{
  gain_exp(ch, -(GET_EXP(ch) / 2));
  if (!IS_NPC(ch))
    REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
  raw_kill(ch);
}



void perform_group_gain(struct char_data *ch, int base,
			     struct char_data *victim)
{
  int share;

  share = MIN(max_exp_gain, MAX(1, base));

  if (share > 1)
    send_to_char(ch, "You receive your share of experience -- %d points.\r\n", share);
  else
    send_to_char(ch, "You receive your share of experience -- one measly little point!\r\n");

  gain_exp(ch, share);
  change_alignment(ch, victim);
}


void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_members, base, tot_gain;
  struct char_data *k;
  struct follow_type *f;

  if (!(k = ch->master))
    k = ch;

  if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
    tot_members = 1;
  else
    tot_members = 0;

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch))
      tot_members++;

  /* round up to the next highest tot_members */
  tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(max_exp_loss * 2 / 3, tot_gain);

  if (tot_members >= 1)
    base = MAX(1, tot_gain / tot_members);
  else
    base = 0;

  if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch))
    perform_group_gain(k, base, victim);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch))
      perform_group_gain(f->follower, base, victim);
}


void solo_gain(struct char_data *ch, struct char_data *victim)
{
  int exp;

  exp = MIN(max_exp_gain, GET_EXP(victim) / 3);

  /* Calculate level-difference bonus */
  if (IS_NPC(ch))
    exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
  else
    exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);

  exp = MAX(exp, 1);

  if (exp > 1)
    send_to_char(ch, "You receive %d experience points.\r\n", exp);
  else
    send_to_char(ch, "You receive one lousy experience point.\r\n");

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[256];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data *ch, struct char_data *victim,
		      int w_type)
{
  char *buf;
  int msgnum;

  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n tries to #w $N, but misses.",	/* 0: 0     */
      "You try to #w $N, but miss.",
      "$n tries to #w you, but misses."
    },

    {
      "$n tickles $N as $e #W $M.",	/* 1: 1..2  */
      "You tickle $N as you #w $M.",
      "$n tickles you as $e #W you."
    },

    {
      "$n barely #W $N.",		/* 2: 3..4  */
      "You barely #w $N.",
      "$n barely #W you."
    },

    {
      "$n #W $N.",			/* 3: 5..6  */
      "You #w $N.",
      "$n #W you."
    },

    {
      "$n #W $N hard.",			/* 4: 7..10  */
      "You #w $N hard.",
      "$n #W you hard."
    },

    {
      "$n #W $N very hard.",		/* 5: 11..14  */
      "You #w $N very hard.",
      "$n #W you very hard."
    },

    {
      "$n #W $N extremely hard.",	/* 6: 15..19  */
      "You #w $N extremely hard.",
      "$n #W you extremely hard."
    },

    {
      "$n massacres $N to small fragments with $s #w.",	/* 7: 19..23 */
      "You massacre $N to small fragments with your #w.",
      "$n massacres you to small fragments with $s #w."
    },

    {
      "$n OBLITERATES $N with $s deadly #w!!",	/* 8: > 23   */
      "You OBLITERATE $N with your deadly #w!!",
      "$n OBLITERATES you with $s deadly #w!!"
    }
  };


  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 4)    msgnum = 2;
  else if (dam <= 6)    msgnum = 3;
  else if (dam <= 10)   msgnum = 4;
  else if (dam <= 14)   msgnum = 5;
  else if (dam <= 19)   msgnum = 6;
  else if (dam <= 23)   msgnum = 7;
  else			msgnum = 8;

  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, CommTarget::TO_NOTVICT);

  /* damage message to damager */
  send_to_char(ch, CCYEL(ch, C_CMP));
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, CommTarget::TO_CHAR);
  send_to_char(ch, CCNRM(ch, C_CMP));

  /* damage message to damagee */
  send_to_char(victim, CCRED(victim, C_CMP));
  buf = replace_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  act(buf, FALSE, ch, NULL, victim, CommTarget::TO_VICT | CommTarget::TO_SLEEP);
  send_to_char(victim, CCNRM(victim, C_CMP));
}


/*
 *  message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype)
{
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT)) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, CommTarget::TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, CommTarget::TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, CommTarget::TO_NOTVICT);
      } else if (dam != 0) {
        /*
         * Don't send redundant color codes for TYPE_SUFFERING & other types
         * of damage without attacker_msg.
         */
	if (GET_POS(vict) == POS_DEAD) {
          if (msg->die_msg.attacker_msg) {
            send_to_char(ch, CCYEL(ch, C_CMP));
            act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, CommTarget::TO_CHAR);
            send_to_char(ch, CCNRM(ch, C_CMP));
          }

	  send_to_char(vict, CCRED(vict, C_CMP));
	  act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, CommTarget::TO_VICT | CommTarget::TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));

	  act(msg->die_msg.room_msg, FALSE, ch, weap, vict, CommTarget::TO_NOTVICT);
	} else {
          if (msg->hit_msg.attacker_msg) {
	    send_to_char(ch, CCYEL(ch, C_CMP));
	    act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, CommTarget::TO_CHAR);
	    send_to_char(ch, CCNRM(ch, C_CMP));
          }

	  send_to_char(vict, CCRED(vict, C_CMP));
	  act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, CommTarget::TO_VICT | CommTarget::TO_SLEEP);
	  send_to_char(vict, CCNRM(vict, C_CMP));

	  act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, CommTarget::TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
        if (msg->miss_msg.attacker_msg) {
	  send_to_char(ch, CCYEL(ch, C_CMP));
	  act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, CommTarget::TO_CHAR);
	  send_to_char(ch, CCNRM(ch, C_CMP));
        }

	send_to_char(vict, CCRED(vict, C_CMP));
	act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, CommTarget::TO_VICT | CommTarget::TO_SLEEP);
	send_to_char(vict, CCNRM(vict, C_CMP));

	act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, CommTarget::TO_NOTVICT);
      }
      return (1);
    }
  }
  return (0);
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype)
{
  if (GET_POS(victim) <= POS_DEAD) {
    /* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
    if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
      return (-1);

    basic_mud_log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
		GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim);
    return (-1);			/* -je, 7/7/92 */
  }

  /* peaceful rooms */
  if (ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return (0);
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    return (0);

  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT))
    dam = 0;

  if (victim != ch) {
    /* Start the attacker fighting the victim */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, victim);

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }
  }

  /* If you attack a pet, it hates your guts */
  if (victim->master == ch)
    stop_follower(victim);

  /* If the attacker is invisible, he becomes visible */
  if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
    appear(ch);

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
    dam /= 2;

  /* Check for PK if this is not a PK MUD */
  if (!pk_allowed) {
    check_killer(ch, victim);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }

  /* Set the maximum damage per round and subtract the hit points */
  dam = MAX(MIN(dam, 100), 0);
  GET_HIT(victim) -= dam;

  /* Gain exp for the hit */
  if (ch != victim)
    gain_exp(ch, GET_LEVEL(victim) * dam);

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   * 
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if (!IS_WEAPON(attacktype))
    skill_message(dam, ch, victim, attacktype);
  else {
    if (GET_POS(victim) == POS_DEAD || dam == 0) {
      if (!skill_message(dam, ch, victim, attacktype))
	dam_message(dam, ch, victim, attacktype);
    } else {
      dam_message(dam, ch, victim, attacktype);
    }
  }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, CommTarget::TO_ROOM);
    send_to_char(victim, "You are mortally wounded, and will die soon, if not aided.\r\n");
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, CommTarget::TO_ROOM);
    send_to_char(victim, "You are incapacitated an will slowly die, if not aided.\r\n");
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, CommTarget::TO_ROOM);
    send_to_char(victim, "You're stunned, but will probably regain consciousness again.\r\n");
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, CommTarget::TO_ROOM);
    send_to_char(victim, "You are dead!  Sorry...\r\n");
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) / 4))
      send_to_char(victim, "That really did HURT!\r\n");

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
      send_to_char(victim, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
		CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))
	do_flee(victim, NULL, 0, 0);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0) {
      send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
      do_flee(victim, NULL, 0, 0);
    }
    break;
  }

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {
    do_flee(victim, NULL, 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, CommTarget::TO_ROOM);
      GET_WAS_IN(victim) = IN_ROOM(victim);
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  /* stop someone from fighting if they're stunned or worse */
  if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
    stop_fighting(victim);

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (AFF_FLAGGED(ch, AFF_GROUP))
	group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, LVL_IMMORT, TRUE, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name.c_str());
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    die(victim);
    return (-1);
  }
  return (dam);
}


/*
 * Calculate the THAC0 of the attacker.
 *
 * 'victim' currently isn't used but you could use it for special cases like
 * weapons that hit evil creatures easier or a weapon that always misses
 * attacking an animal.
 */
int compute_thaco(struct char_data *ch, struct char_data *victim)
{
  (void)victim;
  int calc_thaco;

  if (!IS_NPC(ch))
    calc_thaco = thaco(GET_CLASS(ch), GET_LEVEL(ch));
  else		/* THAC0 for monsters is set in the HitRoll */
    calc_thaco = 20;
  calc_thaco -= str_app[STRENGTH_APPLY_INDEX(ch)].tohit;
  calc_thaco -= GET_HITROLL(ch);
  calc_thaco -= (int) ((GET_INT(ch) - 13) / 1.5);	/* Intelligence helps! */
  calc_thaco -= (int) ((GET_WIS(ch) - 13) / 1.5);	/* So does wisdom */

  return calc_thaco;
}


void hit(struct char_data *ch, struct char_data *victim, int type)
{
  struct obj_data *wielded = GET_EQ(ch, WEAR_WIELD);
  int w_type, victim_ac, calc_thaco, dam, diceroll;

  /* Do some sanity checking, in case someone flees, etc. */
  if (IN_ROOM(ch) != IN_ROOM(victim)) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  /* Find the weapon type (for display purposes only) */
  if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
    w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
  else {
    if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
    else
      w_type = TYPE_HIT;
  }

  /* Calculate chance of hit. Lower THAC0 is better for attacker. */
  calc_thaco = compute_thaco(ch, victim);

  /* Calculate the raw armor including magic armor.  Lower AC is better for defender. */
  victim_ac = compute_armor_class(victim) / 10;

  /* roll the die and take your chances... */
  diceroll = rand_number(1, 20);

  /*
   * Decide whether this is a hit or a miss.
   *
   *  Victim asleep = hit, otherwise:
   *     1   = Automatic miss.
   *   2..19 = Checked vs. AC.
   *    20   = Automatic hit.
   */
  if (diceroll == 20 || !AWAKE(victim))
    dam = TRUE;
  else if (diceroll == 1)
    dam = FALSE;
  else
    dam = (calc_thaco - diceroll <= victim_ac);

  if (!dam)
    /* the attacker missed the victim */
    damage(ch, victim, 0, type == SKILL_BACKSTAB ? SKILL_BACKSTAB : w_type);
  else {
    /* okay, we know the guy has been hit.  now calculate damage. */

    /* Start with the damage bonuses: the damroll and strength apply */
    dam = str_app[STRENGTH_APPLY_INDEX(ch)].todam;
    dam += GET_DAMROLL(ch);

    /* Maybe holding arrow? */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      /* Add weapon-based damage if a weapon is being wielded */
      dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
    } else {
      /* If no weapon, add bare hand damage instead */
      if (IS_NPC(ch))
	dam += dice(ch->mob_specials.damnodice, ch->mob_specials.damsizedice);
      else
	dam += rand_number(0, 2);	/* Max 2 bare hand damage for players */
    }

    /*
     * Include a damage multiplier if victim isn't ready to fight:
     *
     * Position sitting  1.33 x normal
     * Position resting  1.66 x normal
     * Position sleeping 2.00 x normal
     * Position stunned  2.33 x normal
     * Position incap    2.66 x normal
     * Position mortally 3.00 x normal
     *
     * Note, this is a hack because it depends on the particular
     * values of the POSITION_XXX constants.
     */
    if (GET_POS(victim) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);

    if (type == SKILL_BACKSTAB)
      damage(ch, victim, dam * backstab_mult(GET_LEVEL(ch)), SKILL_BACKSTAB);
    else
      damage(ch, victim, dam, w_type);
  }
}



/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(ch);
      continue;
    }

    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
	GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
	continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
	GET_POS(ch) = POS_FIGHTING;
	act("$n scrambles to $s feet!", TRUE, ch, 0, 0, CommTarget::TO_ROOM);
      }
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      send_to_char(ch, "You can't fight while sitting!!\r\n");
      continue;
    }

    hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
      char actbuf[MAX_INPUT_LENGTH] = "";
      (GET_MOB_SPEC(ch)) (ch, ch, 0, actbuf);
    }
  }
}
