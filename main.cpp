#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qss(":style/stylesheet.qss");
    if (qss.open(QFile::ReadOnly)) {
        qDebug() << "open stylesheet.qss success" << endl;
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    } else {
        qDebug() << "open stylesheet.qss failure" << endl;
    }

    MainWindow w;
    w.show();
    return a.exec();
}
