#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "global.h"
#include "HttpMgr.h"

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    // 密码不显式设置
    ui->pass_Edit->setEchoMode(QLineEdit::Password);
    ui->check_Edit->setEchoMode(QLineEdit::Password);

    ui->error_tip_label->setProperty("state", "normal");
    repolish(this);

    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish,
            this, &RegisterDialog::slot_reg_mod_finish);

    initHttpHandlers();
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::showErrorTip(QString str, bool ok)
{
    if (ok) {
        ui->error_tip_label->setProperty("state", "normal");
    } else {
        ui->error_tip_label->setProperty("state", "error");
    }
    ui->error_tip_label->setText(str);
    repolish(ui->error_tip_label);
}

void RegisterDialog::on_getCheckCode_Btn_clicked()
{
    qDebug() << "on_getCheckCode_Btn_clicked()" << endl;
    auto email = ui->email_Edit->text();
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if (match) {
        // 发送验证码
        QJsonObject json_obj;
        json_obj["email"] = email;
        qDebug() << "PostHttpReq:URL: " << gate_url_prefix << "/get_varifycode" << endl;
        HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_varifycode"),
                                            json_obj, ReqId::ID_GET_VARIFY_CODE,
                                            Modules::REGISTERMOD);
    } else {
        showErrorTip(tr("邮箱地址不正确"), false);
    }
}

void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        showErrorTip(tr("网络请求错误"), false);
        return;
    }

    // 解析Json字符串 --> 转化为 QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if (jsonDoc.isNull()) {
        showErrorTip(tr("json解析失败"), false);
        return;
    }

    // 解析错误
    if (!jsonDoc.isObject()) {
        showErrorTip(tr("json解析失败"), false);
        return;
    }

    jsonDoc.object();
    _handlers[id](jsonDoc.object());

    return;
}

void RegisterDialog::initHttpHandlers()
{
    // 注册获取验证码回包的逻辑
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            qDebug() << "-------- error:" << error << endl;
            showErrorTip(tr("参数错误"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showErrorTip(tr("验证码已发送到邮箱,请注意查收"), true);
        qDebug() << "email is:" << email;
    });

    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj) {
       int error = jsonObj["error"].toInt();
       if (error != ErrorCodes::SUCCESS) {
           showErrorTip(tr("参数错误"), false);
           return;
       }
       auto email = jsonObj["email"].toString();
       showErrorTip(tr("用户注册成功"), true);
       qDebug() << "regusr success email is:" << email << endl;
       qDebug() << "user name is : " << jsonObj["uid"].toString() << endl;
    });
}


void RegisterDialog::on_sure_Btn_clicked()
{
    qDebug() << "on_sure_Btn_clicked()" << endl;
    if(ui->user_Edit->text() == ""){
        showErrorTip(tr("用户名不能为空"), false);
        return;
    }

    if(ui->email_Edit->text() == ""){
        showErrorTip(tr("邮箱不能为空"), false);
        return;
    }
    if(ui->pass_Edit->text() == ""){
        showErrorTip(tr("密码不能为空"), false);
        return;
    }
    if(ui->check_Edit->text() == ""){
        showErrorTip(tr("确认密码不能为空"), false);
        return;
    }
    if(ui->check_Edit->text() != ui->pass_Edit->text()){
        showErrorTip(tr("密码和确认密码不匹配"), false);
        return;
    }
    if(ui->checkCode_Edit->text() == ""){
        showErrorTip(tr("验证码不能为空"), false);
        return;
    }

    // 所有条件都符合之后，发送http请求注册用户
    qDebug() << "before send reg_all_information by PostHttpReq" << endl;
    QJsonObject json_obj;
    json_obj["user"] = ui->user_Edit->text();
    json_obj["email"] = ui->email_Edit->text();
    json_obj["passwd"] = ui->pass_Edit->text();
    json_obj["confirm"] = ui->check_Edit->text();
    json_obj["varifycode"] = ui->checkCode_Edit->text();
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                                        json_obj, ReqId::ID_REG_USER, Modules::REGISTERMOD);
}
