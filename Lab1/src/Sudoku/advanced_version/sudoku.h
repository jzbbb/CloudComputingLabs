#ifndef SUDOKU_H
#define SUDOKU_H

const int kMaxNodes = 1 + 81*4 + 9*9*9*4;
const int kMaxColumns = 400;
const int kRow = 100, kCol = 200, kBox = 300, N = 81;
void trans(const char in[N], int* target);
bool solve_sudoku_dancing_links(int*);
bool solved();

#endif
