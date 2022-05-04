/* ************************************************************************
*  file: listrent.c                                  Part of CircleMUD *
*  Usage: list player rent files                                          *
*  Written by Jeremy Elson                                                *
*  All Rights Reserved                                                    *
*  Copyright (C) 1993 The Trustees of The Johns Hopkins University        *
************************************************************************* */

#include <string>

#include "conf.h"
#include "sysdep.h"

#include "structs.h"

void Crash_listrent(char *fname);

int main(int argc, char **argv)
{
  int x;

  for (x = 1; x < argc; x++)
    Crash_listrent(argv[x]);

  return (0);
}


void Crash_listrent(char *fname)
{
  FILE *fl;
//  char buf[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct rent_info rent;
  std::string buf;

  if (!(fl = fopen(fname, "rb"))) {
    printf("%s has no rent file.\r\n", fname);
    return;
  }

  (buf = fname).append("\r\n");

  if (!feof(fl))
    fread(&rent, sizeof(struct rent_info), 1, fl);
  switch (rent.rentcode) {
  case RENT_RENTED:
	buf.append("Rent\r\n");
    break;
  case RENT_CRASH:
	buf.append("Crash\r\n");
    break;
  case RENT_CRYO:
	  buf.append("Cryo\r\n");
    break;
  case RENT_TIMEDOUT:
  case RENT_FORCED:
	buf.append("TimedOut\r\n");
    break;
  default:
	buf.append("Undef\r\n");
    break;
  }
  while (!feof(fl)) {
    fread(&object, sizeof(struct obj_file_elem), 1, fl);
    if (ferror(fl)) {
      fclose(fl);
      return;
    }
    if (!feof(fl)) {
      buf.append(std::to_string(object.item_number)).append(fname).append("\n");
    }
  }
  printf(buf.c_str());
  fclose(fl);
}
