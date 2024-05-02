#pragma once

#include "buffer.h"

#include <mutex>        // std::unique_lock, std::mutex
#include <utility>      // std::move
#include <dirent.h>     // readdir, opendir, closedir
#include <cstddef>      // std::size_t
#include <sys/types.h>  // manpage for readdir says to include this as well
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>     // std::cout, std::endl
#include <fstream>
#include <string>

void stage1(Buffer &buff1, int buffsize, const std::string &dirPath);
void stage2(Buffer &buff1, Buffer &buff2, int buffsize, int filesize, int uid, int gid);
void stage3(Buffer &buff2, Buffer &buff3, int buffsize);
void stage4(Buffer &buff3, Buffer &buff4, int buffsize, const std::string &searchString);
void stage5(Buffer &buff4);
