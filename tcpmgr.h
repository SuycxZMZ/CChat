/******************************************************************************
 *
 * @file       tcpmgr.h
 * @brief      客户端tcp管理类
 *             负责与具体聊天服务器的长连接通信
 *
 * @author     Suycx
 * @date       2024/10/26
 * @history
 *****************************************************************************/
#ifndef TCPMGR_H
#define TCPMGR_H

#include <QTcpSocket>
#include <functional>
#include <QObject>
#include <QJsonArray>
#include <QMap>
#include "singlton.h"
#include "global.h"

class TcpMgr : public QObject, public Singlton<TcpMgr>,
        public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    ~TcpMgr() {}
    friend class Singlton<TcpMgr>;
private:
    TcpMgr();
    void initHandlers();
    void handleMsg(ReqId id, int len, QByteArray data);

private:
    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    QByteArray _buffer;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;
    QMap<ReqId, std::function<void(ReqId, int, QByteArray)>> _handlers;

public slots:
    void slot_tcp_connect(ServerInfo);
    void slot_send_data(ReqId reqid, QByteArray data);

signals:
    void sig_con_success(bool bsuccess);
    void sig_send_data(ReqId reqId, QByteArray data);
    void sig_swich_chatdlg();
    void sig_load_apply_list(QJsonArray json_array);
    void sig_login_failed(int);
    //void sig_user_search(std::shared_ptr<SearchInfo>);
    //void sig_friend_apply(std::shared_ptr<AddFriendApply>);
    //void sig_add_auth_friend(std::shared_ptr<AuthInfo>);
    //void sig_auth_rsp(std::shared_ptr<AuthRsp>);
    //void sig_text_chat_msg(std::shared_ptr<TextChatMsg> msg);
};

#endif // TCPMGR_H
