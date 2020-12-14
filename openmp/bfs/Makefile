# C compiler
CC = clang++

CC_FLAGS = -g -Wall -fopenmp -O2

all: bfs bfs_offload

bfs: bfs.cpp graphgen.cpp
	$(CC) $(CC_FLAGS) $^ -o bfs 

bfs_offload: bfs.cpp graphgen.cpp
	$(CC) $(CC_FLAGS) -fopenmp-targets=nvptx64 $^  -o bfs_offload

clean:
	rm -f bfs bfs_offload result*.txt
