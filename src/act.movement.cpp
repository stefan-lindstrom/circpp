/* ************************************************************************
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "constants.h"
#include "config.h"
#include "act.h"
#include "interpreter.h"
#include "fight.h"


namespace {
  /* simple function to determine if char can walk on water */
  int has_boat(struct char_data *ch)
  {
    struct obj_data *obj;
    int i;

  /*
    if (ROOM_IDENTITY(IN_ROOM(ch)) == DEAD_SEA)
      return (1);
  */

    if (GET_LEVEL(ch) > LVL_IMMORT)
      return (1);

    if (AFF_FLAGGED(ch, AFF_WATERWALK))
      return (1);

    /* non-wearable boats in inventory will do it */
    for (obj = ch->carrying; obj; obj = obj->next_content)
      if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
        return (1);

    /* and any boat you're wearing will do it too */
    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
        return (1);

    return (0);
  }

  int has_key(struct char_data *ch, obj_vnum key)
  {
    struct obj_data *o;

    for (o = ch->carrying; o; o = o->next_content)
      if (GET_OBJ_VNUM(o) == key)
        return (1);

    if (GET_EQ(ch, WEAR_HOLD))
      if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key)
        return (1);

    return (0);
  }

  int find_door(struct char_data *ch, const char *type, char *dir, const char *cmdname)
  {
    int door;

    if (*dir) {     /* a direction was specified */
      if ((door = search_block(dir, dirs, FALSE)) == -1) {  /* Partial Match */
        send_to_char(ch, "That's not a direction.\r\n");
        return (-1);
      }
      if (EXIT(ch, door)) { /* Braces added according to indent. -gg */
        if (!GET_EXIT(ch, door).keyword.empty()) {
    if (isname(type, GET_EXIT(ch, door).keyword.c_str()))
      return (door);
    else {
      send_to_char(ch, "I see no %s there.\r\n", type);
      return (-1);
          }
        } else
    return (door);
      } else {
        send_to_char(ch, "I really don't see how you can %s anything there.\r\n", cmdname);
        return (-1);
      }
    } else {      /* try to locate the keyword */
      if (!*type) {
        send_to_char(ch, "What is it you want to %s?\r\n", cmdname);
        return (-1);
      }
      for (door = 0; door < NUM_OF_DIRS; door++)
        if (EXIT(ch, door))
          if (!GET_EXIT(ch, door).keyword.empty())
            if (isname(type, GET_EXIT(ch, door).keyword.c_str()))
              return (door);

      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
      return (-1);
    }
  }

  #define NEED_OPEN (1 << 0)
  #define NEED_CLOSED (1 << 1)
  #define NEED_UNLOCKED (1 << 2)
  #define NEED_LOCKED (1 << 3)

  const char *cmd_door[] =
  {
    "open",
    "close",
    "unlock",
    "lock",
    "pick"
  };

  const int flags_door[] =
  {
    NEED_CLOSED | NEED_UNLOCKED,
    NEED_OPEN,
    NEED_CLOSED | NEED_LOCKED,
    NEED_CLOSED | NEED_UNLOCKED,
    NEED_CLOSED | NEED_LOCKED
  };


  #define EXITN(room, door)   (std::get<0>(world[room].dir_option[door]))
  #define OPEN_DOOR(room, obj, door)  ((obj) ?\
      (REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
      (REMOVE_BIT(EXITN(room, door).exit_info, EX_CLOSED)))
  #define CLOSE_DOOR(room, obj, door) ((obj) ?\
      (SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :\
      (SET_BIT(EXITN(room, door).exit_info, EX_CLOSED)))
  #define LOCK_DOOR(room, obj, door)  ((obj) ?\
      (SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
      (SET_BIT(EXITN(room, door).exit_info, EX_LOCKED)))
  #define UNLOCK_DOOR(room, obj, door)  ((obj) ?\
      (REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
      (REMOVE_BIT(EXITN(room, door).exit_info, EX_LOCKED)))
  #define TOGGLE_LOCK(room, obj, door)  ((obj) ?\
      (TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :\
      (TOGGLE_BIT(EXITN(room, door).exit_info, EX_LOCKED)))

  void do_doorcmd(struct char_data *ch, struct obj_data *obj, int door, int scmd)
  {
    char buf[MAX_STRING_LENGTH];
    size_t len;
    room_rnum other_room = NOWHERE;
    struct room_direction_data *back = nullptr;

    len = snprintf(buf, sizeof(buf), "$n %ss ", cmd_door[scmd]);
    if (!obj && ((other_room = GET_EXIT(ch, door).to_room) != NOWHERE))
      if (std::get<1>(world[other_room].dir_option[rev_dir[door]])) {
        back = &std::get<0>(world[other_room].dir_option[rev_dir[door]]);
        if (back->to_room != IN_ROOM(ch)) {
    back = nullptr;
        }
      }

    switch (scmd) {
    case SCMD_OPEN:
      OPEN_DOOR(IN_ROOM(ch), obj, door);
      if (back)
        OPEN_DOOR(other_room, obj, rev_dir[door]);
      send_to_char(ch, "%s", OK.c_str());
      break;

    case SCMD_CLOSE:
      CLOSE_DOOR(IN_ROOM(ch), obj, door);
      if (back)
        CLOSE_DOOR(other_room, obj, rev_dir[door]);
      send_to_char(ch, "%s", OK.c_str());
      break;

    case SCMD_LOCK:
      LOCK_DOOR(IN_ROOM(ch), obj, door);
      if (back)
        LOCK_DOOR(other_room, obj, rev_dir[door]);
      send_to_char(ch, "*Click*\r\n");
      break;

    case SCMD_UNLOCK:
      UNLOCK_DOOR(IN_ROOM(ch), obj, door);
      if (back)
        UNLOCK_DOOR(other_room, obj, rev_dir[door]);
      send_to_char(ch, "*Click*\r\n");
      break;

    case SCMD_PICK:
      TOGGLE_LOCK(IN_ROOM(ch), obj, door);
      if (back)
        TOGGLE_LOCK(other_room, obj, rev_dir[door]);
      send_to_char(ch, "The lock quickly yields to your skills.\r\n");
      len = strlcpy(buf, "$n skillfully picks the lock on ", sizeof(buf));
      break;
    }

    /* Notify the room. */
    if (len < sizeof(buf))
      snprintf(buf + len, sizeof(buf) - len, "%s%s.",
         obj ? "" : "the ", obj ? "$p" : !GET_EXIT(ch, door).keyword.empty() ? "$F" : "door");
    if (!obj || IN_ROOM(obj) != NOWHERE)
      act(buf, FALSE, ch, obj, obj ? 0 : GET_EXIT(ch, door).keyword.c_str(), CommTarget::TO_ROOM);

    /* Notify the other room */
    if (back && (scmd == SCMD_OPEN || scmd == SCMD_CLOSE))
        send_to_room(GET_EXIT(ch, door).to_room, "The %s is %s%s from the other side.",
         !back->keyword.empty() ? fname(back->keyword.c_str()) : "door", cmd_door[scmd],
         scmd == SCMD_CLOSE ? "d" : "ed");
  }

  int ok_pick(struct char_data *ch, obj_vnum keynum, int pickproof, int scmd)
  {
    int percent, skill_lvl;

    if (scmd != SCMD_PICK)
      return (1);

    percent = rand_number(1, 101);
    skill_lvl = GET_SKILL(ch, SKILL_PICK_LOCK) + dex_app_skill[GET_DEX(ch)].p_locks;

    if (keynum == NOTHING)
      send_to_char(ch, "Odd - you can't seem to find a keyhole.\r\n");
    else if (pickproof)
      send_to_char(ch, "It resists your attempts to pick it.\r\n");
    else if (percent > skill_lvl)
      send_to_char(ch, "You failed to pick the lock.\r\n");
    else
      return (1);

    return (0);
  }
} // anonymous namespace
  

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
int do_simple_move(struct char_data *ch, int dir, int need_specials_check)
{
  char throwaway[MAX_INPUT_LENGTH] = ""; /* Functions assume writable. */
  room_rnum was_in;
  int need_movement;

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, throwaway))
    return (0);

  /* charmed? */
  if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && IN_ROOM(ch) == IN_ROOM(ch->master)) {
    send_to_char(ch, "The thought of leaving your master makes you weep.\r\n");
    act("$n bursts into tears.", FALSE, ch, 0, 0, CommTarget::TO_ROOM);
    return (0);
  }

  /* if this room or the one we're going to needs a boat, check for one */
  if ((SECT(IN_ROOM(ch)) == SECT_WATER_NOSWIM) ||
      (SECT(GET_EXIT(ch, dir).to_room) == SECT_WATER_NOSWIM)) {
    if (!has_boat(ch)) {
      send_to_char(ch, "You need a boat to go there.\r\n");
      return (0);
    }
  }

  /* move points needed is avg. move loss for src and destination sect type */
  need_movement = (movement_loss[SECT(IN_ROOM(ch))] +
		   movement_loss[SECT(GET_EXIT(ch, dir).to_room)]) / 2;

  if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
    if (need_specials_check && ch->master)
      send_to_char(ch, "You are too exhausted to follow.\r\n");
    else
      send_to_char(ch, "You are too exhausted.\r\n");

    return (0);
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_ATRIUM)) {
    if (!House_can_enter(ch, GET_ROOM_VNUM(GET_EXIT(ch, dir).to_room))) {
      send_to_char(ch, "That's private property -- no trespassing!\r\n");
      return (0);
    }
  }
  if (ROOM_FLAGGED(GET_EXIT(ch, dir).to_room, ROOM_TUNNEL) &&
      num_pc_in_room(&(world[GET_EXIT(ch, dir).to_room])) >= tunnel_size) {
    if (tunnel_size > 1)
      send_to_char(ch, "There isn't enough room for you to go there!\r\n");
    else
      send_to_char(ch, "There isn't enough room there for more than one person!\r\n");
    return (0);
  }
  /* Mortals and low level gods cannot enter greater god rooms. */
  if (ROOM_FLAGGED(GET_EXIT(ch, dir).to_room, ROOM_GODROOM) &&
	GET_LEVEL(ch) < LVL_GRGOD) {
    send_to_char(ch, "You aren't godly enough to use that room!\r\n");
    return (0);
  }

  /* Now we know we're allow to go into the room. */
  if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NPC(ch))
    GET_MOVE(ch) -= need_movement;

  if (!AFF_FLAGGED(ch, AFF_SNEAK)) {
    char buf2[MAX_STRING_LENGTH];

    snprintf(buf2, sizeof(buf2), "$n leaves %s.", dirs[dir]);
    act(buf2, TRUE, ch, 0, 0, CommTarget::TO_ROOM);
  }
  was_in = IN_ROOM(ch);
  char_from_room(ch);
  char_to_room(ch, std::get<0>(world[was_in].dir_option[dir]).to_room);

  if (!AFF_FLAGGED(ch, AFF_SNEAK))
    act("$n has arrived.", TRUE, ch, 0, 0, CommTarget::TO_ROOM);

  if (ch->desc != NULL)
    look_at_room(ch, 0);

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_DEATH) && GET_LEVEL(ch) < LVL_IMMORT) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch);
    return (0);
  }
  return (1);
}


