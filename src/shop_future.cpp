#include <fstream>
#include <algorithm>
#include <cstring>

#include "shop_future.h"
#include "utils.h"
#include "constants.h"


std::vector<std::string> split (const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}

void shop_future::parse_single_idx(const std::string &idx, std::vector<shop_data> &shops) noexcept
{
  std::string shopfile = prefix() + idx;

  std::ifstream shopidx(shopfile);
  std::string line;

  basic_mud_log("Parsing shop file %s", shopfile.c_str());

  do {
    line = read(shopidx);

    if (std::strstr(line.c_str(), VERSION3_TAG)) {
      // version, ignore. 
      line = read(shopidx);
    }
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());

    // Ignore tag for new mode, all shops in package is in new mode, instead:
    if (line.front() == '#') {
      int vnum = std::stoi(line.substr(1));
      shop_data reading;
      reading.vnum = vnum;

      // next, list of vnums produced, always -1 terminated
      for (;;) {
        std::getline(shopidx, line);
        int type = std::stoi(line);
        if (type < 0) {
          break;
        }
        int produces = real_object(type);
        if (produces != NOTHING) {
          reading.producing.push_back(produces);
        }
      }
       
      std::getline(shopidx, line);
      reading.profit_buy = std::stof(line);

      std::getline(shopidx, line);
      reading.profit_sell = std::stof(line);

      // list of types, same as previous lists it is terminated with -1
      for (;;) {
         std::getline(shopidx, line);
         if (line == "-1") {
            break;
         }
         shop_buy_data data;
         data.type = NOTHING;
         data.keywords = strdup(line.c_str());
         reading.type.push_back(data);
      }

      std::getline(shopidx, line);
      reading.no_such_item1 = handle_shop_message(line, reading.vnum, 0);
      
      std::getline(shopidx, line);
      reading.no_such_item2 = handle_shop_message(line, reading.vnum, 1);

      std::getline(shopidx, line);
      reading.do_not_buy    = handle_shop_message(line, reading.vnum, 2);

      std::getline(shopidx, line);
      reading.missing_cash1 = handle_shop_message(line, reading.vnum, 3);

      std::getline(shopidx, line);
      reading.missing_cash2 = handle_shop_message(line, reading.vnum, 4);

      std::getline(shopidx, line);
      reading.message_buy   = handle_shop_message(line, reading.vnum, 5);

      std::getline(shopidx, line);
      reading.message_sell  = handle_shop_message(line, reading.vnum, 6);

      std::getline(shopidx, line);
      reading.temper1 = std::stoi(line);
      
      std::getline(shopidx, line);
      reading.bitvector = std::stoi(line);

      std::getline(shopidx, line);
      reading.keeper = real_mobile(std::stoi(line));

      std::getline(shopidx, line);
      reading.with_who = std::stoi(line);
  
      // Another list, room nums, -1 terminated
      for (;;) {
        std::getline(shopidx, line);
        room_vnum room = std::stoi(line);
        if (room < 0) {
          break;
        }
        reading.in_room.push_back(room);
      }

      std::getline(shopidx, line);
      reading.open1 = real_mobile(std::stoi(line));

      std::getline(shopidx, line);
      reading.close1 = real_mobile(std::stoi(line));

      std::getline(shopidx, line);
      reading.open2 = real_mobile(std::stoi(line));

      std::getline(shopidx, line);
      reading.close2 = real_mobile(std::stoi(line));

      reading.bankAccount = 0;
      reading.lastsort = 0;
      reading.func = nullptr;

      shops.push_back(reading);
    }
  } while('$' != line.front());
  shopidx.close();
}

// Hack, just copied from old code. 
const std::string shop_future::handle_shop_message(const std::string &message, room_vnum roomnum, int messagenum) noexcept
{
  int cht, ss = 0, ds = 0, err = 0;
  const char *tbuf = message.c_str();

  for (cht = 0; tbuf[cht]; cht++) {
    if (tbuf[cht] != '%')
      continue;

    if (tbuf[cht + 1] == 's')
      ss++;
    else if (tbuf[cht + 1] == 'd' && (messagenum == 5 || messagenum == 6)) {
      if (ss == 0) {
        basic_mud_log("SYSERR: Shop #%d has %%d before %%s, message #%d.", roomnum, messagenum);
        err++;
      }
      ds++;
    } else if (tbuf[cht + 1] != '%') {
      basic_mud_log("SYSERR: Shop #%d has invalid format '%%%c' in message #%d.", roomnum, tbuf[cht + 1], messagenum);
      err++;
    }
  }

  if (ss > 1 || ds > 1) {
    basic_mud_log("SYSERR: Shop #%d has too many specifiers for message #%d. %%s=%d %%d=%d", roomnum, messagenum, ss, ds);
    err++;
  }

  if (err) {
    return std::string();
  }

  return message;
}
