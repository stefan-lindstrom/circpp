/*
 * zone_future.h
 *
 *  Created on: 4 maj 2022
 *      Author: asmodean
 */

#ifndef __ZONE_FUTURE
#define __ZONE_FUTURE

#include "future_parse.h"
#include "structs.h"
#include "db.h"

class zone_future final : public future_parse<zone_data> {
  using future_parse::future_parse;
private:
  void parse_single_idx(const std::string &idx, std::vector<zone_data> &zones) noexcept final;
public:
  zone_future(bool mini = false) : future_parse(mini, OBJ_PREFIX) {}

  zone_future(const zone_future &f) = delete;
  zone_future(zone_future &&f) = delete;
  ~zone_future() = default;

  const zone_future &operator=(const zone_future &r) = delete;
  const zone_future &operator=(zone_future &&r) = delete;
};




#endif /* INC_ZONE_FUTURE_H_ */