int perform_move(struct char_data *ch, int dir, int need_specials_check)
{
  room_rnum was_in;
  struct follow_type *k;

  if (ch == nullptr || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch)) {
    return 0;
  }
  else if (!EXIT(ch, dir) || GET_EXIT(ch, dir).to_room == NOWHERE) {
    send_to_char(ch, "Alas, you cannot go that way...\r\n");
  }
  else if (EXIT_FLAGGED(GET_EXIT(ch, dir), EX_CLOSED)) {
    if (!GET_EXIT(ch, dir).keyword.empty()) {
      send_to_char(ch, "The %s seems to be closed.\r\n", fname(GET_EXIT(ch, dir).keyword.c_str()));
    }
    else {
      send_to_char(ch, "It seems to be closed.\r\n");
    }
  } else {
    if (ch->followers.empty()) {
      return (do_simple_move(ch, dir, need_specials_check));
    }

    was_in = IN_ROOM(ch);
    if (!do_simple_move(ch, dir, need_specials_check)) {
      return 0;
    }

    for (auto it = ch->followers.begin(); it != ch->followers.end(); ++it) {
      k = *it;

      if ((IN_ROOM(k->follower) == was_in) && (GET_POS(k->follower) >= POS_STANDING)) {
        act("You follow $N.\r\n", FALSE, k->follower, 0, ch, CommTarget::TO_CHAR);
        perform_move(k->follower, dir, 1);
      }
    }
    return 1;
  }
  return 0;
}


