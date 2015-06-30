#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "diskimage.h"
#include "diskimagepro.h"
#include "folderimage.h"
#include "miscdevices.h"
#include "aspeqtsettings.h"
#include "autobootdialog.h"
#include "autoboot.h"
#include "cassettedialog.h"
#include "bootoptionsdialog.h"
#include "logdisplaydialog.h"

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QScrollBar>
#include <QTranslator>
#include <QMessageBox>
#include <QWidget>
#include <QDrag>
#include <QtDebug>
#include <QDesktopWidget>
#include <QFont>

#include "atarifilesystem.h"
#include "miscutils.h"


AspeqtSettings *aspeqtSettings;
MainWindow *mainWindow;

QFile *logFile;
QMutex *logMutex;
QString g_exefileName;
QString g_aspeclFileName;
QString g_aspeQtAppPath;
QRect g_savedGeometry;
char g_aspeclSlotNo;
bool g_disablePicoHiSpeed;
bool g_printerEmu = true;
bool g_D9DOVisible = true;
bool g_miniMode = false;
bool g_shadeMode = false;
int g_savedWidth;
bool g_logOpen;

// ****************************** END OF GLOBALS ************************************//

// Displayed only in debug mode    "!d"
// Unimportant     (gray)          "!u"
// Normal          (black)         "!n"
// Important       (blue)          "!i"
// Warning         (brown)         "!w"
// Error           (red)           "!e"

void logMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
// void logMessageOutput(QtMsgType type, const char *msg)
{
    logMutex->lock();
    logFile->write(QString::number((quint64)QThread::currentThreadId(), 16).toLatin1());
    switch (type) {
        case QtDebugMsg:
            logFile->write(": [Debug]    ");
            break;
        case QtWarningMsg:
            logFile->write(": [Warning]  ");
            break;
        case QtCriticalMsg:
            logFile->write(": [Critical] ");
            break;
        case QtFatalMsg:
            logFile->write(": [Fatal]    ");
            break;
    }
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray displayMsg = localMsg.mid(3);
    logFile->write(displayMsg);
    logFile->write("\n");
    if (type == QtFatalMsg) {
        logFile->close();
        abort();
    }
    logMutex->unlock();

    if (msg[0] == '!') {
#ifdef QT_NO_DEBUG
        if (msg[1] == 'd') {
            return;
        }
#endif
        mainWindow->doLogMessage(localMsg.at(1), displayMsg);
    }
}

