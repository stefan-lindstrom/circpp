/* ************************************************************************
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include <type_traits>
#include <string>

#include "conf.h"
#include "sysdep.h"

#include <algorithm>
#include <fstream>
#include <streambuf>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "room_future.h"
#include "object_future.h"
#include "zone_future.h"
#include "mob_future.h"
#include "shop.h"
#include "shop_future.h"
#include "class.h"
#include "config.h"
#include "act.h"

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

std::vector<room_data> world;
std::list<char_data *> character_list;
std::vector<index_data> mob_index;
std::vector<char_data> mob_proto;
std::list<obj_data *> object_list;
std::vector<index_data> obj_index;
std::vector<obj_data> obj_proto;
std::vector<zone_data> zone_table;
std::vector<message_list> fight_messages;	/* fighting messages	 */

std::vector<player_index_element> player_table;
FILE *player_fl = nullptr;		/* file desc of player file	 */
long top_idnum = 0;		/* highest idnum in use		 */


int no_mail = 0;		/* mail disabled?		 */
int mini_mud = 0;		/* mini-mud mode?		 */
int no_rent_check = 0;		/* skip rent check on boot?	 */
time_t boot_time = 0;		/* time of mud boot		 */
int circle_restrict = 0;	/* level of game restriction	 */
room_rnum r_mortal_start_room;	/* rnum of mortal start room	 */
room_rnum r_immort_start_room;	/* rnum of immort start room	 */
room_rnum r_frozen_start_room;	/* rnum of frozen start room	 */

std::string credits;		/* game credits			 */
std::string news;		/* mud news			 */
std::string motd;		/* message of the day - mortals */
std::string imotd;		/* message of the day - immorts */
std::string GREETINGS;		/* opening credits screen	*/
std::string help;		/* help screen			 */
std::string info;		/* info page			 */
std::string wizlist;		/* list of higher gods		 */
std::string immlist;		/* list of peon gods		 */
std::string background;	/* background story		 */
std::string handbook;		/* handbook for new immortals	 */
std::string policies;		/* policies page		 */

struct help_index_element *help_table = 0;	/* the help table	 */
int top_of_helpt = 0;		/* top of help index table	 */

struct time_info_data time_info;/* the infomation about the time    */
struct weather_data weather_info;	/* the infomation about the weather */
struct player_special_data dummy_mob;	/* dummy spec area for mobs	*/
struct reset_q_type reset_q;	/* queue of zones to be reset	 */

/* local functions */
int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits);
int check_object_spell_number(struct obj_data *obj, int val);
int check_object_level(struct obj_data *obj, int val);
static void help_boot();
int check_object(struct obj_data *);
void load_zones(FILE *fl, char *zonename);
void load_help(FILE *fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void build_player_index(void);
int is_empty(zone_rnum zone_nr);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
std::string slurp_file_to_string(const char *filename);

void reboot_wizlists(void);
ACMD(do_reboot);
void boot_world(void);
int count_alias_records(FILE *fl);
int count_hash_records(FILE *fl);
void get_one_line(FILE *fl, char *buf);
void save_etext(struct char_data *ch);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);
long get_ptable_by_name(const char *name);

/* external functions */
void paginate_string(char *str, struct descriptor_data *d);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void load_messages(void);
void weather_and_time(int mode);
void mag_assign_spells(void);
void update_obj_file(void);	/* In objsave.c */
void sort_spells(void);
void load_banned(void);
void Read_Invalid_List(void);
int hsort(const void *a, const void *b);
void prune_crlf(char *txt);
void destroy_shops(void);

/* external vars */
extern int no_specials;
extern int scheck;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern struct descriptor_data *descriptor_list;
extern const char *unused_spellname;	/* spell_parser.c */

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  wizlist = slurp_file_to_string(WIZLIST_FILE);
  immlist = slurp_file_to_string(IMMLIST_FILE);
}

/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{
  char arg[MAX_INPUT_LENGTH];
  (void)cmd;
  (void)subcmd;

  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*') {
    GREETINGS = slurp_file_to_string(GREETINGS_FILE);
    prune_crlf(GREETINGS);

    wizlist = slurp_file_to_string(WIZLIST_FILE);
    immlist = slurp_file_to_string(IMMLIST_FILE);
    news = slurp_file_to_string(NEWS_FILE);
    credits = slurp_file_to_string(CREDITS_FILE);
    motd = slurp_file_to_string(MOTD_FILE);
    imotd = slurp_file_to_string(IMOTD_FILE);
    help = slurp_file_to_string(HELP_PAGE_FILE);
    info = slurp_file_to_string(INFO_FILE);
    policies = slurp_file_to_string(POLICIES_FILE);
    handbook = slurp_file_to_string(HANDBOOK_FILE);
    background = slurp_file_to_string(BACKGROUND_FILE);

  } else if (!str_cmp(arg, "wizlist")) {
    wizlist = slurp_file_to_string(WIZLIST_FILE);
  }
  else if (!str_cmp(arg, "immlist")){
    immlist = slurp_file_to_string(IMMLIST_FILE);
  }
  else if (!str_cmp(arg, "news")) {
    news = slurp_file_to_string(NEWS_FILE);
  }
  else if (!str_cmp(arg, "credits")) {
    credits = slurp_file_to_string(CREDITS_FILE);
  }
  else if (!str_cmp(arg, "motd")) {
    motd = slurp_file_to_string(MOTD_FILE);
  }
  else if (!str_cmp(arg, "imotd")) {
    imotd = slurp_file_to_string(IMOTD_FILE);
  }
  else if (!str_cmp(arg, "help")) {
    help = slurp_file_to_string(HELP_PAGE_FILE);
  }
  else if (!str_cmp(arg, "info")) {
    info = slurp_file_to_string(INFO_FILE);
  }
  else if (!str_cmp(arg, "policy")) {
    policies = slurp_file_to_string(POLICIES_FILE);
  }
  else if (!str_cmp(arg, "handbook")) {
    handbook = slurp_file_to_string(HANDBOOK_FILE);
      }
  else if (!str_cmp(arg, "background")) {
    background = slurp_file_to_string(BACKGROUND_FILE);
  }
  else if (!str_cmp(arg, "greetings")) {
    GREETINGS = slurp_file_to_string(GREETINGS_FILE);
    prune_crlf(GREETINGS);
  } else if (!str_cmp(arg, "xhelp")) {
    if (help_table)
      free_help();
    help_boot();
  } else {
    send_to_char(ch, "Unknown reload option.\r\n");
    return;
  }

  send_to_char(ch, "%s", OK.c_str());
}


