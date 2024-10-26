#ifndef USERMGR_H
#define USERMGR_H
#include <QObject>
#include <memory>
#include "singlton.h"

class UserMgr:public QObject,public Singlton<UserMgr>,
        public std::enable_shared_from_this<UserMgr>
{
    Q_OBJECT
public:
    friend class Singlton<UserMgr>;
    ~ UserMgr();
    void SetName(QString name);
    void SetUid(int uid);
    void SetToken(QString token);
    QString GetName() const { return _name; }
    QString GetToken() const { return _token; }
    int GetUid() const { return _uid; }
private:
    UserMgr();
    QString _name;
    QString _token;
    int _uid;
};

#endif // USERMGR_H