void MainWindow::doLogMessage(int type, const QString &msg)
{
    emit logMessage(type, msg);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{

    /* Setup the logging system */
    mainWindow = this;
    g_aspeQtAppPath = QCoreApplication::applicationDirPath();
    g_disablePicoHiSpeed = false;
    logFile = new QFile(QDir::temp().absoluteFilePath("aspeqt.log"));
    logFile->open(QFile::WriteOnly | QFile::Truncate | QFile::Unbuffered | QFile::Text);
    logMutex = new QMutex();
    connect(this, SIGNAL(logMessage(int,QString)), this, SLOT(uiMessage(int,QString)), Qt::QueuedConnection);
    qInstallMessageHandler(logMessageOutput);
    qDebug() << "!d" << tr("AspeQt started at %1.").arg(QDateTime::currentDateTime().toString());

    /* Remove old temporaries */
    QDir tempDir = QDir::temp();
    QStringList filters;
    filters << "aspeqt-*";
    QFileInfoList list = tempDir.entryInfoList(filters, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
    foreach(QFileInfo file, list) {
        deltree(file.absoluteFilePath());
    }
    
    // Check to see if settings repository is already migrated, if not invoke migration process
    QSettings oldSettings("TrayGun Software", "AspeQt");
    oldSettings.setFallbacksEnabled(false);
    QSettings newSettings("atari8warez.com", "AspeQt");
    QStringList oldKeys = oldSettings.allKeys();
    if(oldKeys.size()>0){
        QMessageBox::information(this, tr("Migrate Settings"), tr("This version of AspeQt uses a different repository "
                                          "for storing its global settings.\nWe will now migrate the existing "
                                          "settings to their new repository, note that settings stored in your existing "
                                          "AspeQt session files are not affected by this change."), QMessageBox::Ok);
        for (int i=0; i<oldKeys.size(); ++i) {
            newSettings.setValue(oldKeys.value(i), oldSettings.value(oldKeys.value(i)));
        }
        oldSettings.clear();
        QMessageBox::information(this, tr("Migrate Settings"), tr("Setting were migrated successfuly."), QMessageBox::Ok);
    }
    /* Set application properties */
    QCoreApplication::setOrganizationName("atari8warez.com");
    QCoreApplication::setOrganizationDomain("atari8warez.com");
    QCoreApplication::setApplicationName("AspeQt");
    aspeqtSettings = new AspeqtSettings();
       
    /* Load translators */
    loadTranslators();
    
    /* Setup UI */
    ui->setupUi(this);

    /* Parse command line arguments:
      arg(1): session file (xxxxxxxx.aspeqt)   */

    QStringList AspeQtArgs = QCoreApplication::arguments();
    g_sessionFile = g_sessionFilePath = "";
    if (AspeQtArgs.size() > 1) {
       QFile sess;
       QString s = QDir::separator();             //
       int i = AspeQtArgs.at(1).lastIndexOf(s);   //
       if (i != -1) {
           i++;
           g_sessionFile = AspeQtArgs.at(1).right(AspeQtArgs.at(1).size() - i);
           g_sessionFilePath = AspeQtArgs.at(1).left(i);
           g_sessionFilePath = QDir::fromNativeSeparators(g_sessionFilePath);
           sess.setFileName(g_sessionFilePath+g_sessionFile);
           if (!sess.exists()) {
               QMessageBox::question(this, tr("Session file error"),
               tr("Requested session file not found in the given directory path or the path is incorrect. AspeQt will continue with default session configuration."), QMessageBox::Ok);
               g_sessionFile = g_sessionFilePath = "";
           }
       } else {
           if (AspeQtArgs.at(1) != "") {
               g_sessionFile = AspeQtArgs.at(1);
               g_sessionFilePath = QDir::currentPath();
               sess.setFileName(g_sessionFile);
               if (!sess.exists()) {
                   QMessageBox::question(this, tr("Session file error"),
                   tr("Requested session file not found in the application's current directory path\n (No path was specified). AspeQt will continue with default session configuration."), QMessageBox::Ok);
                   g_sessionFile = g_sessionFilePath = "";
               }
           }
         }
    }
    // Pass Session file name, path and MainWindow title to AspeQtSettings //
    aspeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
    aspeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    // Display Session name, and restore session parameters if session file was specified //
    g_mainWindowTitle = tr("AspeQt - Atari Serial Peripheral Emulator for Qt");
    if (g_sessionFile != "") {
        setWindowTitle(g_mainWindowTitle + tr(" -- Session: ") + g_sessionFile);
        aspeqtSettings->loadSessionFromFile(g_sessionFilePath+g_sessionFile);
    } else {
        setWindowTitle(g_mainWindowTitle);
    }
    setGeometry(aspeqtSettings->lastHorizontalPos(),aspeqtSettings->lastVerticalPos(),aspeqtSettings->lastWidth(),aspeqtSettings->lastHeight());

    /* Setup status bar */
    speedLabel = new QLabel(this);
    onOffLabel = new QLabel(this);
    prtOnOffLabel = new QLabel(this);
    netLabel = new QLabel(this);
    clearMessagesLabel = new QLabel(this);
    speedLabel->setText(tr("19200 bits/sec"));
    onOffLabel->setMinimumWidth(21);
    prtOnOffLabel->setMinimumWidth(18);
    prtOnOffLabel->setPixmap(QIcon(":/icons/silk-icons/icons/printer.png").pixmap(16, 16, QIcon::Normal));  //
    prtOnOffLabel->setToolTip(ui->actionPrinterEmulation->toolTip());
    prtOnOffLabel->setStatusTip(ui->actionPrinterEmulation->statusTip());
    netLabel->setMinimumWidth(18);

    clearMessagesLabel->setMinimumWidth(21);
    clearMessagesLabel->setPixmap(QIcon(":/icons/silk-icons/icons/page_white_c.png").pixmap(16, 16, QIcon::Normal));
    clearMessagesLabel->setToolTip(tr("Clear messages"));
    clearMessagesLabel->setStatusTip(clearMessagesLabel->toolTip());

    speedLabel->setMinimumWidth(80);

    ui->statusBar->addPermanentWidget(speedLabel);
    ui->statusBar->addPermanentWidget(onOffLabel);
    ui->statusBar->addPermanentWidget(prtOnOffLabel);
    ui->statusBar->addPermanentWidget(netLabel);
    ui->statusBar->addPermanentWidget(clearMessagesLabel);
    ui->textEdit->installEventFilter(mainWindow);
    changeFonts();
    g_D9DOVisible =  aspeqtSettings->D9DOVisible();
    showHideDrives();

    /* Connect to the network */
    QString netInterface;
    Network *oNet = new Network();
    if (oNet->openConnection(netInterface)) {
        netLabel->setPixmap(QIcon(":/icons/oxygen-icons/16x16/actions/network_connect.png").pixmap(16, 16, QIcon::Normal));
        netLabel->setToolTip(tr("Connected to the network via: ") +  netInterface);
        netLabel->setStatusTip(netLabel->toolTip());
    } else {
        netLabel->setPixmap(QIcon(":/icons/oxygen-icons/16x16/actions/network_disconnect.png").pixmap(16, 16, QIcon::Normal));
        netLabel->setToolTip(tr("No network connection"));
        netLabel->setStatusTip(netLabel->toolTip());

        //        QMessageBox::information(this,tr("Network connection cannot be opened"), tr("No network interface was found!"));
    }

    /* Initialize diskWidgets array and tool button actions */
    
    for (int i = 0; i < g_numberOfDisks; i++) {      //

        diskWidgets[i].fileNameLabel = findChild <QLabel*> (QString("labelFileName_%1").arg(i + 1));
        diskWidgets[i].imagePropertiesLabel = findChild <QLabel*> (QString("labelImageProperties_%1").arg(i + 1));
        diskWidgets[i].ejectAction = findChild <QAction*> (QString("actionEject_%1").arg(i + 1));
        diskWidgets[i].writeProtectAction = findChild <QAction*> (QString("actionWriteProtect_%1").arg(i + 1));
        diskWidgets[i].editAction = findChild <QAction*> (QString("actionEditDisk_%1").arg(i + 1));
        diskWidgets[i].mountDiskAction = findChild <QAction*> (QString("actionMountDisk_%1").arg(i + 1));
        diskWidgets[i].mountFolderAction = findChild <QAction*> (QString("actionMountFolder_%1").arg(i + 1));
        diskWidgets[i].saveAction = findChild <QAction*> (QString("actionSave_%1").arg(i + 1));
        diskWidgets[i].autoSaveAction = findChild <QAction*> (QString("actionAutoSave_%1").arg(i + 1));  //
        if(i == 0)
            diskWidgets[i].bootOptionAction = findChild <QAction*> (QString("actionBootOption"));       //
        diskWidgets[i].revertAction = findChild <QAction*> (QString("actionRevert_%1").arg(i + 1));
        diskWidgets[i].saveAsAction = findChild <QAction*> (QString("actionSaveAs_%1").arg(i + 1));
        diskWidgets[i].frame = findChild <QFrame*> (QString("horizontalFrame_%1").arg(i + 1));

        if(i == 0)
            diskWidgets[i].frame->insertAction(0, diskWidgets[i].bootOptionAction); //
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].saveAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].autoSaveAction);       //
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].saveAsAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].revertAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].mountDiskAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].mountFolderAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].ejectAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].writeProtectAction);
        diskWidgets[i].frame->insertAction(0, diskWidgets[i].editAction);

        findChild <QToolButton*> (QString("buttonMountDisk_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].mountDiskAction);
        findChild <QToolButton*> (QString("buttonMountFolder_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].mountFolderAction);
        findChild <QToolButton*> (QString("buttonEject_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].ejectAction);
        findChild <QToolButton*> (QString("buttonSave_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].saveAction);
        findChild <QToolButton*> (QString("autoSave_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].autoSaveAction);  //
        findChild <QToolButton*> (QString("buttonEditDisk_%1").arg(i + 1)) -> setDefaultAction(diskWidgets[i].editAction);
    }

    /* Connect SioWorker signals */
    sio = new SioWorker();
    connect(sio, SIGNAL(started()), this, SLOT(sioStarted()));
    connect(sio, SIGNAL(finished()), this, SLOT(sioFinished()));
    connect(sio, SIGNAL(statusChanged(QString)), this, SLOT(sioStatusChanged(QString)));
    shownFirstTime = true;

    /* Restore application state */
    for (int i = 0; i < g_numberOfDisks; i++) {      //
        AspeqtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }
    updateRecentFileActions();

    setAcceptDrops(true);

    // AspeQt Client  //
    AspeCl *acl = new AspeCl(sio);
    sio->installDevice(0x46, acl);

    textPrinterWindow = new TextPrinterWindow();
    // Documentation Display
    docDisplayWindow = new DocDisplayWindow();

    connect(textPrinterWindow, SIGNAL(closed()), this, SLOT(textPrinterWindowClosed()));
    connect(docDisplayWindow, SIGNAL(closed()), this, SLOT(docDisplayWindowClosed()));

    Printer *printer = new Printer(sio);
    connect(printer, SIGNAL(print(QString)), textPrinterWindow, SLOT(print(QString)));
    sio->installDevice(0x40, printer);
    untitledName = 0;

    connect(&trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon.setIcon(windowIcon());

    // Connections needed for remotely mounting a disk image & Toggle Auto-Commit //
    connect (acl, SIGNAL(findNewSlot(int,bool)), this, SLOT(firstEmptyDiskSlot(int,bool)));
    connect (this, SIGNAL(newSlot(int)), acl, SLOT(gotNewSlot(int)));
    connect (acl, SIGNAL(mountFile(int,QString)), this, SLOT(mountFileWithDefaultProtection(int,QString)));
    connect (this, SIGNAL(fileMounted(bool)), acl, SLOT(fileMounted(bool)));
    connect (acl, SIGNAL(toggleAutoCommit(int)), this, SLOT(autoCommit(int)));
}

MainWindow::~MainWindow()
{
    if (ui->actionStartEmulation->isChecked()) {
        ui->actionStartEmulation->trigger();
    }

    delete aspeqtSettings;
    delete sio;

    delete ui;

    qDebug() << "!d" << tr("AspeQt stopped at %1.").arg(QDateTime::currentDateTime().toString());
    qInstallMessageHandler(0);
    delete logMutex;
    delete logFile;
}

 void MainWindow::mousePressEvent(QMouseEvent *event)
 {
     int slot = containingDiskSlot(event->pos());

     if (event->button() == Qt::LeftButton
         && slot >= 0) {

         QDrag *drag = new QDrag((QWidget*)this);
         QMimeData *mimeData = new QMimeData;

         mimeData->setData("application/x-aspeqt-disk-image", QByteArray(1, slot));
         drag->setMimeData(mimeData);

         drag->exec();
     }

     if (event->button() == Qt::LeftButton && onOffLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
         ui->actionStartEmulation->trigger();
     }
     if (event->button() == Qt::LeftButton && prtOnOffLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
         ui->actionPrinterEmulation->trigger();     //
     }
      if (event->button() == Qt::LeftButton && clearMessagesLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
         ui->textEdit->clear();
         emit sendLogText("");
     }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    int i = containingDiskSlot(event->pos());
    if (i >= 0 && (event->mimeData()->hasUrls() ||
                   event->mimeData()->hasFormat("application/x-aspeqt-disk-image"))) {
        event->acceptProposedAction();
    } else {
        i = -1;
    }
    for (int j = 0; j < g_numberOfDisks; j++) { //
        if (i == j) {
            diskWidgets[j].frame->setFrameShadow(QFrame::Sunken);
        } else {
            diskWidgets[j].frame->setFrameShadow(QFrame::Raised);
        }
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    int i = containingDiskSlot(event->pos());
    if (i >= 0 && (event->mimeData()->hasUrls() ||
                   event->mimeData()->hasFormat("application/x-aspeqt-disk-image"))) {
        event->acceptProposedAction();
    } else {
        i = -1;
    }
    for (int j = 0; j < g_numberOfDisks; j++) { //
        if (i == j) {
            diskWidgets[j].frame->setFrameShadow(QFrame::Sunken);
        } else {
            diskWidgets[j].frame->setFrameShadow(QFrame::Raised);
        }
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *)
{
    for (int j = 0; j < g_numberOfDisks; j++) { //
        diskWidgets[j].frame->setFrameShadow(QFrame::Raised);
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    for (int j = 0; j < g_numberOfDisks; j++) { //
        diskWidgets[j].frame->setFrameShadow(QFrame::Raised);
    }
    int slot = containingDiskSlot(event->pos());
    if (!(event->mimeData()->hasUrls() ||
          event->mimeData()->hasFormat("application/x-aspeqt-disk-image")) ||
          slot < 0) {
        return;
    }

    if (event->mimeData()->hasFormat("application/x-aspeqt-disk-image")) {
        int source = event->mimeData()->data("application/x-aspeqt-disk-image").at(0);

        if (slot == source) {
            return;
        }

        sio->swapDevices(slot + 0x31, source + 0x31);

        aspeqtSettings->swapImages(slot, source);

        qDebug() << "!n" << tr("Swapped disk %1 with disk %2.").arg(slot + 1).arg(source + 1);

        return;
    }

    QStringList files;
    foreach (QUrl url, event->mimeData()->urls()) {
        if (!url.toLocalFile().isEmpty()) {
            files.append(url.toLocalFile());
        }
    }
    if (files.isEmpty()) {
        return;
    }

    FileTypes::FileType type = FileTypes::getFileType(files.at(0));

    if (type == FileTypes::Xex) {
        g_exefileName = files.at(0);  //
        bootExe(files.at(0));
        return;
    }

    if (type == FileTypes::Cas) {
        bool restart;
        restart = ui->actionStartEmulation->isChecked();
        if (restart) {
            ui->actionStartEmulation->trigger();
            sio->wait();
            qApp->processEvents();
        }

        CassetteDialog *dlg = new CassetteDialog(this, files.at(0));
        dlg->exec();
        delete dlg;

        if (restart) {
            ui->actionStartEmulation->trigger();
        }
        return;
    }

    mountFileWithDefaultProtection(slot, files[0]);
    files.removeAt(0);
    while (!files.isEmpty() && (slot = firstEmptyDiskSlot(slot, false)) >= 0) {
        mountFileWithDefaultProtection(slot, files[0]);
        files.removeAt(0);
    }
    slot = 0;
    while (!files.isEmpty() && (slot = firstEmptyDiskSlot(slot, false)) >= 0) {
        mountFileWithDefaultProtection(slot, files[0]);
        files.removeAt(0);
    }
    foreach(QString file, files) {
        qCritical() << "!e" << tr("Cannot mount '%1': No empty disk slots.").arg(file);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Save various session settings  //
    if (aspeqtSettings->saveWindowsPos()) {
        if (g_miniMode) {
            saveMiniWindowGeometry();
        } else {
            saveWindowGeometry();
        }
    }
    if (g_sessionFile != "") aspeqtSettings->saveSessionToFile(g_sessionFilePath + "/" + g_sessionFile);
    aspeqtSettings->setD9DOVisible(g_D9DOVisible);
    bool wasRunning = ui->actionStartEmulation->isChecked();
    QMessageBox::StandardButton answer = QMessageBox::No;

    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }

    int toBeSaved = 0;

    for (int i = 0; i < g_numberOfDisks; i++) {      //
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + 0x31));
        if (img && img->isModified()) {
            toBeSaved++;
        }
    }

    for (int i = 0; i < g_numberOfDisks; i++) {      //
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + 0x31));
        if (img && img->isModified()) {
            toBeSaved--;
            answer = saveImageWhenClosing(i, answer, toBeSaved);
            if (answer == QMessageBox::NoToAll) {
                break;
            }
            if (answer == QMessageBox::Cancel) {
                if (wasRunning) {
                    ui->actionStartEmulation->trigger();
                }
                event->ignore();
                return;
            }
        }
    }

    delete textPrinterWindow;
    //
    delete docDisplayWindow;

    for (int i = 0x31; i < 0x39; i++) {
        SimpleDiskImage *s = qobject_cast <SimpleDiskImage*> (sio->getDevice(i));
        if (s) {
            s->close();
        }
    }

    event->accept();

}

void MainWindow::hideEvent(QHideEvent *event)
{
    if (aspeqtSettings->minimizeToTray()) {
        trayIcon.show();
        oldWindowFlags = windowFlags();
        oldWindowStates = windowState();
        setWindowFlags(Qt::Widget);
        hide();
        event->ignore();
        return;
    }
    QMainWindow::hideEvent(event);
}

void MainWindow::show()
{
    QMainWindow::show();
    if (shownFirstTime) {
        /* Open options dialog if it's the first time */
        if (aspeqtSettings->isFirstTime()) {
            if (QMessageBox::Yes == QMessageBox::question(this, tr("First run"),
                                       tr("You are running AspeQt for the first time.\n\nDo you want to open the options dialog?"),
                                       QMessageBox::Yes, QMessageBox::No)) {
                ui->actionOptions->trigger();
            }
        }
        qDebug() << "!d" << "Starting emulation";

        ui->actionStartEmulation->trigger();
    }
}
void MainWindow::enterEvent(QEvent *)
{
    if (g_miniMode && g_shadeMode) {
       setWindowOpacity(1.0);
    }
}
void MainWindow::leaveEvent(QEvent *)
{
    if (g_miniMode && g_shadeMode) {
       setWindowOpacity(0.25);
    }
}
void MainWindow::resizeEvent(QResizeEvent *)
{
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        on_actionLogWindow_triggered();
        return false;
    }
    return true;
}
void MainWindow::on_actionLogWindow_triggered()
{
    if (g_logOpen == false) {
        g_logOpen = true;
        QDialog *ldd = new LogDisplayDialog(this);
        int x, y, w, h;
        x = geometry().x();
        y = geometry().y();
        w = geometry().width();
        h = geometry().height();
        if (!g_miniMode) {
            ldd->setGeometry(x+w/1.9, y+30, ldd->geometry().width(), geometry().height());
        } else {
            ldd->setGeometry(x+20, y+60, w, h*2);
        }
        connect(this, SIGNAL(sendLogText(QString)), ldd, SLOT(getLogText(QString)));
        connect(this, SIGNAL(sendLogTextChange(QString)), ldd, SLOT(getLogTextChange(QString)));
        emit sendLogText(ui->textEdit->toHtml());
        ldd->show();
    }
}
void MainWindow::logChanged(QString text)
{
    emit sendLogTextChange(text);

}

void MainWindow::saveWindowGeometry()
{
    aspeqtSettings->setLastHorizontalPos(geometry().x());
    aspeqtSettings->setLastVerticalPos(geometry().y());
    aspeqtSettings->setLastWidth(geometry().width());
    aspeqtSettings->setLastHeight(geometry().height());
}

void MainWindow::saveMiniWindowGeometry()
{
    aspeqtSettings->setLastMiniHorizontalPos(geometry().x());
    aspeqtSettings->setLastMiniVerticalPos(geometry().y());
}

void MainWindow::on_actionToggleShade_triggered()
{
    if (g_shadeMode) {
        setWindowFlags(Qt::WindowSystemMenuHint);
        setWindowOpacity(1.0);
        g_shadeMode = false;
        QMainWindow::show();
    } else {
        setWindowFlags(Qt::FramelessWindowHint);
        setWindowOpacity(0.25);
        g_shadeMode = true;
        QMainWindow::show();
    }
}

// Toggle Mini Mode //
void MainWindow::on_actionToggleMiniMode_triggered()
{
    if(g_miniMode){
        ui->horizontalFrame_2->setVisible(true);
        ui->horizontalFrame_3->setVisible(true);
        ui->horizontalFrame_4->setVisible(true);
        ui->horizontalFrame_5->setVisible(true);
        ui->horizontalFrame_6->setVisible(true);
        ui->horizontalFrame_7->setVisible(true);
        ui->horizontalFrame_8->setVisible(true);
        if (g_D9DOVisible) {
            ui->horizontalFrame_9->setVisible(true);
            ui->horizontalFrame_10->setVisible(true);
            ui->horizontalFrame_11->setVisible(true);
            ui->horizontalFrame_12->setVisible(true);
            ui->horizontalFrame_13->setVisible(true);
            ui->horizontalFrame_14->setVisible(true);
            ui->horizontalFrame_15->setVisible(true);
            setMinimumWidth(688);
        } else {
            setMinimumWidth(344);
        }
        setMinimumHeight(426);
        setMaximumHeight(QWIDGETSIZE_MAX);
        ui->textEdit->setVisible(true);
        ui->actionHideShowDrives->setEnabled(true);
        saveMiniWindowGeometry();
        setGeometry(g_savedGeometry);
        setWindowOpacity(1.0);
        setWindowFlags(Qt::WindowSystemMenuHint);
        ui->actionToggleShade->setDisabled(true);
        QMainWindow::show();
        g_miniMode = false;
        g_shadeMode = false;
    } else {
        g_savedGeometry = geometry();
        ui->horizontalFrame_2->setVisible(false);
        ui->horizontalFrame_3->setVisible(false);
        ui->horizontalFrame_4->setVisible(false);
        ui->horizontalFrame_5->setVisible(false);
        ui->horizontalFrame_6->setVisible(false);
        ui->horizontalFrame_7->setVisible(false);
        ui->horizontalFrame_8->setVisible(false);
        ui->horizontalFrame_9->setVisible(false);
        ui->horizontalFrame_10->setVisible(false);
        ui->horizontalFrame_11->setVisible(false);
        ui->horizontalFrame_12->setVisible(false);
        ui->horizontalFrame_13->setVisible(false);
        ui->horizontalFrame_14->setVisible(false);
        ui->horizontalFrame_15->setVisible(false);
        ui->textEdit->setVisible(false);
        setMinimumWidth(400);
        setMinimumHeight(100);
        setMaximumHeight(100);
        setGeometry(aspeqtSettings->lastMiniHorizontalPos(), aspeqtSettings->lastMiniVerticalPos(),
                    minimumWidth(), minimumHeight());
        ui->actionHideShowDrives->setDisabled(true);
        ui->actionToggleShade->setEnabled(true);
        if (aspeqtSettings->enableShade()) {
            setWindowOpacity(0.25);
            setWindowFlags(Qt::FramelessWindowHint);
            g_shadeMode = true;
        } else {
            g_shadeMode = false;
        }
        QMainWindow::show();
        g_miniMode = true;
    }
}
void MainWindow::showHideDrives()
{
    if(!g_D9DOVisible) {
        ui->actionHideShowDrives->setText(QApplication::translate("MainWindow", "Show drives D9-DO", 0));
        ui->actionHideShowDrives->setStatusTip(QApplication::translate("MainWindow", "Show drives D9-DO", 0));
        ui->actionHideShowDrives->setIcon(QIcon(":/icons/silk-icons/icons/drive_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
     }
        ui->horizontalFrame_9->setVisible(g_D9DOVisible);
        ui->horizontalFrame_10->setVisible(g_D9DOVisible);
        ui->horizontalFrame_11->setVisible(g_D9DOVisible);
        ui->horizontalFrame_12->setVisible(g_D9DOVisible);
        ui->horizontalFrame_13->setVisible(g_D9DOVisible);
        ui->horizontalFrame_14->setVisible(g_D9DOVisible);
        ui->horizontalFrame_15->setVisible(g_D9DOVisible);
}

// Toggle Hide/Show drives D9-DO  //
void MainWindow::on_actionHideShowDrives_triggered()
{

    if(g_D9DOVisible) {
        ui->actionHideShowDrives->setText(QApplication::translate("MainWindow", "Show drives D9-DO", 0));
        ui->actionHideShowDrives->setStatusTip(QApplication::translate("MainWindow", "Show drives D9-DO", 0));
        ui->actionHideShowDrives->setIcon(QIcon(":/icons/silk-icons/icons/drive_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        ui->horizontalFrame_9->setVisible(false);
        ui->horizontalFrame_10->setVisible(false);
        ui->horizontalFrame_11->setVisible(false);
        ui->horizontalFrame_12->setVisible(false);
        ui->horizontalFrame_13->setVisible(false);
        ui->horizontalFrame_14->setVisible(false);
        ui->horizontalFrame_15->setVisible(false);
        setMinimumWidth(344);
        setGeometry(geometry().x(), geometry().y(), 0, geometry().height());
        saveWindowGeometry();
        g_D9DOVisible = false;
     } else {
        ui->actionHideShowDrives->setText(QApplication::translate("MainWindow", "Hide drives D9-DO", 0));
        ui->actionHideShowDrives->setStatusTip(QApplication::translate("MainWindow", "Hide drives D9-DO", 0));
        ui->actionHideShowDrives->setIcon(QIcon(":/icons/silk-icons/icons/drive_add.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        ui->horizontalFrame_9->setVisible(true);
        ui->horizontalFrame_10->setVisible(true);
        ui->horizontalFrame_11->setVisible(true);
        ui->horizontalFrame_12->setVisible(true);
        ui->horizontalFrame_13->setVisible(true);
        ui->horizontalFrame_14->setVisible(true);
        ui->horizontalFrame_15->setVisible(true);;
        setMinimumWidth(688);
        setGeometry(geometry().x(), geometry().y(), 0, geometry().height());
        g_D9DOVisible = true;
      }
      g_miniMode = false;
}

// Toggle printer Emulation ON/OFF //
void MainWindow::on_actionPrinterEmulation_triggered()
{
    if (g_printerEmu) {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/silk-icons/icons/printer_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setPixmap(QIcon(":/icons/silk-icons/icons/printer_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setToolTip(tr("Start printer emulation"));
        prtOnOffLabel->setStatusTip(prtOnOffLabel->toolTip());
        g_printerEmu = false;
        qWarning() << "!i" << tr("Printer emulation stopped.");
    } else {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/silk-icons/icons/printer.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setPixmap(QIcon(":/icons/silk-icons/icons/printer.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setToolTip(tr("Stop printer emulation"));
        prtOnOffLabel->setStatusTip(prtOnOffLabel->toolTip());
        g_printerEmu = true;
        qWarning() << "!i" << tr("Printer emulation started.");
    }
}

void MainWindow::on_actionStartEmulation_triggered()
{
    if (ui->actionStartEmulation->isChecked()) {
        sio->start(QThread::TimeCriticalPriority);
    } else {
        sio->setPriority(QThread::NormalPriority);
        sio->wait();
        qApp->processEvents();
    }
}

void MainWindow::sioStarted()
{
    ui->actionStartEmulation->setText(tr("&Stop emulation"));
    ui->actionStartEmulation->setToolTip(tr("Stop SIO peripheral emulation"));
    ui->actionStartEmulation->setStatusTip(tr("Stop SIO peripheral emulation"));
    onOffLabel->setPixmap(ui->actionStartEmulation->icon().pixmap(16, QIcon::Normal, QIcon::On));
    onOffLabel->setToolTip(ui->actionStartEmulation->toolTip());
    onOffLabel->setStatusTip(ui->actionStartEmulation->statusTip());
}

void MainWindow::sioFinished()
{
    ui->actionStartEmulation->setText(tr("&Start emulation"));
    ui->actionStartEmulation->setToolTip(tr("Start SIO peripheral emulation"));
    ui->actionStartEmulation->setStatusTip(tr("Start SIO peripheral emulation"));
    ui->actionStartEmulation->setChecked(false);
    onOffLabel->setPixmap(ui->actionStartEmulation->icon().pixmap(16, QIcon::Normal, QIcon::Off));
    onOffLabel->setToolTip(ui->actionStartEmulation->toolTip());
    onOffLabel->setStatusTip(ui->actionStartEmulation->statusTip());
    speedLabel->hide();
    speedLabel->clear();
    qWarning() << "!i" << tr("Emulation stopped.");
}

void MainWindow::sioStatusChanged(QString status)
{
    speedLabel->setText(status);
    speedLabel->show();
}

void MainWindow::deviceStatusChanged(int deviceNo)
{
    if (deviceNo >= 0x31 && deviceNo <= 0x3F) { //
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo));
        if (img) {

            // Show file name without the path and set toolTip & statusTip to show the path separately //
            QString filenamelabel;
            int i;
            if (img->description() == tr("Folder image")) {
                i = img->originalFileName().lastIndexOf("\\");
            } else {
                i = img->originalFileName().lastIndexOf("/");
            }
               if (i == -1) {
                   i = img->originalFileName().lastIndexOf("/");
               }
            if ((i != -1) || (img->originalFileName().mid(0, 14) == "Untitled image")) {
                filenamelabel = img->originalFileName().right(img->originalFileName().size() - ++i);
            } else {
                filenamelabel = "!!!!!!!!.!!!";
                }

            findChild <QLabel*> (QString("labelFileName_%1").arg(deviceNo - 0x31 + 1))->setToolTip(img->originalFileName().left(i - 1));
            findChild <QLabel*> (QString("labelFileName_%1").arg(deviceNo - 0x31 + 1))->setStatusTip(img->originalFileName());
            findChild <QLabel*> (QString("labelImageProperties_%1").arg(deviceNo - 0x31 + 1))->setToolTip(img->description());

            diskWidgets[deviceNo - 0x31].fileNameLabel->setText(filenamelabel);
            diskWidgets[deviceNo - 0x31].imagePropertiesLabel->setText(img->description());
            diskWidgets[deviceNo - 0x31].ejectAction->setEnabled(true);
            diskWidgets[deviceNo - 0x31].editAction->setChecked(img->editDialog() != 0);
            if (img->description() == tr("Folder image")) {
                diskWidgets[deviceNo - 0x31].fileNameLabel->setStyleSheet("color: rgb(54, 168, 164); font-weight: bold");
                diskWidgets[deviceNo - 0x31].editAction->setEnabled(true);              //
                diskWidgets[deviceNo - 0x31].saveAsAction->setEnabled(false);
                diskWidgets[deviceNo - 0x31].saveAction->setEnabled(false);
                diskWidgets[deviceNo - 0x31].autoSaveAction->setEnabled(false);         //
                diskWidgets[deviceNo - 0x31].revertAction->setEnabled(false);
                if(deviceNo - 0x31 == 0)
                    diskWidgets[deviceNo - 0x31].bootOptionAction->setEnabled(true);   //
            } else {
                diskWidgets[deviceNo - 0x31].fileNameLabel->setStyleSheet("color: rgb(0, 0, 0); font-weight: normal");  //
                diskWidgets[deviceNo - 0x31].editAction->setEnabled(true);
                diskWidgets[deviceNo - 0x31].saveAsAction->setEnabled(true);
                diskWidgets[deviceNo - 0x31].autoSaveAction->setEnabled(true);          //
                if(deviceNo - 0x31 == 0)
                    diskWidgets[deviceNo - 0x31].bootOptionAction->setEnabled(false);   //

                if (img->isModified()) {
                    if (!diskWidgets[deviceNo - 0x31].autoSaveAction->isChecked()) {    //
                        diskWidgets[deviceNo - 0x31].saveAction->setEnabled(true);
                        diskWidgets[deviceNo - 0x31].revertAction->setEnabled(true);
                    // Image is modified and autosave is checked, so save the image (no need to lock it)  //
                    } else {
                        bool saved;
                        saved = img->save();
                        if (!saved) {
                            if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
                                .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                                saveDiskAs(deviceNo);
                            }
                        } else {
                            diskWidgets[deviceNo - 0x31].saveAction->setEnabled(false);
                            diskWidgets[deviceNo - 0x31].revertAction->setEnabled(false);
                        }
                    }
                } else {
                    diskWidgets[deviceNo - 0x31].saveAction->setEnabled(false);
                    diskWidgets[deviceNo - 0x31].revertAction->setEnabled(false);
                }
            }
        } else {
            diskWidgets[deviceNo - 0x31].saveAction->setEnabled(false);
            diskWidgets[deviceNo - 0x31].fileNameLabel->clear();
            diskWidgets[deviceNo - 0x31].imagePropertiesLabel->clear();
            diskWidgets[deviceNo - 0x31].ejectAction->setEnabled(false);
            diskWidgets[deviceNo - 0x31].revertAction->setEnabled(false);
            diskWidgets[deviceNo - 0x31].saveAsAction->setEnabled(false);
            diskWidgets[deviceNo - 0x31].editAction->setEnabled(false);
            diskWidgets[deviceNo - 0x31].editAction->setChecked(false);
            diskWidgets[deviceNo - 0x31].autoSaveAction->setEnabled(false);             //
            diskWidgets[deviceNo - 0x31].autoSaveAction->setChecked(false);             //
            if(deviceNo - 0x31 == 0)
                diskWidgets[deviceNo - 0x31].bootOptionAction->setEnabled(false);             //

        }
    }
}

void MainWindow::uiMessage(int t, QString message)
{
    if (message.at(0) == '"') {
        message.remove(0, 1);
    }
    if (message.at(message.count() - 1) == ' ' && message.at(message.count() - 2) == '"') {
        message.resize(message.count() - 2);
    }

    if (message == lastMessage) {
        lastMessageRepeat++;
        message = QString("%1 [x%2]").arg(message).arg(lastMessageRepeat);
        ui->textEdit->moveCursor(QTextCursor::End);
        QTextCursor cursor = ui->textEdit->textCursor();
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
    } else {
        lastMessage = message;
        lastMessageRepeat = 1;
    }

    ui->statusBar->showMessage(message, 3000);

    switch (t) {
        case 'd':
            message = QString("<span style='color:green'>%1</span>").arg(message);
            break;
        case 'u':
            message = QString("<span style='color:gray'>%1</span>").arg(message);
            break;
        case 'n':
            message = QString("<span style='color:black'>%1</span>").arg(message);
            break;
        case 'i':
            message = QString("<span style='color:blue'>%1</span>").arg(message);
            break;
        case 'w':
            message = QString("<span style='color:brown'>%1</span>").arg(message);
            break;
        case 'e':
            message = QString("<span style='color:red'>%1</span>").arg(message);
            break;
        default:
            message = QString("<span style='color:purple'>%1</span>").arg(message);
            break;
    }

    ui->textEdit->append(message);
    ui->textEdit->verticalScrollBar()->setSliderPosition(ui->textEdit->verticalScrollBar()->maximum());
    ui->textEdit->horizontalScrollBar()->setSliderPosition(ui->textEdit->horizontalScrollBar()->minimum());
    logChanged(message);
}

void MainWindow::on_actionOptions_triggered()
{
    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
        sio->wait();
        qApp->processEvents();
    }
    OptionsDialog optionsDialog(this);
    optionsDialog.exec() ;

// Change drive slot description fonts
    changeFonts();

// load translators and retranslate
    loadTranslators();

// retranslate Designer Form
    ui->retranslateUi(this);

    for (int i = 0x31; i <= 0x3F; i++) {    //
        deviceStatusChanged(i);
    }
    
    ui->actionStartEmulation->trigger();
}

void MainWindow::changeFonts()
{
    if (aspeqtSettings->useLargeFont()) {
        QFont font("Arial Black", 9, QFont::Normal);
        ui->labelFileName_1->setFont(font);
        ui->labelFileName_2->setFont(font);
        ui->labelFileName_3->setFont(font);
        ui->labelFileName_4->setFont(font);
        ui->labelFileName_5->setFont(font);
        ui->labelFileName_6->setFont(font);
        ui->labelFileName_7->setFont(font);
        ui->labelFileName_8->setFont(font);
        ui->labelFileName_9->setFont(font);
        ui->labelFileName_10->setFont(font);
        ui->labelFileName_11->setFont(font);
        ui->labelFileName_12->setFont(font);
        ui->labelFileName_13->setFont(font);
        ui->labelFileName_14->setFont(font);
        ui->labelFileName_15->setFont(font);
} else {
        QFont font("MS Shell Dlg 2,8", 8, QFont::Normal);
        ui->labelFileName_1->setFont(font);
        ui->labelFileName_2->setFont(font);
        ui->labelFileName_3->setFont(font);
        ui->labelFileName_4->setFont(font);
        ui->labelFileName_5->setFont(font);
        ui->labelFileName_6->setFont(font);
        ui->labelFileName_7->setFont(font);
        ui->labelFileName_8->setFont(font);
        ui->labelFileName_9->setFont(font);
        ui->labelFileName_10->setFont(font);
        ui->labelFileName_11->setFont(font);
        ui->labelFileName_12->setFont(font);
        ui->labelFileName_13->setFont(font);
        ui->labelFileName_14->setFont(font);
        ui->labelFileName_15->setFont(font);

    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog aboutDialog(this, VERSION);
    aboutDialog.exec();
}
//
void MainWindow::on_actionDocumentation_triggered()
{
    QString dir = aspeqtSettings->lastSessionDir();

    if (ui->actionDocumentation->isChecked()) {
        docDisplayWindow->show();
    } else {
        docDisplayWindow->hide();
    }
}
//
void MainWindow::docDisplayWindowClosed()
{
     ui->actionDocumentation->setChecked(false);
}
// Restart emulation and re-translate following a session load //
void MainWindow::setSession()
{
    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
       sio->wait();
        qApp->processEvents();
    }

    // load translators and retranslate
    loadTranslators();
    ui->retranslateUi(this);
    for (int i = 0x31; i <= 0x3F; i++) {
        deviceStatusChanged(i);
    }

    ui->actionStartEmulation->trigger();
}
void MainWindow::updateRecentFileActions()
{
    ui->actionMountRecent_0->setText(aspeqtSettings->recentImageSetting(0).fileName);
    ui->actionMountRecent_1->setText(aspeqtSettings->recentImageSetting(1).fileName);
    ui->actionMountRecent_2->setText(aspeqtSettings->recentImageSetting(2).fileName);
    ui->actionMountRecent_3->setText(aspeqtSettings->recentImageSetting(3).fileName);
    ui->actionMountRecent_4->setText(aspeqtSettings->recentImageSetting(4).fileName);
    ui->actionMountRecent_5->setText(aspeqtSettings->recentImageSetting(5).fileName);
    ui->actionMountRecent_6->setText(aspeqtSettings->recentImageSetting(6).fileName);
    ui->actionMountRecent_7->setText(aspeqtSettings->recentImageSetting(7).fileName);
    ui->actionMountRecent_8->setText(aspeqtSettings->recentImageSetting(8).fileName);
    ui->actionMountRecent_9->setText(aspeqtSettings->recentImageSetting(9).fileName);

    ui->actionMountRecent_0->setVisible(!ui->actionMountRecent_0->text().isEmpty());
    ui->actionMountRecent_1->setVisible(!ui->actionMountRecent_1->text().isEmpty());
    ui->actionMountRecent_2->setVisible(!ui->actionMountRecent_2->text().isEmpty());
    ui->actionMountRecent_3->setVisible(!ui->actionMountRecent_3->text().isEmpty());
    ui->actionMountRecent_4->setVisible(!ui->actionMountRecent_4->text().isEmpty());
    ui->actionMountRecent_5->setVisible(!ui->actionMountRecent_5->text().isEmpty());
    ui->actionMountRecent_6->setVisible(!ui->actionMountRecent_6->text().isEmpty());
    ui->actionMountRecent_7->setVisible(!ui->actionMountRecent_7->text().isEmpty());
    ui->actionMountRecent_8->setVisible(!ui->actionMountRecent_8->text().isEmpty());
    ui->actionMountRecent_9->setVisible(!ui->actionMountRecent_9->text().isEmpty());
}


bool MainWindow::ejectImage(int no, bool ask)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));

    if (ask && img && img->isModified()) {
        QMessageBox::StandardButton answer;
        answer = saveImageWhenClosing(no, QMessageBox::No, 0);
        if (answer == QMessageBox::Cancel) {
            return false;
        }
    }

    sio->uninstallDevice(no + 0x31);
    if (!img) {
        return true;
    }
    delete img;
    diskWidgets[no].ejectAction->setEnabled(false);
    QString fileName = diskWidgets[no].fileNameLabel->text();
    diskWidgets[no].fileNameLabel->clear();
    diskWidgets[no].writeProtectAction->setChecked(false);
    diskWidgets[no].writeProtectAction->setEnabled(false);
    diskWidgets[no].editAction->setEnabled(false);

    aspeqtSettings->unmountImage(no);
    updateRecentFileActions();
    deviceStatusChanged(no + 0x31);
    qDebug() << "!n" << tr("Unmounted disk %1").arg(no + 1);
    return true;
}

int MainWindow::containingDiskSlot(const QPoint &point)
{
    int i;
    QPoint distance = centralWidget()->geometry().topLeft();
    for (i=0; i < g_numberOfDisks; i++) {    //
        QRect rect = diskWidgets[i].frame->geometry().translated(distance);
        if (rect.contains(point)) {
            break;
        }
    }
    if (i > g_numberOfDisks-1) {   //
        i = -1;
    }
    return i;
}

int MainWindow::firstEmptyDiskSlot(int startFrom, bool createOne)
{
    int i;
    for (i = startFrom; i < g_numberOfDisks; i++) {  //
        if (!sio->getDevice(0x31 + i)) {
            break;
        }
    }
    if (i > g_numberOfDisks-1) {   //
        if (createOne) {
            i = g_numberOfDisks-1;
        } else {
            i = -1;
        }
    }
    emit newSlot(i);        //
    return i;
}

void MainWindow::bootExe(const QString &fileName)
{
    SioDevice *old = sio->getDevice(0x31);
    AutoBoot loader(sio, old);    
    AutoBootDialog dlg(this);
    if (!loader.open(fileName, aspeqtSettings->useHighSpeedExeLoader())) {
        return;
    }

    sio->uninstallDevice(0x31);
    sio->installDevice(0x31, &loader);
    connect(&loader, SIGNAL(booterStarted()), &dlg, SLOT(booterStarted()));
    connect(&loader, SIGNAL(booterLoaded()), &dlg, SLOT(booterLoaded()));
    connect(&loader, SIGNAL(blockRead(int, int)), &dlg, SLOT(blockRead(int, int)));
    connect(&loader, SIGNAL(loaderDone()), &dlg, SLOT(loaderDone()));
    connect(&dlg, SIGNAL(keepOpen()), this, SLOT(keepBootExeOpen()));

    dlg.exec();

    sio->uninstallDevice(0x31);
    if (old) {
        sio->installDevice(0x31, old);
        SimpleDiskImage *d = qobject_cast <SimpleDiskImage*> (old);
        d = qobject_cast <SimpleDiskImage*> (sio->getDevice(0x31));
    }
    if(!g_exefileName.isEmpty()) bootExe(g_exefileName);
}
// Make boot executable dialog persistant until it's manually closed //
void MainWindow::keepBootExeOpen()
{
    bootExe(g_exefileName);
}

void MainWindow::mountFileWithDefaultProtection(int no, const QString &fileName)
{
    // If fileName was passed from AspeCL it is an 8.1 name, so we need to find
    // the full PC name in order to validate it.  //
    QString atariFileName, atariLongName, path;

    g_aspeclFileName = fileName;
    atariFileName = fileName;

    if(atariFileName.left(1) == "*") {
        FolderImage fi(sio);
        atariFileName = atariFileName.mid(1);
        path = aspeqtSettings->lastFolderImageDir();
        atariLongName = fi.longName(path, atariFileName);
        if(atariLongName == "") {
            sio->port()->writeDataNak();
            return;
        } else {
            atariFileName =  path + "/" + atariLongName;
        }
     }

    bool prot = aspeqtSettings->getImageSettingsFromName(atariFileName).isWriteProtected;
    mountFile(no, atariFileName, prot);
}

void MainWindow::mountFile(int no, const QString &fileName, bool /*prot*/)
{
    SimpleDiskImage *disk;
    bool isDir = false;

    if (fileName.isEmpty()) {
        if(g_aspeclFileName.left(1) == "*") emit fileMounted(false);  //
        return;
    }

    FileTypes::FileType type = FileTypes::getFileType(fileName);

    if (type == FileTypes::Dir) {
        disk = new FolderImage(sio);
        isDir = true;
    } else if (type == FileTypes::Pro || type == FileTypes::ProGz) {
        disk = new DiskImagePro(sio);
//    } else if (type == FileTypes::Atx || type == FileTypes::AtxGz) {

    } else {
        disk = new SimpleDiskImage(sio);
    }

    if (disk) {
        if (!disk->open(fileName, type)) {
            aspeqtSettings->unmountImage(no);
            delete disk;
            if(g_aspeclFileName.left(1) == "*") emit fileMounted(false);  //
            return;
        }
        if (!ejectImage(no)) {
            aspeqtSettings->unmountImage(no);
            delete disk;
            if(g_aspeclFileName.left(1) == "*") emit fileMounted(false);  //
            return;
        }

        sio->installDevice(0x31 + no, disk);
        diskWidgets[no].ejectAction->setEnabled(true);
        diskWidgets[no].editAction->setEnabled(true);
        diskWidgets[no].writeProtectAction->setChecked(disk->isReadOnly());
        diskWidgets[no].writeProtectAction->setEnabled(!disk->isUnmodifiable());

        diskWidgets[no].fileNameLabel->setText(fileName);

        aspeqtSettings->mountImage(no, fileName, disk->isReadOnly());
        updateRecentFileActions();
        connect(disk, SIGNAL(statusChanged(int)), this, SLOT(deviceStatusChanged(int)), Qt::QueuedConnection);
        deviceStatusChanged(0x31 + no);

        // Extract the file name without the path //
        QString filenamelabel;
        int i = fileName.lastIndexOf("/");
        if (i != -1) {
            i++;
            filenamelabel = fileName.right(fileName.size() - i);
        }

        qDebug() << "!n" << tr("[%1] Mounted '%2' as '%3'.")
                .arg(disk->deviceName())
                .arg(filenamelabel)
                .arg(disk->description());
        if(g_aspeclFileName.left(1) == "*") emit fileMounted(true);  //

    }
}

void MainWindow::mountDiskImage(int no)
{
    QString dir;
// Always mount from "last image dir" //
//    if (diskWidgets[no].fileNameLabel->text().isEmpty()) {
        dir = aspeqtSettings->lastDiskImageDir();
//    } else {
//        dir = QFileInfo(diskWidgets[no].fileNameLabel->text()).absolutePath();
//    }
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open a disk image"),
                                                    dir,
                                                    tr(
//                                                    "All Atari disk images (*.atr *.xfd *.atx *.pro);;"
                                                    "All Atari disk images (*.atr *.xfd *.pro);;"
                                                    "SIO2PC ATR images (*.atr);;"
                                                    "XFormer XFD images (*.xfd);;"
//                                                    "ATX images (*.atx);;"
                                                    "Pro images (*.pro);;"
                                                    "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
    mountFileWithDefaultProtection(no, fileName);
}

void MainWindow::mountFolderImage(int no)
{
    QString dir;
// Always mount from "last folder dir" //
    dir = aspeqtSettings->lastFolderImageDir();
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Open a folder image"), dir);
    fileName = QDir::fromNativeSeparators(fileName);    //
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastFolderImageDir(fileName);
    mountFileWithDefaultProtection(no, fileName);
}

void MainWindow::toggleWriteProtection(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (diskWidgets[no].writeProtectAction->isChecked()) {
        img->setReadOnly(true);
    } else {
        img->setReadOnly(false);
    }
    aspeqtSettings->setMountedImageSetting(no, diskWidgets[no].fileNameLabel->text(), diskWidgets[no].writeProtectAction->isChecked());
}

void MainWindow::openEditor(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (img->editDialog()) {
        img->editDialog()->close();
    } else {
        DiskEditDialog *dlg = new DiskEditDialog();
        dlg->go(img);
        dlg->show();
    }
}

QMessageBox::StandardButton MainWindow::saveImageWhenClosing(int no, QMessageBox::StandardButton previousAnswer, int number)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));

    if (previousAnswer != QMessageBox::YesToAll) {
        QMessageBox::StandardButtons buttons;
        if (number) {
            buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll | QMessageBox::NoToAll | QMessageBox::Cancel;
        } else {
            buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        }
        previousAnswer = QMessageBox::question(this, tr("Image file unsaved"), tr("'%1' has unsaved changes, do you want to save it?")
                                       .arg(img->originalFileName()), buttons);
    }
    if (previousAnswer == QMessageBox::Yes || previousAnswer == QMessageBox::YesToAll) {
        saveDisk(no);
    }
    if (previousAnswer == QMessageBox::Close) {
        previousAnswer = QMessageBox::Cancel;
    }
    return previousAnswer;
}

void MainWindow::loadTranslators()
{
    qApp->removeTranslator(&aspeqt_qt_translator);
    qApp->removeTranslator(&aspeqt_translator);
    if (aspeqtSettings->i18nLanguage().compare("auto") == 0) {
        QString locale = QLocale::system().name();
        aspeqt_translator.load(":/translations/i18n/aspeqt_" + locale);
        aspeqt_qt_translator.load(":/translations/i18n/qt_" + locale);
        qApp->installTranslator(&aspeqt_qt_translator);
        qApp->installTranslator(&aspeqt_translator);
    } else if (aspeqtSettings->i18nLanguage().compare("en") != 0) {
        aspeqt_translator.load(":/translations/i18n/aspeqt_" + aspeqtSettings->i18nLanguage());
        aspeqt_qt_translator.load(":/translations/i18n/qt_" + aspeqtSettings->i18nLanguage());
        qApp->installTranslator(&aspeqt_qt_translator);
        qApp->installTranslator(&aspeqt_translator);
    }
}

void MainWindow::saveDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));

    if (img->isUnnamed()) {
        saveDiskAs(no);
        return;
    }

    bool saved;

    img->lock();
    saved = img-> save();
    img->unlock();
    if (!saved) {
        if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            saveDiskAs(no);
        }
    }
}
//
void MainWindow::autoCommit(int no)
{
    switch (no)
    {
        case 0 :
        {
            if(ui->autoSave_1->isEnabled()) ui->autoSave_1->click();

        }
        break;
        case 1 :
        {
            if(ui->autoSave_2->isEnabled()) ui->autoSave_2->click();

        }
        break;
        case 2 :
        {
            if(ui->autoSave_3->isEnabled()) ui->autoSave_3->click();

        }
        break;
        case 3 :
        {
            if(ui->autoSave_4->isEnabled()) ui->autoSave_4->click();

        }
        break;
        case 4 :
        {
            if(ui->autoSave_5->isEnabled()) ui->autoSave_5->click();

        }
        break;
        case 5 :
        {
            if(ui->autoSave_6->isEnabled()) ui->autoSave_6->click();

        }
        break;
        case 6 :
        {
            if(ui->autoSave_7->isEnabled()) ui->autoSave_7->click();

        }
        break;
        case 7 :
        {
            if(ui->autoSave_8->isEnabled()) ui->autoSave_8->click();

        }
        break;
        case 8 :
        {
            if(ui->autoSave_9->isEnabled()) ui->autoSave_9->click();

        }
        break;
        case 9 :
        {
            if(ui->autoSave_10->isEnabled()) ui->autoSave_10->click();

        }
        break;
        case 10 :
        {
            if(ui->autoSave_11->isEnabled()) ui->autoSave_11->click();

        }
        break;
        case 11 :
        {
            if(ui->autoSave_12->isEnabled()) ui->autoSave_12->click();

        }
        break;
        case 12 :
        {
            if(ui->autoSave_13->isEnabled()) ui->autoSave_13->click();

        }
        break;
        case 13 :
        {
            if(ui->autoSave_14->isEnabled()) ui->autoSave_14->click();

        }
        break;
        case 14 :
        {
            if(ui->autoSave_15->isEnabled()) ui->autoSave_15->click();

        }
        break;
    }
}

void MainWindow::autoSaveDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));

    if (img->isUnnamed()) {
        saveDiskAs(no);
        diskWidgets[no].saveAction->setEnabled(false);
        diskWidgets[no].revertAction->setEnabled(false);
        return;
    }
    switch (no) {
        case 0 :
            if (ui->autoSave_1->isChecked()) {
                qDebug() << "!n" << tr("[Disk 1] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 1] Auto-commit OFF.");
              }
            break;

        case 1 :
            if (ui->autoSave_2->isChecked()) {
                qDebug() << "!n" << tr("[Disk 2] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 2] Auto-commit OFF.");
              }
            break;

        case 2 :
            if (ui->autoSave_3->isChecked()) {
                qDebug() << "!n" << tr("[Disk 3] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 3] Auto-commit OFF.");
              }
            break;

        case 3 :
            if (ui->autoSave_4->isChecked()) {
                qDebug() << "!n" << tr("[Disk 4] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 4] Auto-commit OFF.");
              }
            break;

        case 4 :
            if (ui->autoSave_5->isChecked()) {
                qDebug() << "!n" << tr("[Disk 5] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 5] Auto-commit OFF.");
              }
            break;

        case 5 :
            if (ui->autoSave_6->isChecked()) {
                qDebug() << "!n" << tr("[Disk 6] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 6] Auto-commit OFF.");
              }
            break;

        case 6 :
            if (ui->autoSave_7->isChecked()) {
                qDebug() << "!n" << tr("[Disk 7] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 7] Auto-commit OFF.");
              }
            break;

        case 7 :
            if (ui->autoSave_8->isChecked()) {
                qDebug() << "!n" << tr("[Disk 8] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 8] Auto-commit OFF.");
              }
            break;
        case 8 :
            if (ui->autoSave_9->isChecked()) {
                qDebug() << "!n" << tr("[Disk 9] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 9] Auto-commit OFF.");
              }
            break;
        case 9 :
            if (ui->autoSave_10->isChecked()) {
                qDebug() << "!n" << tr("[Disk 10] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 10] Auto-commit OFF.");
              }
            break;
        case 10 :
            if (ui->autoSave_11->isChecked()) {
                qDebug() << "!n" << tr("[Disk 11] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 11] Auto-commit OFF.");
              }
            break;
        case 11 :
            if (ui->autoSave_12->isChecked()) {
                qDebug() << "!n" << tr("[Disk 12] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 12] Auto-commit OFF.");
              }
            break;
        case 12 :
            if (ui->autoSave_13->isChecked()) {
                qDebug() << "!n" << tr("[Disk 13] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 13] Auto-commit OFF.");
              }
            break;
        case 13 :
            if (ui->autoSave_14->isChecked()) {
                qDebug() << "!n" << tr("[Disk 14] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 14] Auto-commit OFF.");
              }
            break;
        case 14 :
            if (ui->autoSave_15->isChecked()) {
                qDebug() << "!n" << tr("[Disk 15] Auto-commit ON.");
            } else {
                qDebug() << "!n" << tr("[Disk 15] Auto-commit OFF.");
              }
            break;
    }

    bool saved;

    img->lock();
    saved = img->save();
    img->unlock();
    if (!saved) {
        if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            saveDiskAs(no);
        }
    } else {
            diskWidgets[no].saveAction->setEnabled(false);
            diskWidgets[no].revertAction->setEnabled(false);

    }
}
//
void MainWindow::saveDiskAs(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    QString dir, fileName;
    bool saved = false;

    if (img->isUnnamed()) {
        dir = aspeqtSettings->lastDiskImageDir();
    } else {
        dir = QFileInfo(img->originalFileName()).absolutePath();
    }

    do {
        fileName = QFileDialog::getSaveFileName(this, tr("Save image as"),
                                 dir,
                                 tr(
//                                                    "All Atari disk images (*.atr *.xfd *.atx *.pro);;"
                                                    "All Atari disk images (*.atr *.xfd *.pro);;"
                                                    "SIO2PC ATR images (*.atr);;"
                                                    "XFormer XFD images (*.xfd);;"
//                                                    "ATX images (*.atx);;"
                                                    "Pro images (*.pro);;"
                                                    "All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        img->lock();
        saved = img->saveAs(fileName);
        img->unlock();

        if (!saved) {
            if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
                .arg(fileName), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                break;
            }
        }

    } while (!saved);

    if (saved) {
        aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
    }
    aspeqtSettings->unmountImage(no);
    aspeqtSettings->mountImage(no, fileName, img->isReadOnly());
}

void MainWindow::revertDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + 0x31));
    if (QMessageBox::question(this, tr("Revert to last saved"),
            tr("Do you really want to revert '%1' to its last saved state? You will lose the changes that has been made.")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        img->lock();
        img->reopen();
        img->unlock();
        deviceStatusChanged(0x31 + no);
    }
}

