#ifndef MSGNODE_H
#define MSGNODE_H
#include <cstring>
#include <iostream>

class LogicSystem;

class MsgNode {
public:
  explicit MsgNode(short max_len) : _total_len(max_len), _cur_len(0) {
    _data = new char[max_len + 1];
    _data[max_len] = '\0';
  }
  ~MsgNode() { std::cout << "MsgNode dtor" << std::endl; }
  void Clear() {
    ::memset(_data, 0, _total_len);
    _cur_len = 0;
  }
  short _cur_len;
  short _total_len;
  char *_data;
};

class RecvNode : public MsgNode {
  friend class LogicSystem;

public:
  RecvNode(short max_len, short msg_id);

private:
  short _msg_id;
};

class SendNode : public MsgNode {
  friend class LogicSystem;

public:
  SendNode(const char *msg, short max_len, short msg_id);

private:
  short _msg_id;
};

#endif // MSGNODE_H
