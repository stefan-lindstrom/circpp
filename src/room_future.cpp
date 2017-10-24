#include <functional>
#include <fstream>
#include <algorithm>

#include "room_future.h"
#include "db.h"
#include "utils.h"


void room_future::parse_single_idx(const std::string &idx, std::vector<room_data> &rooms) noexcept
{
  std::string roomfile = WLD_PREFIX + idx;

  std::ifstream wldidx(roomfile);
  for(std::string line; std::getline(wldidx, line) && line[0] != '$'; ) {
    // postponed for now, room is the (of course) lump of data with dependency towards other data (zone)
  }

  wldidx.close();
}

