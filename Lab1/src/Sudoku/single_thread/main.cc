#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <unistd.h>
#include "sudoku.h"
using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //互斥锁


int64_t now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL); // 获取当前精确时间
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char *argv[])
{

    init_neighbors();
    pthread_t threads[THREADNUM];
    char file_path[128];

    char puzzle[128];                                // 输入的谜题
    int total_solved = 0;                            // 已解决的谜题总数
    int total = 0;                                   // 谜题总数
    bool (*solve)(int) = solve_sudoku_dancing_links; // 使用“舞蹈链”算法解决数独

    char *FileName = (char *)malloc(256 * sizeof(char));
    FILE *fp;

    int64_t start = now(); // 计时

    while (fgets(FileName, 256, stdin))
    {
        if (FileName[0] == '\n')
        {
            printf("stop reading the file,please wait\n");
            break;
        }
        if (FileName[strlen(FileName) - 1] == '\n')
            FileName[strlen(FileName) - 1] = '\0';

        fp = fopen(FileName, "r");

        if (fp == NULL)
        {
            printf("%s dose not have data\n", FileName);
            continue;
        }
        while (fgets(puzzle, sizeof puzzle, fp) != NULL)
        {
            if (strlen(puzzle) >= N)
            {
                input(puzzle, total++);
            }
        }
    }
    it = puzzleSet.begin();
    for (int i = 0; i < THREADNUM; ++i)
    {
        pthread_create(&threads[i], NULL, (void *(*)(void *))solveSudoku, NULL);
    }

    for (int i = 0; i < THREADNUM; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    int64_t end = now();
    double sec = (end - start) / 1000000.0;

    return 0;
}
