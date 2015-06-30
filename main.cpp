#include <QtWidgets/QApplication>
#include <QTextCodec>
#include <QLibraryInfo>
#include "mainwindow.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <Mmsystem.h>
#endif

int main(int argc, char *argv[])
{
    int ret;
#ifdef Q_OS_WIN
    timeBeginPeriod(1);
#endif

    QApplication a(argc, argv);
//    QTextCodec::setCodecForTr(QTextCodec::codecForName("utf8"));
    MainWindow w;
    w.show();
    ret = a.exec();
#ifdef Q_OS_WIN
    timeEndPeriod(1);
#endif
    return ret;
}
