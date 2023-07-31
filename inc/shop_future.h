#ifndef __SHOP__FUTURE
#define __SHOP_FUTURE

#include "future_parse.h"
#include "structs.h"
#include "shop.h"
#include "db.h"

class shop_future final : public future_parse<shop_data> {
  using future_parse::future_parse;
private:
  void parse_single_idx(const std::string &idx, std::vector<shop_data> &shops) noexcept final;
  const std::string handle_shop_message(const std::string &message, room_vnum room, int messagenum) noexcept;
public:
  shop_future(bool mini = false) : future_parse(mini, SHP_PREFIX) {}

  shop_future(const shop_future &f) = delete;
  shop_future(shop_future &&f) = delete;
  ~shop_future() = default;

  const shop_future &operator=(const shop_future &r) = delete;
  const shop_future &operator=(shop_future &&r) = delete;
};

#endif