ACMD(do_move)
{
  TEMP_ARG_FIX;

  /*
   * This is basically a mapping of cmd numbers to perform_move indices.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  perform_move(ch, subcmd - 1, 0);
}


#define DOOR_IS_OPENABLE(ch, obj, door)	((obj) ? \
			((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) && \
			OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) :\
			(EXIT_FLAGGED(GET_EXIT(ch, door), EX_ISDOOR)))
#define DOOR_IS_OPEN(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :\
			(!EXIT_FLAGGED(GET_EXIT(ch, door), EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, door)	((obj) ? \
			(!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :\
			(!EXIT_FLAGGED(GET_EXIT(ch, door), EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, door) ((obj) ? \
			(OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) : \
			(EXIT_FLAGGED(GET_EXIT(ch, door), EX_PICKPROOF)))

#define DOOR_IS_CLOSED(ch, obj, door)	(!(DOOR_IS_OPEN(ch, obj, door)))
#define DOOR_IS_LOCKED(ch, obj, door)	(!(DOOR_IS_UNLOCKED(ch, obj, door)))
#define DOOR_KEY(ch, obj, door)		((obj) ? (GET_OBJ_VAL(obj, 2)) : \
					(GET_EXIT(ch, door).key))

ACMD(do_gen_door)
{
  TEMP_ARG_FIX;

  int door = -1;
  obj_vnum keynum;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct obj_data *obj = NULL;
  struct char_data *victim = NULL;

  skip_spaces(&argument);
  if (!*argument) {
    send_to_char(ch, "%c%s what?\r\n", UPPER(*cmd_door[subcmd]), cmd_door[subcmd] + 1);
    return;
  }
  two_arguments(argument, type, dir);
  if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
    door = find_door(ch, type, dir, cmd_door[subcmd]);

  if ((obj) || (door >= 0)) {
    keynum = DOOR_KEY(ch, obj, door);
    if (!(DOOR_IS_OPENABLE(ch, obj, door)))
      act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], CommTarget::TO_CHAR);
    else if (!DOOR_IS_OPEN(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_OPEN))
      send_to_char(ch, "But it's already closed!\r\n");
    else if (!DOOR_IS_CLOSED(ch, obj, door) &&
	     IS_SET(flags_door[subcmd], NEED_CLOSED))
      send_to_char(ch, "But it's currently open!\r\n");
    else if (!(DOOR_IS_LOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_LOCKED))
      send_to_char(ch, "Oh.. it wasn't locked, after all..\r\n");
    else if (!(DOOR_IS_UNLOCKED(ch, obj, door)) &&
	     IS_SET(flags_door[subcmd], NEED_UNLOCKED))
      send_to_char(ch, "It seems to be locked.\r\n");
    else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) &&
	     ((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
      send_to_char(ch, "You don't seem to have the proper key.\r\n");
    else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, door), subcmd))
      do_doorcmd(ch, obj, door, subcmd);
  }
  return;
}



ACMD(do_enter)
{
  TEMP_ARG_FIX;

  char buf[MAX_INPUT_LENGTH];
  int door;

  one_argument(argument, buf);

  if (*buf) {			/* an argument was supplied, search for door
				 * keyword */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (!GET_EXIT(ch, door).keyword.empty())
	  if (!str_cmp(GET_EXIT(ch, door).keyword.c_str(), buf)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "There is no %s here.\r\n", buf);
  } else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_INDOORS))
    send_to_char(ch, "You are already indoors.\r\n");
  else {
    /* try to locate an entrance */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (GET_EXIT(ch, door).to_room != NOWHERE)
	  if (!EXIT_FLAGGED(GET_EXIT(ch, door), EX_CLOSED) &&
	      ROOM_FLAGGED(GET_EXIT(ch, door).to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "You can't seem to find anything to enter.\r\n");
  }
}


