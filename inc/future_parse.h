#ifndef __FUTURE_PARSE
#define __FUTURE_PARSE

#include <vector>
#include <future>
#include <thread>
#include <fstream>
#include <algorithm>
#include <functional>

#include "db.h"

template<class T>
class future_parse {
  std::vector<T> _data;

  std::promise<std::vector<T>> _future;
  std::future<std::vector<T>> _worker_future;

  std::thread _worker;
  bool _mini;
  std::string _prefix;
  
  void __parse(std::promise<std::vector<T>> promise) noexcept
  {
    std::vector<T> items;

    // first parse index file:
    std::string index = _prefix + (_mini ? MINDEX_FILE : INDEX_FILE);
    std::ifstream idx(index);
    std::vector<std::string> indexes;

    for(std::string line; std::getline(idx, line) && line[0] != '$'; ) {
      indexes.push_back(line);
    }
    idx.close();

    std::for_each(indexes.begin(), indexes.end(), [this,&items](std::string &s) { this->parse_single_idx(s, items); });

    promise.set_value(items);
  }

  virtual void parse_single_idx(const std::string &idx, std::vector<T> &items) noexcept = 0;

protected:
  const std::string read(std::ifstream &f) const 
  {
    std::string rc, line;  
    for(; std::getline(f, line); ) {
      rc += line;
      if (line.back() != '~') {
        rc += "\r\n";
      } else {
        break;
      }
    }
    return rc;
  }

 public:
  future_parse(bool mini = false, const std::string &prefix = "") : _mini(mini), _prefix(prefix)
  { 
    _worker_future = _future.get_future(); 
  }

  future_parse(const future_parse &f) = delete;
  future_parse(future_parse &&f) = delete;
  virtual ~future_parse() = default;

  const future_parse &operator=(const future_parse &r) = delete;
  const future_parse &operator=(future_parse &&r) = delete;

  const std::string &prefix() const { return _prefix; }

  std::vector<T> items() 
  {
    std::future<std::vector<T>> &future = _worker_future;

    future.wait();
    auto rc = future.get();
    _worker.join();

    return rc;
  }

  void parse() noexcept 
  {

    using namespace std::placeholders;

    _worker = std::thread(std::bind(std::mem_fn(&future_parse::__parse), this, _1), std::move(_future));
  }
};

#endif
