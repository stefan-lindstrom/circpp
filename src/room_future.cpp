#include <functional>
#include <fstream>
#include <algorithm>

#include "room_future.h"
#include "db.h"
#include "utils.h"
#include "constants.h"

void room_future::parse_single_idx(const std::string &idx, std::vector<room_data> &rooms) noexcept
{
  std::string roomfile = WLD_PREFIX + idx;
  int zone = 0; 
  char flags[128];

  std::ifstream wldidx(roomfile);
  std::string line;
  std::getline(wldidx, line);

  do {
    if (line.front() == '$')
      break;

    room_data reading;
    for (int i = 0; i < NUM_OF_DIRS; ++i) {
      reading.dir_option[i] = std::make_tuple(room_direction_data(), false);
    }

    reading.func = nullptr;
    reading.contents = nullptr;
    reading.people = nullptr;
    reading.light = 0;	/* Zero light sources */

    // first line, #<vnum>
    int vnum = -1;
    if (line[0] == '#') { 
      vnum = std::stoi(line.substr(1));
    } // else, _future.set_exception(some_exception), will throw when doing promise.get()

    auto it = std::find_if(zone_table.begin(), zone_table.end(), [&vnum](const zone_data &z) {
      return (vnum >= z.bot) && (vnum <= z.top); 
    });

    if (zone_table.end() == it) {
      basic_mud_log("SYSERR: Room %d is outside of any zone.", vnum);
      exit(1);
    }
    zone = std::distance(zone_table.begin(), it);

    reading.number = vnum;
    reading.zone = zone;

    line = read(wldidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.name = line; 
    
    line = read(wldidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.description = line; 

    std::string flag;
    int dummy;
    wldidx >> dummy >> flag >> reading.sector_type;
    std::getline(wldidx, line); // just consume whatever remains of this line

    reading.room_flags = asciiflag_conv(flag.c_str());
    sprintf(flags, "object #%d", reading.number);  /* sprintf: OK (until 399-bit integers) */
    check_bitvector_names(reading.room_flags, room_bits_count, flags, "room");

    bool done = false;
    int dir = 0;
    int t[5];
    extra_descr_data d;

    while (!done) {
      std::getline(wldidx, line);

      switch(line.front()) {
      case 'D':
	dir = std::stoi(line.substr(1));

	std::get<1>(reading.dir_option[dir]) = true;

	line = read(wldidx);
	line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
	std::get<0>(reading.dir_option[dir]).general_description = line;

	line = read(wldidx);
	line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
	std::get<0>(reading.dir_option[dir]).keyword = line;

	wldidx >> t[0] >> t[1] >> t[2];
	std::getline(wldidx, line);

	if (t[0] == 1)
	  std::get<0>(reading.dir_option[dir]).exit_info = EX_ISDOOR;
	else if (t[0] == 2)
	  std::get<0>(reading.dir_option[dir]).exit_info = EX_ISDOOR | EX_PICKPROOF;
	else
	  std::get<0>(reading.dir_option[dir]).exit_info = 0;

	std::get<0>(reading.dir_option[dir]).key = t[1];
	std::get<0>(reading.dir_option[dir]).to_room = t[2];

	break;
      case 'E':
	line = read(wldidx);
	line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
	d.keyword = line;

	line = read(wldidx);
	line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
	d.description = line;

	reading.ex_description.push_back(d);
	break;
      case 'S':
	rooms.push_back(reading);
	done = true;
	break;
      default:
	basic_mud_log("%s", line.c_str());
	exit(1);
	break;
      }
    }
    std::getline(wldidx, line);// either "$" == done with file, or new room
  } while(true);

  ++zone;
  wldidx.close();
}

