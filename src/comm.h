/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef __COMM_H__
#define __COMM_H__

#include <type_traits>
#include "bitmask.h"

#define NUM_RESERVED_DESCS	8

/* comm.c */
size_t	send_to_char(struct char_data *ch, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
void	send_to_all(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
void	send_to_room(room_rnum room, const char *messg, ...) __attribute__ ((format (printf, 2, 3)));
void	send_to_outdoor(const char *messg, ...) __attribute__ ((format (printf, 1, 2)));
void	close_socket(struct descriptor_data *d);

enum class CommTarget : unsigned char {
  TO_ROOM    = (1 << 0),
  TO_VICT    = (1 << 1),
  TO_NOTVICT = (1 << 2),
  TO_CHAR    = (1 << 3),
  TO_SLEEP   = (1 << 7)
};
ENABLE_BITMASK_OPERATORS(CommTarget);

void	perform_act(const char *orig, struct char_data *ch, struct obj_data *obj, const void *vict_obj, const struct char_data *to);
void	act(const char *str, int hide_invisible, struct char_data *ch, struct obj_data *obj, const void *vict_obj, CommTarget type);


/* I/O functions */
void	write_to_q(const char *txt, struct txt_q *queue, int aliased);
int	write_to_descriptor(socket_t desc, const char *txt);
size_t	write_to_output(struct descriptor_data *d, const char *txt, ...) __attribute__ ((format (printf, 2, 3)));
size_t	vwrite_to_output(struct descriptor_data *d, const char *format, va_list args);
void	string_add(struct descriptor_data *d, char *str);
void	string_write(struct descriptor_data *d, char **txt, size_t len, long mailto, void *data);

#define PAGE_LENGTH	22
#define PAGE_WIDTH	80
void	page_string(struct descriptor_data *d, char *str, int keep_internal);

typedef RETSIGTYPE sigfunc(int);

#endif
