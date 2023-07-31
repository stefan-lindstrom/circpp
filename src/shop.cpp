/* ************************************************************************
*   File: shop.c                                        Part of CircleMUD *
*  Usage: shopkeepers: loading config files, spec procs.                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "shop.h"
#include "constants.h"

/* External variables */
extern struct time_info_data time_info;

/* Forward/External function declarations */
ACMD(do_tell);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
void sort_keeper_objs(struct char_data *keeper, int shop_nr);


// exported
std::vector<shop_data> shop_index;
int top_shop = -1;

/* Local variables */
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke;

/* local functions */
void shopping_list(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
void shopping_value(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
void shopping_sell(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
struct obj_data *get_selling_obj(struct char_data *ch, char *name, struct char_data *keeper, int shop_nr, int msg);
struct obj_data *slide_obj(struct obj_data *obj, struct char_data *keeper, int shop_nr);
void shopping_buy(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr);
struct obj_data *get_purchase_obj(struct char_data *ch, char *arg, struct char_data *keeper, int shop_nr, int msg);
struct obj_data *get_hash_obj_vis(struct char_data *ch, char *name, struct obj_data *list);
struct obj_data *get_slide_obj_vis(struct char_data *ch, char *name, struct obj_data *list);

char *customer_string(int shop_nr, int detailed);
void list_all_shops(struct char_data *ch);
void list_detailed_shop(struct char_data *ch, int shop_nr);
void show_shops(struct char_data *ch, char *arg);
int is_ok_char(struct char_data *keeper, struct char_data *ch, int shop_nr);
int is_open(struct char_data *keeper, int shop_nr, int msg);
int is_ok(struct char_data *keeper, struct char_data *ch, int shop_nr);
void push(struct stack_data *stack, int pushval);
int top(struct stack_data *stack);
int pop(struct stack_data *stack);
void evaluate_operation(struct stack_data *ops, struct stack_data *vals);
int find_oper_num(char token);
int evaluate_expression(struct obj_data *obj, char *expr);
int trade_with(struct obj_data *item, int shop_nr);
int same_obj(struct obj_data *obj1, struct obj_data *obj2);
bool shop_producing(struct obj_data *item, int shop_nr);
int transaction_amt(char *arg);
char *times_message(struct obj_data *obj, char *name, int num);
int buy_price(struct obj_data *obj, int shop_nr, struct char_data *keeper, struct char_data *buyer);
int sell_price(struct obj_data *obj, int shop_nr, struct char_data *keeper, struct char_data *seller);
char *list_object(struct obj_data *obj, int cnt, int oindex, int shop_nr, struct char_data *keeper, struct char_data *seller);
bool ok_shop_room(int shop_nr, room_vnum room);
SPECIAL(shop_keeper);
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
int fto_list(struct shop_buy_data *list, int type, int *len, int *val);
int end_read_list(struct shop_buy_data *list, int len, int error);
void read_line(FILE *shop_f, const char *string, void *data);
void destroy_shops(void);


/* config arrays */
const char *operator_str[] = {
        "[({",
        "])}",
        "|+",
        "&*",
        "^'"
} ;

/* Constant list for printing out who we sell to */
const char *trade_letters[] = {
        "Good",                 /* First, the alignment based ones */
        "Evil",
        "Neutral",
        "Magic User",           /* Then the class based ones */
        "Cleric",
        "Thief",
        "Warrior",
        "\n"
};


const char *shop_bits[] = {
        "WILL_FIGHT",
        "USES_BANK",
        "\n"
};

int is_ok_char(struct char_data *keeper, struct char_data *ch, int shop_nr)
{
  char buf[MAX_INPUT_LENGTH];

  if (!CAN_SEE(keeper, ch)) {
    char actbuf[MAX_INPUT_LENGTH] = MSG_NO_SEE_CHAR;
    do_say(keeper, actbuf, cmd_say, 0);
    return (FALSE);
  }
  if (IS_GOD(ch))
    return (TRUE);

  if ((IS_GOOD(ch) && NOTRADE_GOOD(shop_nr)) ||
      (IS_EVIL(ch) && NOTRADE_EVIL(shop_nr)) ||
      (IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop_nr))) {
    snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_SELL_ALIGN);
    do_tell(keeper, buf, cmd_tell, 0);
    return (FALSE);
  }
  if (IS_NPC(ch))
    return (TRUE);

  if ((IS_MAGIC_USER(ch) && NOTRADE_MAGIC_USER(shop_nr)) ||
      (IS_CLERIC(ch) && NOTRADE_CLERIC(shop_nr)) ||
      (IS_THIEF(ch) && NOTRADE_THIEF(shop_nr)) ||
      (IS_WARRIOR(ch) && NOTRADE_WARRIOR(shop_nr))) {
    snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_SELL_CLASS);
    do_tell(keeper, buf, cmd_tell, 0);
    return (FALSE);
  }
  return (TRUE);
}


int is_open(struct char_data *keeper, int shop_nr, int msg)
{
  char buf[MAX_INPUT_LENGTH];

  *buf = '\0';
  if (SHOP_OPEN1(shop_nr) > time_info.hours)
    strlcpy(buf, MSG_NOT_OPEN_YET, sizeof(buf));
  else if (SHOP_CLOSE1(shop_nr) < time_info.hours) {
    if (SHOP_OPEN2(shop_nr) > time_info.hours)
      strlcpy(buf, MSG_NOT_REOPEN_YET, sizeof(buf));
    else if (SHOP_CLOSE2(shop_nr) < time_info.hours)
      strlcpy(buf, MSG_CLOSED_FOR_DAY, sizeof(buf));
  }
  if (!*buf)
    return (TRUE);
  if (msg)
    do_say(keeper, buf, cmd_tell, 0);
  return (FALSE);
}


