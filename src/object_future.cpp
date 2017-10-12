#include <fstream>
#include <algorithm>

#include "object_future.h"
#include "utils.h"

void object_future::parse_single_idx(const std::string &idx, std::vector<obj_data> &rooms) noexcept
{
  std::string objfile = prefix() + idx;

  std::ifstream objidx(objfile);
  for(std::string line; std::getline(objidx, line) && line[0] != '$'; ) {
    // TODO: implement
  }

  objidx.close();
}
