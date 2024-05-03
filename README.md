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

2. TODO: (a description of your critical sections)
We need to list all the critical sections here.
All critical sections can be found in the stages.cpp file.
We need to explain where they are, how the locks work, etc...

3. TODO: (the buffer size that gave optimal performance for 30 or more files)
In order to do this one, you'll need to run an experiment and talk about how you
did it. It mentions on page 4 that this will make up a large chunk of our grade.
Talk about the buffer size that gave optimal performance for 30 or more files.
In order to talk about this we will need to test the program on 30 (or more) files with text,
time it with different buffer sizes, and figure out which ran the fastest.

4. TODO: (tell me in which stage you would add an additional thread to improve performance
and why you chose that stage)

5. There are no known bugs in the code after extensive testing on local machines and cs2.

6.  We implemented recursive subdirectory file searching for the addition 20% bonus point credit.
This can be found in the "else if" statement of the stage3 function definition in the stages.cpp
file.