int is_ok(struct char_data *keeper, struct char_data *ch, int shop_nr)
{
  if (is_open(keeper, shop_nr, TRUE))
    return (is_ok_char(keeper, ch, shop_nr));
  else
    return (FALSE);
}


void push(struct stack_data *stack, int pushval)
{
  S_DATA(stack, S_LEN(stack)++) = pushval;
}


int top(struct stack_data *stack)
{
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, S_LEN(stack) - 1));
  else
    return (NOTHING);
}


int pop(struct stack_data *stack)
{
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, --S_LEN(stack)));
  else {
    basic_mud_log("SYSERR: Illegal expression %d in shop keyword list.", S_LEN(stack));
    return (0);
  }
}


void evaluate_operation(struct stack_data *ops, struct stack_data *vals)
{
  int oper;

  if ((oper = pop(ops)) == OPER_NOT)
    push(vals, !pop(vals));
  else {
    int val1 = pop(vals),
	val2 = pop(vals);

    /* Compiler would previously short-circuit these. */
    if (oper == OPER_AND)
      push(vals, val1 && val2);
    else if (oper == OPER_OR)
      push(vals, val1 || val2);
  }
}


int find_oper_num(char token)
{
  int oindex;

  for (oindex = 0; oindex <= MAX_OPER; oindex++)
    if (strchr(operator_str[oindex], token))
      return (oindex);
  return (NOTHING);
}


int evaluate_expression(struct obj_data *obj, char *expr)
{
  struct stack_data ops, vals;
  char *ptr, *end, name[MAX_STRING_LENGTH];
  int temp, eindex;

  if (!expr || !*expr)	/* Allows opening ( first. */
    return (TRUE);

  ops.len = vals.len = 0;
  ptr = expr;
  while (*ptr) {
    if (isspace(*ptr))
      ptr++;
    else {
      if ((temp = find_oper_num(*ptr)) == NOTHING) {
	end = ptr;
	while (*ptr && !isspace(*ptr) && find_oper_num(*ptr) == NOTHING)
	  ptr++;
	strncpy(name, end, ptr - end);	/* strncpy: OK (name/end:MAX_STRING_LENGTH) */
	name[ptr - end] = '\0';
	for (eindex = 0; *extra_bits[eindex] != '\n'; eindex++)
	  if (!str_cmp(name, extra_bits[eindex])) {
	    push(&vals, OBJ_FLAGGED(obj, 1 << eindex));
	    break;
	  }
	if (*extra_bits[eindex] == '\n')
	  push(&vals, isname(name, obj->name));
      } else {
	if (temp != OPER_OPEN_PAREN)
	  while (top(&ops) > temp)
	    evaluate_operation(&ops, &vals);

	if (temp == OPER_CLOSE_PAREN) {
	  if ((temp = pop(&ops)) != OPER_OPEN_PAREN) {
	    basic_mud_log("SYSERR: Illegal parenthesis in shop keyword expression.");
	    return (FALSE);
	  }
	} else
	  push(&ops, temp);
	ptr++;
      }
    }
  }
  while (top(&ops) != NOTHING)
    evaluate_operation(&ops, &vals);
  temp = pop(&vals);
  if (top(&vals) != NOTHING) {
    basic_mud_log("SYSERR: Extra operands left on shop keyword expression stack.");
    return (FALSE);
  }
  return (temp);
}


int trade_with(struct obj_data *item, int shop_nr)
{
  if (GET_OBJ_COST(item) < 1)
    return (OBJECT_NOVAL);

  if (OBJ_FLAGGED(item, ITEM_NOSELL))
    return (OBJECT_NOTOK);

  for (auto it = shop_index[shop_nr].type.begin(); it != shop_index[shop_nr].type.end(); ++it) {
    if (it->type != NOTHING)  {
      if (GET_OBJ_VAL(item, 2) == 0 && (GET_OBJ_TYPE(item) == ITEM_WAND || GET_OBJ_TYPE(item) == ITEM_STAFF)) {
        return OBJECT_DEAD;
      }
    }
    else if (evaluate_expression(item, it->keywords)) {
      return OBJECT_OK;
    }
  }
  return OBJECT_NOTOK;
}


int same_obj(struct obj_data *obj1, struct obj_data *obj2)
{
  int aindex;

  if (!obj1 || !obj2)
    return (obj1 == obj2);

  if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
    return (FALSE);

  if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
    return (FALSE);

  if (GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2))
    return (FALSE);

  for (aindex = 0; aindex < MAX_OBJ_AFFECT; aindex++)
    if ((obj1->affected[aindex].location != obj2->affected[aindex].location) ||
	(obj1->affected[aindex].modifier != obj2->affected[aindex].modifier))
      return (FALSE);

  return (TRUE);
}


bool shop_producing(struct obj_data *item, int shop_nr)
{
  if (GET_OBJ_RNUM(item) == NOTHING) {
    return false;
  }

  for (auto it = shop_index[shop_nr].producing.begin(); it != shop_index[shop_nr].producing.end(); ++it) {
    if (same_obj(item, &obj_proto[*it])) {
      return true;
    }
  }
  return false;
}


