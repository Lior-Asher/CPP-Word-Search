# Files needed
FILES=Main.cpp Query.cpp Query.h QueryResult.h TextQuery.cpp TextQuery.h

# Executable file name
PROGRAM=a.out

# Compiler to use
CC=g++

# Flags
CFLAGS=-std=c++0x -Wall

$(PROGRAM): $(FILES)
	$(CC) $(CFLAGS) -o $(PROGRAM) Main.cpp Query.cpp TextQuery.cpp
	
# Cleanup: Run: make clean
clean:
	rm $(PROGRAM)