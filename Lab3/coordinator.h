#ifndef COORDINATOR_H_INCLUDE
#define COORDINATOR_H_INCLUDE
#include <sys/epoll.h>

#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "kvstore2pcsystem.h"
using namespace std;

class coordinator {
 private:
  Info coordinator_info;
  vector<Info> participant_info;
  vector<bool> participant_work;
  set<int> succeed_participant;
  set<int> failed_participant;
  unordered_map<int, int> connect_participant_pos_socket;
  unordered_map<int, int> connect_participant_socket_pos;
  int participant_number;
  int coordinator_socket;
  int backlog;
  int epollfd;
  Command command;
  void init();
  vector<int> participants_commit_ID;
  void commid_ID_init();
  void participant_work_init();
  int RecvCommand(int conn_socket);
  void Heartbeat_detection(int mode);
  static void try_connect(coordinator* cor, int i, int clientfd,
                          struct sockaddr_in seraddr);
  void addfd(int fd, bool enable_et);
  int send_message(int i, string s);
  int recv_message(string s, int n, int timeout);
  int recv_commid_id(int n, int timeout);
  void recovery();
  void ProcessTimeout();
  string get_message(int i);
  void send_client_message(int conn_socket, string s);

  string get(string value);
  string set_command(string key, string value);
  string del_command();

 public:
  coordinator(char* filename);
  void start();
  ~coordinator();
};

#endif