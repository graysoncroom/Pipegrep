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
