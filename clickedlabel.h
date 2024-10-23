/******************************************************************************
 *
 * @file       clickedlabel.h
 * @brief      显示/隐藏密码  工具类
 *
 * @author     Suycx
 * @date       2024/10/23
 * @history
 *****************************************************************************/

#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H

#include <QLabel>
#include "global.h"

class ClickedLabel : public QLabel
{
    Q_OBJECT
public:
    ClickedLabel(QWidget* parent);
    virtual void mousePressEvent(QMouseEvent *ev) override;
    virtual void mouseReleaseEvent(QMouseEvent *ev) override;
    virtual void enterEvent(QEvent* event) override;
    virtual void leaveEvent(QEvent* event) override;
    void SetState(QString normal="", QString hover="", QString press="",
                      QString select="", QString select_hover="", QString select_press="");
    ClickLbState GetCurState();
    bool SetCurState(ClickLbState state);
    void ResetNormalState();
private:
    // 闭眼
    QString _normal;
    QString _normal_hover;
    QString _normal_press;

    // 睁眼
    QString _selected;
    QString _selected_hover;
    QString _selected_press;

    ClickLbState _curstate;

signals:
    void clicked(QString, ClickLbState);
};

#endif // CLICKEDLABEL_H