void boot_world(void)
{
  basic_mud_log("Started future loading zone table.");
  zone_future z(mini_mud);
  z.parse();

  basic_mud_log("Started future loading objs and generating index.");
  object_future o(mini_mud);
  o.parse();

  // can start mob and shop parsing here when done

  basic_mud_log("Waiting for completion of zone loading...");
  zone_table = z.items();
  basic_mud_log("   %lu zones, %lu bytes.", zone_table.size(), zone_table.size() * sizeof(zone_data));

  std::for_each(zone_table.begin(), zone_table.end(), [](zone_data z) {
      basic_mud_log("Zone %s (%d): bot: %d, top: %d", z.name.c_str(), z.number, z.bot, z.top);
    });

  // Okay to start room parsing here
  basic_mud_log("Started future loading rooms.");
  room_future r(mini_mud);
  r.parse();

  basic_mud_log("Waiting for completion of object loading ... then building index.");
  obj_proto = o.items();

  std::for_each(obj_proto.begin(), obj_proto.end(), [](const obj_data &o) {
      index_data oi;
      oi.vnum = o.vnum;
      oi.number = 0;
      oi.func = nullptr;
      obj_index.push_back(oi);
    });
  basic_mud_log("   %ld objs, %lu bytes in index, %lu bytes in prototypes.", obj_proto.size(), obj_proto.size() * sizeof(index_data), obj_proto.size() * sizeof(obj_data));

  // get rooms.  
  basic_mud_log("Waiting for completion of room loading...");
  world = r.items();
  basic_mud_log("   %ld rooms, %lu bytes.", world.size(), world.size() * sizeof(room_data));

  basic_mud_log("Renumbering rooms.");
  renum_world();

  basic_mud_log("Checking start rooms.");
  check_start_rooms();

  // can wait for all other fuqture based parses to complete. 
  basic_mud_log("Startinf future parsing of mobs and generating index.");
  mob_future mobs(mini_mud);
  mobs.parse();

  basic_mud_log("Waiting for completion of mob loading...");
  mob_proto = mobs.items();

  std::for_each(mob_proto.begin(), mob_proto.end(), [](const char_data &m) {
      index_data mi;
      mi.vnum = m.vnr;
      mi.number = 0;
      mi.func = nullptr;
      mob_index.push_back(mi);
    });

  basic_mud_log("   %ld mobiles, %lu bytes.", mob_proto.size(), mob_proto.size() * sizeof(char_data));


  basic_mud_log("Renumbering zone table.");
  renum_zone_table();

  if (!no_specials) {
    basic_mud_log("Loading shops.");
    shop_future s(mini_mud);
    s.parse();
    shop_index = s.items();
  }
}

/* Free the world, in a memory allocation sense. */
void destroy_db(void)
{
 #if 0
  /* Shops */
  destroy_shops();
#endif
}


/* body of the booting system */
void boot_db(void)
{
  zone_rnum i;

  basic_mud_log("Boot db -- BEGIN.");

  basic_mud_log("Resetting the game time:");
  reset_time();

  basic_mud_log("Reading news, credits, help, bground, info & motds.");
  
  news = slurp_file_to_string(NEWS_FILE);
  credits = slurp_file_to_string(CREDITS_FILE);
  motd = slurp_file_to_string(MOTD_FILE);
  imotd = slurp_file_to_string(IMOTD_FILE);
  help = slurp_file_to_string(HELP_PAGE_FILE);
  info = slurp_file_to_string(INFO_FILE);
  wizlist = slurp_file_to_string(WIZLIST_FILE);
  immlist = slurp_file_to_string(IMMLIST_FILE);
  policies = slurp_file_to_string(POLICIES_FILE);
  handbook = slurp_file_to_string(HANDBOOK_FILE);
  background = slurp_file_to_string(BACKGROUND_FILE);
  GREETINGS = slurp_file_to_string(GREETINGS_FILE);
  prune_crlf(GREETINGS);

  basic_mud_log("Loading spell definitions.");
  mag_assign_spells();

  boot_world();

  basic_mud_log("Loading help entries.");
  help_boot();

  basic_mud_log("Generating player index.");
  build_player_index();

  basic_mud_log("Loading fight messages.");
  load_messages();

  basic_mud_log("Loading social messages.");
  boot_social_messages();

  basic_mud_log("Assigning function pointers:");

  if (!no_specials) {
    basic_mud_log("   Mobiles.");
    assign_mobiles();
    basic_mud_log("   Shopkeepers.");
    assign_the_shopkeepers();
    basic_mud_log("   Objects.");
    assign_objects();
    basic_mud_log("   Rooms.");
    assign_rooms();
  }

  basic_mud_log("Assigning spell and skill levels.");
  init_spell_levels();

  basic_mud_log("Sorting command list and spells.");
  sort_commands();
  sort_spells();

  basic_mud_log("Booting mail system.");
  if (!scan_file()) {
    basic_mud_log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }
  basic_mud_log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check) {
    basic_mud_log("Deleting timed-out crash and rent files:");
    update_obj_file();
    basic_mud_log("   Done.");
  }

  /* Moved here so the object limit code works. -gg 6/24/98 */
  if (!mini_mud) {
    basic_mud_log("Booting houses.");
    House_boot();
  }

  for (i = 0; static_cast<unsigned long>(i) < zone_table.size(); i++) {
    basic_mud_log("Resetting #%d: %s (rooms %d-%d).", zone_table[i].number, zone_table[i].name.c_str(), zone_table[i].bot, zone_table[i].top);
    reset_zone(i);
  }

  reset_q.head = reset_q.tail = NULL;

  boot_time = time(0);

  basic_mud_log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
  time_t beginning_of_time = 0;
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "r")) == NULL)
    basic_mud_log("SYSERR: Can't read from '%s' time file.", TIME_FILE);
  else {
    fscanf(bgtime, "%ld\n", &beginning_of_time);
    fclose(bgtime);
  }
  if (beginning_of_time == 0)
    beginning_of_time = 650336715;

  time_info = *mud_time_passed(time(0), beginning_of_time);

  if (time_info.hours <= 4)
    weather_info.sunlight = SUN_DARK;
  else if (time_info.hours == 5)
    weather_info.sunlight = SUN_RISE;
  else if (time_info.hours <= 20)
    weather_info.sunlight = SUN_LIGHT;
  else if (time_info.hours == 21)
    weather_info.sunlight = SUN_SET;
  else
    weather_info.sunlight = SUN_DARK;

  basic_mud_log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
	  time_info.day, time_info.month, time_info.year);

  weather_info.pressure = 960;
  if ((time_info.month >= 7) && (time_info.month <= 12))
    weather_info.pressure += dice(1, 50);
  else
    weather_info.pressure += dice(1, 80);

  weather_info.change = 0;

  if (weather_info.pressure <= 980)
    weather_info.sky = SKY_LIGHTNING;
  else if (weather_info.pressure <= 1000)
    weather_info.sky = SKY_RAINING;
  else if (weather_info.pressure <= 1020)
    weather_info.sky = SKY_CLOUDY;
  else
    weather_info.sky = SKY_CLOUDLESS;
}


