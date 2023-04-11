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

int Thread_Num = 0; // �߳���

int **Buf; // ��ά���飬�洢������Ŀ������

bool *Is_Solved; // ���飬���ÿ��������Ŀ�Ƿ��н�

thread threads[Max_Thread_Num]; // �洢�̵߳�����

char puzzle[128]; // �洢��ȡ��һ��������Ŀ���ַ�����

void init()
{
    // ios::sync_with_stdio(false);
    // ��̬�����ά����ĵ�һά
    Buf = new int *[Mam_Puzzle];
    // ��̬����������
    Is_Solved = new bool[Mam_Puzzle];
    // ��̬�����ά����ĵڶ�ά
    for (int i = 0; i < Mam_Puzzle; ++i)
    {
        Buf[i] = new int[N];
    }
    // ��ȡӲ���߳���
    Thread_Num = thread::hardware_concurrency();
}

void distribute(const int sta, const int line_num)
{
    for (int i = 0; i < line_num; ++i)
    {
        Is_Solved[sta + i] = solve_sudoku_dancing_links(Buf[sta + i]); // ʹ�ö��߳����������Ŀ
    }
}

void creat_thread(int step, int curr, int line_count)
{
    // �������������������������߳̽�����⣬���������߳�������Ϻ�ͳ�ƽ�����

    for (int i = 0; i < Thread_Num; ++i, curr += step)
    {
        // Ϊ�� i ���̷߳�������sta Ϊ��ʼλ�ã�line_num Ϊ���񳤶�
        threads[i] = thread(distribute, curr, ((curr + step >= line_count) ? (line_count - curr) : step));
    }
    // �ȴ������߳̽���
    for (int i = 0; i < Thread_Num; ++i)
    {
        threads[i].join();
    }
}
void output(int line_count)
{
    // ���ÿ����������Ľ��
    for (int i = 0; i < line_count; ++i)
    {
        if (Is_Solved[i])
        {
            // ������������н⣬�������
            for (int j = 0; j < N; j++)
            {
                putchar('0' + Buf[i][j]); // ʹ��putchar��������ַ��ٶȸ���
            }
            cout << endl;
            // ˢ�����������
            fflush(stdout);
        }
        else
        {
            // ������������޽⣬����� "No result"
            cout << "No result" << endl;
        }
    }
}
void sudoku_solve(string file)
{
    ifstream input_file(file, ios::in); // �������ļ�
    if (!input_file.is_open())
    {
        printf("Filed to open file: %s\n", file.c_str());
        return;
    }
    for (int i = 0; i < Mam_Puzzle; i++)
    {
        Is_Solved[i] = false; // ��ʼ���������
    }
    bool end_of_file = false;
    while (!end_of_file)
    {
        // line_count ͳ�ƶ��������������������ʼ��Ϊ 0
        int line_count = 0;

        // ѭ�������������⣬ֱ�������ļ����߶����� Mam_Puzzle-1 ����������
        do
        {
            // ����Ѿ��������ļ�ĩβ���� end_of_file Ϊ true���˳�ѭ��
            if (input_file.eof())
            {
                end_of_file = true;
                break;
            }

            // ����һ���������⣬ÿ�ζ�ȡ N+1 ���ַ����������з������洢�� puzzle ������
            input_file.getline(puzzle, N + 1);

            // ���������ַ��������ڵ��� N���� puzzle �����е���������ת��Ϊһά���鲢�洢�� Buf �����У�ͬʱ line_count �� 1
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
    // �ر��ļ�
    input_file.close();
}

void free_buf()//�ͷſռ�
{
    for (int i = 0; i < Mam_Puzzle; ++i)
        delete[] Buf[i];
    delete[] Buf;
}