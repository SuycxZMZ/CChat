#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "clickedbtn.h"
#include "global.h"
#include "usermgr.h"
#include <QAction>
#include <QRandomGenerator>
#include "chatuserwid.h"
#include "loadingdlg.h"

ChatDialog::ChatDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChatDialog),
    _mode(ChatUIMode::ChatMode),
    _state(ChatUIMode::ChatMode),
    _b_loading(false)
{
    ui->setupUi(this);
    ui->add_btn->SetState("normal", "hover", "press");
    ui->add_btn->setProperty("state","normal");
    ui->search_edit->setMaxLength(15);

    QAction *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/res/search.png"));
    ui->search_edit->addAction(searchAction,QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));

    // 创建一个清除动作并设置图标
    QAction *clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/res/close_transparent.png"));
    // 初始时不显示清除图标
    // 将清除动作添加到LineEdit的末尾位置
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);

    // 当需要显示清除图标时，更改为实际的清除图标
    connect(ui->search_edit, &QLineEdit::textChanged, [clearAction](const QString &text) {
        if (!text.isEmpty()) {
            clearAction->setIcon(QIcon(":/res/close_search.png"));
        } else {
            clearAction->setIcon(QIcon(":/res/close_transparent.png")); // 文本为空时，切换回透明图标
        }

    });

    // 连接清除动作的触发信号到槽函数，用于清除文本
    connect(clearAction, &QAction::triggered, [this, clearAction]() {
        ui->search_edit->clear();
        clearAction->setIcon(QIcon(":/res/close_transparent.png")); // 清除文本后，切换回透明图标
        ui->search_edit->clearFocus();
        // 清除按钮被按下则不显示搜索框
        ShowSearch(false);
    });
    ShowSearch(false);

    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user, this,
            &ChatDialog::slot_loading_contact_user);
    addChatUserList();

    ui->search_edit->SetMaxLength(15);
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::addChatUserList()
{
//    //先按照好友列表加载聊天记录，等以后客户端实现聊天记录数据库之后再按照最后信息排序
//    auto friend_list = UserMgr::GetInstance()->GetChatListPerPage();
//    if (friend_list.empty() == false) {
//        for(auto & friend_ele : friend_list){
//            auto find_iter = _chat_items_added.find(friend_ele->_uid);
//            if(find_iter != _chat_items_added.end()){
//                continue;
//            }
//            auto *chat_user_wid = new ChatUserWid();
//            auto user_info = std::make_shared<UserInfo>(friend_ele);
//            chat_user_wid->SetInfo(user_info);
//            QListWidgetItem *item = new QListWidgetItem;
//            //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
//            item->setSizeHint(chat_user_wid->sizeHint());
//            ui->chat_user_list->addItem(item);
//            ui->chat_user_list->setItemWidget(item, chat_user_wid);
//            _chat_items_added.insert(friend_ele->_uid, item);
//        }

//        //更新已加载条目
//        UserMgr::GetInstance()->UpdateChatLoadedCount();
//    }

    //模拟测试条目
    // 创建QListWidgetItem，并设置自定义的widget
    for(int i = 0; i < 13; i++){
        int randomValue = QRandomGenerator::global()->bounded(100); // 生成0到99之间的随机整数
        int str_i = randomValue % strs.size();
        int head_i = randomValue % heads.size();
        int name_i = randomValue % names.size();

        auto *chat_user_wid = new ChatUserWid();
        chat_user_wid->SetInfo(names[name_i], heads[head_i], strs[str_i]);
        QListWidgetItem *item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(chat_user_wid->sizeHint());
        ui->chat_user_list->addItem(item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
    }
}

void ChatDialog::ShowSearch(bool show)
{
    if(show){
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        _mode = ChatUIMode::SearchMode;
    }else if(_state == ChatUIMode::ChatMode){
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        _mode = ChatUIMode::ChatMode;
        ui->search_list->hide();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }else if(_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
        ui->search_list->hide();
        ui->search_edit->clear();
        ui->search_edit->clearFocus();
    }
}

void ChatDialog::slot_loading_contact_user()
{
    qDebug() << "slot loading contact user";
    if(_b_loading){
        return;
    }

    _b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";

    // loadMoreConUser();
    addChatUserList();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    _b_loading = false;
}