/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(struct time_info_data *when)
{
  FILE *bgtime;

  if ((bgtime = fopen(TIME_FILE, "w")) == NULL)
    basic_mud_log("SYSERR: Can't write to '%s' time file.", TIME_FILE);
  else {
    fprintf(bgtime, "%ld\n", mud_time_to_secs(when));
    fclose(bgtime);
  }
}


void free_player_index(void)
{
  if (player_table.empty())
    return;

  player_table.clear();
}


/* generate index table for the player file */
void build_player_index(void)
{
  int nr = -1;
  long size, recs;
  struct char_file_u dummy;

  if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
    if (errno != ENOENT) {
      perror("SYSERR: fatal error opening playerfile");
      exit(1);
    } else {
      basic_mud_log("No playerfile.  Creating a new one.");
      touch(PLAYER_FILE);
      if (!(player_fl = fopen(PLAYER_FILE, "r+b"))) {
      	perror("SYSERR: fatal error opening playerfile");
	      exit(1);
      }
    }
  }

  fseek(player_fl, 0L, SEEK_END);
  size = ftell(player_fl);
  rewind(player_fl);
  if (size % sizeof(struct char_file_u))
    basic_mud_log("\aWARNING:  PLAYERFILE IS PROBABLY CORRUPT!");
  recs = size / sizeof(struct char_file_u);
  if (recs) {
    basic_mud_log("   %ld players in database.", recs);
  } else {
    player_table.clear();
    return;
  }

  for (;;) {
    fread(&dummy, sizeof(struct char_file_u), 1, player_fl);
    if (feof(player_fl))
      break;

    /* new record */
    nr++;
    player_table.push_back(player_index_element());    
    player_table[nr].name = std::string(dummy.name);

    basic_mud_log("Name before: %s",player_table[nr].name.c_str());

    std::transform(player_table[nr].name.begin(), player_table[nr].name.end(), player_table[nr].name.begin(), [](unsigned char c){ return std::tolower(c); });

//    std::for_each(player_table[nr].name.begin(), player_table[nr].name.end(), [](unsigned char &ch) { return (ch =  std::tolower(ch)); });
    basic_mud_log("Name after: %s",player_table[nr].name.c_str());

    player_table[nr].id = dummy.char_specials_saved.idnum;
    top_idnum = std::max(top_idnum, dummy.char_specials_saved.idnum);
  }
}

/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *	-gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE *fl)
{
  char key[READ_SIZE], next_key[READ_SIZE];
  char line[READ_SIZE], *scan;
  int total_keywords = 0;

  /* get the first keyword line */
  get_one_line(fl, key);

  while (*key != '$') {
    /* skip the text */
    do {
      get_one_line(fl, line);
      if (feof(fl))
	goto ackeof;
    } while (*line != '#');

    /* now count keywords */
    scan = key;
    do {
      scan = one_word(scan, next_key);
      if (*next_key)
        ++total_keywords;
    } while (*next_key);

    /* get next keyword line (or $) */
    get_one_line(fl, key);

    if (feof(fl))
      goto ackeof;
  }

  return (total_keywords);

  /* No, they are not evil. -gg 6/24/98 */
ackeof:	
  basic_mud_log("SYSERR: Unexpected end of help file.");
  exit(1);	/* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE *fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return (count);
}

// Only handles help files now
static void help_boot()
{
  const char *index_filename, *prefix = NULL;	/* NULL or egcs 1.1 complains */
  FILE *db_index, *db_file;
  int rec_count = 0, size[2];
  char buf2[PATH_MAX], buf1[MAX_STRING_LENGTH];

  prefix = HLP_PREFIX;

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  snprintf(buf2, sizeof(buf2), "%s%s", prefix, index_filename);
  if (!(db_index = fopen(buf2, "r"))) {
    basic_mud_log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
   exit(1);
  }

  /* first, count the number of records in the file so we can malloc */
  fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$') {
	std::string tmp = prefix;
	tmp.append(buf1);

    if (!(db_file = fopen(tmp.c_str(), "r"))) {
      basic_mud_log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix,
	  index_filename, strerror(errno));
      fscanf(db_index, "%s\n", buf1);
      continue;
    } else {
      rec_count += count_alias_records(db_file);
    }

    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
  }

  if (!rec_count) {
    basic_mud_log("SYSERR: boot error - 0 records counted in %s/%s.", prefix, index_filename);
    exit(1);
  }

  /*
   * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
   */
  help_table = new help_index_element[rec_count];
  size[0] = sizeof(struct help_index_element) * rec_count;
  basic_mud_log("   %d entries, %d bytes.", rec_count, size[0]);

  rewind(db_index);
  fscanf(db_index, "%s\n", buf1);
  while (*buf1 != '$') {
	std::string tmp = prefix;
	tmp.append(buf1);

    if (!(db_file = fopen(tmp.c_str(), "r"))) {
      basic_mud_log("SYSERR: %s: %s", buf2, strerror(errno));
      exit(1);
    }
    load_help(db_file);
    fclose(db_file);
    fscanf(db_index, "%s\n", buf1);
  }
  fclose(db_index);

  qsort(help_table, top_of_helpt, sizeof(struct help_index_element), hsort);
  top_of_helpt--;
}

/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  if ((r_mortal_start_room = real_room(mortal_start_room)) == NOWHERE) {
    basic_mud_log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    exit(1);
  }
  if ((r_immort_start_room = real_room(immort_start_room)) == NOWHERE) {
    if (!mini_mud)
      basic_mud_log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(frozen_start_room)) == NOWHERE) {
    if (!mini_mud)
      basic_mud_log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
}


/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  int room, door;

  for (room = 0; static_cast<unsigned long>(room) < world.size(); room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (std::get<1>(world[room].dir_option[door]))
	if (std::get<0>(world[room].dir_option[door]).to_room != NOWHERE)
	  std::get<0>(world[room].dir_option[door]).to_room =
	    real_room(std::get<0>(world[room].dir_option[door]).to_room);
}


#define ZCMD zone_table[zone].cmd[cmd_no]

