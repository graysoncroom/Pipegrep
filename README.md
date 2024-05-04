# Pipegrep (CS 4348 Programming Project 4)
Team members: William Grayson Croom (wgc180002) and Ashton Smith (ajs190019)

## How to run the code

Example:

```bash
make && ./bin/pipegrep -1 -1 -1 -1 if

```

This searches the current working directory (and subdirectories)
for all files that contain the string "if".

The argument order can be found by looking in the project description pdf or
by running the program with incomplete arguments (the usage string will be
printed in this case).

## Notes To Grader

1. All functionality described in the project description has been implemented and tested.
The results match the output of the normal grep command in our tests.

2. Description of your critical sections

The heart of the project is located in the stages.cpp file. In this file we implement 1 function
for each of the 5 stages.

The Stage 1 and Stage 5 functions each take, as an argument, a reference to a custom data
structure called a *Buffer*.

The *Buffer* data structure contains a container serving as an ordered collection of
strings, a *std::mutex*, a *std::condition_variable*, and a boolean flag indicating that the
producer of items for said buffer will not produce any more items.

The other stages, namely, Stage 1 through 4, all take (among other things) two Buffers each.

Each Stage has one critical region for each buffer argument.

The general structure of each of the critical sections is as follows:

    1. Stage1: Once the current directory is opened, we look through each of the
    entires using the `readdir` C syscall wrapper. Whenever the entry is a regular
    file, we construct a *std::unique_lock<std::mutex>* and pass in buffer1's mutex.
    The standard implementation of *std::unique_lock* ensures that another lock cannot
    be acquired around this mutex until the *std::unique_lock<T>::unlock* method has
    been called. Once the buffer1 mutex has been constructed, we call the wait method on
    buffer1's condition variable with a lambda that ensures the thread does not proceed
    while the maximum buffer size (provided as an argument by the user of the program)
    is equal to the capacity of the underlying storage container.
    
    2. Stage2 through Stage4: The structure of the critical regions for these stages
    is almost identical. We enter a while loop, construct a *std::unique_lock<std::mutex>`*
    around the input buffer's mutex. This input buffer is buffer1 for stage2, buffer2 for stage3,
    and so on. We then, using the condition variable of the input buffer, wait until
    the input buffer is empty or the input buffer's *done* flag has been set by the
    previous stage. After this, we break out of the loop if the input buffer's
    collection is empty and the *done* flag has been set. After this, the next item
    in the collection is moved from the collection to a local variable and we immediately
    unlock the mutex. Once the buffer is unlocked we notify all threads waiting for the input
    buffer so that they can check to see if the right conditions have been satisfied for them
    to acquire the same input buffers mutex. After this, the stage specific logic is performed
    and once the stage is ready to push something into the output buffer, we acquire that
    buffer's mutex, wait (in a similar fashion) for the output buffer to have more
    space available before pushing the item onto the collection. The output buffer
    is then immediately unlocked. Note that once we break out of the loop, the stage
    sets its output buffer's *done* flag to true, which affects the next stages wait
    condition for its input buffer (the same buffer).
    
    3. Stage5: This stage only has 1 buffer (namely, buffer4). The logic is exactly the
    same as stages 2 through 4 with the excepction that there is only a single critical section.
    Why is there only one critical section? This is the stage where we output text
    to standard output instead of pushing into an output buffer. Note that the critical section
    is made slightly longer in this case to keep threads from writing over each other. This
    would allow us to safely print text even in the case where multiple threads are being
    used for stage5 (although we don't do that as we have only 2 team members).

For a more complete understanding of the critical regions, please see ./src/stages.cpp.
The code has been documented via inline comments in such a way that the reader
shouldn't have any issues understanding how the code works.

An informal description of the critical region code is described below:

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
