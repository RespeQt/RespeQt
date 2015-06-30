#include <QTimer>
#include <QMessageBox>
#include "network.h"

Network::Network(QWidget *parent)
    : QMainWindow(parent)
{
//    QTimer::singleShot(0, this, SLOT(openConnection()));
}

Network::~Network()
{
    if (m_session)
        m_session->close();
}

bool Network::openConnection(QString &netInterface)
{
    // Internet Access Point
    QNetworkConfigurationManager manager;

    const bool canStartIAP = (manager.capabilities()
        & QNetworkConfigurationManager::CanStartAndStopInterfaces);

    // If there is a default access point, use it
    QNetworkConfiguration cfg = manager.defaultConfiguration();

    if (!cfg.isValid() || !canStartIAP) {
        return false;
    }

    // Open session
    m_session = new QNetworkSession(cfg);
    m_session->open();
    // Waits for session to be open and continues after that
    m_session->waitForOpened();

    // Show interface name to the user
    QNetworkInterface iff = m_session->interface();
    netInterface = iff.humanReadableName();
    return true;
}
