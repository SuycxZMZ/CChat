#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::slotSwitchToReg()
{
    setCentralWidget(_register_dlg);
    _login_dlg->hide();
    _register_dlg->show();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _login_dlg(new LoginDialog())
    , _register_dlg(new RegisterDialog())
{
    ui->setupUi(this);
    setCentralWidget(_login_dlg);
    _login_dlg->show();

    // 信号槽创建
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::slotSwitchToReg);

}

MainWindow::~MainWindow()
{
    delete ui;
    if (_login_dlg) {
        delete  _login_dlg;
        _login_dlg = nullptr;
    }
    if (_register_dlg) {
        delete _register_dlg;
        _register_dlg = nullptr;
    }
}

