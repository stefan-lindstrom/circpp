/* ************************************************************************
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */
#ifndef __DB_H__
#define __DB_H__

#include "structs.h"

#include <vector>

/* arbitrary constants used by index_boot() (must be unique) */
enum class DBBoot : unsigned char  {
  DB_BOOT_MOB = 0,
  DB_BOOT_SHP,
  DB_BOOT_HLP,
};

#if defined(CIRCLE_MACINTOSH)
#define LIB_WORLD	":world:"
#define LIB_TEXT	":text:"
#define LIB_TEXT_HELP	":text:help:"
#define LIB_MISC	":misc:"
#define LIB_ETC		":etc:"
#define LIB_PLRTEXT	":plrtext:"
#define LIB_PLROBJS	":plrobjs:"
#define LIB_PLRALIAS	":plralias:"
#define LIB_HOUSE	":house:"
#define SLASH		":"
#elif defined(CIRCLE_AMIGA) || defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS) || defined(CIRCLE_ACORN) || defined(CIRCLE_VMS)
#define LIB_WORLD	"world/"
#define LIB_TEXT	"text/"
#define LIB_TEXT_HELP	"text/help/"
#define LIB_MISC	"misc/"
#define LIB_ETC		"etc/"
#define LIB_PLRTEXT	"plrtext/"
#define LIB_PLROBJS	"plrobjs/"
#define LIB_PLRALIAS	"plralias/"
#define LIB_HOUSE	"house/"
#define SLASH		"/"
#else
#error "Unknown path components."
#endif

#define SUF_OBJS	"objs"
#define SUF_TEXT	"text"
#define SUF_ALIAS	"alias"

#if defined(CIRCLE_AMIGA)
#define FASTBOOT_FILE   "/.fastboot"    /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "/.killscript"  /* autorun: shut mud down       */
#define PAUSE_FILE      "/pause"        /* autorun: don't restart mud   */
#elif defined(CIRCLE_MACINTOSH)
#define FASTBOOT_FILE	"::.fastboot"	/* autorun: boot without sleep	*/
#define KILLSCRIPT_FILE	"::.killscript"	/* autorun: shut mud down	*/
#define PAUSE_FILE	"::pause"	/* autorun: don't restart mud	*/
#else
#define FASTBOOT_FILE   "../.fastboot"  /* autorun: boot without sleep  */
#define KILLSCRIPT_FILE "../.killscript"/* autorun: shut mud down       */
#define PAUSE_FILE      "../pause"      /* autorun: don't restart mud   */
#endif

/* names of various files and directories */
#define INDEX_FILE	"index"		/* index of world files		*/
#define MINDEX_FILE	"index.mini"	/* ... and for mini-mud-mode	*/
#define WLD_PREFIX	LIB_WORLD "wld" SLASH	/* room definitions	*/
#define MOB_PREFIX	LIB_WORLD "mob" SLASH	/* monster prototypes	*/
#define OBJ_PREFIX	LIB_WORLD "obj" SLASH	/* object prototypes	*/
#define ZON_PREFIX	LIB_WORLD "zon" SLASH	/* zon defs & command tables */
#define SHP_PREFIX	LIB_WORLD "shp" SLASH	/* shop definitions	*/
#define HLP_PREFIX	LIB_TEXT "help" SLASH	/* for HELP <keyword>	*/

#define CREDITS_FILE	LIB_TEXT "credits" /* for the 'credits' command	*/
#define NEWS_FILE	LIB_TEXT "news"	/* for the 'news' command	*/
#define MOTD_FILE	LIB_TEXT "motd"	/* messages of the day / mortal	*/
#define IMOTD_FILE	LIB_TEXT "imotd"	/* messages of the day / immort	*/
#define GREETINGS_FILE	LIB_TEXT "greetings"	/* The opening screen.	*/
#define HELP_PAGE_FILE	LIB_TEXT_HELP "screen"	/* for HELP <CR>	*/
#define INFO_FILE	LIB_TEXT "info"		/* for INFO		*/
#define WIZLIST_FILE	LIB_TEXT "wizlist"	/* for WIZLIST		*/
#define IMMLIST_FILE	LIB_TEXT "immlist"	/* for IMMLIST		*/
#define BACKGROUND_FILE	LIB_TEXT "background"/* for the background story	*/
#define POLICIES_FILE	LIB_TEXT "policies"  /* player policies/rules	*/
#define HANDBOOK_FILE	LIB_TEXT "handbook"  /* handbook for new immorts	*/

#define IDEA_FILE	LIB_MISC "ideas"	   /* for the 'idea'-command	*/
#define TYPO_FILE	LIB_MISC "typos"	   /*         'typo'		*/
#define BUG_FILE	LIB_MISC "bugs"	   /*         'bug'		*/
#define MESS_FILE	LIB_MISC "messages" /* damage messages		*/
#define SOCMESS_FILE	LIB_MISC "socials"  /* messages for social acts	*/
#define XNAME_FILE	LIB_MISC "xnames"   /* invalid name substrings	*/

#define PLAYER_FILE	LIB_ETC "players"   /* the player database	*/
#define MAIL_FILE	LIB_ETC "plrmail"   /* for the mudmail system	*/
#define BAN_FILE	LIB_ETC "badsites"  /* for the siteban system	*/
#define HCONTROL_FILE	LIB_ETC "hcontrol"  /* for the house system	*/
#define TIME_FILE	LIB_ETC "time"	   /* for calendar system	*/