/*
 * "resulve vnums into rnums in the zone reset tables"
 *
 * Or in English: Once all of the zone reset tables have been loaded, we
 * resolve the virtual numbers into real numbers all at once so we don't have
 * to do it repeatedly while the game is running.  This does make adding any
 * room, mobile, or object a little more difficult while the game is running.
 *
 * NOTE 1: Assumes NOWHERE == NOBODY == NOTHING.
 * NOTE 2: Assumes sizeof(room_rnum) >= (sizeof(mob_rnum) and sizeof(obj_rnum))
 */
void renum_zone_table(void)
{
  unsigned int cmd_no;
  room_rnum a, b, c, olda, oldb, oldc;
  zone_rnum zone;
  char buf[128];

  for (zone = 0; static_cast<unsigned long>(zone) < zone_table.size(); zone++)
    for (cmd_no = 0; cmd_no < zone_table[zone].cmd.size(); cmd_no++) {
      a = b = c = 0;
      olda = ZCMD.arg1;
      oldb = ZCMD.arg2;
      oldc = ZCMD.arg3;
      switch (ZCMD.command) {
      case 'M':
	a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
	c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'O':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	if (ZCMD.arg3 != NOWHERE)
	  c = ZCMD.arg3 = real_room(ZCMD.arg3);
	break;
      case 'G':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'E':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	break;
      case 'P':
	a = ZCMD.arg1 = real_object(ZCMD.arg1);
	c = ZCMD.arg3 = real_object(ZCMD.arg3);
	break;
      case 'D':
	a = ZCMD.arg1 = real_room(ZCMD.arg1);
	break;
      case 'R': /* rem obj from room */
        a = ZCMD.arg1 = real_room(ZCMD.arg1);
	b = ZCMD.arg2 = real_object(ZCMD.arg2);
        break;
      }
      if (a == NOWHERE || b == NOWHERE || c == NOWHERE) {
	if (!mini_mud) {
	  snprintf(buf, sizeof(buf), "Invalid vnum %d, cmd disabled",
			 a == NOWHERE ? olda : b == NOWHERE ? oldb : oldc);
	  log_zone_error(zone, cmd_no, buf);
	}
	ZCMD.command = '*';
      }
    }
}

void get_one_line(FILE *fl, char *buf)
{
  if (fgets(buf, READ_SIZE, fl) == NULL) {
    basic_mud_log("SYSERR: error reading help file: not terminated with $?");
    exit(1);
  }

  buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}


void free_help(void)
{
  int hp;

  if (!help_table)
     return;

  for (hp = 0; hp <= top_of_helpt; hp++) {
    if (help_table[hp].keyword)
      free(help_table[hp].keyword);
    if (help_table[hp].entry && !help_table[hp].duplicate)
      free(help_table[hp].entry);
  }

  free(help_table);
  help_table = NULL;
  top_of_helpt = 0;
}


void load_help(FILE *fl)
{
#if defined(CIRCLE_MACINTOSH)
  static char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384]; /* too big for stack? */
#else
  char key[READ_SIZE + 1], next_key[READ_SIZE + 1], entry[32384];
#endif
  size_t entrylen;
  char line[READ_SIZE + 1], *scan;
  struct help_index_element el;

  /* get the first keyword line */
  get_one_line(fl, key);
  while (*key != '$') {
    strcat(key, "\r\n");	/* strcat: OK (READ_SIZE - "\n" + "\r\n" == READ_SIZE + 1) */
    entrylen = strlcpy(entry, key, sizeof(entry));

    /* read in the corresponding help entry */
    get_one_line(fl, line);
    while (*line != '#' && entrylen < sizeof(entry) - 1) {
      entrylen += strlcpy(entry + entrylen, line, sizeof(entry) - entrylen);

      if (entrylen + 2 < sizeof(entry) - 1) {
        strcpy(entry + entrylen, "\r\n");	/* strcpy: OK (size checked above) */
        entrylen += 2;
      }
      get_one_line(fl, line);
    }

    if (entrylen >= sizeof(entry) - 1) {
      int keysize;
      const char *truncmsg = "\r\n*TRUNCATED*\r\n";

      strcpy(entry + sizeof(entry) - strlen(truncmsg) - 1, truncmsg);	/* strcpy: OK (assuming sane 'entry' size) */

      keysize = strlen(key) - 2;
      basic_mud_log("SYSERR: Help entry exceeded buffer space: %.*s", keysize, key);

      /* If we ran out of buffer space, eat the rest of the entry. */
      while (*line != '#')
        get_one_line(fl, line);
    }

    /* now, add the entry to the index with each keyword on the keyword line */
    el.duplicate = 0;
    el.entry = strdup(entry);
    scan = one_word(key, next_key);
    while (*next_key) {
      el.keyword = strdup(next_key);
      help_table[top_of_helpt++] = el;
      el.duplicate++;
      scan = one_word(scan, next_key);
    }

    /* get next keyword line (or $) */
    get_one_line(fl, key);
  }
}


int hsort(const void *a, const void *b)
{
  const struct help_index_element *a1, *b1;

  a1 = (const struct help_index_element *) a;
  b1 = (const struct help_index_element *) b;

  return (str_cmp(a1->keyword, b1->keyword));
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/


int vnum_mobile(char *searchname, struct char_data *ch)
{
  unsigned long int nr;
  int found = 0;

  for (nr = 0; nr < mob_proto.size(); nr++) {
    if (isname(searchname, mob_proto[nr].player.name)) {
      send_to_char(ch, "%3d. [%5d] %s\r\n", ++found, mob_index[nr].vnum, mob_proto[nr].player.short_descr.c_str());
    }
  }

  return found;
}



int vnum_object(char *searchname, struct char_data *ch)
{
  int nr, found = 0;

  for (nr = 0; static_cast<unsigned long>(nr) < obj_proto.size(); nr++)
    if (isname(searchname, obj_proto[nr].name.c_str()))
      send_to_char(ch, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].vnum, obj_proto[nr].short_description.c_str());

  return (found);
}


/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  ch = new char_data;
  clear_char(ch);

  return (ch);
}


/* create a new mobile from a prototype */
struct char_data *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
  mob_rnum i;
  struct char_data *mob;

  if (type == VIRTUAL) {
    if ((i = real_mobile(nr)) == NOBODY) {
      basic_mud_log("WARNING: Mobile vnum %d does not exist in database.", nr);
      return nullptr;
    }
  } else
    i = nr;

  mob = new char_data;
  clear_char(mob);
  *mob = mob_proto[i];
  character_list.push_back(mob);

  if (!mob->points.max_hit) {
    mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
      mob->points.move;
  } else
    mob->points.max_hit = rand_number(mob->points.hit, mob->points.mana);

  mob->points.hit = mob->points.max_hit;
  mob->points.mana = mob->points.max_mana;
  mob->points.move = mob->points.max_move;

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);

  mob_index[i].number++;

  return (mob);
}