void MainWindow::on_actionMountDisk_1_triggered() {mountDiskImage(0);}
void MainWindow::on_actionMountDisk_2_triggered() {mountDiskImage(1);}
void MainWindow::on_actionMountDisk_3_triggered() {mountDiskImage(2);}
void MainWindow::on_actionMountDisk_4_triggered() {mountDiskImage(3);}
void MainWindow::on_actionMountDisk_5_triggered() {mountDiskImage(4);}
void MainWindow::on_actionMountDisk_6_triggered() {mountDiskImage(5);}
void MainWindow::on_actionMountDisk_7_triggered() {mountDiskImage(6);}
void MainWindow::on_actionMountDisk_8_triggered() {mountDiskImage(7);}

// Additional disks //
void MainWindow::on_actionMountDisk_9_triggered() {mountDiskImage(8);}
void MainWindow::on_actionMountDisk_10_triggered() {mountDiskImage(9);}
void MainWindow::on_actionMountDisk_11_triggered() {mountDiskImage(10);}
void MainWindow::on_actionMountDisk_12_triggered() {mountDiskImage(11);}
void MainWindow::on_actionMountDisk_13_triggered() {mountDiskImage(12);}
void MainWindow::on_actionMountDisk_14_triggered() {mountDiskImage(13);}
void MainWindow::on_actionMountDisk_15_triggered() {mountDiskImage(14);}

