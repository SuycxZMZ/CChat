#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class ChatDialog;
}

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);
    ~ChatDialog();
    void addChatUserList();
private:
    void ShowSearch(bool show = false);

public slots:
    void slot_loading_contact_user();
private:
    Ui::ChatDialog *ui;
    ChatUIMode _mode;
    ChatUIMode _state;

    bool _b_loading;
};

#endif // CHATDIALOG_H
