/******************************************************************************
 *
 * @file       mainwindow.h
 * @brief      主窗口 Function
 *
 * @author     Suycx
 * @date       2024/10/18
 * @history
 *****************************************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ResetDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public slots:
    void slotSwitchToReg();
    void SlotSwitchLogin();
    void SlotSwitchLogin2(); // 重置密码界面返回登陆界面的槽
    void SlotSwitchReset();
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    LoginDialog * _login_dlg;
    RegisterDialog * _register_dlg;
    ResetDialog* _reset_dlg;
};
#endif // MAINWINDOW_H
