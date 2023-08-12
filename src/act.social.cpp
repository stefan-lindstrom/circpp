/* ************************************************************************
*   File: act.social.c                                  Part of CircleMUD *
*  Usage: Functions to handle socials                                     *
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
#include "act.h"

struct social_messg {
  int act_nr;
  int hide;
  int min_victim_position;	/* Position of victim */

  /* No argument was supplied */
  std::string char_no_arg;
  std::string others_no_arg;

  /* An argument was there, and a victim was found */
  std::string char_found;		/* if NULL, read no further, ignore args */
  std::string others_found;
  std::string vict_found;

  /* An argument was there, but no victim was found */
  std::string not_found;

  /* The victim turned out to be the character */
  std::string char_auto;
  std::string others_auto;
};

std::vector<social_messg> soc_mess_list;

namespace {
  int find_action(int cmd)
  {
    auto found = std::find_if(soc_mess_list.begin(), soc_mess_list.end(), [=](social_messg msg) { return msg.act_nr == cmd; });
    if (found == soc_mess_list.end()) {
      return -1;
    }
    return std::distance(soc_mess_list.begin(), found);
  }
}

ACMD(do_action)
{
  TEMP_ARG_FIX;
  char buf[MAX_INPUT_LENGTH];
  int act_nr;
  struct social_messg *action;
  struct char_data *vict;

  if ((act_nr = find_action(cmd)) < 0) {
    send_to_char(ch, "That action is not supported.\r\n");
    return;
  }
  action = &soc_mess_list[act_nr];

  if (!action->char_found.empty() && argument)
    one_argument(argument, buf);
  else
    *buf = '\0';

  if (!*buf) {
    send_to_char(ch, "%s\r\n", action->char_no_arg.c_str());
    act(action->others_no_arg.c_str(), action->hide, ch, 0, 0, CommTarget::TO_ROOM);
    return;
  }
  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s\r\n", action->not_found.c_str());
  else if (vict == ch) {
    send_to_char(ch, "%s\r\n", action->char_auto.c_str());
    act(action->others_auto.c_str(), action->hide, ch, 0, 0, CommTarget::TO_ROOM);
  } else {
    if (GET_POS(vict) < action->min_victim_position)
      act("$N is not in a proper position for that.", FALSE, ch, 0, vict, CommTarget::TO_CHAR | CommTarget::TO_SLEEP);
    else {
      act(action->char_found.c_str(), 0, ch, 0, vict, CommTarget::TO_CHAR | CommTarget::TO_SLEEP);
      act(action->others_found.c_str(), action->hide, ch, 0, vict, CommTarget::TO_NOTVICT);
      act(action->vict_found.c_str(), action->hide, ch, 0, vict, CommTarget::TO_VICT);
    }
  }
}



ACMD(do_insult)
{
  TEMP_ARG_FIX;
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  one_argument(argument, arg);

  if (*arg) {
    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "Can't hear you!\r\n");
    else {
      if (victim != ch) {
	send_to_char(ch, "You insult %s.\r\n", GET_NAME(victim));

	switch (rand_number(0, 2)) {
	case 0:
	  if (GET_SEX(ch) == SEX_MALE) {
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of fighting like a woman!", FALSE, ch, 0, victim, CommTarget::TO_VICT);
	    else
	      act("$n says that women can't fight.", FALSE, ch, 0, victim, CommTarget::TO_VICT);
	  } else {		/* Ch == Woman */
	    if (GET_SEX(victim) == SEX_MALE)
	      act("$n accuses you of having the smallest... (brain?)",
		  FALSE, ch, 0, victim, CommTarget::TO_VICT);
	    else
	      act("$n tells you that you'd lose a beauty contest against a troll.",
		  FALSE, ch, 0, victim, CommTarget::TO_VICT);
	  }
	  break;
	case 1:
	  act("$n calls your mother a bitch!", FALSE, ch, 0, victim, CommTarget::TO_VICT);
	  break;
	default:
	  act("$n tells you to get lost!", FALSE, ch, 0, victim, CommTarget::TO_VICT);
	  break;
	}			/* end switch */

	act("$n insults $N.", TRUE, ch, 0, victim, CommTarget::TO_NOTVICT);
      } else {			/* ch == victim */
	send_to_char(ch, "You feel insulted.\r\n");
      }
    }
  } else
    send_to_char(ch, "I'm sure you don't want to insult *everybody*...\r\n");
}

std::string fread_action(FILE *fl, int nr)
{
  char buf[MAX_STRING_LENGTH];

  fgets(buf, MAX_STRING_LENGTH, fl);
  if (feof(fl)) {
    basic_mud_log("SYSERR: fread_action: unexpected EOF near action #%d", nr);
    exit(1);
  }
  if (*buf == '#')
    return std::string("");

  buf[strlen(buf) - 1] = '\0';
  return std::string(buf);
}

void boot_social_messages(void)
{
  FILE *fl;
  int nr, hide, min_pos;
  char next_soc[100];

  /* open social file */
  if (!(fl = fopen(SOCMESS_FILE, "r"))) {
    basic_mud_log("SYSERR: can't open socials file '%s': %s", SOCMESS_FILE, strerror(errno));
    exit(1);
  }

  /* now read 'em */
  for (;;) {
    fscanf(fl, " %s ", next_soc);
    if (*next_soc == '$')
      break;
    if (fscanf(fl, " %d %d \n", &hide, &min_pos) != 2) {
      basic_mud_log("SYSERR: format error in social file near social '%s'", next_soc);
      exit(1);
    }

    soc_mess_list.push_back(social_messg());

    /* read the stuff */
    soc_mess_list.back().act_nr = nr = find_command(next_soc);
    soc_mess_list.back().hide = hide;
    soc_mess_list.back().min_victim_position = min_pos;

#ifdef CIRCLE_ACORN
    if (fgetc(fl) != '\n')
      basic_mud_log("SYSERR: Acorn bug workaround failed.");
#endif

    soc_mess_list.back().char_no_arg = fread_action(fl, nr);
    soc_mess_list.back().others_no_arg = fread_action(fl, nr);
    soc_mess_list.back().char_found = fread_action(fl, nr);

    /* if no char_found, the rest is to be ignored */
    if (soc_mess_list.back().char_found.empty())
      continue;

    soc_mess_list.back().others_found = fread_action(fl, nr);
    soc_mess_list.back().vict_found = fread_action(fl, nr);
    soc_mess_list.back().not_found = fread_action(fl, nr);
    soc_mess_list.back().char_auto = fread_action(fl, nr);
    soc_mess_list.back().others_auto = fread_action(fl, nr);

    /* If social not found, re-use this slot.  'curr_soc' will be reincremented. */
    if (nr < 0) {
      basic_mud_log("SYSERR: Unknown social '%s' in social file.", next_soc);
      soc_mess_list.pop_back();
      continue;
    }

    /* If the command we found isn't do_action, we didn't count it for the CREATE(). */
    if (cmd_info[nr].command_pointer != do_action) {
      basic_mud_log("SYSERR: Social '%s' already assigned to a command.", next_soc);
      soc_mess_list.pop_back();
    }
  }
  /* close file & set top */
  fclose(fl);

  /* now, sort 'em */
  std::sort(soc_mess_list.begin(), soc_mess_list.end(), [](social_messg a, social_messg b) { return a.act_nr < b.act_nr; });
}
