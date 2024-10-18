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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public slots:
    void slotSwitchToReg();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    LoginDialog * _login_dlg;
    RegisterDialog * _register_dlg;
};
#endif // MAINWINDOW_H
