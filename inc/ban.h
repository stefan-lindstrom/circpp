#ifndef __BAN_H__
#define __BAN_H__

#include <string>
#include <list>

/* don't change these */
#define BAN_NOT 	0
#define BAN_NEW 	1
#define BAN_SELECT	2
#define BAN_ALL		3

// exported types
struct ban_list_element {
   std::string site;
   int	type;
   time_t date;
   std::string name;

   bool operator==(const ban_list_element &other) const {
     return site == other.site;
   }
   bool operator!=(const ban_list_element &other) const {
     return site != other.site;
   }
};

// exported data
extern std::list<ban_list_element> ban_list;

// exported functions
void load_banned(void);
int isbanned(char *hostname);
int Valid_Name(char *newname);
void Read_Invalid_List(void);
void Free_Invalid_List(void);

#endif
