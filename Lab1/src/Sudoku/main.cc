#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <list>
#include <sys/time.h>
#include <iostream>
#include "sudoku.h"
using namespace std;

// 1. 初始化,主要是线程同步变量
// 2. 生产者，消费者
// 3. 复用数独处理函数
// 4. main函数

// 1.定义所需变量
namespace
{

	pthread_mutex_t file_buf = PTHREAD_MUTEX_INITIALIZER;	// 缓冲区互斥量
	pthread_mutex_t lock_print = PTHREAD_MUTEX_INITIALIZER; // 打印互斥量
	pthread_mutex_t lock_file = PTHREAD_MUTEX_INITIALIZER;	// 文件队列锁
	pthread_cond_t empty = PTHREAD_COND_INITIALIZER;		// 生产者条件变量
	pthread_cond_t full = PTHREAD_COND_INITIALIZER;			// 消费者条件变量
	pthread_cond_t *print_order;							// 控制输出顺序
	pthread_cond_t fileout;									// 控制接收线程接收数据
	pthread_t *consumer;									// 解数独
	pthread_t file_thread;									// 读取文件
	pthread_t produce;										// 放入缓存区

	char **buf;					 // 存放题目的缓冲区
	int n_pthread;				 // 线程个数
	int total = 0;				 // 已解决问题
	int n_data = 0;				 // 剩余题目个数
	int use_ptr = 0;			 // 消费下标
	int fill_ptr = 0;			 // 生产下标
	int cur_print = 0;			 // 要打印的线程编号
	int finish_num = 0;			 // 已处理文件完成数量
	bool flag_end_file = false;	 // 判断当前是否已经不再有输入
	bool data_empty = false;	 // 判断是否已无题目输入
	char *data;					 // 存储数组问题的字符数组
	FILE *fp;					 // 获取所需要读取文件的文件指针
	std::list<char *> file_list; // 文件名队列
	int64_t start;				 // 开始时间

}
/*一些需要的函数*/

// 开辟空间释放函数
void exit_sudoku()
{
	free(print_order);
	free(consumer);
	for (int i = 0; i < n_pthread; i++)
	{
		free(buf[i]);
	}
	free(buf);
}
// 获取当前时间函数
int64_t now()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
	// 换算纳秒？
}

// 文件读入线程函数
void *get_file(void *argv)
{
	char *FileName = (char *)malloc(256 * sizeof(char));
	// 循环读入，直到读入文件结束符结束
	while (fgets(FileName, 256, stdin))
	{
		if (FileName[0] == '\n')
        {
            printf("stop reading the file,please wait\n");
            break;
        }
		if (FileName[strlen(FileName) - 1] == '\n')
            FileName[strlen(FileName) - 1] = '\0';
		// 用动态指针复制输入的文件名，不会受到其他空字符影响
		//char *tmp_name = (char *)malloc(256 * sizeof(char));
		// 复制文件名到动态字符指针
		//strcpy(tmp_name, FileName);
		pthread_mutex_lock(&lock_file);
		// 将包含文件名的动态字符指针加入文件队列
		file_list.push_back(FileName);
		// 唤醒生产者线程获取数据
		pthread_cond_signal(&fileout);
		pthread_mutex_unlock(&lock_file);
	}
	// 当读入结束符，设置一个flag控制线程在处理剩余所有问题后结束
	flag_end_file = true;
	// 唤醒生产者线程获取数据
	pthread_cond_signal(&fileout);
	// 退出
	pthread_exit(NULL);
}