/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  obj = new obj_data;

  obj->ex_description = std::list<extra_descr_data>();
  clear_object(obj);
  object_list.push_back(obj);

  return obj;
}


/* create a new object from a prototype */
struct obj_data *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
  struct obj_data *obj;
  obj_rnum i = type == VIRTUAL ? real_object(nr) : nr;

  if (i == NOTHING || static_cast<unsigned long>(i) > obj_proto.size()) {
    basic_mud_log("Object (%c) %d does not exist in database.", type == VIRTUAL ? 'V' : 'R', nr);
    return (NULL);
  }

  
  obj = new obj_data;
  clear_object(obj);

  *obj = obj_proto[i];

  object_list.push_back(obj);

  obj_index[i].number++;

  return (obj);
}



#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; static_cast<unsigned long>(i) < zone_table.size(); i++) {
      if (zone_table[i].age < zone_table[i].lifespan &&
	  zone_table[i].reset_mode)
	(zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan &&
	  zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
	/* enqueue zone */

  update_u = new reset_q_element;

	update_u->zone_to_reset = i;
	update_u->next = 0;

	if (!reset_q.head)
	  reset_q.head = reset_q.tail = update_u;
	else {
	  reset_q.tail->next = update_u;
	  reset_q.tail = update_u;
	}

	zone_table[i].age = ZO_DEAD;
      }
    }
  }	/* end - one minute has passed */


  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
	is_empty(update_u->zone_to_reset)) {
      reset_zone(update_u->zone_to_reset);
      mudlog(CMP, LVL_GOD, FALSE, "Auto zone reset: %s", zone_table[update_u->zone_to_reset].name.c_str());
      /* dequeue */
      if (update_u == reset_q.head)
	reset_q.head = reset_q.head->next;
      else {
	for (temp = reset_q.head; temp->next != update_u;
	     temp = temp->next);

	if (!update_u->next)
	  reset_q.tail = temp;

	temp->next = update_u->next;
      }

      free(update_u);
      break;
    }
}

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
  mudlog(NRM, LVL_GOD, TRUE, "SYSERR: zone file: %s", message);
  mudlog(NRM, LVL_GOD, TRUE, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
	ZCMD.command, zone_table[zone].number, ZCMD.line);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
  unsigned int cmd_no, last_cmd = 0;
  struct char_data *mob = NULL;
  struct obj_data *obj, *obj_to;

  for (cmd_no = 0; cmd_no < zone_table[zone].cmd.size(); cmd_no++) {

    if (ZCMD.if_flag && !last_cmd)
      continue;

    /*  This is the list of actual zone commands.  If any new
     *  zone commands are added to the game, be certain to update
     *  the list of commands in load_zone() so that the counting
     *  will still be correct. - ae.
     */
    switch (ZCMD.command) {
    case '*':			/* ignore command */
      last_cmd = 0;
      break;

    case 'M':			/* read a mobile */
      if (mob_index[ZCMD.arg1].number < ZCMD.arg2) {
	mob = read_mobile(ZCMD.arg1, REAL);
	char_to_room(mob, ZCMD.arg3);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'O':			/* read an object */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	if (ZCMD.arg3 != NOWHERE) {
	  obj = read_object(ZCMD.arg1, REAL);
	  obj_to_room(obj, ZCMD.arg3);
	  last_cmd = 1;
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  IN_ROOM(obj) = NOWHERE;
	  last_cmd = 1;
	}
      } else
	last_cmd = 0;
      break;

    case 'P':			/* object to object */
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	obj = read_object(ZCMD.arg1, REAL);
	if (!(obj_to = get_obj_num(ZCMD.arg3))) {
	  ZONE_ERROR("target obj not found, command disabled");
	  ZCMD.command = '*';
	  break;
	}
	obj_to_obj(obj, obj_to);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'G':			/* obj_to_char */
      if (!mob) {
	ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
	ZCMD.command = '*';
	break;
      }
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	obj = read_object(ZCMD.arg1, REAL);
	obj_to_char(obj, mob);
	last_cmd = 1;
      } else
	last_cmd = 0;
      break;

    case 'E':			/* object to equipment list */
      if (!mob) {
	ZONE_ERROR("trying to equip non-existant mob, command disabled");
	ZCMD.command = '*';
	break;
      }
      if (obj_index[ZCMD.arg1].number < ZCMD.arg2) {
	if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
	  ZONE_ERROR("invalid equipment pos number");
	} else {
	  obj = read_object(ZCMD.arg1, REAL);
	  equip_char(mob, obj, ZCMD.arg3);
	  last_cmd = 1;
	}
      } else
	last_cmd = 0;
      break;

    case 'R': /* rem obj from room */
      if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL)
        extract_obj(obj);
      last_cmd = 1;
      break;


    case 'D':			/* set state of door */
      if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS ||
	  (!std::get<1>(world[ZCMD.arg1].dir_option[ZCMD.arg2]))) {
	ZONE_ERROR("door does not exist, command disabled");
	ZCMD.command = '*';
      } else
	switch (ZCMD.arg3) {
	case 0:
	  REMOVE_BIT(std::get<0>(world[ZCMD.arg1].dir_option[ZCMD.arg2]).exit_info, EX_LOCKED);
	  REMOVE_BIT(std::get<0>(world[ZCMD.arg1].dir_option[ZCMD.arg2]).exit_info, EX_CLOSED);
	  break;
	case 1:
	  SET_BIT(std::get<0>(world[ZCMD.arg1].dir_option[ZCMD.arg2]).exit_info, EX_CLOSED);
	  REMOVE_BIT(std::get<0>(world[ZCMD.arg1].dir_option[ZCMD.arg2]).exit_info, EX_LOCKED);
	  break;
	case 2:
	  SET_BIT(std::get<0>(world[ZCMD.arg1].dir_option[ZCMD.arg2]).exit_info, EX_LOCKED);
	  SET_BIT(std::get<0>(world[ZCMD.arg1].dir_option[ZCMD.arg2]).exit_info, EX_CLOSED);
	  break;
	}
      last_cmd = 1;
      break;

    default:
      ZONE_ERROR("unknown cmd in reset table; cmd disabled");
      ZCMD.command = '*';
      break;
    }
  }

  zone_table[zone].age = 0;
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING)
      continue;
    if (IN_ROOM(i->character) == NOWHERE)
      continue;
    if (GET_LEVEL(i->character) >= LVL_IMMORT)
      continue;
    if (world[IN_ROOM(i->character)].zone != zone_nr)
      continue;

    return (0);
  }

  return (1);
}