int transaction_amt(char *arg)
{
  char buf[MAX_INPUT_LENGTH];

  char *buywhat;

  /*
   * If we have two arguments, it means 'buy 5 3', or buy 5 of #3.
   * We don't do that if we only have one argument, like 'buy 5', buy #5.
   * Code from Andrey Fidrya <andrey@ALEX-UA.COM>
   */
  buywhat = one_argument(arg, buf);
  if (*buywhat && *buf && is_number(buf)) {
    strcpy(arg, arg + strlen(buf) + 1);	/* strcpy: OK (always smaller) */
    return (atoi(buf));
  }
  return (1);
}


char *times_message(struct obj_data *obj, char *name, int num)
{
  static char buf[256];
  size_t len;
  char *ptr;

  if (obj)
    len = strlcpy(buf, obj->short_description.c_str(), sizeof(buf));
  else {
    if ((ptr = strchr(name, '.')) == NULL)
      ptr = name;
    else
      ptr++;
    len = snprintf(buf, sizeof(buf), "%s %s", AN(ptr), ptr);
  }

  if (num > 1 && len < sizeof(buf))
    snprintf(buf + len, sizeof(buf) - len, " (x %d)", num);

  return (buf);
}


struct obj_data *get_slide_obj_vis(struct char_data *ch, char *name,
				            struct obj_data *list)
{
  struct obj_data *i, *last_match = NULL;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  strlcpy(tmpname, name, sizeof(tmpname));
  tmp = tmpname;
  if (!(number = get_number(&tmp)))
    return (NULL);

  for (i = list, j = 1; i && (j <= number); i = i->next_content)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i)) {
	if (j == number)
	  return (i);
	last_match = i;
	j++;
      }
  return (NULL);
}


struct obj_data *get_hash_obj_vis(struct char_data *ch, char *name,
				           struct obj_data *list)
{
  struct obj_data *loop, *last_obj = NULL;
  int qindex;

  if (is_number(name))
    qindex = atoi(name);
  else if (is_number(name + 1))
    qindex = atoi(name + 1);
  else
    return (NULL);

  for (loop = list; loop; loop = loop->next_content)
    if (CAN_SEE_OBJ(ch, loop) && GET_OBJ_COST(loop) > 0)
      if (!same_obj(last_obj, loop)) {
	if (--qindex == 0)
	  return (loop);
	last_obj = loop;
      }
  return (NULL);
}


