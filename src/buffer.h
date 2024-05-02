#pragma once

#include <vector>
#include <string>
#include <condition_variable>

struct Buffer 
{
  std::vector<std::string> items;
  std::mutex mtx;
  std::condition_variable cv;
  bool done = false;  // so we can signal completion
};