void MainWindow::on_actionMountFolder_1_triggered() {mountFolderImage(0);}
void MainWindow::on_actionMountFolder_2_triggered() {mountFolderImage(1);}
void MainWindow::on_actionMountFolder_3_triggered() {mountFolderImage(2);}
void MainWindow::on_actionMountFolder_4_triggered() {mountFolderImage(3);}
void MainWindow::on_actionMountFolder_5_triggered() {mountFolderImage(4);}
void MainWindow::on_actionMountFolder_6_triggered() {mountFolderImage(5);}
void MainWindow::on_actionMountFolder_7_triggered() {mountFolderImage(6);}
void MainWindow::on_actionMountFolder_8_triggered() {mountFolderImage(7);}

// Additional Folders  //
void MainWindow::on_actionMountFolder_9_triggered() {mountFolderImage(8);}
void MainWindow::on_actionMountFolder_10_triggered() {mountFolderImage(9);}
void MainWindow::on_actionMountFolder_11_triggered() {mountFolderImage(10);}
void MainWindow::on_actionMountFolder_12_triggered() {mountFolderImage(11);}
void MainWindow::on_actionMountFolder_13_triggered() {mountFolderImage(12);}
void MainWindow::on_actionMountFolder_14_triggered() {mountFolderImage(13);}
void MainWindow::on_actionMountFolder_15_triggered() {mountFolderImage(14);}

