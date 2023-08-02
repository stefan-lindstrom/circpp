#ifndef __FIGHT_H__
#define __FIGHT_H__

#include <list>

// extern data
extern struct std::list<char_data *> combat_list;

/* External procedures */
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
int compute_armor_class(struct char_data *ch);
void death_cry(struct char_data *ch);
void raw_kill(struct char_data *ch);
void check_killer(struct char_data *ch, struct char_data *vict);
void appear(struct char_data *ch);
void die(struct char_data *ch);

#endif
