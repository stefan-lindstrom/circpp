#ifndef __OBJECT_FUTURE
#define __OBJECT_FUTURE

#include "future_parse.h"
#include "structs.h"
#include "db.h"

class object_future final : public future_parse<obj_data> {
  using future_parse::future_parse;
private:
  void parse_single_idx(const std::string &idx, std::vector<obj_data> &rooms) noexcept final;
public:
  object_future(bool mini = false) : future_parse(mini, OBJ_PREFIX) {}

  object_future(const object_future &f) = delete;
  object_future(object_future &&f) = delete;
  ~object_future() = default;

  const object_future &operator=(const object_future &r) = delete;
  const object_future &operator=(object_future &&r) = delete;
};

#endif
