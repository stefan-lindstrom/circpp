#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <string>

#include "structs.h"

#define YES true
#define NO  false

extern bool siteok_everyone;
extern bool pk_allowed;
extern bool pt_allowed;
extern int level_can_shout;
extern int holler_move_cost;
extern int tunnel_size;
extern int max_exp_gain;
extern int max_exp_loss;
extern int max_npc_corpse_time;
extern int max_pc_corpse_time;
extern int idle_void;
extern int idle_rent_time;
extern int idle_max_level;
extern bool dts_are_dumps;
extern bool load_into_inventory;
extern const std::string OK;
extern const std::string NOPERSON;
extern const std::string NOEFFECT;
extern bool track_through_doorsextern ;
extern int immort_level_ok;
extern bool free_rent;
extern int max_obj_save;
extern int min_rent_cost;
extern bool auto_save;
extern int autosave_time;
extern int crash_file_timeout;
extern int rent_file_timeout;
extern room_vnum mortal_start_room;
extern room_vnum immort_start_room;
extern room_vnum frozen_start_room;
extern room_vnum donation_room_1;
extern room_vnum donation_room_2;
extern room_vnum donation_room_3;
extern ush_int DFLT_PORT;
extern const char *DFLT_IP;
extern const char *DFLT_DIR;
extern const char *LOGNAME;
extern int max_playing;
extern int max_filesize;
extern int max_bad_pws;
extern bool siteok_everyone;
extern bool nameserver_is_slow;
extern const std::string MENU;
extern const std::string WELC_MESSG;
extern const std::string START_MESSG;
extern bool use_autowiz;
extern int min_wizlist_lev;

#endif