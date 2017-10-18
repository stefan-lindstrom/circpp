
/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <string>

#include "structs.h"

/* handling the affected-structures */
void	affect_total(struct char_data *ch);
void	affect_modify(struct char_data *ch, byte loc, sbyte mod, bitvector_t bitv, bool add);
void	affect_to_char(struct char_data *ch, struct affected_type *af);
void	affect_remove(struct char_data *ch, struct affected_type *af);
void	affect_from_char(struct char_data *ch, int type);
bool	affected_by_spell(struct char_data *ch, int type);
void	affect_join(struct char_data *ch, struct affected_type *af,
bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);


/* utility */
const char *money_desc(int amount);
struct obj_data *create_money(int amount);
bool    isname(const std::string &str, const std::string &list) noexcept;
int	isname(const char *str, const char *namelist);
char	*fname(const char *namelist);
int	get_number(char **name);

/* ******** objects *********** */

void	obj_to_char(struct obj_data *object, struct char_data *ch);
void	obj_from_char(struct obj_data *object);

void	equip_char(struct char_data *ch, struct obj_data *obj, int pos);
struct obj_data *unequip_char(struct char_data *ch, int pos);
int	invalid_align(struct char_data *ch, struct obj_data *obj);

void	obj_to_room(struct obj_data *object, room_rnum room);
void	obj_from_room(struct obj_data *object);
void	obj_to_obj(struct obj_data *obj, struct obj_data *obj_to);
void	obj_from_obj(struct obj_data *obj);
void	object_list_new_owner(struct obj_data *list, struct char_data *ch);

void	extract_obj(struct obj_data *obj);

/* ******* characters ********* */

struct char_data *get_char_room(char *name, int *num, room_rnum room);
struct char_data *get_char_num(mob_rnum nr);

void	char_from_room(struct char_data *ch);
void	char_to_room(struct char_data *ch, room_rnum room);
void	extract_char(struct char_data *ch);
void	extract_char_final(struct char_data *ch);
void	extract_pending_chars(void);

/* find if character can see */
struct char_data *get_player_vis(struct char_data *ch, char *name, int *number, int inroom);
struct char_data *get_char_vis(struct char_data *ch, char *name, int *number, int where);
struct char_data *get_char_room_vis(struct char_data *ch, char *name, int *number);
struct char_data *get_char_world_vis(struct char_data *ch, char *name, int *number);

struct obj_data *get_obj_in_list_num(int num, struct obj_data *list);
struct obj_data *get_obj_num(obj_rnum nr);
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, int *number, struct obj_data *list);
struct obj_data *get_obj_vis(struct char_data *ch, char *name, int *num);
struct obj_data *get_obj_in_equip_vis(struct char_data *ch, char *arg, int *number, struct obj_data *equipment[]);
int              get_obj_pos_in_equip_vis(struct char_data *ch, char *arg, int *num, struct obj_data *equipment[]);


/* find all dots */

int	find_all_dots(char *arg);

#define FIND_INDIV	0
#define FIND_ALL	1
#define FIND_ALLDOT	2


/* Generic Find */

int	generic_find(char *arg, bitvector_t bitvector, struct char_data *ch,
		struct char_data **tar_ch, struct obj_data **tar_obj);

#define FIND_CHAR_ROOM     (1 << 0)
#define FIND_CHAR_WORLD    (1 << 1)
#define FIND_OBJ_INV       (1 << 2)
#define FIND_OBJ_ROOM      (1 << 3)
#define FIND_OBJ_WORLD     (1 << 4)
#define FIND_OBJ_EQUIP     (1 << 5)


/* prototypes from crash save system */

int	Crash_delete_file(char *name);
int	Crash_delete_crashfile(struct char_data *ch);
int	Crash_clean_file(char *name);
void	Crash_listrent(struct char_data *ch, char *name);
int	Crash_load(struct char_data *ch);
void	Crash_crashsave(struct char_data *ch);
void	Crash_idlesave(struct char_data *ch);
void	Crash_save_all(void);

/* prototypes from fight.c */
void	set_fighting(struct char_data *ch, struct char_data *victim);
void	stop_fighting(struct char_data *ch);
void	hit(struct char_data *ch, struct char_data *victim, int type);
void	forget(struct char_data *ch, struct char_data *victim);
void	remember(struct char_data *ch, struct char_data *victim);
int	damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype);
int	skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype);

#endif
