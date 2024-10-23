/******************************************************************************
 *
 * @file       timerbtn.h
 * @brief      验证码倒计时工具类
 *
 * @author     Suycx
 * @date       2024/10/23
 * @history
 *****************************************************************************/
#ifndef TIMERBTN_H
#define TIMERBTN_H

#include <QPushButton>
#include <QMouseEvent>
#include <QTimer>

class TimerBtn : public QPushButton
{
public:
    TimerBtn(QWidget* parent = nullptr);
    ~TimerBtn();
    virtual void mouseReleaseEvent(QMouseEvent* e) override;
private:
    QTimer * _timer;
    int _counter;
};

#endif // TIMERBTN_H
