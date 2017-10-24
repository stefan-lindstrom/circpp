#ifndef __ROOM_FUTURE
#define __ROOM_FUTURE 

#include <future>
#include <thread>
#include <vector>

#include "future_parse.h"
#include "structs.h"
#include "db.h"

class room_future final : public future_parse<room_data>
{
  using future_parse::future_parse;
 private:
  void parse_single_idx(const std::string &idx, std::vector<room_data> &zones) noexcept final;
 public:
 room_future(bool mini = false) : future_parse(mini, WLD_PREFIX) {}

  room_future(const room_future &f) = delete;
  room_future(room_future &&f) = delete;
  ~room_future() = default;

  const room_future &operator=(const room_future &r) = delete;
  const room_future &operator=(room_future &&r) = delete;

};

#endif
