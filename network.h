#ifndef NETWORK_H
#define NETWORK_H
#include <QtWidgets/QMainWindow>
#include <QPointer>
#include <QTimer>
#include <QMessageBox>
#include <qnetworkinterface.h>
#include <qnetworksession.h>
#include <qnetworkconfigmanager.h>


class Network : public QMainWindow
{
    Q_OBJECT

public:
    Network(QWidget *parent = 0);
    ~Network();

public slots:
    bool openConnection(QString &netInterface);

private:
    QPointer<QNetworkSession> m_session;

};
#endif // NETWORK_H
