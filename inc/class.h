#ifndef __CLASS_H__
#define __CLASS_H__

#include "structs.h"

// exported data
extern const char *class_abbrevs[];
extern const char *pc_class_types[];
extern const char *class_menu;
extern int prac_params[4][NUM_CLASSES];
extern struct guild_info_type guild_info[];

// exported functions
int parse_class(char arg);
bitvector_t find_class_bitvector(const char *arg);
void snoop_check(struct char_data *ch);
byte saving_throws(int class_num, int type, int level);
int thaco(int class_num, int level);
void roll_real_abils(struct char_data *ch);
void do_start(struct char_data *ch);
int backstab_mult(int level);
int invalid_class(struct char_data *ch, struct obj_data *obj);
int level_exp(int chclass, int level);
const char *title_male(int chclass, int level);
const char *title_female(int chclass, int level);
void advance_level(struct char_data *ch);
void init_spell_levels(void);

#endif
