GCC = g++

all: test 

test: test.cc queues.h
	${GCC} -O0 -g -std=c++0x -pthread test.cc -o test

clean:
	rm test 
