/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
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
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"

/* local functions */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void prune_crlf(char *txt);

/* creates a random number in interval [from;to] */
int rand_number(int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
    basic_mud_log("SYSERR: rand_number() should be called with lowest, then highest. (%d, %d), not (%d, %d).", from, to, to, from);
  }

  /*
   * This should always be of the form:
   *
   *	((float)(to - from + 1) * rand() / (float)(RAND_MAX + from) + from);
   *
   * if you are using rand() due to historical non-randomness of the
   * lower bits in older implementations.  We always use circle_random()
   * though, which shouldn't have that problem. Mean and standard
   * deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken.
   */
  return ((circle_random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(1, size);

  return (sum);
}


/* Be wary of sign issues with this. */
int MIN(int a, int b)
{
  return (a < b ? a : b);
}

/* Be wary of sign issues with this. */
int MAX(int a, int b)
{
  return (a > b ? a : b);
}


char *CAP(char *txt)
{
  *txt = UPPER(*txt);
  return (txt);
}


#if !defined(HAVE_STRLCPY)
/*
 * A 'strlcpy' function in the same fashion as 'strdup' below.
 *
 * This copies up to totalsize - 1 bytes from the source string, placing
 * them and a trailing NUL into the destination string.
 *
 * Returns the total length of the string it tried to copy, not including
 * the trailing NUL.  So a '>= totalsize' test says it was truncated.
 * (Note that you may have _expected_ truncation because you only wanted
 * a few characters from the source string.)
 */
size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
  strncpy(dest, source, totalsize - 1);	/* strncpy: OK (we must assume 'totalsize' is correct) */
  dest[totalsize - 1] = '\0';
  return strlen(source);
}
#endif


#if !defined(HAVE_STRDUP)
/* Create a duplicate of a string */
char *strdup(const char *source)
{
  char *new_z = new char[strlen(source) + 1];
  return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif

void prune_crlf(std::string &txt) {
  if (txt.back() == '\n') {
    txt.pop_back();
  }
  if (txt.back() == '\r') {
    txt.pop_back();
  }
}

/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}


#ifndef str_cmp
/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    basic_mud_log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; arg1[i] || arg2[i]; i++)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif


#ifndef strn_cmp
/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    basic_mud_log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif


/* log a death trap hit */
void log_death_trap(struct char_data *ch)
{
  mudlog(BRF, LVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), world[IN_ROOM(ch)].name.c_str());
}


/*
 * New variable argument basic_mud_log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_vlog(const char *format, va_list args)
{
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL) {
    puts("SYSERR: Using basic_mud_log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: basic_mud_log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}


/* So mudlog() can use the same function. */
void basic_mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  basic_mud_vlog(format, args);
  va_end(args);
}


/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    basic_mud_log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
    return;	/* eh, oh well. */

  if (file) {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }

  if (level < 0)
    return;

  strcpy(buf, "[ ");	/* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n");	/* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_LEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "%s%s%s", CCGRN(i->character, C_NRM), buf, CCNRM(i->character, C_NRM));
  }
}



/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
  size_t len = 0;
  int nlen;
  long nr;

  *result = '\0';

  for (nr = 0; bitvector && len < reslen; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
      nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
      if (len + nlen >= reslen || nlen < 0)
        break;
      len += nlen;
    }

    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    len = strlcpy(result, "NOBITS ", reslen);

  return (len);
}


size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

  now.month = -1;
  now.year = -1;

  return (&now);
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35;	/* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17;	/* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return (&now);
}


time_t mud_time_to_secs(struct time_info_data *now)
{
  time_t when = 0;

  when += now->year  * SECS_PER_MUD_YEAR;
  when += now->month * SECS_PER_MUD_MONTH;
  when += now->day   * SECS_PER_MUD_DAY;
  when += now->hours * SECS_PER_MUD_HOUR;

  return (time(NULL) - when);
}


struct time_info_data *age(struct char_data *ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17;	/* All players start at 17 */

  return (&player_age);
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data *ch)
{
  if (ch->master == nullptr) {
    core_dump();
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, CommTarget::TO_CHAR);
    act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, CommTarget::TO_NOTVICT);
    act("$n hates your guts!", FALSE, ch, 0, ch->master, CommTarget::TO_VICT);
    if (affected_by_spell(ch, SPELL_CHARM))
      affect_from_char(ch, SPELL_CHARM);
  } else {
    act("You stop following $N.", FALSE, ch, 0, ch->master, CommTarget::TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, CommTarget::TO_NOTVICT);
    act("$n stops following you.", TRUE, ch, 0, ch->master, CommTarget::TO_VICT);
  }


  auto find = std::find_if(
    ch->master->followers.begin(),
    ch->master->followers.end(),
    [ch](follow_type *f) { return f->follower == ch; }
  );

  if (find == ch->master->followers.end()) {
    return;    
  }

  ch->master->followers.remove(*find);
  delete *find;
  ch->master = nullptr;
  REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP);
}


