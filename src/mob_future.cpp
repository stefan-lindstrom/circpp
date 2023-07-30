#include <fstream>
#include <algorithm>
#include <map>

#include "mob_future.h"
#include "utils.h"
#include "constants.h"

void mob_future::parse_single_idx(const std::string &idx, std::vector<char_data> &mobs) noexcept
{
  std::string mobfile = prefix() + idx;

  std::ifstream mobidx(mobfile);
  std::string line;
  std::getline(mobidx, line);

  basic_mud_log("Read first line: %s", line.c_str());

  do {
    if ('$' == line[0]) {
      break;
    }

    char_data reading;
    reading.player_specials = &dummy_mob;

    // first line, #<vnum>
    int vnum = -1;
    if (line[0] == '#') { 
       vnum = std::stoi(line.substr(1));
    } // else, _future.set_exception(some_exception), will throw when doing promise.get()
    reading.vnr = vnum; // store, for building index
    reading.nr = mobs.size();

    line = read(mobidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.player.name = line;

    line = read(mobidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.player.short_descr = line;

    line = read(mobidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.player.long_descr = line;

    line = read(mobidx);
    line.erase(std::remove_if(line.begin(), line.end(), [](char c) { return '~' == c;} ), line.end());
    reading.player.description = line;

    // Numeric data
    std::string f1, f2;
    int t;
    char letter;
    mobidx >> f1 >> f2 >> t >> letter;
    std::getline(mobidx, line); // consume rest, including newline

    MOB_FLAGS(&reading) = asciiflag_conv(f1.c_str());
    SET_BIT(MOB_FLAGS(&reading), MOB_ISNPC);

    if (MOB_FLAGGED(&reading, MOB_NOTDEADYET)) {
      /* Rather bad to load mobiles with this bit already set. */
      basic_mud_log("SYSERR: Mob #%d has reserved bit MOB_NOTDEADYET set.", vnum);
      REMOVE_BIT(MOB_FLAGS(&reading), MOB_NOTDEADYET);
    }
    check_bitvector_names(MOB_FLAGS(&reading), action_bits_count, reading.player.description.c_str(), "mobile");

    AFF_FLAGS(&reading) = asciiflag_conv(f2.c_str());
    check_bitvector_names(AFF_FLAGS(&reading), affected_bits_count, reading.player.description.c_str(), "mobile affect");
    GET_ALIGNMENT(&reading) = t;

    if (MOB_FLAGGED(&reading, MOB_AGGRESSIVE) && MOB_FLAGGED(&reading, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL)) {
      basic_mud_log("SYSERR: Mob #%d both Aggressive and Aggressive_to_Alignment.", vnum);
    }

    switch (UPPER(letter)) {
      case 'S': /* Simple monsters */
        parse_simple_mob(mobidx, reading, mobs.size());
        break;
      case 'E': /* Circle3 Enhanced monsters */
        parse_enhanced_mob(mobidx, reading, mobs.size());
        break;
      /* add new mob types here.. */
      default:
        basic_mud_log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, vnum);
        exit(1);
    }

    reading.aff_abils = reading.real_abils;

    for (int j = 0; j < NUM_WEARS; j++) {
      reading.equipment[j] = nullptr;
    } 

    reading.nr = mobs.size();
    reading.desc = nullptr;
    mobs.push_back(reading);

    if ('#' == line.front()) {
      continue; // new object
    } else {
     break;
    }
  } while('$' != line.front());
  mobidx.close();
}

void mob_future::parse_simple_mob(std::ifstream &file, char_data &reading, int rnum) noexcept 
{  
  reading.real_abils.str = 11;
  reading.real_abils.intel = 11;
  reading.real_abils.wis = 11;
  reading.real_abils.dex = 11;
  reading.real_abils.con = 11;
  reading.real_abils.cha = 11;

  int t[10];
  std::string line;
  std::getline(file, line);

  if (sscanf(line.c_str(), " %d %d %d %dd%d+%d %dd%d+%d ", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    basic_mud_log("SYSERR: Format error in mob #%d, first line after S flag\n...expecting line of form '# # # #d#+# #d#+#'", rnum);
    basic_mud_log("  got: %s", line.c_str());
    exit(1);
  }

  GET_LEVEL(&reading) = t[0];
  GET_HITROLL(&reading) = 20 - t[1];
  GET_AC(&reading) = 10 * t[2];

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  GET_MAX_HIT(&reading) = 0;
  GET_HIT(&reading) = t[3];
  GET_MANA(&reading) = t[4];
  GET_MOVE(&reading) = t[5];

  GET_MAX_MANA(&reading) = 10;
  GET_MAX_MOVE(&reading) = 50;

  reading.mob_specials.damnodice = t[6];
  reading.mob_specials.damsizedice = t[7];
  GET_DAMROLL(&reading) = t[8];

  std::getline(file, line);
  if (sscanf(line.c_str(), " %d %d ", t, t + 1) != 2) {
    basic_mud_log("SYSERR: Format error in mob #%d, second line after S flag\n...expecting line of form '# #'", rnum);
    exit(1);
  }

  GET_GOLD(&reading) = t[0];
  GET_EXP(&reading) = t[1];

  std::getline(file, line);

  if (sscanf(line.c_str(), " %d %d %d ", t, t + 1, t + 2) != 3) {
    basic_mud_log("SYSERR: Format error in last line of mob #%d\n...expecting line of form '# # #'", rnum);
    exit(1);
  }

  GET_POS(&reading) = t[0];
  GET_DEFAULT_POS(&reading) = t[1];
  GET_SEX(&reading) = t[2];

  GET_CLASS(&reading) = 0;
  GET_WEIGHT(&reading) = 200;
  GET_HEIGHT(&reading) = 198;

  for (int j = 0; j < 5; j++) {
    GET_SAVE(&reading, j) = 0;
  }
}

void mob_future::parse_enhanced_mob(std::ifstream &file, char_data &reading, int rnum) noexcept 
{
  parse_simple_mob(file, reading, rnum);

  std::string line;
  std::getline(file, line);

  while (!line.empty()) {
    if (line[0] == 'E') {
      return;
    } else if (line[0] == '#') {
      basic_mud_log("SYSERR: Unterminated E section in mob #%d", rnum);
      exit(1);
    } else {
      parse_espec(line, reading, rnum);
    }
    std::getline(file, line);  
  }
  basic_mud_log("SYSERR: Unexpected end of file reached after mob #%d", rnum);
  exit(1);
}

struct value_range {
  int min; 
  int max;
  std::function<void(char_data &, int)> setter;

  value_range(int mn, int mx, std::function<void(char_data &, int)> func) : min(mn), max(mx), setter(func) {    
  }
  int cap(int value) const { return std::max((min), std::min((max), value)); }
};


static const std::map<std::string, value_range> keyword2ranges{
  { "BareHandAttack", value_range(0, 99,  [](char_data &reading, int value) { reading.mob_specials.attack_type = value; })},
  { "Str",            value_range(3, 25,  [](char_data &reading, int value) { reading.real_abils.str = value; })},
  { "StrAdd",         value_range(0, 100, [](char_data &reading, int value) { reading.real_abils.str_add = value; })},
  { "Int",            value_range(3, 25,  [](char_data &reading, int value) { reading.real_abils.intel = value; })},
  { "Wis",            value_range(3, 25,  [](char_data &reading, int value) { reading.real_abils.wis = value; })}, 
  { "Dex",            value_range(3, 25,  [](char_data &reading, int value) { reading.real_abils.dex = value; })},
  { "Con",            value_range(3, 25,  [](char_data &reading, int value) { reading.real_abils.con = value; })},
  { "Cha",            value_range(3, 25,  [](char_data &reading, int value) { reading.real_abils.cha = value; })},
};

void mob_future::parse_espec(std::string &line, char_data &reading, int rnum) noexcept
{  
  std::string::size_type n = line.find(":");
  std::string keyword, value;
  if (n == std::string::npos) {
      keyword = line;
  } else {
      keyword = line.substr(0, n);
      value = line.substr(n); 
      value.erase(std::remove_if(value.begin(), value.end(), [](char c) { return ':' == c || ' ' == c;} ), value.end());
  }
  interpret_espec(reading, rnum, keyword, value);
}

void mob_future::interpret_espec(char_data &reading, int rnum, const std::string &keyword, const std::string &value) noexcept
{
  int num_arg = 0;
  if (!value.empty()) {
    num_arg = std::stoi(value);
  }
  auto found = keyword2ranges.find(keyword);
  if (found == keyword2ranges.end()) {
    basic_mud_log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d", keyword.c_str(), rnum);
    return;
  } 
  num_arg = found->second.cap(num_arg);
  found->second.setter(reading, num_arg);
}