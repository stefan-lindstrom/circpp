#include <functional>

#include "room_future.h"

std::vector<room_data> room_future::rooms() 
{
  std::future<std::vector<room_data>> &future = _worker_future;

  future.wait();
  auto rc = future.get();
  _worker.join();

  return rc;
}

void room_future::__parse_rooms(std::promise<std::vector<room_data>> promise) noexcept
{
  std::vector<room_data> rooms;

  // actual parsing, just single hard code for now
  room_data r;
  r.number = 42424;
  r.zone = 9999;
  rooms.push_back(r);

  promise.set_value(rooms);
}

void room_future::parse_rooms() noexcept
{
  using namespace std::placeholders;

  _worker = std::thread(std::bind(std::mem_fn(&room_future::__parse_rooms), this, _1), std::move(_room_future));
}