void MainWindow::on_actionEject_1_triggered() {ejectImage(0);}
void MainWindow::on_actionEject_2_triggered() {ejectImage(1);}
void MainWindow::on_actionEject_3_triggered() {ejectImage(2);}
void MainWindow::on_actionEject_4_triggered() {ejectImage(3);}
void MainWindow::on_actionEject_5_triggered() {ejectImage(4);}
void MainWindow::on_actionEject_6_triggered() {ejectImage(5);}
void MainWindow::on_actionEject_7_triggered() {ejectImage(6);}
void MainWindow::on_actionEject_8_triggered() {ejectImage(7);}
//
void MainWindow::on_actionEject_9_triggered() {ejectImage(8);}
void MainWindow::on_actionEject_10_triggered() {ejectImage(9);}
void MainWindow::on_actionEject_11_triggered() {ejectImage(10);}
void MainWindow::on_actionEject_12_triggered() {ejectImage(11);}
void MainWindow::on_actionEject_13_triggered() {ejectImage(12);}
void MainWindow::on_actionEject_14_triggered() {ejectImage(13);}
void MainWindow::on_actionEject_15_triggered() {ejectImage(14);}

void MainWindow::on_actionWriteProtect_1_triggered() {toggleWriteProtection(0);}
void MainWindow::on_actionWriteProtect_2_triggered() {toggleWriteProtection(1);}
void MainWindow::on_actionWriteProtect_3_triggered() {toggleWriteProtection(2);}
void MainWindow::on_actionWriteProtect_4_triggered() {toggleWriteProtection(3);}
void MainWindow::on_actionWriteProtect_5_triggered() {toggleWriteProtection(4);}
void MainWindow::on_actionWriteProtect_6_triggered() {toggleWriteProtection(5);}
void MainWindow::on_actionWriteProtect_7_triggered() {toggleWriteProtection(6);}
void MainWindow::on_actionWriteProtect_8_triggered() {toggleWriteProtection(7);}
//
void MainWindow::on_actionWriteProtect_9_triggered() {toggleWriteProtection(8);}
void MainWindow::on_actionWriteProtect_10_triggered() {toggleWriteProtection(9);}
void MainWindow::on_actionWriteProtect_11_triggered() {toggleWriteProtection(10);}
void MainWindow::on_actionWriteProtect_12_triggered() {toggleWriteProtection(11);}
void MainWindow::on_actionWriteProtect_13_triggered() {toggleWriteProtection(12);}
void MainWindow::on_actionWriteProtect_14_triggered() {toggleWriteProtection(13);}
void MainWindow::on_actionWriteProtect_15_triggered() {toggleWriteProtection(14);}

