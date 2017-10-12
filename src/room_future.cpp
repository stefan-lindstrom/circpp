#include <functional>
#include <fstream>
#include <algorithm>

#include "room_future.h"
#include "db.h"
#include "utils.h"

std::vector<room_data> room_future::rooms() 
{
  std::future<std::vector<room_data>> &future = _worker_future;

  future.wait();
  auto rc = future.get();
  _worker.join();

  return rc;
}

void room_future::parse_single_idx(const std::string &idx, std::vector<room_data> &rooms) noexcept
{
  std::string roomfile = WLD_PREFIX + idx;

  std::ifstream wldidx(roomfile);
  for(std::string line; std::getline(wldidx, line) && line[0] != '$'; ) {
    // postponed for now, room is the (of course) lump of data with dependency towards other data (zone)
  }

  wldidx.close();
}

void room_future::__parse(std::promise<std::vector<room_data>> promise) noexcept
{
  std::vector<room_data> rooms;

  // first parse index file:
  std::string index = std::string(WLD_PREFIX) + (_mini ? MINDEX_FILE : INDEX_FILE);
  std::ifstream idx(index);
  std::vector<std::string> indexes;

  for(std::string line; std::getline(idx, line) && line[0] != '$'; ) {
    indexes.push_back(line);
  }
  idx.close();
  std::for_each(indexes.begin(), indexes.end(), [this,&rooms](std::string idx) { this->parse_single_idx(idx, rooms); }); 

  promise.set_value(rooms);
}

void room_future::parse_rooms() noexcept
{
  using namespace std::placeholders;

  _worker = std::thread(std::bind(std::mem_fn(&room_future::__parse), this, _1), std::move(_room_future));
}
