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

// 加密前端密码，防止明文被截获
extern std::function<QString(QString)> xorString;

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

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VARIFY_ERR = 5,
    TIP_USER_ERR = 6
};

enum ClickLbState{
    Normal = 0,
    Selected = 1
};

extern QString gate_url_prefix;

#endif // GLOBAL_H
