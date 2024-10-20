#include "mainwindow.h"
#include "global.h"
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

    // 加载 GateServer配置
    qDebug() << "----------- 加载 GateServer配置 -----------" << endl;
    QString file_name = "config.ini";
    QString app_path = QCoreApplication::applicationDirPath();
    QString config_path = QDir::toNativeSeparators(app_path + QDir::separator() + file_name);
    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    gate_url_prefix = "http://" + gate_host + ":" + gate_port;

    MainWindow w;
    w.show();
    return a.exec();
}
