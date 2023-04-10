CXXFLAGS+=-O2 -ggdb -DDEBUG
CXXFLAGS+=-Wall -Wextra

all: sudoku_solve

sudoku_solve: main.cc neighbor.cc sudoku_dancing_links.cc
	g++ -O2 -pthread -o $@ $^ 

clean:
	rm sudoku_solve