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
    // TODO: switch to std::list or other container, get rid of dynamic allocation, and use std::string instead of char*
    reading.ex_description = nullptr;
    // TODO: switch to std::list for affects as well, ffs. 
    for (int j = 0; j < MAX_OBJ_AFFECT; j++) {
      reading.affected[j].location = APPLY_NONE;
      reading.affected[j].modifier = 0;
    }
    // first line, #<vnum>
    int vnum = -1;
    if (line[0] == '#') { 
      vnum = std::stoi(line.substr(1));
      log("Reading object #%d", vnum);
    } // else, _future.set_exception(some_exception), will throw when doing promise.get()
    reading.item_number = objs.size();
    reading.vnum = vnum; // store, for building index

    line = read(objidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.name = strdup(line.c_str());

    line = read(objidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.short_description = strdup(line.c_str());

    line = read(objidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.description = strdup(line.c_str());

    line = read(objidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.action_description = strdup(line.c_str());

    // then, 3 lines of numeric data
    std::getline(objidx, line); // numeric data, line 1
    std::getline(objidx, line); // numeric data, line 2
    std::getline(objidx, line); // numeric data, line 3

    // Then, possible extra descriptions, and affects stuff
    std::getline(objidx, line); 
    while ('#' != line.front() && '$' != line.front()) { 
      // can be multiple exdescs, and/or affects
      if ('E' == line.front())  {
        extra_descr_data *d = new extra_descr_data;

        line = read(objidx);
        line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
        d->keyword = strdup(line.c_str());

        line = read(objidx);
        line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
        d->description = strdup(line.c_str());      

        d->next = reading.ex_description;
        reading.ex_description = d;
      } else if ('A' == line.front()) {
        // next, line of 2 integers
        int first, second;
        objidx >> first >> second;
        log("Affect number %d, read: (%d,%d)", aff, first, second);
        if (aff >= MAX_OBJ_AFFECT) {
          // many errors, handle it. 
        }
        reading.affected[aff].location = first;
        reading.affected[aff].location = second;
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