struct obj_data *get_purchase_obj(struct char_data *ch, char *arg,
		            struct char_data *keeper, int shop_nr, int msg)
{
  char name[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(arg, name);
  do {
    if (*name == '#' || is_number(name))
      obj = get_hash_obj_vis(ch, name, keeper->carrying);
    else
      obj = get_slide_obj_vis(ch, name, keeper->carrying);
    if (!obj) {
      if (msg) {
        char buf[MAX_INPUT_LENGTH];

	snprintf(buf, sizeof(buf), shop_index[shop_nr].no_such_item1.c_str(), GET_NAME(ch));
	do_tell(keeper, buf, cmd_tell, 0);
      }
      return nullptr;
    }
    if (GET_OBJ_COST(obj) <= 0) {
      extract_obj(obj);
      obj = nullptr;
    }
  } while (!obj);
  return obj;
}


/*
 * Shop purchase adjustment, based on charisma-difference from buyer to keeper.
 *
 * for i in `seq 15 -15`; do printf " * %3d: %6.4f\n" $i \
 * `echo "scale=4; 1+$i/70" | bc`; done
 *
 *  Shopkeeper higher charisma (markup)
 *  ^  15: 1.2142  14: 1.2000  13: 1.1857  12: 1.1714  11: 1.1571
 *  |  10: 1.1428   9: 1.1285   8: 1.1142   7: 1.1000   6: 1.0857
 *  |   5: 1.0714   4: 1.0571   3: 1.0428   2: 1.0285   1: 1.0142
 *  +   0: 1.0000
 *  |  -1: 0.9858  -2: 0.9715  -3: 0.9572  -4: 0.9429  -5: 0.9286
 *  |  -6: 0.9143  -7: 0.9000  -8: 0.8858  -9: 0.8715 -10: 0.8572
 *  v -11: 0.8429 -12: 0.8286 -13: 0.8143 -14: 0.8000 -15: 0.7858
 *  Player higher charisma (discount)
 *
 * Most mobiles have 11 charisma so an 18 charisma player would get a 10%
 * discount beyond the basic price.  That assumes they put a lot of points
 * into charisma, because on the flip side they'd get 11% inflation by
 * having a 3.
 */
int buy_price(struct obj_data *obj, int shop_nr, struct char_data *keeper, struct char_data *buyer)
{
  return (int) (GET_OBJ_COST(obj) * SHOP_BUYPROFIT(shop_nr)
	* (1 + (GET_CHA(keeper) - GET_CHA(buyer)) / (float)70));
}

/*
 * When the shopkeeper is buying, we reverse the discount. Also make sure
 * we don't buy for more than we sell for, to prevent infinite money-making.
 */
int sell_price(struct obj_data *obj, int shop_nr, struct char_data *keeper, struct char_data *seller)
{
  float sell_cost_modifier = SHOP_SELLPROFIT(shop_nr) * (1 - (GET_CHA(keeper) - GET_CHA(seller)) / (float)70);
  float buy_cost_modifier = SHOP_BUYPROFIT(shop_nr) * (1 + (GET_CHA(keeper) - GET_CHA(seller)) / (float)70);

  if (sell_cost_modifier > buy_cost_modifier)
    sell_cost_modifier = buy_cost_modifier;

  return (int) (GET_OBJ_COST(obj) * sell_cost_modifier);
}


void shopping_buy(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char tempstr[MAX_INPUT_LENGTH], tempbuf[2*MAX_INPUT_LENGTH];
  struct obj_data *obj, *last_obj = NULL;
  int goldamt = 0, buynum, bought = 0;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  if ((buynum = transaction_amt(arg)) < 0) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s A negative amount?  Try selling me something.",
	    GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if (!*arg || !buynum) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s What do you want to buy??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
    return;

  if (buy_price(obj, shop_nr, keeper, ch) > GET_GOLD(ch) && !IS_GOD(ch)) {
    char actbuf[MAX_INPUT_LENGTH];

    snprintf(actbuf, sizeof(actbuf), shop_index[shop_nr].missing_cash2.c_str(), GET_NAME(ch));
    do_tell(keeper, actbuf, cmd_tell, 0);

    switch (SHOP_BROKE_TEMPER(shop_nr)) {
    case 0:
      do_action(keeper, strcpy(actbuf, GET_NAME(ch)), cmd_puke, 0);	/* strcpy: OK (MAX_NAME_LENGTH < MAX_INPUT_LENGTH) */
      return;
    case 1:
      do_echo(keeper, strcpy(actbuf, "smokes on his joint."), cmd_emote, SCMD_EMOTE);	/* strcpy: OK */
      return;
    default:
      return;
    }
  }
  if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
    send_to_char(ch, "%s: You can't carry any more items.\r\n", fname(obj->name.c_str()));
    return;
  }
  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch)) {
    send_to_char(ch, "%s: You can't carry that much weight.\r\n", fname(obj->name.c_str()));
    return;
  }
  while (obj && (GET_GOLD(ch) >= buy_price(obj, shop_nr, keeper, ch) || IS_GOD(ch))
	 && IS_CARRYING_N(ch) < CAN_CARRY_N(ch) && bought < buynum
	 && IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch)) {
    int charged;

    bought++;
    /* Test if producing shop ! */
    if (shop_producing(obj, shop_nr))
      obj = read_object(GET_OBJ_RNUM(obj), REAL);
    else {
      obj_from_char(obj);
      SHOP_SORT(shop_nr)--;
    }
    obj_to_char(obj, ch);

    charged = buy_price(obj, shop_nr, keeper, ch);
    goldamt += charged;
    if (!IS_GOD(ch))
      GET_GOLD(ch) -= charged;

    last_obj = obj;
    obj = get_purchase_obj(ch, arg, keeper, shop_nr, FALSE);
    if (!same_obj(obj, last_obj))
      break;
  }

  if (bought < buynum) {
    char buf[MAX_INPUT_LENGTH];

    if (!obj || !same_obj(last_obj, obj))
      snprintf(buf, sizeof(buf), "%s I only have %d to sell you.", GET_NAME(ch), bought);
    else if (GET_GOLD(ch) < buy_price(obj, shop_nr, keeper, ch))
      snprintf(buf, sizeof(buf), "%s You can only afford %d.", GET_NAME(ch), bought);
    else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      snprintf(buf, sizeof(buf), "%s You can only hold %d.", GET_NAME(ch), bought);
    else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
      snprintf(buf, sizeof(buf), "%s You can only carry %d.", GET_NAME(ch), bought);
    else
      snprintf(buf, sizeof(buf), "%s Something screwy only gave you %d.", GET_NAME(ch), bought);
    do_tell(keeper, buf, cmd_tell, 0);
  }
  if (!IS_GOD(ch))
    GET_GOLD(keeper) += goldamt;

  strlcpy(tempstr, times_message(ch->carrying, 0, bought), sizeof(tempstr));

  snprintf(tempbuf, sizeof(tempbuf), "$n buys %s.", tempstr);
  act(tempbuf, FALSE, ch, obj, 0, CommTarget::TO_ROOM);

  snprintf(tempbuf, sizeof(tempbuf), shop_index[shop_nr].message_buy.c_str(), GET_NAME(ch), goldamt);
  do_tell(keeper, tempbuf, cmd_tell, 0);

  send_to_char(ch, "You now have %s.\r\n", tempstr);

  if (SHOP_USES_BANK(shop_nr))
    if (GET_GOLD(keeper) > MAX_OUTSIDE_BANK) {
      SHOP_BANK(shop_nr) += (GET_GOLD(keeper) - MAX_OUTSIDE_BANK);
      GET_GOLD(keeper) = MAX_OUTSIDE_BANK;
    }
}


