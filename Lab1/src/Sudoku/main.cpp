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

string file_path;    // 文件路径

int main()
{
	//变量初始化
	init();

	// 从标准输入读入文件路径
	while (getline(cin, file_path))
	{
		//刷新输入缓冲区(可注释掉。getline会自动清空缓冲区)
		//fflush(stdin);
		// Process one file at a time
		sudoku_solve(file_path);
	}
	free_buf();
	return 0;
}
