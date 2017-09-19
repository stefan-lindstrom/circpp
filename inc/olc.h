#ifndef __OLC_H__
#define __OLC_H__

enum class OlcMode : unsigned short {
  OLC_SET = 0,
  OLC_SHOW = 1,
  OLC_REPEAT = 2,
  OLC_ROOM = 3,
  OLC_MOB = 4,
  OLC_OBJ = 5,
};

enum class OlcCmd : unsigned short {
  OLC_COPY = 0,
  OLC_NAME = 1,
  OLC_DESC = 2,
  OLC_ALIASES = 3,
};


#define MAX_ROOM_NAME	75
#define MAX_MOB_NAME	50
#define MAX_OBJ_NAME	50
#define MAX_ROOM_DESC	1024
#define MAX_MOB_DESC	512
#define MAX_OBJ_DESC	512

#endif