struct obj_data *get_selling_obj(struct char_data *ch, char *name, struct char_data *keeper, int shop_nr, int msg)
{
  char buf[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int result;

  if (!(obj = get_obj_in_list_vis(ch, name, NULL, ch->carrying))) {
    if (msg) {
      char tbuf[MAX_INPUT_LENGTH];

      snprintf(tbuf, sizeof(tbuf), shop_index[shop_nr].no_such_item2.c_str(), GET_NAME(ch));
      do_tell(keeper, tbuf, cmd_tell, 0);
    }
    return (NULL);
  }
  if ((result = trade_with(obj, shop_nr)) == OBJECT_OK)
    return (obj);

  if (!msg)
    return (0);

  switch (result) {
  case OBJECT_NOVAL:
    snprintf(buf, sizeof(buf), "%s You've got to be kidding, that thing is worthless!", GET_NAME(ch));
    break;
  case OBJECT_NOTOK:
    snprintf(buf, sizeof(buf), shop_index[shop_nr].do_not_buy.c_str(), GET_NAME(ch));
    break;
  case OBJECT_DEAD:
    snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
    break;
  default:
    basic_mud_log("SYSERR: Illegal return value of %d from trade_with() (%s)", result, __FILE__);	/* Someone might rename it... */
    snprintf(buf, sizeof(buf), "%s An error has occurred.", GET_NAME(ch));
    break;
  }
  do_tell(keeper, buf, cmd_tell, 0);
  return (NULL);
}


struct obj_data *slide_obj(struct obj_data *obj, struct char_data *keeper,
			            int shop_nr)
/*
   This function is a slight hack!  To make sure that duplicate items are
   only listed once on the "list", this function groups "identical"
   objects together on the shopkeeper's inventory list.  The hack involves
   knowing how the list is put together, and manipulating the order of
   the objects on the list.  (But since most of DIKU is not encapsulated,
   and information hiding is almost never used, it isn't that big a deal) -JF
*/
{
  struct obj_data *loop;
  int temp;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  /* Extract the object if it is identical to one produced */
  if (shop_producing(obj, shop_nr)) {
    temp = GET_OBJ_RNUM(obj);
    extract_obj(obj);
    return (&obj_proto[temp]);
  }
  SHOP_SORT(shop_nr)++;
  loop = keeper->carrying;
  obj_to_char(obj, keeper);
  keeper->carrying = loop;
  while (loop) {
    if (same_obj(obj, loop)) {
      obj->next_content = loop->next_content;
      loop->next_content = obj;
      return (obj);
    }
    loop = loop->next_content;
  }
  keeper->carrying = obj;
  return (obj);
}


void sort_keeper_objs(struct char_data *keeper, int shop_nr)
{
  struct obj_data *list = NULL, *temp;

  while (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper)) {
    temp = keeper->carrying;
    obj_from_char(temp);
    temp->next_content = list;
    list = temp;
  }

  while (list) {
    temp = list;
    list = list->next_content;
    if (shop_producing(temp, shop_nr) &&
	!get_obj_in_list_num(GET_OBJ_RNUM(temp), keeper->carrying)) {
      obj_to_char(temp, keeper);
      SHOP_SORT(shop_nr)++;
    } else
      slide_obj(temp, keeper, shop_nr);
  }
}