void MainWindow::on_actionMountRecent_0_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_0->text());}
void MainWindow::on_actionMountRecent_1_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_1->text());}
void MainWindow::on_actionMountRecent_2_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_2->text());}
void MainWindow::on_actionMountRecent_3_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_3->text());}
void MainWindow::on_actionMountRecent_4_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_4->text());}
void MainWindow::on_actionMountRecent_5_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_5->text());}
void MainWindow::on_actionMountRecent_6_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_6->text());}
void MainWindow::on_actionMountRecent_7_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_7->text());}
void MainWindow::on_actionMountRecent_8_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_8->text());}
void MainWindow::on_actionMountRecent_9_triggered() {mountFileWithDefaultProtection(firstEmptyDiskSlot(), ui->actionMountRecent_9->text());}

void MainWindow::on_actionEditDisk_1_triggered() {openEditor(0);}
void MainWindow::on_actionEditDisk_2_triggered() {openEditor(1);}
void MainWindow::on_actionEditDisk_3_triggered() {openEditor(2);}
void MainWindow::on_actionEditDisk_4_triggered() {openEditor(3);}
void MainWindow::on_actionEditDisk_5_triggered() {openEditor(4);}
void MainWindow::on_actionEditDisk_6_triggered() {openEditor(5);}
void MainWindow::on_actionEditDisk_7_triggered() {openEditor(6);}
void MainWindow::on_actionEditDisk_8_triggered() {openEditor(7);}
//
void MainWindow::on_actionEditDisk_9_triggered() {openEditor(8);}
void MainWindow::on_actionEditDisk_10_triggered() {openEditor(9);}
void MainWindow::on_actionEditDisk_11_triggered() {openEditor(10);}
void MainWindow::on_actionEditDisk_12_triggered() {openEditor(11);}
void MainWindow::on_actionEditDisk_13_triggered() {openEditor(12);}
void MainWindow::on_actionEditDisk_14_triggered() {openEditor(13);}
void MainWindow::on_actionEditDisk_15_triggered() {openEditor(14);}