/*************************************************************************
*  stuff related to the save/load player system				 *
*************************************************************************/


long get_ptable_by_name(const char *name)
{
  unsigned int i;

  for (i = 0; i < player_table.size(); i++)
    if (!str_cmp(player_table[i].name.c_str(), name))
      return i;

  return -1;
}


long get_id_by_name(const char *name)
{
  unsigned int i;

  for (i = 0; i < player_table.size(); i++)
    if (!str_cmp(player_table[i].name.c_str(), name))
      return (player_table[i].id);

  return -1;
}


std::string get_name_by_id(long id)
{
  unsigned int i;

  for (i = 0; i < player_table.size(); i++)
    if (player_table[i].id == id)
      return (player_table[i].name);

  return std::string();
}


/* Load a char, TRUE if loaded, FALSE if not */
int load_char(const char *name, struct char_file_u *char_element)
{
  int player_i;

  if ((player_i = get_ptable_by_name(name)) >= 0) {
    fseek(player_fl, player_i * sizeof(struct char_file_u), SEEK_SET);
    fread(char_element, sizeof(struct char_file_u), 1, player_fl);
    return (player_i);
  } else
    return (-1);
}




/*
 * write the vital data of a player to the player file
 *
 * And that's it! No more fudging around with the load room.
 * Unfortunately, 'host' modifying is still here due to lack
 * of that variable in the char_data structure.
 */
void save_char(struct char_data *ch)
{
  struct char_file_u st;

  if (IS_NPC(ch) || !ch->desc || GET_PFILEPOS(ch) < 0)
    return;

  char_to_store(ch, &st);

  strncpy(st.host, ch->desc->host, HOST_LENGTH);	/* strncpy: OK (s.host:HOST_LENGTH+1) */
  st.host[HOST_LENGTH] = '\0';

  fseek(player_fl, GET_PFILEPOS(ch) * sizeof(struct char_file_u), SEEK_SET);
  fwrite(&st, sizeof(struct char_file_u), 1, player_fl);
}



/* copy data from the file structure to a char struct */
void store_to_char(struct char_file_u *st, struct char_data *ch)
{
  int i;

  /* to save memory, only PC's -- not MOB's -- have player_specials */
  if (ch->player_specials == NULL)
    ch->player_specials = new player_special_data;

  GET_SEX(ch) = st->sex;
  GET_CLASS(ch) = st->chclass;
  GET_LEVEL(ch) = st->level;

  ch->player.short_descr = "";
  ch->player.long_descr = "";
  ch->player.title = strdup(st->title);
  ch->player.description = strdup(st->description);

  ch->player.hometown = st->hometown;
  ch->player.time.birth = st->birth;
  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);

  ch->player.weight = st->weight;
  ch->player.height = st->height;

  ch->real_abils = st->abilities;
  ch->aff_abils = st->abilities;
  ch->points = st->points;
  ch->char_specials.saved = st->char_specials_saved;
  ch->player_specials->saved = st->player_specials_saved;
  POOFIN(ch) = NULL;
  POOFOUT(ch) = NULL;
  GET_LAST_TELL(ch) = NOBODY;

  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;

  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  ch->points.armor = 100;
  ch->points.hitroll = 0;
  ch->points.damroll = 0;

  ch->player.name = std::string(st->name);
  strlcpy(ch->player.passwd, st->pwd, sizeof(ch->player.passwd));

  /* Add all spell effects */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (st->affected[i].type)
      affect_to_char(ch, st->affected[i]);
  }

  /*
   * If you're not poisioned and you've been away for more than an hour of
   * real time, we'll set your HMV back to full
   */

  if (!AFF_FLAGGED(ch, AFF_POISON) &&
	time(0) - st->last_logon >= SECS_PER_REAL_HOUR) {
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
  }
}				/* store_to_char */




/* copy vital data from a players char-structure to the file structure */
void char_to_store(struct char_data *ch, struct char_file_u *st)
{
  int i;
  struct obj_data *char_eq[NUM_WEARS];

  /* Unaffect everything a character can be affected by */

  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i))
      char_eq[i] = unequip_char(ch, i);
    else
      char_eq[i] = NULL;
  }

  i = 0;
  std::for_each(st->affected, st->affected+MAX_AFFECT, [](affected_type &a) { a = affected_type(); });
  for (auto af = ch->affected.begin(); af != ch->affected.end() && (i < MAX_AFFECT); ++af, i++) {
    st->affected[i] = *af;
  }

  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  auto sz = ch->affected.size();
  while (!ch->affected.empty()) {
    auto &aff = ch->affected.front();
    affect_remove(ch, aff);
  }

  if (sz > MAX_AFFECT)
    basic_mud_log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;

  st->birth = ch->player.time.birth;
  st->played = ch->player.time.played;
  st->played += time(0) - ch->player.time.logon;
  st->last_logon = time(0);

  ch->player.time.played = st->played;
  ch->player.time.logon = time(0);

  st->hometown = ch->player.hometown;
  st->weight = GET_WEIGHT(ch);
  st->height = GET_HEIGHT(ch);
  st->sex = GET_SEX(ch);
  st->chclass = GET_CLASS(ch);
  st->level = GET_LEVEL(ch);
  st->abilities = ch->real_abils;
  st->points = ch->points;
  st->char_specials_saved = ch->char_specials.saved;
  st->player_specials_saved = ch->player_specials->saved;

  st->points.armor = 100;
  st->points.hitroll = 0;
  st->points.damroll = 0;

  if (GET_TITLE(ch))
    strlcpy(st->title, GET_TITLE(ch), MAX_TITLE_LENGTH);
  else
    *st->title = '\0';

  if (!ch->player.description.empty()) {
    if (ch->player.description.length() >= sizeof(st->description)) {
      basic_mud_log("SYSERR: char_to_store: %s's description length: %lu, max: %lu! "
		    "Truncated.", GET_PC_NAME(ch), ch->player.description.length(),
		    sizeof(st->description));
      ch->player.description = ch->player.description.substr(0,sizeof(st->description)-3);      
      ch->player.description += "\r\n";	/* strcat: OK (previous line makes room) */
    }
    strcpy(st->description, ch->player.description.c_str());	/* strcpy: OK (checked above) */
  } else
    *st->description = '\0';

  strcpy(st->name, GET_NAME(ch));	/* strcpy: OK (that's what GET_NAME came from) */
  strcpy(st->pwd, GET_PASSWD(ch));	/* strcpy: OK (that's what GET_PASSWD came from) */

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (st->affected[i].type)
      affect_to_char(ch, st->affected[i]);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i])
      equip_char(ch, char_eq[i], i);
  }
