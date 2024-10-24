#include "logindialog.h"
#include "ui_logindialog.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    // 注册界面信号连接
    connect(ui->register_Btn, &QPushButton::clicked, this, &LoginDialog::switchRegister);

    // 忘记密码标签ui
    ui->forget_label->SetState("normal","hover","","selected","selected_hover","");
    ui->forget_label->setCursor(Qt::PointingHandCursor);

    // 忘记密码标签信号绑定
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::slot_forget_pwd()
{
    qDebug() << "slot forget pwd" << endl;
    emit switchReset();
}
