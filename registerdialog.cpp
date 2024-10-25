#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "global.h"
#include "HttpMgr.h"

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog),
    _countdown(5)
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

    // --------------- 注册页错误信息提示信号 --------------- //
    ui->error_tip_label->clear();
    connect(ui->user_Edit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });
    connect(ui->email_Edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });
    connect(ui->pass_Edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });
    connect(ui->check_Edit, &QLineEdit::editingFinished, this, [this](){
        checkConfirmValid();
    });
    connect(ui->checkCode_Edit, &QLineEdit::editingFinished, this, [this](){
        checkVarifyValid();
    });

    // 设置隐藏密码初始化
    ui->passVisible_label->setCursor(Qt::PointingHandCursor);
    ui->checkVisible_label->setCursor(Qt::PointingHandCursor);

    ui->passVisible_label->SetState("unvisible","unvisible_hover","","visible",
                               "visible_hover","");

    ui->checkVisible_label->SetState("unvisible","unvisible_hover","","visible",
                                  "visible_hover","");

    // 点击隐藏密码信号注册
    connect(ui->passVisible_label, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->passVisible_label->GetCurState();
        if (state == ClickLbState::Normal) {
            ui->pass_Edit->setEchoMode(QLineEdit::Password);
        } else {
            ui->pass_Edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "clickedLabel was clicked" << endl;
    });

    connect(ui->checkVisible_label, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->checkVisible_label->GetCurState();
        if(state == ClickLbState::Normal){
            ui->check_Edit->setEchoMode(QLineEdit::Password);
        }else{
            ui->check_Edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "clickedLabel was clicked" << endl;
    });

    // 自动返回登陆界面定时器和信号槽
    _countdown_timer = new QTimer(this);
    connect(_countdown_timer, &QTimer::timeout, [this]() {
        if (_countdown == 0) {
            _countdown_timer->stop();
            emit sigSwitchLogin();
            return;
        }
        --_countdown;
        auto str = QString("注册成功，%1 s返回登陆").arg(_countdown);
        ui->tip_lib->setText(str);
    });
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
            qDebug() << "-------- error:" << error << " --------" << endl;
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
       ChangeTipPage();
    });
}

bool RegisterDialog::checkUserValid()
{
    if (ui->user_Edit->text() == "") {
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool RegisterDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_Edit->text();
    // 邮箱地址的正则表达式
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
    if(!match){
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool RegisterDialog::checkPassValid()
{
    auto pass = ui->pass_Edit->text();
    if(pass.length() < 6 || pass.length()>15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }
    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);
    return true;
}

bool RegisterDialog::checkConfirmValid()
{
    auto pass = ui->pass_Edit->text();
    auto confirm = ui->check_Edit->text();

    if(confirm.length() < 6 || confirm.length() > 15 ){
        //提示长度不准确
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(confirm).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("不能包含非法字符"));
        return false;
    }

    DelTipErr(TipErr::TIP_CONFIRM_ERR);

    if(pass != confirm){
        //提示密码不匹配
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("确认密码和密码不匹配"));
        return false;
    }else{
       DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }
    return true;
}

bool RegisterDialog::checkVarifyValid()
{
    auto pass = ui->checkCode_Edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

void RegisterDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showErrorTip(tips, false);
}

void RegisterDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if (_tip_errs.empty()) {
        ui->error_tip_label->clear();
        return;
    }
    showErrorTip(_tip_errs.first(), false);
}

void RegisterDialog::ChangeTipPage()
{
    _countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);
    _countdown_timer->start(1000); // 启动定时器，设置间隔为1000毫秒（1秒）
}


void RegisterDialog::on_sure_Btn_clicked()
{
    qDebug() << "on_sure_Btn_clicked()" << endl;
    bool valid = checkUserValid();
    if(!valid){
        return;
    }
    valid = checkEmailValid();
    if(!valid){
        return;
    }
    valid = checkPassValid();
    if(!valid){
        return;
    }
    valid = checkVarifyValid();
    if(!valid){
        return;
    }

    // 所有条件都符合之后，发送http请求注册用户，这里是发送给GateServer
    // 简单加密 用户密码
    qDebug() << "before send reg_all_information by PostHttpReq" << endl;
    QJsonObject json_obj;
    json_obj["user"] = ui->user_Edit->text();
    json_obj["email"] = ui->email_Edit->text();
    json_obj["passwd"] = xorString(ui->pass_Edit->text());
    json_obj["confirm"] = xorString(ui->check_Edit->text());
    json_obj["varifycode"] = ui->checkCode_Edit->text();
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                                        json_obj, ReqId::ID_REG_USER, Modules::REGISTERMOD);
}

void RegisterDialog::on_return_Btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}

void RegisterDialog::on_cancel_Btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}
