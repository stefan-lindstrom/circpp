/* ************************************************************************
*   File: ban.c                                         Part of CircleMUD *
*  Usage: banning/unbanning/checking sites and player names               *
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
#include "act.h"
#include "ban.h"

std::list<ban_list_element> ban_list;


namespace {
  const int BANNED_SITE_LENGTH = 100;

  const char *ban_types[] = {
    "no",
    "new",
    "select",
    "all",
    "ERROR"
  };

  void _write_one_node(FILE *fp, const ban_list_element &node)
  {
    fprintf(fp, "%s %s %ld %s\n", ban_types[node.type], node.site.c_str(), (long) node.date, node.name.c_str());
  }
  void write_ban_list(void)
  {
  FILE *fl;

  if (!(fl = fopen(BAN_FILE, "w"))) {
    perror("SYSERR: Unable to open '" BAN_FILE "' for writing");
    return;
  }
  std::for_each(
    ban_list.begin(),
    ban_list.end(), 
    [fl](ban_list_element elem) { _write_one_node(fl, elem); }
  );
  fclose(fl);
  return;
}

}

void load_banned(void)
{
  FILE *fl;
  int i, date;
  char site_name[BANNED_SITE_LENGTH + 1], ban_type[100];
  char name[MAX_NAME_LENGTH + 1];

  ban_list.clear();

  if (!(fl = fopen(BAN_FILE, "r"))) {
    if (errno != ENOENT) {
      basic_mud_log("SYSERR: Unable to open banfile '%s': %s", BAN_FILE, strerror(errno));
    } else
      basic_mud_log("   Ban file '%s' doesn't exist.", BAN_FILE);
    return;
  }
  while (fscanf(fl, " %s %s %d %s ", ban_type, site_name, &date, name) == 4) {
    ban_list_element next_node;

    next_node.site = site_name;	
    next_node.name = name;	/* strncpy: OK (n_n->name:MAX_NAME_LENGTH+1) */
    next_node.date = date;

    for (i = BAN_NOT; i <= BAN_ALL; i++)
      if (!strcmp(ban_type, ban_types[i]))
        next_node.type = i;

    ban_list.push_back(next_node);
  }

  fclose(fl);
}


int isbanned(char *hostname)
{
  int i;
  char *nextchar;

  if (!hostname || !*hostname)
    return 0;

  i = 0;
  for (nextchar = hostname; *nextchar; nextchar++) {
    *nextchar = LOWER(*nextchar);
  }

  for (auto it = ban_list.begin(); it != ban_list.end(); ++it) {
    if (strstr(hostname, it->site.c_str()))	{ /* if hostname is a substring */
      i = MAX(i, it->type);
    }
  }

  return i;
}


#define BAN_LIST_FORMAT "%-25.25s  %-8.8s  %-10.10s  %-16.16s\r\n"
ACMD(do_ban)
{
  TEMP_ARG_FIX;
  char flag[MAX_INPUT_LENGTH], site[MAX_INPUT_LENGTH], *nextchar;
  char timestr[16];
  int i;

  if (!*argument) {
    if (ban_list.empty()) {
      send_to_char(ch, "No sites are banned.\r\n");
      return;
    }
    send_to_char(ch, BAN_LIST_FORMAT,
	    "Banned Site Name",
	    "Ban Type",
	    "Banned On",
	    "Banned By");
    send_to_char(ch, BAN_LIST_FORMAT,
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------",
	    "---------------------------------");

    for (auto it = ban_list.begin(); it != ban_list.end(); ++it) {
      if (it->date) {
        strlcpy(timestr, asctime(localtime(&(it->date))), 10);
        timestr[10] = '\0';
      } else {
        strcpy(timestr, "Unknown");	/* strcpy: OK (strlen("Unknown") < 16) */
      }
      send_to_char(ch, BAN_LIST_FORMAT, it->site.c_str(), ban_types[it->type], timestr, it->name.c_str());
    }
    return;
  }

  two_arguments(argument, flag, site);
  if (!*site || !*flag) {
    send_to_char(ch, "Usage: ban {all | select | new} site_name\r\n");
    return;
  }
  if (!(!str_cmp(flag, "select") || !str_cmp(flag, "all") || !str_cmp(flag, "new"))) {
    send_to_char(ch, "Flag must be ALL, SELECT, or NEW.\r\n");
    return;
  }

  for (auto it = ban_list.begin(); it != ban_list.end(); ++it) {
    if (it->site == site) {
      send_to_char(ch, "That site has already been banned -- unban it to change the ban type.\r\n");
      return;
    }
  }


  for (nextchar = site; *nextchar; nextchar++) {
    *nextchar = LOWER(*nextchar);
  }
  ban_list_element ban_node;
  ban_node.site = site; 
  ban_node.name =  GET_NAME(ch);
  ban_node.date = time(0);

  for (i = BAN_NEW; i <= BAN_ALL; i++) {
    if (!str_cmp(flag, ban_types[i])) {
      ban_node.type = i;
    }
  }

  ban_list.push_back(ban_node);
  
  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s has banned %s for %s players.", GET_NAME(ch), site, ban_types[ban_node.type]);
  send_to_char(ch, "Site banned.\r\n");
  write_ban_list();
}
#undef BAN_LIST_FORMAT


