// #include <assert.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <string.h>
#include <thread>
// #include <pthread.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

#include "sudoku.h"
using namespace std;

int Thread_Num = 0; // 线程数

int **Buf; // 二维数组，存储数独题目的数字

bool *Is_Solved; // 数组，标记每个数独题目是否有解

thread threads[Max_Thread_Num]; // 存储线程的数组

char puzzle[128]; // 存储读取的一行数独题目的字符数组

void init()
{
    // ios::sync_with_stdio(false);
    // 动态分配二维数组的第一维
    Buf = new int *[Mam_Puzzle];
    // 动态分配标记数组
    Is_Solved = new bool[Mam_Puzzle];
    // 动态分配二维数组的第二维
    for (int i = 0; i < Mam_Puzzle; ++i)
    {
        Buf[i] = new int[N];
    }
    // 获取硬件线程数
    Thread_Num = thread::hardware_concurrency();
}

void distribute(const int sta, const int line_num)
{
    for (int i = 0; i < line_num; ++i)
    {
        Is_Solved[sta + i] = solve_sudoku_dancing_links(Buf[sta + i]); // 使用多线程求解数独题目
    }
}

void creat_thread(int step, int curr, int line_count)
{
    // 将处理过的数独谜题分配给多个线程进行求解，并在所有线程运行完毕后统计结果输出

    for (int i = 0; i < Thread_Num; ++i, curr += step)
    {
        // 为第 i 个线程分配任务，sta 为起始位置，line_num 为任务长度
        threads[i] = thread(distribute, curr, ((curr + step >= line_count) ? (line_count - curr) : step));
    }
    // 等待所有线程结束
    for (int i = 0; i < Thread_Num; ++i)
    {
        threads[i].join();
    }
}
void output(int line_count)
{
    // 输出每个数独谜题的结果
    for (int i = 0; i < line_count; ++i)
    {
        if (Is_Solved[i])
        {
            // 如果数独谜题有解，则输出解
            for (int j = 0; j < N; j++)
            {
                putchar('0' + Buf[i][j]); // 使用putchar输出单个字符速度更快
            }
            cout << endl;
            // 刷新输出缓冲区
            fflush(stdout);
        }
        else
        {
            // 如果数独谜题无解，则输出 "No result"
            cout << "No result" << endl;
        }
    }
}
void sudoku_solve(string file)
{
    ifstream input_file(file, ios::in); // 打开输入文件
    if (!input_file.is_open())
    {
        printf("Filed to open file: %s\n", file.c_str());
        return;
    }
    for (int i = 0; i < Mam_Puzzle; i++)
    {
        Is_Solved[i] = false; // 初始化标记数组
    }
    bool end_of_file = false;
    while (!end_of_file)
    {
        // line_count 统计读入的数独谜题数量，初始化为 0
        int line_count = 0;

        // 循环读入数独谜题，直到读完文件或者读入了 Mam_Puzzle-1 个数独谜题
        do
        {
            // 如果已经读到了文件末尾，置 end_of_file 为 true，退出循环
            if (input_file.eof())
            {
                end_of_file = true;
                break;
            }

            // 读入一行数独谜题，每次读取 N+1 个字符（包括换行符），存储到 puzzle 数组中
            input_file.getline(puzzle, N + 1);

            // 如果读入的字符数量大于等于 N，则将 puzzle 数组中的数独谜题转换为一维数组并存储到 Buf 数组中，同时 line_count 加 1
            if (strlen(puzzle) >= N)
            {
                trans(puzzle, Buf[line_count]);
                line_count++;
            }
        } while (line_count < Mam_Puzzle - 1);
        int step = (line_count + Thread_Num - 1) / Thread_Num, curr = 0;
        creat_thread(step, curr, line_count);

        output(line_count);
    }
    // 关闭文件
    input_file.close();
}

void free_buf()//释放空间
{
    for (int i = 0; i < Mam_Puzzle; ++i)
        delete[] Buf[i];
    delete[] Buf;
}