#ifndef HTTP_CON_H
#define HTTP_CON_H

#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

using namespace std;

void http_request(const char* request, int cfd,int len);
void send_respond_head(int cfd, int no, const char* desp, const char* type, long len);
void send_file(int cfd, const char* filename);
void send_dir(int cfd, const char* dirname);
void encode_str(char* to, int tosize, const char* from);
void decode_str(char *to, char *from);
const char *get_file_type(char *name);
int get_line(int sock, char *buf, int size);

class HttpCon {
public:
    static int m_epollfd;
    static int m_user_cnt;

    HttpCon() {
        m_socketfd = -1;
    }
    ~HttpCon() {}

    void process();
    //非阻塞的读写
    bool read();
    bool write();
    void init(int ,const struct sockaddr_in &);
    void close();

private:
    int m_socketfd;
    struct sockaddr_in m_addr;

};

#endif