ACMD(do_unban)
{ 
  TEMP_ARG_FIX;
  char site[MAX_INPUT_LENGTH];

  one_argument(argument, site);
  if (!*site) {
    send_to_char(ch, "A site to unban might help.\r\n");
    return;
  }


  std::string ssite = site;
  auto find = std::find_if(
    ban_list.begin(),
    ban_list.end(),
    [ssite](ban_list_element elem) { return elem.site == ssite; }
  );

  if (find == ban_list.end()) {
    send_to_char(ch, "That site is not currently banned.\r\n");
    return;
  }

  ban_list.remove(*find);

  send_to_char(ch, "Site unbanned.\r\n");
  mudlog(NRM, MAX(LVL_GOD, GET_INVIS_LEV(ch)), TRUE, "%s removed the %s-player ban on %s.", GET_NAME(ch), ban_types[find->type], find->site.c_str());

  write_ban_list();
}


/**************************************************************************
 *  Code to check for invalid names (i.e., profanity, etc.)		  *
 *  Written by Sharon P. Goza						  *
 **************************************************************************/

#define MAX_INVALID_NAMES	200

std::vector<std::string> invalid_list;

bool valid_name(char *newname)
{
  unsigned int i;
  struct descriptor_data *dt;
  char tempname[MAX_INPUT_LENGTH];

  /*
   * Make sure someone isn't trying to create this same name.  We want to
   * do a 'str_cmp' so people can't do 'Bob' and 'BoB'.  The creating login
   * will not have a character name yet and other people sitting at the
   * prompt won't have characters yet.
   */
  for (dt = descriptor_list; dt; dt = dt->next) {
    if (dt->character && GET_NAME(dt->character) && !str_cmp(GET_NAME(dt->character), newname)) {
      return (STATE(dt) == CON_PLAYING);
    }
  }

  /* return valid if list doesn't exist */
  if (invalid_list.empty()) {
    return true;
  }

  /* change to lowercase */
  strlcpy(tempname, newname, sizeof(tempname));
  for (i = 0; tempname[i]; i++) {
    tempname[i] = LOWER(tempname[i]);
  }

  /* Does the desired name contain a string in the invalid list? */
  for (i = 0; i < invalid_list.size(); i++) {
    if (strstr(tempname, invalid_list[i].c_str()))
      return false;
  }

  return true;
}


/* What's with the wacky capitalization in here? */
void free_invalid_list(void)
{
  invalid_list.clear();
}


void read_invalid_list(void)
{
  FILE *fp;
  char temp[256];

  if (!(fp = fopen(XNAME_FILE, "r"))) {
    perror("SYSERR: Unable to open '" XNAME_FILE "' for reading");
    return;
  }

  while (get_line(fp, temp)) {
    invalid_list.push_back(temp);
  }

  fclose(fp);
}