// 初始化函数
void init(int pth_number)
{
	// 设置消费者线程个数
	int n = pth_number;
	n_pthread = pth_number;

	// 缓冲区开辟n行
	buf = (char **)malloc(n * sizeof(char *));
	for (int i = 0; i < n_pthread; ++i)
		buf[i] = (char *)malloc(83);

	// n个条件变量控制输出
	print_order = (pthread_cond_t *)malloc(n * sizeof(pthread_cond_t));

	// n个线程号
	consumer = (pthread_t *)malloc(n * sizeof(pthread_t));

	// 初始化 输出顺序的条件变量
	for (int i = 0; i < n_pthread; ++i)
	{
		print_order[i] = PTHREAD_COND_INITIALIZER;
	}
}
/*生产者*/
void *producer(void *argv)
{
	// 文件名
	char *FileName = (char *)malloc(256 * sizeof(char));
	// 开始获取文件名
	while (1)
	{
		// 加锁，原子性防止file_list被修改
		pthread_mutex_lock(&lock_file);
		// 如果文件列表为空，生产者睡眠，等待读取文件线程读取文件，如果此时读入了EOF，则flag_end_file==true，唤醒所有消费者进程，并让其退出，最后让生产者退出
		// 如果此时读入文件，则唤醒生产者线程，读取文件名，并读取文件中的数独题目
		while (file_list.size() == 0)
		{
			// 如果读入了EOF
			if (flag_end_file == true)
			{
				// 设置题目数目为空，让消费者线程退出
				data_empty = true;
				// 唤醒所有的等待的消费者线程，让其退出
				for (int i = 0; i < n_pthread; i++)
				{
					pthread_cond_signal(&full);
				}
				// 释放锁，退出线程
				pthread_mutex_unlock(&lock_file);
				pthread_exit(NULL);
			}
			// 如果不是读入EOF，等待文件输入
			pthread_cond_wait(&fileout, &lock_file);
		}
		// 复制文件列表的第一个文件名到filenam用于读取
		strcpy(FileName, file_list.front());
		file_list.pop_front();
		pthread_mutex_unlock(&lock_file);
		// 判断文件是否存在，不存在重复上述
		if (access(FileName, F_OK) == -1)
		{
			printf("文件不存在\n");
			continue;
		}
		// 使用文件指针读取文件
		fp = fopen(FileName, "r");
		if (fp == NULL)
        {
            printf("%s dose not have data\n", FileName);
            continue;
        }
		char puzzle[128];
		// 将文件中的数独问题存储到缓存区buf
		while (1)
		{
			pthread_mutex_lock(&file_buf);
			// 当缓存区存满线程数的问题时等待线程解决问题
			while (n_data == n_pthread)
			{
				pthread_cond_wait(&empty, &file_buf);
			}
			// 读取文件中的一行到缓存区，如果已经到了文件末尾，则重新等待输入新的文件
			if (fgets(puzzle, sizeof puzzle, fp) != NULL)
			{
				// 如果已经读入一个数组，将其存入buf中
				if (strlen(puzzle) >= 81)
				{

					for (int i = 0; i < 81; i++)
					{
						buf[fill_ptr][i] = puzzle[i];
					}
					// printf("读取文件%s到buf\n",filename);
					// 缓存区指针向后一个位置
					fill_ptr = (fill_ptr + 1) % n_pthread;
					++n_data;
					// printf("data:%d\n", n_data);
					// 计算问题个数
					finish_num += 1;
				}
			}
			// 如果已经读不到问题，释放锁退出循环读取新的文件
			else
			{
				pthread_mutex_unlock(&file_buf);
				break;
			}
			pthread_cond_signal(&full);
			pthread_mutex_unlock(&file_buf);
		}
	}
}
/*消费者*/
// 获取问题的函数
char *get()
{
	// 获取当前还未计算的第一个数独问题
	char *tmp = buf[use_ptr];
	// 让指针后移问题总数减少
	use_ptr = (use_ptr + 1) % n_pthread;
	n_data--;
	return tmp;
}
// 消费者解题函数
void *consumed_solver(void *arg)
{
	int board[81];
	// 循环解题
	while (1)
	{
		// 加锁防止改变数据
		pthread_mutex_lock(&file_buf);
		// 当缓存区没有数组题时等待，如果由于读入了文件结束符，所有等待线程需要退出，因此当data_empty为空时退出
		while (n_data == 0)
		{
			if (data_empty)
			{
				// 解锁并退出
				pthread_mutex_unlock(&file_buf);
				pthread_exit(NULL);
			}
			// 如果当前没有读入文件结束，等待生产者读文件写入数独问题
			pthread_cond_wait(&full, &file_buf);
		}

		++total;							  // 当前数据的行数
		int myturn = (total - 1) % n_pthread; // 应该的打印顺序
		// 读一行题放到board里面
		data = get();
		for (int i = 0; i < 81; ++i)
		{
			board[i] = data[i] - '0';
		}

		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&file_buf);
		// 解决问题
		solve_sudoku_dancing_links(board);
		// 输出顺序
		pthread_mutex_lock(&lock_print);
		// 根据文件的行数进行输入，有五个顺序，每个cur_print对于total%线程数
		while (myturn != cur_print)
		{ // 没有轮到则在自己的条件变量上等待
			pthread_cond_wait(&print_order[myturn], &lock_print);
		}
		// 打印到屏幕 注释掉可以省不少时间

		for (int i = 0; i < 81; ++i)
		{
			cout << board[i];
		}
		cout << endl;
		fflush(stdout);
		cur_print = (cur_print + 1) % n_pthread;					 // 下一个该打印的编号
		pthread_cond_signal(&print_order[(myturn + 1) % n_pthread]); // 唤醒下一个，如果对方在睡的话
		// 这里也可以只用一个条件变量，到这里用broadcast唤醒所有其他线程，但是效率可能会低一点，没有尝试-.-
		pthread_mutex_unlock(&lock_print);
	}
}
/*主程序*/
int main(int argc, char *argv[])
{


	// 初始化，参数设置为消费者线程的个数
	init(4);//根据CPU核来设置
	start = now();
	// 创建文件读取线程
	pthread_create(&file_thread, NULL, get_file, NULL);
	// 创建生产者线程
	pthread_create(&produce, NULL, producer, NULL);
	// 创建n个消费者线程
	for (int i = 0; i < n_pthread; ++i)
	{
		pthread_create(&consumer[i], NULL, consumed_solver, NULL); // 解题
	}
	// 等待所有线程执行完毕
	for (int i = 0; i < n_pthread; ++i)
	{
		pthread_join(consumer[i], NULL);
	}
	// 计算时间
	int64_t end = now();
	double sec = (end - start) / 1000000.0;
	exit_sudoku();
	return 0;
}
