CXXFLAGS+=-O2 -ggdb -DDEBUG
CXXFLAGS+=-Wall -Wextra

all: sudoku_solve

sudoku_solve: main.cpp sudoku_dancing_links.cpp sudoku.cpp
	g++ -std=c++11 -O2 -pthread -o $@ $^ 

clean: 
	rm -rf sudoku_solve
