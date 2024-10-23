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