/* public procedures in db.c */
void	boot_db(void);
void	destroy_db(void);
int	create_entry(const char *name);
void	zone_update(void);
char	*fread_string(FILE *fl, const char *error);
long	get_id_by_name(const char *name);
char	*get_name_by_id(long id);
void	save_mud_time(struct time_info_data *when);
void	free_extra_descriptions(struct extra_descr_data *edesc);
void	free_text_files(void);
void	free_player_index(void);
void	free_help(void);

zone_rnum real_zone(zone_vnum vnum);
room_rnum real_room(room_vnum vnum);
mob_rnum real_mobile(mob_vnum vnum);
obj_rnum real_object(obj_vnum vnum);

int check_bitvector_names(bitvector_t bits, size_t namecount, const char *whatami, const char *whatbits);


void	char_to_store(struct char_data *ch, struct char_file_u *st);
void	store_to_char(struct char_file_u *st, struct char_data *ch);
int	load_char(const char *name, struct char_file_u *char_element);
void	save_char(struct char_data *ch);
void	init_char(struct char_data *ch);
struct char_data* create_char(void);
struct char_data *read_mobile(mob_vnum nr, int type);
int	vnum_mobile(char *searchname, struct char_data *ch);
void	clear_char(struct char_data *ch);
void	reset_char(struct char_data *ch);
void	free_char(struct char_data *ch);

struct obj_data *create_obj(void);
void	clear_object(struct obj_data *obj);
void	free_obj(struct obj_data *obj);
struct obj_data *read_object(obj_vnum nr, int type);
int	vnum_object(char *searchname, struct char_data *ch);

#define REAL 0
#define VIRTUAL 1

/* structure for the reset commands */
struct reset_com {
   char	command;   /* current command                      */

   bool if_flag;	/* if TRUE: exe only if preceding exe'd */
   int	arg1;		/*                                      */
   int	arg2;		/* Arguments to the command             */
   int	arg3;		/*                                      */
   int line;		/* line number this command appears on  */

   /* 
	*  Commands:              *
	*  'M': Read a mobile     *
	*  'O': Read an object    *
	*  'G': Give obj to mob   *
	*  'P': Put obj in obj    *
	*  'G': Obj to char       *
	*  'E': Obj to char equip *
	*  'D': Set state of door *
   */
};



/* zone definition structure. for the 'zone-table'   */
struct zone_data {
  std::string name;		    /* name of this zone                  */
  int	lifespan;           /* how long between resets (minutes)  */
  int	age;                /* current age of this zone (minutes) */
  room_vnum bot;           /* starting room number for this zone */
  room_vnum top;           /* upper limit for rooms in this zone */
  
  int	reset_mode;         /* conditions for reset (see below)   */
  zone_vnum number;	    /* virtual number of this zone	  */

  std::vector<reset_com> cmd;   /* command table for reset	          */  
  /*
   * Reset mode:
   *   0: Don't reset, and don't update age.
   *   1: Reset if no PC's are located in zone.
   *   2: Just reset.
   */
};



/* for queueing zones for update   */
struct reset_q_element {
   zone_rnum zone_to_reset;            /* ref to zone_data */
   struct reset_q_element *next;
};



/* structure for the update queue     */
struct reset_q_type {
   struct reset_q_element *head;
   struct reset_q_element *tail;
};



struct player_index_element {
   char	*name;
   long id;
};


struct help_index_element {
   char	*keyword;
   char *entry;
   int duplicate;
};


/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

#define BANNED_SITE_LENGTH    50
struct ban_list_element {
   char	site[BANNED_SITE_LENGTH+1];
   int	type;
   time_t date;
   char	name[MAX_NAME_LENGTH+1];
   struct ban_list_element *next;
};


/* global buffering system */

extern std::vector<room_data> world;
extern room_rnum top_of_world;

extern std::vector<zone_data> zone_table;
extern zone_rnum top_of_zone_table;

extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct player_special_data dummy_mob;

extern std::vector<index_data> mob_index;
extern std::vector<char_data> mob_proto;
extern mob_rnum top_of_mobt;

extern std::vector<index_data> obj_index;
extern std::vector<obj_data> obj_proto;
extern struct obj_data *object_list;
extern obj_rnum top_of_objt;

extern char	*OK;
extern char	*NOPERSON;
extern char	*NOEFFECT;

extern int top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
extern struct time_info_data time_info;


extern struct time_info_data time_info;
extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern const char *class_abbrevs[];

extern room_rnum donation_room_1;
#if 0
extern room_rnum donation_room_2;  /* uncomment if needed! */
extern room_rnum donation_room_3;  /* uncomment if needed! */
#endif

extern struct spell_info_type spell_info[];
extern int free_rent;
extern int pt_allowed;
extern int max_filesize;
extern int nameserver_is_slow;
extern int auto_save;
extern int track_through_doors;
extern int tunnel_size;

extern FILE *player_fl;
extern struct attack_hit_type attack_hit_text[];
extern time_t boot_time;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int load_into_inventory;
extern int buf_switches, buf_largecount, buf_overflows;
extern int top_of_p_table;
extern int mini_mud;

/* for chars */
extern const char *pc_class_types[];

extern struct message_list fight_messages[MAX_MESSAGES];
extern int pk_allowed;          /* see config.c */
extern int max_exp_gain;        /* see config.c */
extern int max_exp_loss;        /* see config.c */
extern int max_npc_corpse_time, max_pc_corpse_time;
extern int no_mail;
extern int crash_file_timeout;
extern int rent_file_timeout;
extern struct player_index_element *player_table;
extern int max_obj_save;
extern int min_rent_cost;
extern room_rnum r_mortal_start_room;

#endif
