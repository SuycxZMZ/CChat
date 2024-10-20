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
    auto email = ui->email_Edit->text();
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if (match) {
        // 发送验证码
        QJsonObject json_obj;
        json_obj["email"] = email;
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
            showErrorTip(tr("参数错误"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showErrorTip(tr("验证码已发送到邮箱,请注意查收"), true);
        qDebug() << "email is : " << email;
    });
}
