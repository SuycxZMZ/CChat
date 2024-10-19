#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QWidget>

void MainWindow::slotSwitchToReg()
{
    setCentralWidget(_register_dlg);
    _login_dlg->hide();
    _register_dlg->show();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _login_dlg(new LoginDialog(this))
    , _register_dlg(new RegisterDialog(this))
{
    ui->setupUi(this);
    setCentralWidget(_login_dlg);

    // 信号槽创建
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::slotSwitchToReg);

    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    _register_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    _register_dlg->hide();
}

MainWindow::~MainWindow()
{
    delete ui;
}

