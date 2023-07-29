/* ***********************************************************************
*  File: alias.c				A utility to CircleMUD	 *
* Usage: writing/reading player's aliases.				 *
*									 *
* Code done by Jeremy Hess and Chad Thompson				 *
* Modifed by George Greer for inclusion into CircleMUD bpl15.		 *
*									 *
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.		 *
*********************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"

void write_aliases(struct char_data *ch);
void read_aliases(struct char_data *ch);
void delete_aliases(const char *charname);

void write_aliases(struct char_data *ch)
{
  FILE *file;
  char fn[MAX_STRING_LENGTH];

  get_filename(fn, sizeof(fn), ALIAS_FILE, GET_NAME(ch));
  remove(fn);

  if (GET_ALIASES(ch).empty())
    return;

  if ((file = fopen(fn, "w")) == NULL) {
    basic_mud_log("SYSERR: Couldn't save aliases for %s in '%s'.", GET_NAME(ch), fn);
    perror("SYSERR: write_aliases");
    return;
  }

  for (auto temp = GET_ALIASES(ch).begin(); temp != GET_ALIASES(ch).end(); ++temp) {
    int aliaslen = temp->alias.length();
    int repllen = temp->replacement.length();

    fprintf(file, "%d\n%s\n"	/* Alias */
		  "%d\n%s\n"	/* Replacement */
		  "%d\n",	/* Type */
		aliaslen, temp->alias.c_str(),
		repllen, temp->replacement.c_str() + 1,
		temp->type);
  }
  
  fclose(file);
}

void read_aliases(struct char_data *ch)
{   
  FILE *file;
  char xbuf[MAX_STRING_LENGTH];
  int length;

  get_filename(xbuf, sizeof(xbuf), ALIAS_FILE, GET_NAME(ch));

  if ((file = fopen(xbuf, "r")) == NULL) {
    if (errno != ENOENT) {
      basic_mud_log("SYSERR: Couldn't open alias file '%s' for %s.", xbuf, GET_NAME(ch));
      perror("SYSERR: read_aliases");
    }
    return;
  }
 
  for (;;) {
    alias_data t2;
    /* Read the aliased command. */
    if (fscanf(file, "%d\n", &length) != 1)
      goto read_alias_error;

    fgets(xbuf, length + 1, file);
    t2.alias = std::string(xbuf);

    /* Build the replacement. */
    if (fscanf(file, "%d\n", &length) != 1)
       goto read_alias_error;

    *xbuf = ' ';		/* Doesn't need terminated, fgets() will. */
    fgets(xbuf + 1, length + 1, file);
    t2.replacement = std::string(xbuf);
    t2.replacement.pop_back();

    /* Figure out the alias type. */
    if (fscanf(file, "%d\n", &length) != 1)
      goto read_alias_error;

    t2.type = length; 

    GET_ALIASES(ch).push_back(t2);

    if (feof(file))
      break;
  }; 
  
  fclose(file);
  return;

read_alias_error:
  GET_ALIASES(ch).clear();
  fclose(file);
} 

void delete_aliases(const char *charname)
{
  char filename[PATH_MAX];

  if (!get_filename(filename, sizeof(filename), ALIAS_FILE, charname))
    return;

  if (remove(filename) < 0 && errno != ENOENT)
    basic_mud_log("SYSERR: deleting alias file %s: %s", filename, strerror(errno));
}

