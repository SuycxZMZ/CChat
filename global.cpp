#include "global.h"
#include <QWidget>
#include <functional>

std::function<void(QWidget*)> repolish = [](QWidget* w) {
    w->style()->unpolish(w);
    w->style()->polish(w);
};

QString gate_url_prefix = "";

std::function<QString(QString)> xorString = [](QString input) -> QString {
    QString ans = input;
    int len = input.length();
    ushort xor_code = len % 255;
    for (int i = 0; i < len; ++i) {
        ans[i] = QChar(static_cast<ushort>(input[i].unicode() ^ xor_code));
    }
    return ans;
};


// ----------------- 测试 ----------------- //
const std::vector<QString> strs ={"hello world !",
                             "nice to meet u",
                             "New year，new life",
                            "You have to love yourself",
                            "My love is written in the wind ever since the whole world is you"};

const std::vector<QString> heads = {
    ":/res/head_1.jpg",
    ":/res/head_2.jpg",
    ":/res/head_3.jpg",
    ":/res/head_4.jpg",
    ":/res/head_5.jpg"
};

const std::vector<QString> names = {
    "HanMeiMei",
    "Lily",
    "Ben",
    "Androw",
    "Max",
    "Summer",
    "Candy",
    "Hunter"
};
