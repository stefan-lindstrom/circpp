#ifndef __MOB_FUTURE
#define __MOB_FUTURE 

#include <future>
#include <thread>
#include <vector>

#include "future_parse.h"
#include "structs.h"
#include "db.h"

class mob_future final : public future_parse<char_data>
{
  using future_parse::future_parse;
 private:
  void parse_single_idx(const std::string &idx, std::vector<char_data> &zones) noexcept final;

  void parse_simple_mob(std::ifstream &file, char_data &reading, int rnum) noexcept;
  void parse_enhanced_mob(std::ifstream &file, char_data &reading, int rnum) noexcept;
  void parse_espec(std::string &line, char_data &reading, int rnum) noexcept;
  void interpret_espec(char_data &reading, int rnum, const std::string &keyword, const std::string &value) noexcept;
 public:
  mob_future(bool mini = false) : future_parse(mini, MOB_PREFIX) {}

  mob_future(const mob_future &f) = delete;
  mob_future(mob_future &&f) = delete;
  ~mob_future() = default;

  const mob_future &operator=(const mob_future &r) = delete;
  const mob_future &operator=(mob_future &&r) = delete;
};

#endif
