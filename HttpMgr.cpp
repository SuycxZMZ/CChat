#include "HttpMgr.h"


HttpMgr::~HttpMgr()
{

}

HttpMgr::HttpMgr()
{
    connect(this, &HttpMgr::sig_http_finish, this, &HttpMgr::slot_http_finish);
}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId req_id, Modules mod)
{
    QByteArray data = QJsonDocument(json).toJson();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(data.length()));
    auto self = shared_from_this();
    QNetworkReply * reply = _manager.post(request, data);
    QObject::connect(reply, &QNetworkReply::finished, [self, reply, req_id, mod](){
        // 处理错误情况
        qDebug() << "-------------- posthttp finish ----------------";
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << reply->errorString();
            // 发送信号通知完成
            emit self->sig_http_finish(req_id, "", ErrorCodes::ERR_NETWORK, mod);
            // 出错了就需要释放资源，leater方式异步回收
            reply->deleteLater();
            return;
        }
        qDebug() << "QNetworkReply::finished" << endl;
        // 没出现错误
        QString res = reply->readAll();
        // 发送信号通知
        emit self->sig_http_finish(req_id, res, ErrorCodes::SUCCESS, mod);
        // reply用完了，成功也要回收
        reply->deleteLater();
        return;
    });
}

void HttpMgr::slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod)
{
    if (mod == Modules::REGISTERMOD) {
        // 发送信号通知指定模块 http 的响应结束
        emit sig_reg_mod_finish(id, res, err);
    }

    if (mod == Modules::RESETMOD) {
        emit sig_reset_mod_finish(id, res, err);
    }

    if (mod == Modules::LOGINMOD) {
        emit sig_login_mod_finish(id, res, err);
    }
}
