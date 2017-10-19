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

  std::string line;
  std::getline(zonidx, line);

  int vnum = -1;
  if (line[0] == '#') { 
    vnum = std::stoi(line.substr(1));
  } // else, _future.set_exception(some_exception), will throw when doing promise.get()
  
  zone_data reading;
  reading.number = vnum;
  line = read(zonidx);
  line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
  reading.name = strdup(line.c_str());
 
  zonidx >> reading.bot >> reading.top >> reading.lifespan >> reading.reset_mode;
  if (reading.bot > reading.top) {
    basic_mud_log("SYSERR: Zone %d bottom (%d) > top (%d).", reading.number, reading.bot, reading.top);
    exit(1);
  }

  std::list<reset_com> coms;
  while(1) {
    std::getline(zonidx, line);

    if ("" == line || '*' == line.front()) {
      continue;
    }
    if ('S' == line.front() || '$' == line.front()) {
      zones.push_back(reading);
      reading.cmd = new reset_com[coms.size()];
      int cnt = 0;
      std::for_each(coms.begin(), coms.end(), [&cnt,&reading](reset_com &c) { reading.cmd[cnt++] = c; });
      break;
    }
    reset_com command;
    command.command = line.front();
    std::stringstream ss(line.substr(2)); // skip cmd + initial space 
    if (std::string::npos == line.find("MOEPD")) {
      ss >> command.if_flag >> command.arg1 >> command.arg2;
    } else {
      ss >> command.if_flag >> command.arg1 >> command.arg2 >> command.arg3;
    }
    coms.push_back(command);
  }
  zonidx.close();
}
