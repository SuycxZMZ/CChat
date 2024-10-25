#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

    void showErrorTip(QString str, bool ok);

private slots:
    void on_getCheckCode_Btn_clicked();
    void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);

    void on_sure_Btn_clicked();

    void on_return_Btn_clicked();

    void on_cancel_Btn_clicked();

private:
    void initHttpHandlers();

    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkConfirmValid();
    bool checkVarifyValid();

    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);

    void ChangeTipPage();
private:
    Ui::RegisterDialog *ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    QMap<TipErr, QString> _tip_errs;
    QTimer * _countdown_timer;
    int _countdown;
signals:
    void sigSwitchLogin();
};

#endif // REGISTERDIALOG_H
