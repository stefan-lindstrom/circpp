#ifndef __FIGHT_H__
#define __FIGHT_H__

#include <list>

// extern data
extern struct std::list<char_data *> combat_list;

/* External procedures */
extern char *fread_action(FILE *fl, int nr);
extern ACMD(do_flee);
extern int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);

#endif
