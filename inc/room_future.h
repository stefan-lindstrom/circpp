#ifndef __ROOM_FUTURE
#define __ROOM_FUTURE 

#include <future>
#include <thread>
#include <vector>

#include "structs.h"

class room_future final {
 private:
  std::promise<std::vector<room_data>> _room_future;
  std::future<std::vector<room_data>> _worker_future;

  std::thread _worker;
  
  void __parse_rooms(std::promise<std::vector<room_data>> promise) noexcept;

 public:
  room_future() { _worker_future = _room_future.get_future(); }

  room_future(const room_future &f) = delete;
  room_future(room_future &&f) = delete;
  ~room_future() = default;

  const room_future &operator=(const room_future &r) = delete;
  const room_future &operator=(room_future &&r) = delete;

  std::vector<room_data> rooms();
  void parse_rooms() noexcept;
};

#endif
