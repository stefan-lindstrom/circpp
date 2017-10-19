#include <fstream>
#include <algorithm>
#include <sstream>

#include "zone_future.h"
#include "utils.h"

// TODO: proper error handling
void zone_future::parse_single_idx(const std::string &idx, std::vector<zone_data> &zones) noexcept
{
  std::string zonfile = prefix() + idx;
  std::ifstream zonidx(zonfile);

  int linenr = 0;
  std::string line;
  std::getline(zonidx, line);
  linenr++;

  int vnum = -1;
  if (line[0] == '#') { 
    vnum = std::stoi(line.substr(1));
  } // else, _future.set_exception(some_exception), will throw when doing promise.get()
  
  zone_data reading;
  reading.number = vnum;
  line = read(zonidx);
  linenr++;

  line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
  reading.name = line;
 
  zonidx >> reading.bot >> reading.top >> reading.lifespan >> reading.reset_mode;
  linenr++;

  if (reading.bot > reading.top) {
    basic_mud_log("SYSERR: Zone %d bottom (%d) > top (%d).", reading.number, reading.bot, reading.top);
    exit(1);
  }

  while(1) {
    std::getline(zonidx, line);
    linenr++;

    if ("" == line || '*' == line.front()) {
      continue;
    }
    if ('S' == line.front() || '$' == line.front()) {
      zones.push_back(reading);
      break;
    }
    reset_com command;
    command.command = line.front();
    std::stringstream ss(line.substr(2)); // skip cmd + initial space 

    if (std::string::npos == line.find_first_of("MOEPD")) {
      ss >> command.if_flag >> command.arg1 >> command.arg2;
    } else {
      ss >> command.if_flag >> command.arg1 >> command.arg2 >> command.arg3;
    }
    command.line = (linenr-1);
    reading.cmd.push_back(command);
  }
  zonidx.close();
}
