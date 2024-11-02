#include "MsgNode.h"
#include "const.h"
#include <boost/asio/detail/socket_ops.hpp>
#include <cstring>

RecvNode::RecvNode(short max_len, short msg_id)
    : MsgNode(max_len), _msg_id(msg_id) {}

SendNode::SendNode(const char *msg, short max_len, short msg_id)
    : MsgNode(max_len + HEAD_TOTAL_LEN), _msg_id(msg_id) {
  short msg_id_net =
      boost::asio::detail::socket_ops::host_to_network_short(msg_id);
  memcpy(_data, &msg_id_net, HEAD_ID_LEN);
  short max_len_net =
      boost::asio::detail::socket_ops::host_to_network_short(max_len);
  memcpy(_data + HEAD_ID_LEN, &max_len_net, HEAD_DATA_LEN);
  memcpy(_data + HEAD_ID_LEN + HEAD_DATA_LEN, msg, max_len);
}