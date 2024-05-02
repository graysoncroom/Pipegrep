#include "stages.h"

/*
 * Stage 1: Filename Acquisition
 * This function recurses through the directory given by `dirPath`
 * and adds all filenames to `buff1`.
 */
void stage1(Buffer &buff1, int buffsize, const std::string &dirPath) {
  DIR *dir;
  struct dirent *entry;

  if ((dir = opendir(dirPath.c_str())) != NULL) {
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_type == DT_REG) { // if it is a regular file then...
        std::unique_lock<std::mutex> lock(buff1.mtx);
        buff1.cv.wait(lock, [&] { return buff1.items.size() < buffsize; });
        buff1.items.push_back(dirPath + "/" + std::string(entry->d_name)); // Store full path
        lock.unlock();
        buff1.cv.notify_one();
      }
    }
    closedir(dir);
  }

  buff1.done = true;
  buff1.cv.notify_all();
}

/*
 * Stage 2: File Filter
 * The function reads filenames from `buff1`, filters out files that satisfy:
 *    (1) the file has a size larger than `filesize`
 *    (2) the file has uid different than `uid`
 *    (3) the file has gid different than `gid`
 * For the each file that make it through this filter, its name is stored in `buff2`.
 */
void stage2(Buffer &buff1, Buffer &buff2, int buffsize, int filesize, int uid, int gid) {
  struct stat fileStat;
  std::string filename;

  while (true) {
    std::unique_lock<std::mutex> lock(buff1.mtx);
    buff1.cv.wait(lock, [&] { return !buff1.items.empty() || buff1.done; });

    if (buff1.items.empty()) {
      if (buff1.done) {
        break;
      }
      else {
        continue;
      }
    }

    filename = std::move(buff1.items.back());
    buff1.items.pop_back();

    lock.unlock();
    buff1.cv.notify_one();

    if (stat(filename.c_str(), &fileStat) == 0) {
      if ((filesize == -1 || fileStat.st_size > filesize) &&
               (uid == -1 || fileStat.st_uid == uid)      &&
               (gid == -1 || fileStat.st_gid == gid)) {
        std::unique_lock<std::mutex> lock2(buff2.mtx);
        buff2.cv.wait(lock2, [&] { return buff2.items.size() < buffsize; });
        buff2.items.push_back(filename);
        lock2.unlock();
        buff2.cv.notify_one();
      }
    }
  }
  buff2.done = true;
  buff2.cv.notify_all();
}

/*
 * Stage 3: Line Generation
 * The function reads each filename from `buff2` and, for each of these filenames,
 * puts each line into `buff3` in the following format:
 *    "<filename>:<lineNumber>:<lineContent>"
 */
void stage3(Buffer &buff2, Buffer &buff3, int buffsize) {
  std::ifstream file;
  std::string line;
  std::string filename;

  while (true) {
    std::unique_lock<std::mutex> lock(buff2.mtx);
    buff2.cv.wait(lock, [&] { return !buff2.items.empty() || buff2.done; });

    if (buff2.items.empty()) {
      if (buff2.done) {
        break;
      }
      else {
        continue;
      }
    }

    filename = std::move(buff2.items.back());
    buff2.items.pop_back();

    lock.unlock();
    buff2.cv.notify_one();

    file.open(filename);
    int lineNumber = 0;
    if (file.is_open()) {
      while (std::getline(file, line)) {
        std::unique_lock<std::mutex> lock3(buff3.mtx);
        buff3.cv.wait(lock3, [&] { return buff3.items.size() < buffsize; });
        buff3.items.push_back(filename + ":" + std::to_string(++lineNumber) + ":" + line); // Prepare formatted line
        lock3.unlock();
        buff3.cv.notify_one();
      }
      file.close();
    }
  }
  buff3.done = true;
  buff3.cv.notify_all();
}

/*
 * Stage 4: Line Filter
 * The thread reads the lines from `buff3` and determines if any contain `searchString`
 * where `buff3` and `searchString` are arguments passed to the function.
 * If any lines containing `searchString` are found, then the information from that
 * line will be added to `buff4`.
 */
void stage4(Buffer &buff3, Buffer &buff4, int buffsize, const std::string &searchString) {
  std::string lineData;

  while (true) {
    std::unique_lock<std::mutex> lock(buff3.mtx);
    buff3.cv.wait(lock, [&] { return !buff3.items.empty() || buff3.done; });

    if (buff3.items.empty()) {
      if (buff3.done) {
        break;
      }
      else {
        continue;
      }
    }

    lineData = std::move(buff3.items.back());
    buff3.items.pop_back();

    lock.unlock();
    buff3.cv.notify_one();

    // check if the current line contains the search string
    std::size_t pos = lineData.find_last_of(':');
    if (pos != std::string::npos && lineData.substr(pos + 1).find(searchString) != std::string::npos) {
      std::unique_lock<std::mutex> lock2(buff4.mtx);
      buff4.cv.wait(lock2, [&] { return buff4.items.size() < buffsize; });
      buff4.items.push_back(lineData);
      lock2.unlock();
      buff4.cv.notify_one();
    }
  }
  buff4.done = true;
  buff4.cv.notify_all();
}

/*
 * Stage 5: Output
 * This function removes lines from `buff4` and prints the filename and linenumber,
 * according to the format specified in the stage 4 description, to standard output
 *
 * Note: We know when to exit based on whether or not our buff4.items is empty and
 * whether or not the buff4.done signal indicates that we are done. This condition
 * is set to true whenever stage4 finishes processing buff3, which figures out it
 * is done by checking if buff2 is done, and so on...
 */
void stage5(Buffer &buff4) {
  int matchCount = 0;

  while (true) {
    std::unique_lock<std::mutex> lock(buff4.mtx);
    buff4.cv.wait(lock, [&] { return !buff4.items.empty() || buff4.done; });

    if (buff4.items.empty()) {
      if (buff4.done) {
        break;
      }
      else {
        continue;
      }
    }

    std::string lineData = std::move(buff4.items.back());
    buff4.items.pop_back();

    std::size_t firstColon = lineData.find(':');
    std::size_t secondColon = lineData.find(':', firstColon + 1);
    std::string filename = lineData.substr(0, firstColon);
    std::string lineNumber = lineData.substr(firstColon + 1, secondColon - firstColon - 1);
    //std::string lineContent = lineData.substr(secondColon + 1);

    std::cout << filename << "(" << lineNumber << ")" << std::endl;
    ++matchCount;

    lock.unlock();
    buff4.cv.notify_one();
  }
  std::cout << "***** You found " << matchCount << " matches *****" << std::endl;
}