void shopping_sell(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char tempstr[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], tempbuf[2*MAX_INPUT_LENGTH];
  struct obj_data *obj;
  int sellnum, sold = 0, goldamt = 0;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if ((sellnum = transaction_amt(arg)) < 0) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s A negative amount?  Try buying something.", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if (!*arg || !sellnum) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), "%s What do you want to sell??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
    return;

  if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < sell_price(obj, shop_nr, keeper, ch)) {
    char buf[MAX_INPUT_LENGTH];

    snprintf(buf, sizeof(buf), shop_index[shop_nr].missing_cash1.c_str(), GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  while (obj && GET_GOLD(keeper) + SHOP_BANK(shop_nr) >= sell_price(obj, shop_nr, keeper, ch) && sold < sellnum) {
    int charged = sell_price(obj, shop_nr, keeper, ch);

    goldamt += charged;
    GET_GOLD(keeper) -= charged;

    sold++;
    obj_from_char(obj);
    slide_obj(obj, keeper, shop_nr);	/* Seems we don't use return value. */
    obj = get_selling_obj(ch, name, keeper, shop_nr, FALSE);
  }

  if (sold < sellnum) {
    char buf[MAX_INPUT_LENGTH];

    if (!obj)
      snprintf(buf, sizeof(buf), "%s You only have %d of those.", GET_NAME(ch), sold);
    else if (GET_GOLD(keeper) + SHOP_BANK(shop_nr) < sell_price(obj, shop_nr, keeper, ch))
      snprintf(buf, sizeof(buf), "%s I can only afford to buy %d of those.", GET_NAME(ch), sold);
    else
      snprintf(buf, sizeof(buf), "%s Something really screwy made me buy %d.", GET_NAME(ch), sold);

    do_tell(keeper, buf, cmd_tell, 0);
  }
  GET_GOLD(ch) += goldamt;

  strlcpy(tempstr, times_message(0, name, sold), sizeof(tempstr));
  snprintf(tempbuf, sizeof(tempbuf), "$n sells %s.", tempstr);
  act(tempbuf, FALSE, ch, obj, 0, CommTarget::TO_ROOM);

  snprintf(tempbuf, sizeof(tempbuf), shop_index[shop_nr].message_sell.c_str(), GET_NAME(ch), goldamt);
  do_tell(keeper, tempbuf, cmd_tell, 0);

  send_to_char(ch, "The shopkeeper now has %s.\r\n", tempstr);

  if (GET_GOLD(keeper) < MIN_OUTSIDE_BANK) {
    goldamt = MIN(MAX_OUTSIDE_BANK - GET_GOLD(keeper), SHOP_BANK(shop_nr));
    SHOP_BANK(shop_nr) -= goldamt;
    GET_GOLD(keeper) += goldamt;
  }
}


void shopping_value(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  if (!is_ok(keeper, ch, shop_nr))
    return;

  if (!*arg) {
    snprintf(buf, sizeof(buf), "%s What do you want me to evaluate??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
    return;

  snprintf(buf, sizeof(buf), "%s I'll give you %d gold coins for that!", GET_NAME(ch), sell_price(obj, shop_nr, keeper, ch));
  do_tell(keeper, buf, cmd_tell, 0);
}


char *list_object(struct obj_data *obj, int cnt, int aindex, int shop_nr, struct char_data *keeper, struct char_data *ch)
{
  static char result[256];
  char	itemname[128],
	quantity[16];	/* "Unlimited" or "%d" */

  if (shop_producing(obj, shop_nr))
    strcpy(quantity, "Unlimited");	/* strcpy: OK (for 'quantity >= 10') */
  else
    sprintf(quantity, "%d", cnt);	/* sprintf: OK (for 'quantity >= 11', 32-bit int) */

  switch (GET_OBJ_TYPE(obj)) {
  case ITEM_DRINKCON:
    if (GET_OBJ_VAL(obj, 1))
      snprintf(itemname, sizeof(itemname), "%s of %s", obj->short_description.c_str(), drinks[GET_OBJ_VAL(obj, 2)]);
    else
      strlcpy(itemname, obj->short_description.c_str(), sizeof(itemname));
    break;

  case ITEM_WAND:
  case ITEM_STAFF:
    snprintf(itemname, sizeof(itemname), "%s%s", obj->short_description.c_str(),
	GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1) ? " (partially used)" : "");
    break;

  default:
    strlcpy(itemname, obj->short_description.c_str(), sizeof(itemname));
    break;
  }
  CAP(itemname);

  snprintf(result, sizeof(result), " %2d)  %9s   %-48s %6d\r\n",
	aindex, quantity, itemname, buy_price(obj, shop_nr, keeper, ch));
  return (result);
}


void shopping_list(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr)
{
  char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
  struct obj_data *obj, *last_obj = NULL;
  int cnt = 0, lindex = 0, found = FALSE;
  size_t len;
  /* cnt is the number of that particular object available */

  if (!is_ok(keeper, ch, shop_nr))
    return;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  one_argument(arg, name);

  len = strlcpy(buf,   " ##   Available   Item                                               Cost\r\n"
		"-------------------------------------------------------------------------\r\n", sizeof(buf));
  if (keeper->carrying)
    for (obj = keeper->carrying; obj; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_COST(obj) > 0) {
	if (!last_obj) {
	  last_obj = obj;
	  cnt = 1;
	} else if (same_obj(last_obj, obj))
	  cnt++;
	else {
	  lindex++;
	  if (!*name || isname(name, last_obj->name)) {
	    strncat(buf, list_object(last_obj, cnt, lindex, shop_nr, keeper, ch), sizeof(buf) - len - 1);	/* strncat: OK */
            len = strlen(buf);
            if (len + 1 >= sizeof(buf))
              break;
            found = TRUE;
          }
	  cnt = 1;
	  last_obj = obj;
	}
      }
  lindex++;
  if (!last_obj)	/* we actually have nothing in our list for sale, period */
    send_to_char(ch, "Currently, there is nothing for sale.\r\n");
  else if (*name && !found)	/* nothing the char was looking for was found */
    send_to_char(ch, "Presently, none of those are for sale.\r\n");
  else {
    if (!*name || isname(name, last_obj->name))	/* show last obj */
      if (len < sizeof(buf))
        strncat(buf, list_object(last_obj, cnt, lindex, shop_nr, keeper, ch), sizeof(buf) - len - 1);	/* strncat: OK */
    page_string(ch->desc, buf, TRUE);
  }
}


bool ok_shop_room(int shop_nr, room_vnum room)
{
  for (auto it = shop_index[shop_nr].in_room.begin(); it != shop_index[shop_nr].in_room.end(); ++it ) {
    if (*it == room) {
      return true;
    }
  }
  return false;
}


SPECIAL(shop_keeper)
{
  struct char_data *keeper = (struct char_data *)me;
  int shop_nr;

  for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
    if (SHOP_KEEPER(shop_nr) == keeper->nr)
      break;

  if (shop_nr > top_shop)
    return (FALSE);

  if (SHOP_FUNC(shop_nr))	/* Check secondary function */
    if ((SHOP_FUNC(shop_nr)) (ch, me, cmd, argument))
      return (TRUE);

  if (keeper == ch) {
    if (cmd)
      SHOP_SORT(shop_nr) = 0;	/* Safety in case "drop all" */
    return (FALSE);
  }
  if (!ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch))))
    return (0);

  if (!AWAKE(keeper))
    return (FALSE);

  if (CMD_IS("steal")) {
    char argm[MAX_INPUT_LENGTH];

    snprintf(argm, sizeof(argm), "$N shouts '%s'", MSG_NO_STEAL_HERE);
    act(argm, FALSE, ch, 0, keeper, CommTarget::TO_CHAR);

    do_action(keeper, const_cast<char *>(GET_NAME(ch)), cmd_slap, 0);
    return (TRUE);
  }

  if (CMD_IS("buy")) {
    shopping_buy(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("sell")) {
    shopping_sell(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("value")) {
    shopping_value(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("list")) {
    shopping_list(argument, ch, keeper, shop_nr);
    return (TRUE);
  }
  return (FALSE);
}


int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim)
{
  int sindex;

  if (!IS_MOB(victim) || mob_index[GET_MOB_RNUM(victim)].func != shop_keeper)
    return (TRUE);

  /* Prevent "invincible" shopkeepers if they're charmed. */
  if (AFF_FLAGGED(victim, AFF_CHARM))
    return (TRUE);

  for (sindex = 0; sindex <= top_shop; sindex++)
    if (GET_MOB_RNUM(victim) == SHOP_KEEPER(sindex) && !SHOP_KILL_CHARS(sindex)) {
      char buf[MAX_INPUT_LENGTH];

      snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_CANT_KILL_KEEPER);
      do_tell(victim, buf, cmd_tell, 0);

      do_action(victim, const_cast<char *>(GET_NAME(ch)), cmd_slap, 0);
      return (FALSE);
    }

  return (TRUE);
}