void MainWindow::on_actionSave_1_triggered() {saveDisk(0);}
void MainWindow::on_actionSave_2_triggered() {saveDisk(1);}
void MainWindow::on_actionSave_3_triggered() {saveDisk(2);}
void MainWindow::on_actionSave_4_triggered() {saveDisk(3);}
void MainWindow::on_actionSave_5_triggered() {saveDisk(4);}
void MainWindow::on_actionSave_6_triggered() {saveDisk(5);}
void MainWindow::on_actionSave_7_triggered() {saveDisk(6);}
void MainWindow::on_actionSave_8_triggered() {saveDisk(7);}
//
void MainWindow::on_actionSave_9_triggered() {saveDisk(8);}
void MainWindow::on_actionSave_10_triggered() {saveDisk(9);}
void MainWindow::on_actionSave_11_triggered() {saveDisk(10);}
void MainWindow::on_actionSave_12_triggered() {saveDisk(11);}
void MainWindow::on_actionSave_13_triggered() {saveDisk(12);}
void MainWindow::on_actionSave_14_triggered() {saveDisk(13);}
void MainWindow::on_actionSave_15_triggered() {saveDisk(14);}

//
void MainWindow::on_actionAutoSave_1_triggered() {autoSaveDisk(0);}
void MainWindow::on_actionAutoSave_2_triggered() {autoSaveDisk(1);}
void MainWindow::on_actionAutoSave_3_triggered() {autoSaveDisk(2);}
void MainWindow::on_actionAutoSave_4_triggered() {autoSaveDisk(3);}
void MainWindow::on_actionAutoSave_5_triggered() {autoSaveDisk(4);}
void MainWindow::on_actionAutoSave_6_triggered() {autoSaveDisk(5);}
void MainWindow::on_actionAutoSave_7_triggered() {autoSaveDisk(6);}
void MainWindow::on_actionAutoSave_8_triggered() {autoSaveDisk(7);}
//
void MainWindow::on_actionAutoSave_9_triggered() {autoSaveDisk(8);}
void MainWindow::on_actionAutoSave_10_triggered() {autoSaveDisk(9);}
void MainWindow::on_actionAutoSave_11_triggered() {autoSaveDisk(10);}
void MainWindow::on_actionAutoSave_12_triggered() {autoSaveDisk(11);}
void MainWindow::on_actionAutoSave_13_triggered() {autoSaveDisk(12);}
void MainWindow::on_actionAutoSave_14_triggered() {autoSaveDisk(13);}
void MainWindow::on_actionAutoSave_15_triggered() {autoSaveDisk(14);}

