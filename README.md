CS 4348 Programming Project 4 
worked on by William Grayson Croom wgc180002 and Ashton Smith ajs190019

# Pipegrep

Project 4 for Operating Systems

## How to run the code

Example:

```bash
make && ./bin/pipegrep -1 -1 -1 -1 if

```

This searches the current working directory for all files that contain the string "if".

Another Example:

```bash
make && cd ./src && ../bin/pipegrep -1 -1 -1 -1 if
```

In the first example, you will not get many matches since only the Makefile contains
the string "if". So what we do here is go into the source directory for Pipegrep
before calling the pipegrep executable as to get more matches.

## Notes To Grader

1. All functionality described in the project description has been implemented and tested.
The results match the output of the normal grep command in our tests.

2. Description of your critical sections
Generally, critical sections are protected by mutex objects and unique_lock is used to acquire 
and release them.
We can find  critical sections frequently in cases such as:

-In the aqcuisition of the filenames in stage1() where we use a unique_lock for buff1.mtx 
    whenever a regular file is found and we add it to the buffer before releasing our lock.

-In the processing of filenames in stage2() essentially on the opposite side of the stage1 
    unique_lock buff1.mtx interaction wherin we want to take the item that had previously been 
    placed there and release the lock after we have finished checking the contents.
-In the placement of the filename in stage2(), we place it in buff2 similar to stage1() 
    after a unique_lock buff2.mtx and release after it has been successfully placed.

-Very similar to stage 2, in stage3() we use a unique_lock buff2.mtx in our while loop 
    to take out and process the filenames, which releases the lock when it is done.
-Very similar to stage 2, in stage3() we use a unique lock buff3.mtx in our line generation
    section, where the processed line is added to buff3 and the mutex is released afterwards.

-Very similar to stage 2 and 3, in stage 4() we use the unique_lock buff3.mtx in the while loop
    to grab the contents of the buff3 buffer and release after we have.
-In stage4 we check the lines read in if they contain the searched string, if so then we enter
    a second critical section when we get buff4.mtx where we wait for buff4.cv to be notified, 
    and then add the line to buff4.items and then the lock is released.

-Very similar to stage 2, 3 and 4, in stage5() we use the unique lock buff4.mtx in the while 
    loop to gather what line was passed through from stage 4, and then we release the lock after
    we're done.

3. Buffer size that gave optimal performance for 30 or more files
I ran an experiment where I varied the size of the buffer between each run of pipegrep.
To do this I simply changed the length of the buffer passed as argv[1] and ran it 12 times 
before averaging the collected runtimes. Each of the 31 files varied in "if"s from 1 to hundreds, 
and number of lines per file was anywhere from 200 to 40000 lines of around 20 characters length.
Our buffer lengths tested ran from 1 to 50 with average runtimes like so:
Buffer length 1: Average time = 2945.33 ms
Buffer length 2: Average time = 2882.17 ms
Buffer length 5: Average time = 2876 ms
Buffer length 20: Average time = 3007 ms
Buffer length 50: Average time = 3013.58 ms
For us to determine the optimal buffer size we need to take into consideration the memory 
overhead possibly imposed on the threads by an excessively long buffer, and that a shorter 
buffer might not transfer enough per go for us to benefit with the management of lock and such 
for each buffer transferred. This is why the data tends to lean towards somewhere above 1 and 
under 20 being optimal buffer length for the program. From what was tested, 5 seems to be effective.

4. Which stage you would add an additional thread to improve performance
In stage 2 we could benefit from an extra thread, as the filtering work between the files that do 
and dont meet the critera could be pretty intensive with enough files. With a second thread we could
split the task via parallel processing and potentially faster execution time of the whole process.
The other stages simply wouldnt benefit nearly as much as this specific one either.

5. There are no known bugs in the code after extensive testing on local machines and cs2.

6. We implemented recursive subdirectory file searching for the addition 20% bonus point credit.
This can be found in the "else if" statement of the stage3 function definition in the stages.cpp
file.
