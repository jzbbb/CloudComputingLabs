#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#include "sudoku.h"

using namespace std;

const int lim_puzzle = 1024; // 数独题目的最大数量

const int max_thread_num = 100; // 最大线程数

int thread_num = 0; // 线程数

int **buf; // 二维数组，存储数独题目的数字

bool *is_solved; // 数组，标记每个数独题目是否有解

string infile_buffer; // 存储输入文件路径的字符串

thread ths[max_thread_num]; // 存储线程的数组

char puzzle[128]; // 存储读取的一行数独题目的字符数组

void init_before_all()
{
    // ios::sync_with_stdio(false);
    buf = new int* [lim_puzzle]; // 动态分配二维数组的第一维
    is_solved = new bool [lim_puzzle]; // 动态分配标记数组
    for(int i = 0; i < lim_puzzle; ++i) buf[i] = new int [N]; // 动态分配二维数组的第二维
    thread_num = thread::hardware_concurrency(); // 获取硬件线程数
}

void init_before_one_round()
{
    for(int i = 0; i < lim_puzzle; i++)
    {
        is_solved[i] = false; // 初始化标记数组
    }
}

void thread_work(const int sta, const int line_num)
{
    for(int i = 0; i < line_num; ++i)
    {
        is_solved[sta+i] = solve_sudoku_dancing_links(buf[sta+i]); // 使用多线程求解数独题目
    }
}


void one_file_work(string file_path)
{
    ifstream input_file(file_path, ios::in); // 打开输入文件
    if(!input_file.is_open())
    {
        // printf("Filed to open file: %s\n", file_path.c_str());
        return;
    }
    init_before_one_round(); 
    bool end_of_file = false;
    while(!end_of_file)
	{
	    // line_count 统计读入的数独谜题数量，初始化为 0
	    int line_count = 0;

	    // 循环读入数独谜题，直到读完文件或者读入了 lim_puzzle-1 个数独谜题
	    do{
		// 如果已经读到了文件末尾，置 end_of_file 为 true，退出循环
		if(input_file.eof())
		{
		    end_of_file = true;
		    break;
		}

		// 读入一行数独谜题，每次读取 N+1 个字符（包括换行符），存储到 puzzle 数组中
		input_file.getline(puzzle, N+1);

		// 如果读入的字符数量大于等于 N，则将 puzzle 数组中的数独谜题转换为一维数组并存储到 buf 数组中，同时 line_count 加 1
		if (strlen(puzzle) >= N)
		{
		    trans(puzzle, buf[line_count]);
		    line_count++;
		}
	    } while(line_count < lim_puzzle-1);

	    // 将处理过的数独谜题分配给多个线程进行求解，并在所有线程运行完毕后统计结果输出
	    int step = (line_count+thread_num-1)/thread_num, curr = 0;
	    for(int i = 0; i < thread_num; ++i,curr+=step)
	    {
		// 为第 i 个线程分配任务，sta 为起始位置，line_num 为任务长度
		ths[i] = thread(thread_work, curr, ((curr+step>=line_count)?(line_count-curr):step));
	    }
	    // 等待所有线程结束
	    for(int i = 0; i < thread_num; ++i) ths[i].join();

	    // 输出每个数独谜题的结果
	    for(int i = 0; i < line_count; ++i)
	    {
		if(is_solved[i])
		{
		    // 如果数独谜题有解，则输出解
		    for(int j = 0; j < N; j++) putchar('0'+buf[i][j]);
		    putchar('\n');
		}
		else
		{
		    // 如果数独谜题无解，则输出 "No result."
		    puts("No result.");
		}
	    }
	}
	// 关闭文件
	input_file.close();
}

int main()
{
    // Initialize variables and memory
    init_before_all();
    string file_path;
    // Read file paths from standard input
    while(getline(cin, file_path))
    {
        // Process one file at a time
        one_file_work(file_path);
    }
    // Free memory
    for(int i = 0; i < lim_puzzle; ++i) delete [] buf[i];
    delete [] buf;
    // Return success
    return 0;
}





