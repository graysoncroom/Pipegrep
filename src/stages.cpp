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
      std::string entryName{entry->d_name};

      // don't consider the current directory or parent directory
      // links (this will lead to infinite recursion - really bad)
      if (entryName == "." || entryName == "..") {
        continue;
      }

      if (entry->d_type == DT_REG) { // if it is a regular file then...
        std::unique_lock<std::mutex> lock(buff1.mtx); // construct buff1 mutex lock
        // wait until the buffer has at least 1 element's worth of storage
        // capactiy (up to buffsize as given by the user through a command
        // line argument)
        buff1.cv.wait(lock, [&] { return buff1.items.size() < buffsize; });
        // there's room in the buffer so we push an item onto it
        buff1.items.push_back(dirPath + "/" + entryName); // Store full path
        lock.unlock(); // release buff1 mutex
        buff1.cv.notify_one(); // notify consumers that we've added an item for them
      }
      else if (entry->d_type == DT_DIR) { // if the file is of type directory then...
        // construct path to this directory as a string
        std::string subdirectory_path = dirPath + "/" + entryName;
        // call the stage1 function recursively on that directory we found.
        stage1(buff1, buffsize, subdirectory_path);
      } // Note that this marks the completion of the bonus point problem
    }
    closedir(dir); // close file descriptor for current (sub)directory
  }

  buff1.done = true; // change the buff1 done flag to indicate we've produced all items

  // the stages that are waiting on buff1's condition variable  will be notified
  // giving them a chance to check the wait condition which will depend (partially)
  // on the buff1 done status flag
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
    // construct the buff1 input buffer mutex lock
    std::unique_lock<std::mutex> lock(buff1.mtx);

    // wait until buff1 contains new items or buff1 has signaled the end of
    // item production
    buff1.cv.wait(lock, [&] { return !buff1.items.empty() || buff1.done; });
    
    // if buff1 is done, then we are done as well.
    // however, we first need to finish consuming the remaining items from buff1
    // thus we break out of the while loop if buff1 is done and items are all used
    // and otherwise we continue, effectively waiting on buff1's condition variable
    // to let us pass once it produces the last few items and puts them into buff1
    if (buff1.items.empty()) {
      if (buff1.done) {
        break;
      }
      else {
        continue;
      }
    }

    // take an item from buff1
    // by using `std::move` we ensure the string won't be copied
    // Why? Because we immediately remove it from buff1's items, so
    // it will only have one thing pointing to it.
    filename = std::move(buff1.items.back());
    buff1.items.pop_back();

    lock.unlock(); // release mutex from buff1

    // send a notification to buff1's condition variable 
    buff1.cv.notify_one();

    if (stat(filename.c_str(), &fileStat) == 0) { // if call to stat doesn't fail, then...
      // check that we're satisfying all conditions given as command line arguments
      // to the program. In particular, the file should have a uid, gid, and filesize
      // determined by the user's passed arguments.
      // E.g. We don't want to process a file if the uid doesn't match what the user
      // requested.
      // Note: If the user passes -1, this indicates to the program that this field
      // can be ignored.
      if ((filesize == -1 || fileStat.st_size > filesize) &&
               (uid == -1 || fileStat.st_uid == uid)      &&
               (gid == -1 || fileStat.st_gid == gid)) {
        // construct lock for buff2's mutex since we're going to be adding files
        // that passed the above condition check to it
        std::unique_lock<std::mutex> lock2(buff2.mtx);
        // wait until there's room in the buffer buff2
        // (as determined by the buffsize argument given to us by user)
        buff2.cv.wait(lock2, [&] { return buff2.items.size() < buffsize; });
        // add an item  to the buffer buff2
        buff2.items.push_back(filename);
        // release buff2's mutex
        lock2.unlock();
        // notify a consumer that, since we've added an item to it, they should check
        // to see if they are going to process it or not. Since here we know that stage3
        // is the consumer of buff2, we know that this will cause stage3 to start consuming
        // this produced item once it gets scheduled
        buff2.cv.notify_one();
      }
    }
  }
  // we've broken out of the loop, so the same story as described at the end of stage1
  // applies here.
  // we indicate to all consumers of buff2 that we won't be adding any more items to
  // buff2 and send them a notification to check their waiting status
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
    std::unique_lock<std::mutex> lock(buff2.mtx); // construct lock for buff2's mutex
    // same as in stage1/stage2
    buff2.cv.wait(lock, [&] { return !buff2.items.empty() || buff2.done; });

    // if buff2 empty and done, stop processing by breaking out of the loop
    // if buff2 empty is empty but not done, someone else must have beaten us
    // to consuming it so let's continue which effectively starts waiting on
    // buff2's condition variable again
    if (buff2.items.empty()) {
      if (buff2.done) {
        break;
      }
      else {
        continue;
      }
    }

    // non-copy reference memory grab of the latest buff2 item
    filename = std::move(buff2.items.back());
    buff2.items.pop_back(); // remove buff2's reference to that item

    lock.unlock(); // release the buff2 mutex
    buff2.cv.notify_one(); // notify buff2 consumer that they can get their own item now

    file.open(filename); // open the file with filename taken earlier from buff2
    int lineNumber = 0;
    if (file.is_open()) { // make sure it opened correctly
      while (std::getline(file, line)) { // for each line in the file
        std::unique_lock<std::mutex> lock3(buff3.mtx); // construct buff3 mutex lock
        // wait until buff3 has more room
        buff3.cv.wait(lock3, [&] { return buff3.items.size() < buffsize; });
        // add the filename, linenumber, and line itself (contents) to buff3
        buff3.items.push_back(filename + ":" + std::to_string(++lineNumber) + ":" + line); // Prepare formatted line
        lock3.unlock(); // release buff3 mutex
        buff3.cv.notify_one(); // notify next guy that they can use buff3 now
      }
      file.close(); // close the file since we're done using it (no dangling file descriptors!)
    }
  }

  // signal to buff3 consumers that we aren't adding any more stuff to buff3
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
    std::unique_lock<std::mutex> lock(buff3.mtx); // construct buff3 mutex lock
    // wait until buff3 has an item or stage3 indicates it is done producing items
    buff3.cv.wait(lock, [&] { return !buff3.items.empty() || buff3.done; });

    
    // if buff3 empty and done, stop processing by breaking out of the loop
    // if buff3 empty is empty but not done, someone else must have beaten us
    // to consuming it so let's continue which effectively starts waiting on
    // buff3's condition variable again
    if (buff3.items.empty()) {
      if (buff3.done) {
        break;
      }
      else {
        continue;
      }
    }

    // grab an item and avoid string copy (expensive copy mitigated)
    lineData = std::move(buff3.items.back());
    buff3.items.pop_back();

    lock.unlock(); // release buff3 mutex
    buff3.cv.notify_one(); // notify next guy that they can use buff3 if they want

    // starting from the last semicolon (recall that lineData will be in the format
    // filename:linenumber:linecontents, so this really just avoids the situation
    // where the file's name matches the searchString or the searchString is a number
    // which could be conflated with lineNumber)
    std::size_t pos = lineData.find_last_of(':');
    if (pos != std::string::npos && lineData.substr(pos + 1).find(searchString) != std::string::npos) {
      std::unique_lock<std::mutex> lock2(buff4.mtx); // construct buff4 mutex lock
      // wait until there's room in buff4 because we want to add the found searchstring
      buff4.cv.wait(lock2, [&] { return buff4.items.size() < buffsize; });
      // pass lineData along from buff3 to buff4 if we found it and we aren't
      // exceeding buffsize
      buff4.items.push_back(lineData);
      lock2.unlock(); // release buff4 mutex
      buff4.cv.notify_one(); // notify consumer of buff4 that they can consume from buff4 if they want to now
    }
  }

  // same story as before
  // we broke out of the while(true) loop so, if this line of code is reached,
  // we know that buff3 is empty and the producer of items for buff3 has set the
  // condition flag done = true which indicates they are no longer producing new
  // items and storing them into buff3.
  // Hence there won't be any more items to pass along to buff4 if they
  // pass a searchstring match.
  // Thus we indicate to consumers of buff4 that we won't be producing any more items
  // in an identical manner.
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
    std::unique_lock<std::mutex> lock(buff4.mtx); // construct buff4 mutex lock
    // wait until there's a new buff4 item or stage4 says it's done producing into buff4
    buff4.cv.wait(lock, [&] { return !buff4.items.empty() || buff4.done; });

    
    // if buff4 empty and done, stop processing by breaking out of the loop
    // if buff4 empty is empty but not done, someone else must have beaten us
    // to consuming it so let's continue which effectively starts waiting on
    // buff4's condition variable again
    if (buff4.items.empty()) {
      if (buff4.done) {
        break;
      }
      else {
        continue;
      }
    }

    // obtain the lineData item efficiently with a std::move (non-copy)
    // I haven't gone into this in the comments (as I'm assuming the grader
    // understands C++11's move semantics) but basically the reason we like to
    // do things this way is because it will invoke the std::string move
    // constructor rather than its copy constructor. For more info on this
    // there's lots of really great explainations from Cppcon's
    // Back To Basics series on youtube. Highly recommend it.
    std::string lineData = std::move(buff4.items.back());
    buff4.items.pop_back(); // remove from buffer once we have it

    // Find position of first/second colon as these are what indicate where the
    // filename, linenumber, and line contents are
    std::size_t firstColon = lineData.find(':');
    std::size_t secondColon = lineData.find(':', firstColon + 1);
    // extract filename/linenumber using the position of firstColon and secondColon
    std::string filename = lineData.substr(0, firstColon);
    std::string lineNumber = lineData.substr(firstColon + 1, secondColon - firstColon - 1);
    //std::string lineContent = lineData.substr(secondColon + 1);

    // Print the filename and linenumber where the search was found
    std::cout << filename << "(" << lineNumber << ")" << std::endl;
    ++matchCount;

    // note that the unlock happens a little later here to prevent the
    // std::cout output from getting messed up from multiple threads outputting to
    // standard output. In this program this doesn't happen, but it may happen in 
    // future iterations of pipegrep, so we will play it safe for now
    lock.unlock();
    buff4.cv.notify_one();
  }
  // print number of matches found as described in the pdf.
  std::cout << "***** You found " << matchCount << " matches *****" << std::endl;
}
