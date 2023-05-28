#ifndef PARTICIPANT_H_INCLUDE
#define PARTICIPANT_H_INCLUDE
#include <sys/epoll.h>

#include <iostream>
#include <map>
#include <string>

#include "kvstore2pcsystem.h"
using namespace std;

class participant {
 private:
  map<string, string> memory_old;
  map<string, string> memory_new;
  Command command;
  Info coordinator_info;
  Info participant_info;
  int participant_socket;

  int commit_ID;
  int backlog;
  int del_number;
  void back_up();
  string execute_command();
  void rollback();
  int send_message(int conn_socket, string s);
  void process(int conn_socket);
  void connect_coordinator();
  void init();
  string get_memory_string();
  void recovery(char* msg);
  void parse_message(char* msg);
  int get_nextrn(char* msg, int l);
  int get_number(char* msg, int l, int r);
  string get_string(char* msg, int l, int r);

 public:
  participant(char* filename);
  void start();
  ~participant();
};

#endif