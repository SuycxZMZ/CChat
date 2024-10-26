#include "tcpmgr.h"
#include "usermgr.h"
#include <QDebug>
#include <QAbstractSocket>
#include <QJsonDocument>

TcpMgr::TcpMgr() :
    _host(""), _port(0), _b_recv_pending(false),
    _message_id(0), _message_len(0)
{
    // tcp连接信号槽
    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
        qDebug() << "Connected to server ...";

        // 连接成功信号
        emit sig_con_success(true);
    });

    // tcp可读信号槽，其实就是可读事件的异步回调
    QObject::connect(&_socket, &QTcpSocket::readyRead, [&]() {
        // 读取socket上所有可读数据
        _buffer.append(_socket.readAll());

        QDataStream stream(&_buffer, QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_5_0);
        // 死循环解析读到的数据
        for ( ; ; ) {
            //先解析头部
            if(!_b_recv_pending){
                // 检查缓冲区中的数据是否足够解析出一个消息头（消息ID + 消息长度）
                if (_buffer.size() < static_cast<int>(sizeof(quint16) * 2)) {
                    return; // 数据不够，等待更多数据
                }

                // 预读取消息ID和消息长度，但不从缓冲区中移除
                stream >> _message_id >> _message_len;

                //将buffer 中的前四个字节移除
                _buffer = _buffer.mid(sizeof(quint16) * 2);

                // 输出读取的数据
                qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;
            }

            // buffer剩余长读是否满足消息体长度，不满足则退出继续等待新的数据到来
            if(_buffer.size() < _message_len){
                _b_recv_pending = true;
                return;
            }

            // 可以解析完整的包
            _b_recv_pending = false;
            QByteArray messageBody = _buffer.mid(0, _message_len);
            qDebug() << "receive body msg is " << messageBody;

            // buffer偏移掉本次解析的数据包
            _buffer = _buffer.mid(_message_len);
            // 调用处理数据包的回调
            handleMsg(ReqId(_message_id),_message_len, messageBody);
        }
    });

    // 处理错误（适用于Qt 5.15之前的版本）
    QObject::connect(&_socket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),
                        [&](QTcpSocket::SocketError socketError) {
           qDebug() << "Error:" << _socket.errorString() ;
           switch (socketError) {
               case QTcpSocket::ConnectionRefusedError:
                   qDebug() << "Connection Refused!";
                   emit sig_con_success(false);
                   break;
               case QTcpSocket::RemoteHostClosedError:
                   qDebug() << "Remote Host Closed Connection!";
                   break;
               case QTcpSocket::HostNotFoundError:
                   qDebug() << "Host Not Found!";
                   emit sig_con_success(false);
                   break;
               case QTcpSocket::SocketTimeoutError:
                   qDebug() << "Connection Timeout!";
                   emit sig_con_success(false);
                   break;
               case QTcpSocket::NetworkError:
                   qDebug() << "Network Error!";
                   break;
               default:
                   qDebug() << "Other Error!";
                   break;
           }
    });

    // 断开连接信号槽
    QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
                qDebug() << "Disconnected from server.";
    });

    // 发送信息信号槽，由 sig_send_data 触发
    QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);

    // 初始化一些回调 _handlers
    initHandlers();
}

void TcpMgr::initHandlers()
{
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is:" << id;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        qDebug()<< "data jsonobj is " << jsonObj;
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err;
            emit sig_login_failed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS) {
            qDebug() << "Login Failed, err is " << err;
            emit sig_login_failed(err);
            return;
        }

        UserMgr::GetInstance()->SetUid(jsonObj["uid"].toInt());
        UserMgr::GetInstance()->SetName(jsonObj["name"].toString());
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());
        qDebug() << "uid:" << UserMgr::GetInstance()->GetUid()
                 << " name:" << UserMgr::GetInstance()->GetName()
                 << " token:" << UserMgr::GetInstance()->GetToken();
        emit sig_swich_chatdlg();
    });
}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_it = _handlers.find(id);
    if (find_it == _handlers.end()) {
        qDebug()<< "not found id ["<< id << "] to handle";
        return;
    }
    find_it.value()(id, len, data);
}

void TcpMgr::slot_tcp_connect(ServerInfo server_info)
{
    qDebug()<< "receive tcp connect signal";
    qDebug() << "Connecting to server...";

    _host = server_info.Host;
    _port = static_cast<uint16_t>(server_info.Port.toUInt());
    // 连接，连接结果已经注册完信号槽回调
    _socket.connectToHost(_host, _port);
}

void TcpMgr::slot_send_data(ReqId reqid, QByteArray data)
{
    uint16_t id = reqid;

    // 组包的长度信息，要转换网络字节序
    quint16 len = static_cast<quint16>(data.length());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    // 网络大端字节序
    out.setByteOrder(QDataStream::BigEndian);

    // 消息id和长度
    out << id << len;
    // 数据
    block.append(data);

    _socket.write(block);
    qDebug() << "tcp mgr send byte data is:" << block ;
}