ACMD(do_leave)
{
  TEMP_ARG_FIX;
  int door;

  if (OUTSIDE(ch))
    send_to_char(ch, "You are outside.. where do you want to go?\r\n");
  else {
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door))
	if (GET_EXIT(ch, door).to_room != NOWHERE)
	  if (!EXIT_FLAGGED(GET_EXIT(ch, door), EX_CLOSED) &&
	    !ROOM_FLAGGED(GET_EXIT(ch, door).to_room, ROOM_INDOORS)) {
	    perform_move(ch, door, 1);
	    return;
	  }
    send_to_char(ch, "I see no obvious exits to the outside.\r\n");
  }
}


ACMD(do_stand)
{
  TEMP_ARG_FIX;

  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char(ch, "You are already standing.\r\n");
    break;
  case POS_SITTING:
    send_to_char(ch, "You stand up.\r\n");
    act("$n clambers to $s feet.", TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    /* Will be sitting after a successful bash and may still be fighting. */
    GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and stand up.\r\n");
    act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first!\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Do you not consider fighting as standing?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and put your feet on the ground.\r\n");
    act("$n stops floating around, and puts $s feet on the ground.",
	TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_STANDING;
    break;
  }
}


ACMD(do_sit)
{
  TEMP_ARG_FIX;

  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char(ch, "You sit down.\r\n");
    act("$n sits down.", FALSE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SITTING:
    send_to_char(ch, "You're sitting already.\r\n");
    break;
  case POS_RESTING:
    send_to_char(ch, "You stop resting, and sit up.\r\n");
    act("$n stops resting.", TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sit down while fighting? Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and sit down.\r\n");
    act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_rest)
{
  TEMP_ARG_FIX;

  switch (GET_POS(ch)) {
  case POS_STANDING:
    send_to_char(ch, "You sit down and rest your tired bones.\r\n");
    act("$n sits down and rests.", TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_SITTING:
    send_to_char(ch, "You rest your tired bones.\r\n");
    act("$n rests.", TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    break;
  case POS_RESTING:
    send_to_char(ch, "You are already resting.\r\n");
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You have to wake up first.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Rest while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and stop to rest your tired bones.\r\n");
    act("$n stops floating around, and rests.", FALSE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_SITTING;
    break;
  }
}


ACMD(do_sleep)
{
  TEMP_ARG_FIX;

  switch (GET_POS(ch)) {
  case POS_STANDING:
  case POS_SITTING:
  case POS_RESTING:
    send_to_char(ch, "You go to sleep.\r\n");
    act("$n lies down and falls asleep.", TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  case POS_SLEEPING:
    send_to_char(ch, "You are already sound asleep.\r\n");
    break;
  case POS_FIGHTING:
    send_to_char(ch, "Sleep while fighting?  Are you MAD?\r\n");
    break;
  default:
    send_to_char(ch, "You stop floating around, and lie down to sleep.\r\n");
    act("$n stops floating around, and lie down to sleep.",
	TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_SLEEPING;
    break;
  }
}


ACMD(do_wake)
{
  TEMP_ARG_FIX;

  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  if (*arg) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char(ch, "Maybe you should wake yourself up first.\r\n");
    else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
      send_to_char(ch, "%s", NOPERSON.c_str());
    else if (vict == ch)
      self = 1;
    else if (AWAKE(vict))
      act("$E is already awake.", FALSE, ch, 0, vict, CommTarget::TO_CHAR);
    else if (AFF_FLAGGED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, CommTarget::TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("$E's in pretty bad shape!", FALSE, ch, 0, vict, CommTarget::TO_CHAR);
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, CommTarget::TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, CommTarget::TO_VICT | CommTarget::TO_SLEEP);
      GET_POS(vict) = POS_SITTING;
    }
    if (!self)
      return;
  }
  if (AFF_FLAGGED(ch, AFF_SLEEP))
    send_to_char(ch, "You can't wake up!\r\n");
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char(ch, "You are already awake...\r\n");
  else {
    send_to_char(ch, "You awaken, and sit up.\r\n");
    act("$n awakens.", TRUE, ch, 0, 0, CommTarget::TO_ROOM);
    GET_POS(ch) = POS_SITTING;
  }
}


ACMD(do_follow)
{
  TEMP_ARG_FIX;
  char buf[MAX_INPUT_LENGTH];
  struct char_data *leader;

  one_argument(argument, buf);

  if (*buf) {
    if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
      send_to_char(ch, "%s", NOPERSON.c_str());
      return;
    }
  } else {
    send_to_char(ch, "Whom do you wish to follow?\r\n");
    return;
  }

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, CommTarget::TO_CHAR);
    return;
  }
  if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master)) {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, CommTarget::TO_CHAR);
  } else {			/* Not Charmed follow person */
    if (leader == ch) {
      if (!ch->master) {
	send_to_char(ch, "You are already following yourself.\r\n");
	return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader)) {
	send_to_char(ch, "Sorry, but following in loops is not allowed.\r\n");
	return;
      }
      if (ch->master)
	stop_follower(ch);
      REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
      add_follower(ch, leader);
    }
  }
}
