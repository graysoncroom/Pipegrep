#include <iostream>   // std::cerr, std::endl
#include <thread>     // std::thread
#include <functional> // std::ref
#include <string>
#include <cstring>    // atoi

#include "buffer.h"
#include "stages.h"

#define NO_ERR_CODE 0
#define ARGUMENT_ERR_CODE 1

// TODO:
// (1) Recursively search directories in stage1
// (2) Add README.md file and populate with the information specified on page 3/4 of
//     the project description pdf
// (3) Add more comments to the code so that the grader has an easier time understanding
//     what exactly we are doing in each stage. Code is complicated so this will be
//     very important.

int main(int argc, char *argv[])
{
  if (argc != 6) 
  {
    std::cerr << "Usage: " 
              << argv[0] 
              << " <buffsize> <filesize> <uid> <gid> <string>" 
              << std::endl;
    return ARGUMENT_ERR_CODE;
  }

  int buffsize = atoi(argv[1]);
  int filesize = atoi(argv[2]);
  int uid = atoi(argv[3]);
  int gid = atoi(argv[4]);

  std::string searchString = argv[5];

  Buffer buff1, buff2, buff3, buff4;

  // create 1 thread to handle each of the 5 stages
  std::thread t1(stage1, std::ref(buff1), buffsize, ".");
  std::thread t2(stage2, std::ref(buff1), std::ref(buff2), buffsize, filesize, uid, gid);
  std::thread t3(stage3, std::ref(buff2), std::ref(buff3), buffsize);
  std::thread t4(stage4, std::ref(buff3), std::ref(buff4), buffsize, searchString);
  std::thread t5(stage5, std::ref(buff4));

  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();

  return NO_ERR_CODE;
}
