#include <fstream>
#include <algorithm>

#include "object_future.h"
#include "utils.h"

// prettifying in the future, more a POC atm...
// TODO: needs error handling, badly. 
void object_future::parse_single_idx(const std::string &idx, std::vector<obj_data> &objs) noexcept
{
  std::string objfile = prefix() + idx;

  std::ifstream objidx(objfile);
  std::string line;
  std::getline(objidx, line);

  int aff;
  do {
    if ('$' == line[0]) {
      break;
    }

    aff = 0;
    obj_data reading;
    reading.clear();

    // TODO: switch to std::list for affects as well, ffs. 
    for (int j = 0; j < MAX_OBJ_AFFECT; j++) {
      reading.affected[j].location = APPLY_NONE;
      reading.affected[j].modifier = 0;
    }
    // first line, #<vnum>
    int vnum = -1;
    if (line[0] == '#') { 
      vnum = std::stoi(line.substr(1));
    } // else, _future.set_exception(some_exception), will throw when doing promise.get()
    reading.item_number = objs.size();
    reading.vnum = vnum; // store, for building index

    line = read(objidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.name = line; 

    line = read(objidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.short_description = line;

    line = read(objidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.description = line;

    line = read(objidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.action_description = line;

    // then, 3 lines of numeric data
    // TODO: should really move from macros to members, with sanity checking. 
    int v1, v2, v3, v4;
    std::string s1, s2;
    objidx >> v1 >> s1 >> s2;
    GET_OBJ_TYPE(&reading) = v1;
    GET_OBJ_EXTRA(&reading) = asciiflag_conv(s1.c_str());
    GET_OBJ_WEAR(&reading) = asciiflag_conv(s2.c_str());

    objidx >> v1 >> v2 >> v3 >> v4;
    GET_OBJ_VAL(&reading, 0) = v1;
    GET_OBJ_VAL(&reading, 1) = v2;
    GET_OBJ_VAL(&reading, 2) = v3;
    GET_OBJ_VAL(&reading, 3) = v4;

    objidx >> v1 >> v2 >> v3;
    GET_OBJ_WEIGHT(&reading) = v1;
    GET_OBJ_COST(&reading) = v2;
    GET_OBJ_RENT(&reading) = v3;

    /* check to make sure that weight of containers exceeds curr. quantity */
    if (GET_OBJ_TYPE(&reading) == ITEM_DRINKCON || GET_OBJ_TYPE(&reading) == ITEM_FOUNTAIN) {
      if (GET_OBJ_WEIGHT(&reading) < GET_OBJ_VAL(&reading, 1))
	GET_OBJ_WEIGHT(&reading) = GET_OBJ_VAL(&reading, 1) + 5;
    }

    // Then, possible extra descriptions, and affects stuff
    std::getline(objidx, line); 
    while ('#' != line.front() && '$' != line.front()) { 
      // can be multiple exdescs, and/or affects
      if ('E' == line.front())  {
        extra_descr_data d;

        line = read(objidx);
        line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
        d.keyword = strdup(line.c_str());

        line = read(objidx);
        line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
        d.description = strdup(line.c_str());      

        reading.ex_description.push_back(d);

      } else if ('A' == line.front()) {
        // next, line of 2 integers
        int first, second;
        objidx >> first >> second;

        if (aff >= MAX_OBJ_AFFECT) {
          // many errors, handle it. 
        }
        reading.affected[aff].location = first;
        reading.affected[aff].modifier = second;
        aff++;
      } else {
        // unexpected, error
      }
      std::getline(objidx, line);
    } // while

    // save object...
    objs.push_back(reading);

    if ('#' == line.front()) {
      continue; // new object
    } else {
      break;
    }
  } while('$' != line.front());
  objidx.close();
}
