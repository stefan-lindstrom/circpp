#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__
 
extern const char *circlemud_version;
extern const char *dirs[];
extern const char *room_bits[];
extern const char *exit_bits[];
extern const char *sector_types[];
extern const char *genders[];
extern const char *position_types[];
extern const char *player_bits[];
extern const char *action_bits[];
extern const char *preference_bits[];
extern const char *affected_bits[];
extern const char *connected_types[];
extern const char *wear_where[];
extern const char *equipment_types[];
extern const char *item_types[];
extern const char *wear_bits[];
extern const char *extra_bits[];
extern const char *apply_types[];
extern const char *container_bits[];
extern const char *drinks[];
extern const char *drinknames[];
extern const char *color_liquid[];
extern const char *fullness[];
extern const char *npc_class_types[];
extern const char *weekdays[];
extern const char *month_name[];
extern const struct str_app_type str_app[];
extern const struct dex_skill_type dex_app_skill[];
extern const struct dex_app_type dex_app[];
extern const struct con_app_type con_app[];
extern const struct int_app_type int_app[];
extern const struct wis_app_type wis_app[];
extern int rev_dir[];
extern int movement_loss[];
extern int drink_aff[][3];
extern size_t room_bits_count;
extern size_t action_bits_count;
extern size_t affected_bits_count;
extern size_t extra_bits_count;
extern size_t wear_bits_count;

#endif