/* val == obj_vnum and obj_rnum (?) */
int add_to_list(struct shop_buy_data *list, int type, int *len, int *val)
{
  if (*val != NOTHING) {
    if (*len < MAX_SHOP_OBJ) {
      if (type == LIST_PRODUCE)
	*val = real_object(*val);
      if (*val != NOTHING) {
	BUY_TYPE(list[*len]) = *val;
	BUY_WORD(list[(*len)++]) = NULL;
      } else
	*val = NOTHING;
      return (FALSE);
    } else
      return (TRUE);
  }
  return (FALSE);
}


int end_read_list(struct shop_buy_data *list, int len, int error)
{
  if (error)
    basic_mud_log("SYSERR: Raise MAX_SHOP_OBJ constant in shop.h to %d", len + error);
  BUY_WORD(list[len]) = NULL;
  BUY_TYPE(list[len++]) = NOTHING;
  return (len);
}


void read_line(FILE *shop_f, const char *string, void *data)
{
  char buf[READ_SIZE];

  if (!get_line(shop_f, buf) || !sscanf(buf, string, data)) {
    basic_mud_log("SYSERR: Error in shop #%d, near '%s' with '%s'", SHOP_NUM(top_shop), buf, string);
    exit(1);
  }
}

void assign_the_shopkeepers(void)
{
  cmd_say = find_command("say");
  cmd_tell = find_command("tell");
  cmd_emote = find_command("emote");
  cmd_slap = find_command("slap");
  cmd_puke = find_command("puke");

  for (auto it = shop_index.begin(); it != shop_index.end(); ++it) {
    shop_data shop = *it;

    if (shop.keeper == NOBODY) {
      continue;
    }

    /* Having SHOP_FUNC() as 'shop_keeper' will cause infinite recursion. */
    if (mob_index[shop.keeper].func && mob_index[shop.keeper].func != shop_keeper) {
      shop.func = mob_index[shop.keeper].func;
    }

    mob_index[shop.keeper].func = shop_keeper;
  }
}


char *customer_string(int shop_nr, int detailed)
{
  int sindex = 0, flag = 1, nlen;
  size_t len = 0;
  static char buf[256];

  while (*trade_letters[sindex] != '\n' && len + 1 < sizeof(buf)) {
    if (detailed) {
      if (!IS_SET(flag, SHOP_TRADE_WITH(shop_nr))) {
	nlen = snprintf(buf + len, sizeof(buf) - len, ", %s", trade_letters[sindex]);

        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;

        len += nlen;
      }
    } else {
      buf[len++] = (IS_SET(flag, SHOP_TRADE_WITH(shop_nr)) ? '_' : *trade_letters[sindex]);
      buf[len] = '\0';

      if (len >= sizeof(buf))
        break;
    }

    sindex++;
    flag <<= 1;
  }

  buf[sizeof(buf) - 1] = '\0';
  return (buf);
}


/* END_OF inefficient */
void list_all_shops(struct char_data *ch)
{
  const char *list_all_shops_header =
	" ##   Virtual   Where    Keeper    Buy   Sell   Customers\r\n"
	"---------------------------------------------------------\r\n";
  int shop_nr, headerlen = strlen(list_all_shops_header);
  size_t len = 0;
  char buf[MAX_STRING_LENGTH], buf1[16];

  *buf = '\0';
  for (shop_nr = 0; shop_nr <= top_shop && len < sizeof(buf); shop_nr++) {
    /* New page in page_string() mechanism, print the header again. */
    if (!(shop_nr % (PAGE_LENGTH - 2))) {
      /*
       * If we don't have enough room for the header, or all we have room left
       * for is the header, then don't add it and just quit now.
       */
      if (len + headerlen + 1 >= sizeof(buf))
        break;
      strcpy(buf + len, list_all_shops_header);	/* strcpy: OK (length checked above) */
      len += headerlen;
    }

    if (SHOP_KEEPER(shop_nr) == NOBODY)
      strcpy(buf1, "<NONE>");	/* strcpy: OK (for 'buf1 >= 7') */
    else
      sprintf(buf1, "%6d", mob_index[SHOP_KEEPER(shop_nr)].vnum);	/* sprintf: OK (for 'buf1 >= 11', 32-bit int) */

    len += snprintf(buf + len, sizeof(buf) - len,
               "%3d   %6d   %6d    %s   %3.2f   %3.2f    %s\r\n",
	       shop_nr + 1, shop_index[shop_nr].vnum, 
         shop_index[shop_nr].in_room.empty() ? -1 : shop_index[shop_nr].in_room.front(),
         buf1,
         shop_index[shop_nr].profit_sell, 
         shop_index[shop_nr].profit_buy,
	       customer_string(shop_nr, FALSE));
  }

  page_string(ch->desc, buf, TRUE);
}