void MainWindow::on_actionSaveAs_1_triggered() {saveDiskAs(0);}
void MainWindow::on_actionSaveAs_2_triggered() {saveDiskAs(1);}
void MainWindow::on_actionSaveAs_3_triggered() {saveDiskAs(2);}
void MainWindow::on_actionSaveAs_4_triggered() {saveDiskAs(3);}
void MainWindow::on_actionSaveAs_5_triggered() {saveDiskAs(4);}
void MainWindow::on_actionSaveAs_6_triggered() {saveDiskAs(5);}
void MainWindow::on_actionSaveAs_7_triggered() {saveDiskAs(6);}
void MainWindow::on_actionSaveAs_8_triggered() {saveDiskAs(7);}
//
void MainWindow::on_actionSaveAs_9_triggered() {saveDiskAs(8);}
void MainWindow::on_actionSaveAs_10_triggered() {saveDiskAs(9);}
void MainWindow::on_actionSaveAs_11_triggered() {saveDiskAs(10);}
void MainWindow::on_actionSaveAs_12_triggered() {saveDiskAs(11);}
void MainWindow::on_actionSaveAs_13_triggered() {saveDiskAs(12);}
void MainWindow::on_actionSaveAs_14_triggered() {saveDiskAs(13);}
void MainWindow::on_actionSaveAs_15_triggered() {saveDiskAs(14);}

void MainWindow::on_actionRevert_1_triggered() {revertDisk(0);}
void MainWindow::on_actionRevert_2_triggered() {revertDisk(1);}
void MainWindow::on_actionRevert_3_triggered() {revertDisk(2);}
void MainWindow::on_actionRevert_4_triggered() {revertDisk(3);}
void MainWindow::on_actionRevert_5_triggered() {revertDisk(4);}
void MainWindow::on_actionRevert_6_triggered() {revertDisk(5);}
void MainWindow::on_actionRevert_7_triggered() {revertDisk(6);}
void MainWindow::on_actionRevert_8_triggered() {revertDisk(7);}
//
void MainWindow::on_actionRevert_9_triggered() {revertDisk(8);}
void MainWindow::on_actionRevert_10_triggered() {revertDisk(9);}
void MainWindow::on_actionRevert_11_triggered() {revertDisk(10);}
void MainWindow::on_actionRevert_12_triggered() {revertDisk(11);}
void MainWindow::on_actionRevert_13_triggered() {revertDisk(12);}
void MainWindow::on_actionRevert_14_triggered() {revertDisk(13);}
void MainWindow::on_actionRevert_15_triggered() {revertDisk(14);}

void MainWindow::on_actionEjectAll_triggered()
{
    QMessageBox::StandardButton answer = QMessageBox::No;

    int toBeSaved = 0;

    for (int i = 0; i < g_numberOfDisks; i++) {  //
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + 0x31));
        if (img && img->isModified()) {
            toBeSaved++;
        }
    }

    if (!toBeSaved) {
        for (int i = g_numberOfDisks-1; i >= 0; i--) {
            ejectImage(i);
        }
        return;
    }

    bool wasRunning = ui->actionStartEmulation->isChecked();
    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }

    for (int i = g_numberOfDisks-1; i >= 0; i--) {
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + 0x31));
        if (img && img->isModified()) {
            toBeSaved--;
            answer = saveImageWhenClosing(i, answer, toBeSaved);
            if (answer == QMessageBox::NoToAll) {
                break;
            }
            if (answer == QMessageBox::Cancel) {
                if (wasRunning) {
                    ui->actionStartEmulation->trigger();
                }
                return;
            }
        }
    }
    for (int i = g_numberOfDisks-1; i >= 0; i--) {
        ejectImage(i, false);
    }
    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }
}
void MainWindow::on_actionMountDisk_triggered()
{
    mountDiskImage(firstEmptyDiskSlot(0, true));
}

void MainWindow::on_actionMountFolder_triggered()
{
    mountFolderImage(firstEmptyDiskSlot(0, true));
}

void MainWindow::on_actionNewImage_triggered()
{
    CreateImageDialog dlg(this);
    if (!dlg.exec()) {
        return;
    };

    SimpleDiskImage *disk = new SimpleDiskImage(sio);
    connect(disk, SIGNAL(statusChanged(int)), this, SLOT(deviceStatusChanged(int)), Qt::QueuedConnection);

    if (!disk->create(++untitledName)) {
        delete disk;
        return;
    }

    DiskGeometry g;
    uint size = dlg.sectorCount() * dlg.sectorSize();
    if (dlg.sectorSize() == 256) {
        if (dlg.sectorCount() >= 3) {
            size -= 384;
        } else {
            size -= dlg.sectorCount() * 128;
        }
    }
    g.initialize(size, dlg.sectorSize());

    if (!disk->format(g)) {
        delete disk;
        return;
    }

    int no = firstEmptyDiskSlot(0, true);

    if (!ejectImage(no)) {
        delete disk;
        return;
    }

    sio->installDevice(0x31 + no, disk);
    deviceStatusChanged(0x31 + no);
    qDebug() << "!n" << tr("[%1] Mounted '%2' as '%3'.")
            .arg(disk->deviceName())
            .arg(disk->originalFileName())
            .arg(disk->description());
}

void MainWindow::on_actionOpenSession_triggered()
{
    QString dir = aspeqtSettings->lastSessionDir();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open session"),
                                 dir,
                                 tr(
                                         "AspeQt sessions (*.aspeqt);;"
                                         "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
// First eject existing images, then mount session images and restore mainwindow position and size //
    MainWindow::on_actionEjectAll_triggered();

    aspeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());
    g_sessionFile = QFileInfo(fileName).fileName();
    g_sessionFilePath = QFileInfo(fileName).absolutePath();

// Pass Session file name, path and MainWindow title to AspeQtSettings //
    aspeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
    aspeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    aspeqtSettings->loadSessionFromFile(fileName);

    setWindowTitle(g_mainWindowTitle + tr(" -- Session: ") + g_sessionFile);
    setGeometry(aspeqtSettings->lastHorizontalPos(), aspeqtSettings->lastVerticalPos(), aspeqtSettings->lastWidth() , aspeqtSettings->lastHeight());

    for (int i = 0; i < g_numberOfDisks; i++) {  //
        AspeqtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }
    g_D9DOVisible =  aspeqtSettings->D9DOVisible();
    on_actionHideShowDrives_triggered();
    setSession();
}
void MainWindow::on_actionSaveSession_triggered()
{
    QString dir = aspeqtSettings->lastSessionDir();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save session as"),
                                 dir,
                                 tr(
                                         "AspeQt sessions (*.aspeqt);;"
                                         "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());

// Save mainwindow position and size to session file //
    if (aspeqtSettings->saveWindowsPos()) {
        aspeqtSettings->setLastHorizontalPos(geometry().x());
        aspeqtSettings->setLastVerticalPos(geometry().y());
        aspeqtSettings->setLastWidth(geometry().width());
        aspeqtSettings->setLastHeight(geometry().height());
    }
    aspeqtSettings->saveSessionToFile(fileName);
}

void MainWindow::on_actionBootExe_triggered()
{
    QString dir = aspeqtSettings->lastExeDir();
    g_exefileName = QFileDialog::getOpenFileName(this, tr("Open executable"),
                                 dir,
                                 tr(
                                         "Atari executables (*.xex *.com *.exe);;"
                                         "All files (*)"));
    if (g_exefileName.isEmpty()) {
        return;
    }
        aspeqtSettings->setLastExeDir(QFileInfo(g_exefileName).absolutePath());
    bootExe(g_exefileName);
}

void MainWindow::on_actionShowPrinterTextOutput_triggered()
{
    if (ui->actionShowPrinterTextOutput->isChecked()) {
        textPrinterWindow->setGeometry(aspeqtSettings->lastPrtHorizontalPos() ,aspeqtSettings->lastPrtVerticalPos(),aspeqtSettings->lastPrtWidth(),aspeqtSettings->lastPrtHeight());
        textPrinterWindow->show();
    } else {
        textPrinterWindow->hide();
    }
}

void MainWindow::textPrinterWindowClosed()
{
    ui->actionShowPrinterTextOutput->setChecked(false);
}

void MainWindow::on_actionPlaybackCassette_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open a cassette image"),
                                                    aspeqtSettings->lastCasDir(),
                                                    tr(
                                                    "CAS images (*.cas);;"
                                                    "All files (*)"));

    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastCasDir(QFileInfo(fileName).absolutePath());

    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
        sio->wait();
        qApp->processEvents();
    }

    CassetteDialog *dlg = new CassetteDialog(this, fileName);
    dlg->exec();
    delete dlg;

    if (restart) {
        ui->actionStartEmulation->trigger();
    }
}

void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        setWindowFlags(oldWindowFlags);
        setWindowState(oldWindowStates);
        show();
        activateWindow();
        raise();
        trayIcon.hide();
    }
}

void MainWindow::folderPath(int slot)
{
   emit takeFolderPath(diskWidgets[slot].fileNameLabel->statusTip());
}

void MainWindow::on_actionBootOption_triggered()
{
    BootOptionsDialog bod(this);
    connect(&bod, SIGNAL(giveFolderPath(int)), this, SLOT(folderPath(int)));
    connect(this, SIGNAL(takeFolderPath(QString)), &bod, SLOT(folderPath(QString)));
    bod.exec();
}