int num_followers_charmed(struct char_data *ch)
{
  struct follow_type *lackey;
  int total = 0;

  for (auto it = ch->followers.begin(); it != ch->followers.end(); ++it) {
    lackey = *it;

    if (AFF_FLAGGED(lackey->follower, AFF_CHARM) && lackey->follower->master == ch) {
      total++;
    }
  }

  return total;
}


/* Called when a character that follows/is followed dies */
void die_follower(struct char_data *ch)
{
  if (ch->master) {
    stop_follower(ch);
  }

  while (!ch->followers.empty()) {
    auto follower = ch->followers.front();
    ch->followers.pop_front();
    stop_follower(follower->follower);

  }
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data *ch, struct char_data *leader)
{
  struct follow_type *k;

  if (ch->master) {
    core_dump();
    return;
  }

  ch->master = leader;

  k = new follow_type;

  k->follower = ch;
  leader->followers.push_back(k);

  act("You now follow $N.", FALSE, ch, 0, leader, CommTarget::TO_CHAR);
  if (CAN_SEE(leader, ch)) {
    act("$n starts following you.", TRUE, ch, 0, leader, CommTarget::TO_VICT);
  }
  act("$n starts to follow $N.", TRUE, ch, 0, leader, CommTarget::TO_NOTVICT);
}


/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file. Buffer given must
 * be at least READ_SIZE (256) characters large.
 */
int get_line(FILE *fl, char *buf)
{
  char temp[READ_SIZE];
  int lines = 0;
  int sl;

  do {
    if (!fgets(temp, READ_SIZE, fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  /* Last line of file doesn't always have a \n, but it should. */
  sl = strlen(temp);
  while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
    temp[--sl] = '\0';

  strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
  return (lines);
}


int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
  const char *prefix, *middle, *suffix;
  char name[PATH_MAX], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    basic_mud_log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
		orig_name, filename);
    return (0);
  }

  switch (mode) {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case ALIAS_FILE:
    prefix = LIB_PLRALIAS;
    suffix = SUF_ALIAS;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  default:
    return (0);
  }

  strlcpy(name, orig_name, sizeof(name));
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  snprintf(filename, fbufsize, "%s%s" SLASH "%s.%s", prefix, middle, name, suffix);
  return (1);
}


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;
  for (auto it = room->people.begin(); it != room->people.end(); ++it) {
    ch = *it;

    if (!IS_NPC(ch)) {
      i++;
    }
  }

  return i;
}

/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
extern FILE *player_fl;
void core_dump_real(const char *who, int line)
{
  basic_mud_log("SYSERR: Assertion failed at %s:%d!", who, line);

#if 0	/* By default, let's not litter. */
#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise...make very sure. */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);
  fflush(player_fl);
  /* Everything, just in case, for the systems that support it. */
  fflush(NULL);

  /*
   * Kill the child so the debugger or script doesn't think the MUD
   * crashed.  The 'autorun' script would otherwise run it again.
   */
  if (fork() == 0)
    abort();
#endif
#endif
}


/*
 * Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.
 */
int room_is_dark(room_rnum room)
{
  if (!VALID_ROOM_RNUM(room)) {
    basic_mud_log("room_is_dark: Invalid room rnum %d. (0-%ld)", room, world.size() - 1);
    return (FALSE);
  }

  if (world[room].light)
    return (FALSE);

  if (ROOM_FLAGGED(room, ROOM_DARK))
    return (TRUE);

  if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY)
    return (FALSE);

  if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
    return (TRUE);

  return (FALSE);
}

bitvector_t asciiflag_conv(const char *flag)
{
  bitvector_t flags = 0;
  int is_num = TRUE;
  const char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_num = FALSE;
  }

  if (is_num)
    flags = atol(flag);

  return (flags);
}
