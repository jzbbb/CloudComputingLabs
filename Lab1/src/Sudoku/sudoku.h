
#include <string>
#include <cstring>
#include <thread>

using std::thread;
using std::string;

#ifndef SUDOKU_H
#define SUDOKU_H

enum {N = 81};

const int Mam_Puzzle = 1024; // 数独题目的最大数量

const int Max_Thread_Num = 100; // 最大线程数

extern int **Buf; // 二维数组，存储数独题目的数字

extern bool *Is_Solved; // 数组，标记每个数独题目是否有解

extern thread threads[Max_Thread_Num]; // 存储线程的数组

extern char puzzle[128]; // 存储读取的一行数独题目的字符数组

extern string file_path;	//文件路径

void init();
void free_buf();
void sudoku_solve(string file);
void distribute(const int sta, const int line_num);
void creat_thread(int step,int curr,int line_count);
void output(int line_count);

void trans(const char in[N], int *target);
bool solve_sudoku_dancing_links(int *);
bool solved();

#endif
