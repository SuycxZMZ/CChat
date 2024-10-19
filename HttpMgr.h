/******************************************************************************
 *
 * @file       HttpMgr.h
 * @brief      Http管理类
 *
 * @author     Suycx
 * @date       2024/10/18
 * @history
 *****************************************************************************/

#ifndef HTTPMGR_H
#define HTTPMGR_H

#include "singlton.h"
#include <QUrl>
#include <QString>
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonDocument>

class HttpMgr : public QObject,
                public Singlton<HttpMgr>,
                public std::enable_shared_from_this<HttpMgr> {
    Q_OBJECT
public:
    ~HttpMgr();
private:
    friend class Singlton<HttpMgr>;
    HttpMgr();
    void PostHttpReq(QUrl url, QJsonObject ison, ReqId req_id, Modules mod);

private:
    QNetworkAccessManager _manager;
private slots:
    void slot_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
signals:
    void sig_http_finish(ReqId id, QString res, ErrorCodes err, Modules mod);
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // HTTPMGR_H
