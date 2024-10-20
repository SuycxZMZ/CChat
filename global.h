/******************************************************************************
 *
 * @file       global.h
 * @brief      一些全局变量和函数
 *
 * @author     Suycx
 * @date       2024/10/18
 * @history
 *****************************************************************************/
#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <QByteArray>
#include <QNetworkReply>
#include <QMap>
#include <QJsonObject>
#include <QDir>
#include <QRegularExpression>
#include <QSettings>
#include <functional>
#include <memory>
#include <iostream>
#include <mutex>
#include "QStyle"

// 刷新qss
extern std::function<void(QWidget*)> repolish;

enum ReqId{
    ID_GET_VARIFY_CODE = 1001, // 获取验证码
    ID_REG_USER = 1002,        // 注册用户
};

enum ErrorCodes {
    SUCCESS = 0,
    ERR_JSON,
    ERR_NETWORK,
};

enum Modules {
    REGISTERMOD = 0,
};

extern QString gate_url_prefix;

#endif // GLOBAL_H