void list_detailed_shop(struct char_data *ch, int shop_nr)
{
  struct char_data *k;
  int cnt = 0, column;
  char *ptrsave;

  send_to_char(ch, "Vnum:       [%5d], Rnum: [%5d]\r\n", SHOP_NUM(shop_nr), shop_nr + 1);


  send_to_char(ch, "Rooms:      ");
  column = 12;	/* ^^^ strlen ^^^ */
  for (auto it = shop_index[shop_nr].in_room.begin(); it != shop_index[shop_nr].in_room.end(); ++it) {
    char buf1[128];
    int linelen, temp;

    if (cnt) {
      send_to_char(ch, ", ");
      column += 2;
    }
    ++cnt;

    if ((temp = real_room(*it)) != NOWHERE) {
      linelen = snprintf(buf1, sizeof(buf1), "%s (#%d)", world[temp].name.c_str(), GET_ROOM_VNUM(temp));
    }  else {
      linelen = snprintf(buf1, sizeof(buf1), "<UNKNOWN> (#%d)", *it);
    }

    /* Implementing word-wrapping: assumes screen-size == 80 */
    if (linelen + column >= 78 && column >= 20) {
      send_to_char(ch, "\r\n            ");
      /* 12 is to line up with "Rooms:" printed first, and spaces above. */
      column = 12;
    }

    if (!send_to_char(ch, "%s", buf1))
      return;
    column += linelen;
  }
  if (!cnt)
    send_to_char(ch, "Rooms:      None!");

  send_to_char(ch, "\r\nShopkeeper: ");
  if (SHOP_KEEPER(shop_nr) != NOBODY) {
    send_to_char(ch, "%s (#%d), Special Function: %s\r\n",
	GET_NAME(&mob_proto[SHOP_KEEPER(shop_nr)]),
	mob_index[SHOP_KEEPER(shop_nr)].vnum,
	YESNO(SHOP_FUNC(shop_nr)));

    if ((k = get_char_num(SHOP_KEEPER(shop_nr))))
      send_to_char(ch, "Coins:      [%9d], Bank: [%9d] (Total: %d)\r\n",
	 GET_GOLD(k), SHOP_BANK(shop_nr), GET_GOLD(k) + SHOP_BANK(shop_nr));
  } else
    send_to_char(ch, "<NONE>\r\n");


  send_to_char(ch, "Customers:  %s\r\n", (ptrsave = customer_string(shop_nr, TRUE)) ? ptrsave : "None");


  send_to_char(ch, "Produces:   ");
  column = 12;	/* ^^^ strlen ^^^ */
  cnt = 0;
  for (auto it = shop_index[shop_nr].producing.begin(); it != shop_index[shop_nr].producing.end(); ++it) {
    char buf1[128];
    int linelen;

    if (cnt) {
      send_to_char(ch, ", ");
      column += 2;
    }
    linelen = snprintf(buf1, sizeof(buf1), "%s (#%d)",
		       obj_proto[*it].short_description.c_str(),
		       obj_index[*it].vnum);

    /* Implementing word-wrapping: assumes screen-size == 80 */
    if (linelen + column >= 78 && column >= 20) {
      send_to_char(ch, "\r\n            ");
      /* 12 is to line up with "Produces:" printed first, and spaces above. */
      column = 12;
    }

    if (!send_to_char(ch, "%s", buf1))
      return;
    column += linelen;
  }
  if (!cnt)
    send_to_char(ch, "Produces:   Nothing!");

  send_to_char(ch, "\r\nBuys:       ");
  column = 12;	/* ^^^ strlen ^^^ */
  for (auto it = shop_index[shop_nr].type.begin(); it != shop_index[shop_nr].type.end(); ++it) {
    auto buy_data = *it;
    char buf1[128];
    size_t linelen;

    if (cnt) {
      send_to_char(ch, ", ");
      column += 2;
    }

    linelen = snprintf(buf1, sizeof(buf1), "%s (#%d) [%s]",
		item_types[buy_data.type],
		buy_data.type,
    buy_data.keywords ? buy_data.keywords : "all");

    /* Implementing word-wrapping: assumes screen-size == 80 */
    if (linelen + column >= 78 && column >= 20) {
      send_to_char(ch, "\r\n            ");
      /* 12 is to line up with "Buys:" printed first, and spaces above. */
      column = 12;
    }

    if (!send_to_char(ch, "%s", buf1))
      return;
    column += linelen;
  }
  if (!cnt)
    send_to_char(ch, "Buys:       Nothing!");

  send_to_char(ch, "\r\nBuy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]\r\n",
	SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr), SHOP_OPEN1(shop_nr),
	SHOP_CLOSE1(shop_nr), SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr));


  /* Need a local buffer. */
  {
    char buf1[128];
    sprintbit(SHOP_BITVECTOR(shop_nr), shop_bits, buf1, sizeof(buf1));
    send_to_char(ch, "Bits:       %s\r\n", buf1);
  }
}


void show_shops(struct char_data *ch, char *arg)
{
  int shop_nr;

  if (!*arg)
    list_all_shops(ch);
  else {
    if (!str_cmp(arg, ".")) {
      for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
	if (ok_shop_room(shop_nr, GET_ROOM_VNUM(IN_ROOM(ch))))
	  break;

      if (shop_nr > top_shop) {
	send_to_char(ch, "This isn't a shop!\r\n");
	return;
      }
    } else if (is_number(arg))
      shop_nr = atoi(arg) - 1;
    else
      shop_nr = -1;

    if (shop_nr < 0 || shop_nr > top_shop) {
      send_to_char(ch, "Illegal shop number.\r\n");
      return;
    }
    list_detailed_shop(ch, shop_nr);
  }
}


void destroy_shops(void)
{
  shop_index.clear();
}