/*   affect_total(ch); unnecessary, I think !?! */
}				/* Char to store */


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(const char *name)
{
  int i;

  player_table.push_back(player_index_element());
  player_table.back().name = new char[strlen(name) + 1];

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (player_table.back().name[i] = LOWER(name[i])); i++)
    ;

  return (player_table.size() - 1);
}



/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/


/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[513];
  char *point;
  int done = 0, length = 0, templength;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      basic_mud_log("SYSERR: fread_string: format error at or near %s", error);
      exit(1);
    }
    /* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
    if ((point = strchr(tmp, '~')) != NULL) {
      *point = '\0';
      done = 1;
    } else {
      point = tmp + strlen(tmp) - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
    }

    templength = strlen(tmp);

    if (length + templength >= MAX_STRING_LENGTH) {
      basic_mud_log("SYSERR: fread_string: string too large (db.c)");
      basic_mud_log("%s", error);
      exit(1);
    } else {
      strcat(buf + length, tmp);	/* strcat: OK (size checked above) */
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  return (strlen(buf) ? strdup(buf) : NULL);
}


/* release memory allocated for a char struct */
void free_char(struct char_data *ch)
{
  if (ch->player_specials != NULL && ch->player_specials != &dummy_mob) {
    GET_ALIASES(ch).clear();

    if (ch->player_specials->poofin)
      free(ch->player_specials->poofin);
    if (ch->player_specials->poofout)
      free(ch->player_specials->poofout);
    free(ch->player_specials);
    if (IS_NPC(ch))
      basic_mud_log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
  }

  std::for_each(ch->affected.begin(), ch->affected.end(), [&ch](affected_type &a) { affect_remove(ch, a); });

  if (ch->desc)
    ch->desc->character = nullptr;

  free(ch);
}

/* release memory allocated for an obj struct */
// TODO: will be deprectaed once completely switched to std::
void free_obj(struct obj_data *obj)
{
  delete obj;
}


std::string slurp_file_to_string(const char *filename)
{
  std::ifstream t(filename);
  return std::string((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
}

/*
 * Steps:
 *   1: Read contents of a text file.
 *   2: Make sure no one is using the pointer in paging.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * strdup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 *
 * If someone is reading a global copy we're trying to
 * replace, give everybody using it a different copy so
 * as to avoid special cases.
 */
int file_to_string_alloc(const char *name, char **buf)
{
  int temppage;
  char temp[MAX_STRING_LENGTH];
  struct descriptor_data *in_use;

  for (in_use = descriptor_list; in_use; in_use = in_use->next)
    if (in_use->showstr_vector && *in_use->showstr_vector == *buf)
      return (-1);

  /* Lets not free() what used to be there unless we succeeded. */
  if (file_to_string(name, temp) < 0)
    return (-1);

  for (in_use = descriptor_list; in_use; in_use = in_use->next) {
    if (!in_use->showstr_count || *in_use->showstr_vector != *buf)
      continue;

    /* Let's be nice and leave them at the page they were on. */
    temppage = in_use->showstr_page;
    paginate_string((in_use->showstr_head = strdup(*in_use->showstr_vector)), in_use);
    in_use->showstr_page = temppage;
  }

  if (*buf)
    free(*buf);

  *buf = strdup(temp);
  return (0);
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
  FILE *fl;
  char tmp[READ_SIZE + 3];
  int len;

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    basic_mud_log("SYSERR: reading %s: %s", name, strerror(errno));
    return (-1);
  }

  for (;;) {
    if (!fgets(tmp, READ_SIZE, fl))	/* EOF check */
      break;
    if ((len = strlen(tmp)) > 0)
      tmp[len - 1] = '\0'; /* take off the trailing \n */
    strcat(tmp, "\r\n");	/* strcat: OK (tmp:READ_SIZE+3) */

    if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH) {
      basic_mud_log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
      *buf = '\0';
      fclose(fl);
      return (-1);
    }
    strcat(buf, tmp);	/* strcat: OK (size checked above) */
  }

  fclose(fl);

  return (0);
}



/* clear some of the the working variables of a char */
void reset_char(struct char_data *ch)
{
  int i;

  for (i = 0; i < NUM_WEARS; i++)
    GET_EQ(ch, i) = NULL;

  ch->followers = NULL;
  ch->master = NULL;
  IN_ROOM(ch) = NOWHERE;
  ch->carrying = NULL;
  ch->next = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;

  GET_LAST_TELL(ch) = NOBODY;
  GET_ALIASES(ch).clear();
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch)
{
  IN_ROOM(ch) = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_MOB_RNUM(ch) = NOBODY;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;

  GET_AC(ch) = 100;		/* Basic Armor */
  if (ch->points.max_mana < 100)
    ch->points.max_mana = 100;
}


void clear_object(struct obj_data *obj)
{
  obj->clear();
  obj->item_number = NOTHING;
  IN_ROOM(obj) = NOWHERE;
  obj->worn_on = NOWHERE;
}




/*
 * Called during character creation after picking character class
 * (and then never again for that character).
 */
void init_char(struct char_data *ch)
{
  int i;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
    ch->player_specials = new player_special_data;

  /* *** if this is our first player --- he be God *** */
  if (player_table.empty()) {
    GET_LEVEL(ch) = LVL_IMPL;
    GET_EXP(ch) = 7000000;

    /* The implementor never goes through do_start(). */
    GET_MAX_HIT(ch) = 500;
    GET_MAX_MANA(ch) = 100;
    GET_MAX_MOVE(ch) = 82;
    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);
  }

  set_title(ch, NULL);
  ch->player.short_descr = "";
  ch->player.long_descr = "";
  ch->player.description = "";

  ch->player.time.birth = time(0);
  ch->player.time.logon = time(0);
  ch->player.time.played = 0;

  GET_HOME(ch) = 1;
  GET_AC(ch) = 100;

  for (i = 0; i < MAX_TONGUE; i++)
    GET_TALK(ch, i) = 0;

  /*
   * make favors for sex -- or in English, we bias the height and weight of the
   * character depending on what gender they've chosen for themselves. While it
   * is possible to have a tall, heavy female it's not as likely as a male.
   *
   * Height is in centimeters. Weight is in pounds.  The only place they're
   * ever printed (in stock code) is SPELL_IDENTIFY.
   */
  if (GET_SEX(ch) == SEX_MALE) {
    GET_WEIGHT(ch) = rand_number(120, 180);
    GET_HEIGHT(ch) = rand_number(160, 200); /* 5'4" - 6'8" */
  } else {
    GET_WEIGHT(ch) = rand_number(100, 160);
    GET_HEIGHT(ch) = rand_number(150, 180); /* 5'0" - 6'0" */
  }

  if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
    player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
  else
    basic_mud_log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));

  for (i = 1; i <= MAX_SKILLS; i++) {
    if (GET_LEVEL(ch) < LVL_IMPL)
      SET_SKILL(ch, i, 0);
    else
      SET_SKILL(ch, i, 100);
  }

  AFF_FLAGS(ch) = 0;

  for (i = 0; i < 5; i++)
    GET_SAVE(ch, i) = 0;

  ch->real_abils.intel = 25;
  ch->real_abils.wis = 25;
  ch->real_abils.dex = 25;
  ch->real_abils.str = 25;
  ch->real_abils.str_add = 100;
  ch->real_abils.con = 25;
  ch->real_abils.cha = 25;

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);

  GET_LOADROOM(ch) = NOWHERE;
}



/* returns the real number of the room with given virtual number */
room_rnum real_room(room_vnum vnum)
{
  auto it = std::find_if(world.begin(), world.end(), [&vnum](const room_data &r) { return (r.number == vnum); });
  return (it == world.end()) ? NOWHERE : std::distance(world.begin(), it);
}


/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
  auto found = std::find_if(mob_index.begin(), mob_index.end(), [=](index_data idx) { return idx.vnum == vnum; });
  if (found == mob_index.end()) {
    return NOBODY;
  }
  return std::distance(mob_index.begin(), found);
}


/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
  auto it = std::find_if(obj_proto.begin(), obj_proto.end(), [&vnum](const obj_data &o) { return (o.vnum == vnum); });
  return (it == obj_proto.end()) ? NOTHING : std::distance(obj_proto.begin(), it);
}


/* returns the real number of the zone with given virtual number */
room_rnum real_zone(room_vnum vnum)
{
  auto it = std::find_if(zone_table.begin(), zone_table.end(), [&vnum](const zone_data &z) { return z.number == vnum; });
  return (it == zone_table.end()) ? NOWHERE : std::distance(zone_table.begin(), it);
}

/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
int check_object(struct obj_data *obj)
{
  char objname[MAX_INPUT_LENGTH + 32];
  int error = FALSE;

  if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) has negative weight (%d).",
		  GET_OBJ_VNUM(obj), obj->short_description.c_str(), GET_OBJ_WEIGHT(obj));

  if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
		  GET_OBJ_VNUM(obj), obj->short_description.c_str(), GET_OBJ_RENT(obj));

  snprintf(objname, sizeof(objname), "Object #%d (%s)", GET_OBJ_VNUM(obj), obj->short_description.c_str());
  error |= check_bitvector_names(GET_OBJ_WEAR(obj), wear_bits_count, objname, "object wear");
  error |= check_bitvector_names(GET_OBJ_EXTRA(obj), extra_bits_count, objname, "object extra");
  error |= check_bitvector_names(GET_OBJ_AFFECT(obj), affected_bits_count, objname, "object affect");

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_DRINKCON:
  {
    char *name = strdup(obj->name.c_str());
    char onealias[MAX_INPUT_LENGTH], *space = strrchr(name, ' ');

    strlcpy(onealias, space ? space + 1 : obj->name.c_str(), sizeof(onealias));
    if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE))
      basic_mud_log("SYSERR: Object #%d (%s) doesn't have drink type as last alias. (%s)",
		    GET_OBJ_VNUM(obj), obj->short_description.c_str(), obj->name.c_str());
    free(name);
  }
  /* Fall through. */
  case ITEM_FOUNTAIN:
    if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
      basic_mud_log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
		    GET_OBJ_VNUM(obj), obj->short_description.c_str(),
		    GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
    break;
  case ITEM_SCROLL:
  case ITEM_POTION:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 1);
    error |= check_object_spell_number(obj, 2);
    error |= check_object_spell_number(obj, 3);
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    error |= check_object_level(obj, 0);
    error |= check_object_spell_number(obj, 3);
    if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
      basic_mud_log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
		    GET_OBJ_VNUM(obj), obj->short_description.c_str(),
		    GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
    break;
 }
  return (error);
}

int check_object_spell_number(struct obj_data *obj, int val)
{
  int error = FALSE;
  const char *spellname;

  if (GET_OBJ_VAL(obj, val) == -1)	/* i.e.: no spell */
    return (error);

  /*
   * Check for negative spells, spells beyond the top define, and any
   * spell which is actually a skill.
   */
  if (GET_OBJ_VAL(obj, val) < 0)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
    error = TRUE;
  if (GET_OBJ_VAL(obj, val) > MAX_SPELLS && GET_OBJ_VAL(obj, val) <= MAX_SKILLS)
    error = TRUE;
  if (error)
    basic_mud_log("SYSERR: Object #%d (%s) has out of range spell #%d.",
		  GET_OBJ_VNUM(obj), obj->short_description.c_str(), GET_OBJ_VAL(obj, val));

  /*
   * This bug has been fixed, but if you don't like the special behavior...
   */
#if 0
  if (GET_OBJ_TYPE(obj) == ITEM_STAFF &&
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
    basic_mud_log("... '%s' (#%d) uses %s spell '%s'.",
	obj->short_description,	GET_OBJ_VNUM(obj),
	HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
	skill_name(GET_OBJ_VAL(obj, val)));
#endif

  if (scheck)		/* Spell names don't exist in syntax check mode. */
    return (error);

  /* Now check for unnamed spells. */
  spellname = skill_name(GET_OBJ_VAL(obj, val));

  if ((spellname == unused_spellname || !str_cmp("UNDEFINED", spellname)) && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) uses '%s' spell #%d.",
		  GET_OBJ_VNUM(obj), obj->short_description.c_str(), spellname,
		GET_OBJ_VAL(obj, val));

  return (error);
}

int check_object_level(struct obj_data *obj, int val)
{
  int error = FALSE;

  if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL) && (error = TRUE))
    basic_mud_log("SYSERR: Object #%d (%s) has out of range level #%d.",
		  GET_OBJ_VNUM(obj), obj->short_description.c_str(), GET_OBJ_VAL(obj, val));

  return (error);
}

int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits)
{
  unsigned int flagnum;
  bool error = FALSE;

  /* See if any bits are set above the ones we know about. */
  if (bits <= (~(bitvector_t)0 >> (sizeof(bitvector_t) * 8 - namecount)))
    return (FALSE);

  for (flagnum = namecount; flagnum < sizeof(bitvector_t) * 8; flagnum++)
    if ((1 << flagnum) & bits) {
      basic_mud_log("SYSERR: %s has unknown %s flag, bit %u (0 through %lu known).", whatami, whatbits, flagnum, namecount - 1);
      error = TRUE;
    }

  return (error);
}

