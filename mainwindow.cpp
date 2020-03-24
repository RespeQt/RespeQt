/*
 * mainwindow.cpp
 *
 * Copyright 2015, 2017 Joseph Zatarski
 * Copyright 2016 TheMontezuma
 * Copyright 2017 josch1710
 * Copyright 2017 blind
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "FirmwareDiskImage.hpp"
#include "drivewidget.h"
#include "folderimage.h"
#include "pclink.h"
#include "rcl.h"
#include "smartdevice.h"
#include "respeqtsettings.h"
#include "autobootdialog.h"
#include "autoboot.h"
#include "cassettedialog.h"
#include "bootoptionsdialog.h"
#include "logdisplaydialog.h"
#include "infowidget.h"
#include "printerwidget.h"
#include "printers/printerfactory.h"
#include "printers/outputfactory.h"
#include "printers/printers.h"
#include "printers/outputs.h"

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
#include <QPrintDialog>
#include <QPrinter>
#include <QToolButton>
#include <QHBoxLayout>
#include <QPrinterInfo>
#include <typeinfo>
#include <memory>

#include "atarifilesystem.h"
#include "miscutils.h"

#include <QFontDatabase>

RespeqtSettings *respeqtSettings;

static QFile *logFile;
static QMutex *logMutex;

QString g_exefileName;
static QString g_rclFileName;
QString g_respeQtAppPath;
static QRect g_savedGeometry;
bool g_disablePicoHiSpeed;
static bool g_D9DOVisible = true;
bool g_miniMode = false;
static bool g_shadeMode = false;
SimpleDiskImage *g_translator = nullptr;
//static int g_savedWidth;

// ****************************** END OF GLOBALS ************************************//

// Displayed only in debug mode    "!d"
// Unimportant     (gray)          "!u"
// Normal          (black)         "!n"
// Important       (blue)          "!i"
// Warning         (brown)         "!w"
// Error           (red)           "!e"

void logMessageOutput(QtMsgType type, const QMessageLogContext &/*context*/, const QString &msg)
// void logMessageOutput(QtMsgType type, const char *msg)
{
    logMutex->lock();
    logFile->write(QString::number((quint64)QThread::currentThreadId(), 16).toLatin1());
    switch (type) {
#if QT_VERSION >= 0x050500
        case QtInfoMsg:
            logFile->write(": [Info]    ");
            break;
#endif
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
        MainWindow::getInstance()->doLogMessage(localMsg.at(1), displayMsg);
    }
}

void MainWindow::doLogMessage(int type, const QString &msg)
{
    emit logMessage(type, msg);
}

MainWindow *MainWindow::instance = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      isClosing(false)
{
    /* Setup the logging system */
    instance = this;
    g_respeQtAppPath = QCoreApplication::applicationDirPath();
    g_disablePicoHiSpeed = false;
    logFile = new QFile(QDir::temp().absoluteFilePath("respeqt.log"));
    logFile->open(QFile::WriteOnly | QFile::Truncate | QFile::Unbuffered | QFile::Text);
    logMutex = new QMutex();
    connect(this, SIGNAL(logMessage(int,QString)), this, SLOT(uiMessage(int,QString)), Qt::QueuedConnection);
    qInstallMessageHandler(logMessageOutput);
    qDebug() << "!d" << tr("RespeQt started at %1.").arg(QDateTime::currentDateTime().toString());

    logWindow_ = nullptr;

    /* Remove old temporaries */
    QDir tempDir = QDir::temp();
    QStringList filters;
    filters << "respeqt-*";
    QFileInfoList list = tempDir.entryInfoList(filters, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
    foreach(QFileInfo file, list) {
        deltree(file.absoluteFilePath());
    }

    QCoreApplication::setOrganizationName("ZeeSoft");
    QCoreApplication::setOrganizationDomain("https://github.com/jzatarski/RespeQt");
    QCoreApplication::setApplicationName("RespeQt");
    respeqtSettings = new RespeqtSettings();

    /* Load translators */
    loadTranslators();

#ifdef Q_OS_MAC
    /// This HAS to be executed before setupUi
    bool noNative = !respeqtSettings->nativeMenu();
    QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, noNative);
#endif

    /* Setup UI */
    ui->setupUi(this);

    /* Setup the printer factory */
    auto& pfactory = Printers::PrinterFactory::instance();
    pfactory->registerPrinter<Printers::Atari1020>(Printers::Atari1020::typeName());
    pfactory->registerPrinter<Printers::Atari1025>(Printers::Atari1025::typeName());
    pfactory->registerPrinter<Printers::Atari1027>(Printers::Atari1027::typeName());
    pfactory->registerPrinter<Printers::Atari1029>(Printers::Atari1029::typeName());
    pfactory->registerPrinter<Printers::Passthrough>(Printers::Passthrough::typeName());

    /* Setup the output factory */
    auto& ofactory = Printers::OutputFactory::instance();
    ofactory->registerOutput<Printers::SVGOutput>(Printers::SVGOutput::typeName());
    ofactory->registerOutput<Printers::TextPrinterWindow>(Printers::TextPrinterWindow::typeName());
    ofactory->registerOutput<Printers::RawOutput>(Printers::RawOutput::typeName());
    QStringList printers = QPrinterInfo::availablePrinterNames();
    for (QStringList::const_iterator sit = printers.cbegin(); sit != printers.cend(); ++sit)
    {
        ofactory->registerOutput<Printers::NativePrinter>(*sit);
    }

    /* Add QActions for most recent */
    for( int i = 0; i < NUM_RECENT_FILES; ++i ) {
        auto recentAction = new QAction(this);
        connect(recentAction,SIGNAL(triggered()), this, SLOT(openRecent()));
        recentFilesActions_.append(recentAction);
    }

    auto diskMenu = (QWidget*)menuBar()->children().at(1);

    diskMenu->addActions( recentFilesActions_ );

    /* Initialize diskWidgets array and tool button actions */
    createDeviceWidgets();

    /* Parse command line arguments:
      arg(1): session file (xxxxxxxx.respeqt)   */

    QStringList RespeQtArgs = QCoreApplication::arguments();
    g_sessionFile = g_sessionFilePath = "";
    if (RespeQtArgs.size() > 1) {
       QFile sess;
       QString s = QDir::separator();             //
       int i = RespeQtArgs.at(1).lastIndexOf(s);   //
       if (i != -1) {
           i++;
           g_sessionFile = RespeQtArgs.at(1).right(RespeQtArgs.at(1).size() - i);
           g_sessionFilePath = RespeQtArgs.at(1).left(i);
           g_sessionFilePath = QDir::fromNativeSeparators(g_sessionFilePath);
           sess.setFileName(g_sessionFilePath+g_sessionFile);
           if (!sess.exists()) {
               QMessageBox::question(this, tr("Session file error"),
               tr("Requested session file not found in the given directory path or the path is incorrect. RespeQt will continue with default session configuration."), QMessageBox::Ok);
               g_sessionFile = g_sessionFilePath = "";
           }
       } else {
           if (RespeQtArgs.at(1) != "") {
                g_sessionFile = RespeQtArgs.at(1);
                g_sessionFilePath = QDir::currentPath();
                sess.setFileName(g_sessionFile);
                if (!sess.exists()) {
                    QMessageBox::question(this, tr("Session file error"),
                    tr("Requested session file not found in the application's current directory path\n (No path was specified). RespeQt will continue with default session configuration."), QMessageBox::Ok);
                    g_sessionFile = g_sessionFilePath = "";
                }
            }
        }
    }
    // Pass Session file name, path and MainWindow title to RespeQtSettings //
    respeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
    respeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    // Display Session name, and restore session parameters if session file was specified //
    g_mainWindowTitle = tr("RespeQt - Atari Serial Peripheral Emulator for Qt");
    if (g_sessionFile != "") {
        setWindowTitle(g_mainWindowTitle + tr(" -- Session: ") + g_sessionFile);
        respeqtSettings->loadSessionFromFile(g_sessionFilePath+g_sessionFile);
    } else {
        setWindowTitle(g_mainWindowTitle);
    }
    setGeometry(respeqtSettings->lastHorizontalPos(),respeqtSettings->lastVerticalPos(),respeqtSettings->lastWidth(),respeqtSettings->lastHeight());

    /* Setup status bar */
    speedLabel = new QLabel(this);
    onOffLabel = new QLabel(this);
    prtOnOffLabel = new QLabel(this);
    netLabel = new QLabel(this);
    clearMessagesLabel = new QLabel(this);
    speedLabel->setText(tr("19200 bits/sec"));
    onOffLabel->setMinimumWidth(21);
    prtOnOffLabel->setMinimumWidth(18);
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
    ui->textEdit->installEventFilter(MainWindow::getInstance());
    changeFonts();
    g_D9DOVisible =  respeqtSettings->D9DOVisible();
    showHideDrives();

#ifndef QT_NO_DEBUG
    snapshot = new QLabel(this);
    snapshot->setMinimumWidth(21);
    snapshot->setPixmap(QIcon(":/icons/silk-icons/icons/camera.png").pixmap(16, 16, QIcon::Normal));
    snapshot->setToolTip(tr("Record SIO traffic"));
    snapshot->setStatusTip(snapshot->toolTip());
    ui->statusBar->addPermanentWidget(snapshot);
#endif

    m_romProvider = new RomProvider();

    /* Connect to the network */
    QString netInterface;
    auto oNet = new Network();
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

    /* Connect SioWorker signals */
    sio = SioWorkerPtr::create();
    connect(sio.data(), SIGNAL(started()), this, SLOT(sioStarted()));
    connect(sio.data(), SIGNAL(finished()), this, SLOT(sioFinished()));
    connect(sio.data(), SIGNAL(statusChanged(QString)), this, SLOT(sioStatusChanged(QString)));
    shownFirstTime = true;
    sio->setAutoReconnect(respeqtSettings->sioAutoReconnect());

    PCLINK* pclink = new PCLINK(sio);
    sio->installDevice(PCLINK_CDEVIC, pclink);

    /* Restore application state */
    for (int i = 0; i < DISK_COUNT; i++) {
        RespeqtSettings::ImageSettings is;
        is = respeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }
    updateRecentFileActions();

    setAcceptDrops(true);

    // SmartDevice (ApeTime + URL submit)
    SmartDevice *smart = new SmartDevice(sio);
    sio->installDevice(SMART_CDEVIC, smart);

    // RespeQt Client  //
    RCl *rcl = new RCl(sio);
    sio->installDevice(RESPEQT_CLIENT_CDEVIC, rcl);

    // Documentation Display
    docDisplayWindow = new DocDisplayWindow();

    connect(docDisplayWindow, SIGNAL(closed()), this, SLOT(docDisplayWindowClosed()));

    setUpPrinterEmulationWidgets(respeqtSettings->printerEmulation());

    untitledName = 0;

    connect(&trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon.setIcon(windowIcon());

    // Connections needed for remotely mounting a disk image & Toggle Auto-Commit //
    connect (rcl, SIGNAL(findNewSlot(int,bool)), this, SLOT(firstEmptyDiskSlot(int,bool)));
    connect (this, SIGNAL(newSlot(int)), rcl, SLOT(gotNewSlot(int)));
    connect (rcl, SIGNAL(mountFile(int,QString)), this, SLOT(mountFileWithDefaultProtection(int,QString)));
    connect (this, SIGNAL(fileMounted(bool)), rcl, SLOT(fileMounted(bool)));
    connect (rcl, SIGNAL(toggleAutoCommit(int, bool)), this, SLOT(autoCommit(int, bool)));
    connect (rcl, SIGNAL(toggleHappy(int, bool)), this, SLOT(happy(int, bool)));
    connect (rcl, SIGNAL(toggleChip(int, bool)), this, SLOT(chip(int, bool)));
    connect (rcl, SIGNAL(bootExe(QString)), this, SLOT(bootExeTriggered(QString)));
}

MainWindow::~MainWindow()
{
    if (ui->actionStartEmulation->isChecked()) {
        ui->actionStartEmulation->trigger();
    }

    delete respeqtSettings;

    delete ui;

    qDebug() << "!d" << tr("RespeQt stopped at %1.").arg(QDateTime::currentDateTime().toString());
    qInstallMessageHandler(0);
    delete logMutex;
    delete logFile;
}


void MainWindow::createDeviceWidgets()
{

    ui->rightColumn->setAlignment(Qt::AlignTop);

    for (int i = 0; i < DISK_COUNT; i++) {      //
        auto deviceWidget = new DriveWidget(i);

        if (i<8) {
            ui->leftColumn->addWidget( deviceWidget );
        } else {
            ui->rightColumn->addWidget( deviceWidget );
        }

        deviceWidget->setup(respeqtSettings->hideHappyMode(), respeqtSettings->hideChipMode(), respeqtSettings->hideNextImage(), respeqtSettings->hideOSBMode(), respeqtSettings->hideToolDisk());
        diskWidgets[i] = deviceWidget;

        // connect signals to slots
        connect(deviceWidget, SIGNAL(actionMountDisk(int)),this, SLOT(on_actionMountDisk_triggered(int)));
        connect(deviceWidget, SIGNAL(actionMountFolder(int)),this, SLOT(on_actionMountFolder_triggered(int)));
        connect(deviceWidget, SIGNAL(actionAutoSave(int)),this, SLOT(on_actionAutoSave_triggered(int)));
        connect(deviceWidget, SIGNAL(actionEject(int)),this, SLOT(on_actionEject_triggered(int)));
        connect(deviceWidget, SIGNAL(actionNextSide(int)),this, SLOT(on_actionNextSide_triggered(int)));
        connect(deviceWidget, SIGNAL(actionToggleHappy(int,bool)),this, SLOT(on_actionToggleHappy_triggered(int,bool)));
        connect(deviceWidget, SIGNAL(actionToggleChip(int,bool)),this, SLOT(on_actionToggleChip_triggered(int,bool)));
        connect(deviceWidget, SIGNAL(actionToggleOSB(int,bool)),this, SLOT(on_actionToggleOSB_triggered(int,bool)));
        connect(deviceWidget, SIGNAL(actionToolDisk(int,bool)),this, SLOT(on_actionToolDisk_triggered(int,bool)));
        connect(deviceWidget, SIGNAL(actionWriteProtect(int,bool)),this, SLOT(on_actionWriteProtect_triggered(int,bool)));
        connect(deviceWidget, SIGNAL(actionEditDisk(int)),this, SLOT(on_actionEditDisk_triggered(int)));
        connect(deviceWidget, SIGNAL(actionSave(int)),this, SLOT(on_actionSave_triggered(int)));
        connect(deviceWidget, SIGNAL(actionRevert(int)),this, SLOT(on_actionRevert_triggered(int)));
        connect(deviceWidget, SIGNAL(actionSaveAs(int)),this, SLOT(on_actionSaveAs_triggered(int)));
        connect(deviceWidget, SIGNAL(actionAutoSave(int)),this, SLOT(on_actionAutoSave_triggered(int)));
        connect(deviceWidget, SIGNAL(actionBootOptions(int)),this, SLOT(on_actionBootOption_triggered()));

        connect(this, SIGNAL(setFont(const QFont&)),deviceWidget, SLOT(setFont(const QFont&)));
    }



    for (int i = 0; i < PRINTER_COUNT; i++) {      //
        auto printerWidget = new PrinterWidget(i);
        if (i<2) {
            ui->leftColumn2->addWidget( printerWidget );
        } else {
            ui->rightColumn2->addWidget( printerWidget );
        }
        printerWidget->setup();
        printerWidgets[i] = printerWidget;
   }


    changeFonts();
}

 void MainWindow::mousePressEvent(QMouseEvent *event)
 {
     int slot = containingDiskSlot(event->pos());

     if (event->button() == Qt::LeftButton
         && slot >= 0) {

         auto drag = new QDrag((QWidget*)this);
         auto mimeData = new QMimeData;

         mimeData->setData("application/x-respeqt-disk-image", QByteArray(1, slot));
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
     if (event->button() == Qt::LeftButton && !speedLabel->isHidden() && speedLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
        ui->actionOptions->trigger();
     }
#ifndef QT_NO_DEBUG
     if (sio && event->button() == Qt::LeftButton && !snapshot->isHidden() && snapshot->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
        if (!sio->isSnapshotRunning())
            sio->startSIOSnapshot();
        else
            sio->stopSIOSnapshot();
     }
#endif
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    int i = containingDiskSlot(event->pos());
    if (i >= 0 && (event->mimeData()->hasUrls() ||
                   event->mimeData()->hasFormat("application/x-respeqt-disk-image"))) {
        event->acceptProposedAction();
    } else {
        i = -1;
    }
    for (int j = 0; j < DISK_COUNT; j++) { //
        if (i == j) {
            diskWidgets[j]->setFrameShadow(QFrame::Sunken);
        } else {
            diskWidgets[j]->setFrameShadow(QFrame::Raised);
        }
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    int i = containingDiskSlot(event->pos());
    if (i >= 0 && (event->mimeData()->hasUrls() ||
                   event->mimeData()->hasFormat("application/x-respeqt-disk-image"))) {
        event->acceptProposedAction();
    } else {
        i = -1;
    }
    for (int j = 0; j < DISK_COUNT; j++) { //
        if (i == j) {
            diskWidgets[j]->setFrameShadow(QFrame::Sunken);
        } else {
            diskWidgets[j]->setFrameShadow(QFrame::Raised);
        }
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *)
{
    for (int j = 0; j < DISK_COUNT; j++) { //
        diskWidgets[j]->setFrameShadow(QFrame::Raised);
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    for (int j = 0; j < DISK_COUNT; j++) { //
        diskWidgets[j]->setFrameShadow(QFrame::Raised);
    }
    int slot = containingDiskSlot(event->pos());
    if (!(event->mimeData()->hasUrls() ||
          event->mimeData()->hasFormat("application/x-respeqt-disk-image")) ||
          slot < 0) {
        return;
    }

    if (event->mimeData()->hasFormat("application/x-respeqt-disk-image")) {
        int source = event->mimeData()->data("application/x-respeqt-disk-image").at(0);
        if (slot != source) {
            sio->swapDevices(slot + DISK_BASE_CDEVIC, source + DISK_BASE_CDEVIC);
            respeqtSettings->swapImages(slot, source);

            auto pclink = reinterpret_cast<PCLINK*>(sio->getDevice(PCLINK_CDEVIC));
            if(pclink->hasLink(slot+1) || pclink->hasLink(source+1))
            {
                sio->uninstallDevice(PCLINK_CDEVIC);
                pclink->swapLinks(slot+1,source+1);
                sio->installDevice(PCLINK_CDEVIC,pclink);
            }
            qDebug() << "!n" << tr("Swapped disk %1 with disk %2.").arg(slot + 1).arg(source + 1);
        }
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

        auto dlg = new CassetteDialog(this, files.at(0));
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
    if(isClosing)
        return;
    isClosing = true;

    // Save various session settings  //
    if (respeqtSettings->saveWindowsPos()) {
        if (g_miniMode) {
            saveMiniWindowGeometry();
        } else {
            saveWindowGeometry();
        }
    }
    if (g_sessionFile != "")
        respeqtSettings->saveSessionToFile(g_sessionFilePath + "/" + g_sessionFile);
    respeqtSettings->setD9DOVisible(g_D9DOVisible);
    bool wasRunning = ui->actionStartEmulation->isChecked();
    QMessageBox::StandardButton answer = QMessageBox::No;

    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }

    int toBeSaved = 0;

    for (int i = 0; i < DISK_COUNT; i++) {      //
        auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
        if (img && img->isModified()) {
            toBeSaved++;
        }
    }

    for (int i = 0; i < DISK_COUNT; i++) {      //
        auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
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

    //close any disk edit dialogs we have open
    for (int i = 0; i < DISK_COUNT; i++) {
        auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
        if (img && img->editDialog()) img->editDialog()->close();
    }

    //
    delete docDisplayWindow;
    docDisplayWindow = nullptr;

    for (int i = DISK_BASE_CDEVIC; i < (DISK_BASE_CDEVIC+DISK_COUNT); i++) {
        auto s = qobject_cast <SimpleDiskImage*> (sio->getDevice(i));
        if (s) {
            s->close();
        }
    }

    event->accept();

}

void MainWindow::hideEvent(QHideEvent *event)
{
    if (respeqtSettings->minimizeToTray()) {
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
        if (respeqtSettings->isFirstTime()) {
            if (QMessageBox::Yes == QMessageBox::question(this, tr("First run"),
                                       tr("You are running RespeQt for the first time.\n\nDo you want to open the options dialog?"),
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

bool MainWindow::eventFilter(QObject * /*obj*/, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        on_actionLogWindow_triggered();
    }
    return false;
}

void MainWindow::on_actionLogWindow_triggered()
{
    if (logWindow_ == nullptr)
    {
        logWindow_ = new LogDisplayDialog(this);
        int x, y, w, h;
        x = geometry().x();
        y = geometry().y();
        w = geometry().width();
        h = geometry().height();
        if (!g_miniMode) {
            logWindow_->setGeometry(x+w/1.9, y+30, logWindow_->geometry().width(), geometry().height());
        } else {
            logWindow_->setGeometry(x+20, y+60, w, h*2);
        }
        connect(this, SIGNAL(sendLogText(QString)), logWindow_, SLOT(getLogText(QString)));
        connect(this, SIGNAL(sendLogTextChange(QString)), logWindow_, SLOT(getLogTextChange(QString)));
        emit sendLogText(ui->textEdit->toHtml());
    }

    logWindow_->show();
}

void MainWindow::logChanged(QString text)
{
    emit sendLogTextChange(text);
}

void MainWindow::saveWindowGeometry()
{
    respeqtSettings->setLastHorizontalPos(geometry().x());
    respeqtSettings->setLastVerticalPos(geometry().y());
    respeqtSettings->setLastWidth(geometry().width());
    respeqtSettings->setLastHeight(geometry().height());
}

void MainWindow::saveMiniWindowGeometry()
{
    respeqtSettings->setLastMiniHorizontalPos(geometry().x());
    respeqtSettings->setLastMiniVerticalPos(geometry().y());
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
    g_miniMode = !g_miniMode;

    int i;
    for( i = 1; i < 8; ++i ) {
        diskWidgets[i]->setVisible( !g_miniMode );
    }

    for( ; i < DISK_COUNT; ++i ) {
        diskWidgets[i]->setVisible( !g_miniMode && g_D9DOVisible );
    }

    if(!g_miniMode) {
        /*if (g_D9DOVisible) {
            setMinimumWidth(688);
        } else
        {
            setMinimumWidth(344);
        }*/

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
        g_shadeMode = false;
    } else {
        g_savedGeometry = geometry();
        ui->textEdit->setVisible(false);
        setMinimumWidth(400);
        setMinimumHeight(100);
        setMaximumHeight(100);
        setGeometry(respeqtSettings->lastMiniHorizontalPos(), respeqtSettings->lastMiniVerticalPos(),
                    minimumWidth(), minimumHeight());
        ui->actionHideShowDrives->setDisabled(true);
        ui->actionToggleShade->setEnabled(true);
        if (respeqtSettings->enableShade()) {
            setWindowOpacity(0.25);
            setWindowFlags(Qt::FramelessWindowHint);
            g_shadeMode = true;
        } else {
            g_shadeMode = false;
        }
        QMainWindow::show();
    }
}

void MainWindow::showHideDrives()
{
    for( int i = 8; i < DISK_COUNT; ++i ) {
        diskWidgets[i]->setVisible(g_D9DOVisible);
    }
    for (int i = 2; i < PRINTER_COUNT; ++i ) {
        printerWidgets[i]->setVisible(g_D9DOVisible);
    }

    // infoWidget->setVisible(g_D9DOVisible);

    if( g_D9DOVisible ) {
        ui->actionHideShowDrives->setText(QApplication::translate("MainWindow", "Hide drives D9-DO", 0));
        ui->actionHideShowDrives->setStatusTip(QApplication::translate("MainWindow", "Hide drives D9-DO", 0));
        ui->actionHideShowDrives->setIcon(QIcon(":/icons/silk-icons/icons/drive_add.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        //setMinimumWidth(688);
    } else {
        ui->actionHideShowDrives->setText(QApplication::translate("MainWindow", "Show drives D9-DO", 0));
        ui->actionHideShowDrives->setStatusTip(QApplication::translate("MainWindow", "Show drives D9-DO", 0));
        ui->actionHideShowDrives->setIcon(QIcon(":/icons/silk-icons/icons/drive_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        //setMinimumWidth(344);
    }
}

// Toggle Hide/Show drives D9-DO  //
void MainWindow::on_actionHideShowDrives_triggered()
{
    g_D9DOVisible = !g_D9DOVisible;
    g_miniMode = false;

    showHideDrives();

    //setGeometry(geometry().x(), geometry().y(), 0, geometry().height());
    saveWindowGeometry();
}

// Toggle printer Emulation ON/OFF //
void MainWindow::on_actionPrinterEmulation_triggered()
{
    if (respeqtSettings->printerEmulation()) {
        setUpPrinterEmulationWidgets(false);
        respeqtSettings->setPrinterEmulation(false);
        qWarning() << "!i" << tr("Printer emulation stopped.");
    } else {
        setUpPrinterEmulationWidgets(true);
        respeqtSettings->setPrinterEmulation(true);
        qWarning() << "!i" << tr("Printer emulation started.");
    }
}

void MainWindow::setUpPrinterEmulationWidgets(bool enable)
{
    if (enable) {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/silk-icons/icons/printer.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setPixmap(QIcon(":/icons/silk-icons/icons/printer.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
    } else {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/silk-icons/icons/printer_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setPixmap(QIcon(":/icons/silk-icons/icons/printer_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
    }
    prtOnOffLabel->setToolTip(ui->actionPrinterEmulation->toolTip());
    prtOnOffLabel->setStatusTip(ui->actionPrinterEmulation->statusTip());
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
    for (int i = 0; i < PRINTER_COUNT; i++) {
        printerWidgets[i]->setSioWorker(sio);
    }
#ifndef QT_NO_DEBUG
    snapshot->show();
#endif
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
#ifndef QT_NO_DEBUG
    //snapshot->hide();
#endif
    qWarning() << "!i" << tr("Emulation stopped.");
}

void MainWindow::sioStatusChanged(QString status)
{
    speedLabel->setText(status);
    speedLabel->show();
    sio->setDisplayCommandName(respeqtSettings->isCommandName());
}

void MainWindow::deviceStatusChanged(int deviceNo)
{
    if (deviceNo >= DISK_BASE_CDEVIC && deviceNo < (DISK_BASE_CDEVIC+DISK_COUNT)) { // 0x31 - 0x3E
        auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo));

        DriveWidget *diskWidget = diskWidgets[deviceNo - DISK_BASE_CDEVIC];

        if (img) {

            // Show file name without the path and set toolTip & statusTip to show the path separately //
            QString filenamelabel;
            int i = -1;

            if (img->description() == tr("Folder image")) {
                i = img->originalFileName().lastIndexOf("\\"); // NOTE: This expects folder separators to be '\\'
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

            diskWidget->setLabelToolTips(img->originalFileName().left(i - 1),
                                         img->originalFileName(),
                                         img->description());

            bool enableEdit = img->editDialog() != 0;

            if (img->description() == tr("Folder image")) {
                diskWidget->showAsFolderMounted(filenamelabel, img->description(), enableEdit);
            } else {
                bool enableSave = false;

                if (img->isModified()) {
                    if (!diskWidget->isAutoSaveEnabled()) {    //
                        enableSave = true;
                    } else {
                        // Image is modified and autosave is checked, so save the image (no need to lock it)  //
                        bool saved;
                        saved = img->save();
                        if (!saved) {
                            int response = QMessageBox::question(this, tr("Save failed"),
                                            tr("'%1' cannot be saved, do you want to save the image with another name?").arg(img->originalFileName()),
                                            QMessageBox::Yes, QMessageBox::No);
                            if (response == QMessageBox::Yes) {
                                saveDiskAs(deviceNo);
                            }
                        }
                    }
                }
                diskWidget->showAsImageMounted(filenamelabel, img->description(), enableEdit, enableSave, img->isLeverOpen(), img->isHappyEnabled(), img->isChipOpen(),
                                               img->isTranslatorActive(), img->isToolDiskActive(), img->hasSeveralSides(), respeqtSettings->hideHappyMode(), respeqtSettings->hideChipMode(),
                                               respeqtSettings->hideNextImage(), respeqtSettings->hideOSBMode(), respeqtSettings->hideToolDisk());
            }

            img->setDisplayTransmission(respeqtSettings->displayTransmission());
            img->setSpyMode(respeqtSettings->isSpyMode());
            img->setDisassembleUploadedCode(respeqtSettings->disassembleUploadedCode());
            img->setTranslatorAutomaticDetection(respeqtSettings->translatorAutomaticDetection());
            img->setTranslatorDiskImagePath(respeqtSettings->translatorDiskImagePath());
            img->setToolDiskImagePath(respeqtSettings->toolDiskImagePath());
            img->setActivateChipModeWithTool(respeqtSettings->activateChipModeWithTool());
            img->setActivateHappyModeWithTool(respeqtSettings->activateHappyModeWithTool());
            auto fimg = qobject_cast <FirmwareDiskImage*> (sio->getDevice(deviceNo));
            if (fimg) {
                fimg->SetDisplayDriveHead(respeqtSettings->displayDriveHead());
                fimg->SetDisplayFdcCommands(respeqtSettings->displayFdcCommands());
                fimg->SetDisplayIndexPulse(respeqtSettings->displayIndexPulse());
                fimg->SetDisplayMotorOnOff(respeqtSettings->displayMotorOnOff());
                fimg->SetDisplayIDAddressMarks(respeqtSettings->displayIDAddressMarks());
                fimg->SetDisplayTrackInformation(respeqtSettings->displayTrackInformation());
                fimg->SetDisplayCpuInstructions(respeqtSettings->displayCpuInstructions());
                fimg->SetTraceFilename(respeqtSettings->traceFilename());
            }
        } else {
            diskWidget->showAsEmpty(respeqtSettings->hideHappyMode(), respeqtSettings->hideChipMode(), respeqtSettings->hideNextImage(), respeqtSettings->hideOSBMode(), respeqtSettings->hideToolDisk());
        }
    }
    updateHighSpeed();
}

void MainWindow::updateHighSpeed()
{
    if (sio->port() != nullptr) {
        int nbChip = 0;
        for(int i = 0 ; i < DISK_COUNT; ++i ) {
            auto disk = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
            Board *board = disk != nullptr ? disk->getBoardInfo() : nullptr;
            if ((board != nullptr) && (board->isChipOpen())) {
                nbChip++;
            }
        }
        if (nbChip > 0) {
            sio->port()->forceHighSpeed(52400);
        } else {
            sio->port()->forceHighSpeed(0);
        }
    }
}

void MainWindow::uiMessage(int t, QString message)
{
    if (message.at(0) == '"') {
        message.remove(0, 1);
    }
    if (message.at(message.count() - 1) == '"') {
        message.resize(message.count() - 1);
    }
    if (message.at(message.count() - 1) == '"') {
        message.resize(message.count() - 1);
    }

    // paragraph symbol is used to display the next characters, up to the end of line, with a fixed font
    int indexParagraph = message.indexOf("§");
    if (indexParagraph != -1) {
        message.remove(indexParagraph, 1);
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
            if (indexParagraph != -1) {
                QString header = message.left(indexParagraph);
                QString fixed = message.right(message.count() - indexParagraph);
                message = QString("<span style='color:gray'>%1 <span style='font-family:courier,monospace'>%2</span></span>").arg(header).arg(fixed);
            }
            else {
                message = QString("<span style='color:gray'>%1</span>").arg(message);
            }
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

    for (int i = DISK_BASE_CDEVIC; i < (DISK_BASE_CDEVIC+DISK_COUNT); i++) {    // 0x31 - 0x3E
        deviceStatusChanged(i);
    }
    sio->setAutoReconnect(respeqtSettings->sioAutoReconnect());

    ui->actionStartEmulation->trigger();
}

void MainWindow::changeFonts()
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    if (respeqtSettings->useLargeFont()) {
        //QFont font("Arial Black", 9, QFont::Normal);
        font.setPointSize(9);
        emit setFont(font);
    } else {
        font.setPointSize(8);
        //QFont font("MS Shell Dlg 2,8", 8, QFont::Normal);
        emit setFont(font);
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
    QString dir = respeqtSettings->lastSessionDir();

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
    for (int i = DISK_BASE_CDEVIC; i < (DISK_BASE_CDEVIC+DISK_COUNT); i++) {
        deviceStatusChanged(i);
    }

    ui->actionStartEmulation->trigger();
}



void MainWindow::openRecent()
{
    qDebug("open recent");
    auto action = qobject_cast<QAction*>(sender());
    if(action)
    {
        mountFileWithDefaultProtection(firstEmptyDiskSlot(), action->text());
    }
}

void MainWindow::updateRecentFileActions()
{
    for(int i = 0; i < NUM_RECENT_FILES; ++i)
    {
        QAction* action = this->recentFilesActions_[i];
        const RespeqtSettings::ImageSettings& image = respeqtSettings->recentImageSetting(i);

        if(image.fileName != "" )
        {
            action->setVisible(true);
            action->setText(image.fileName);
        }
        else
        {
            action->setVisible(false);
        }
    }
}


bool MainWindow::ejectImage(int no, bool ask)
{
    auto pclink = reinterpret_cast<PCLINK*>(sio->getDevice(PCLINK_CDEVIC));
    if(pclink->hasLink(no+1))
    {
        sio->uninstallDevice(PCLINK_CDEVIC);
        pclink->resetLink(no+1);
        sio->installDevice(PCLINK_CDEVIC,pclink);
    }

    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));

    if (ask && img && img->isModified()) {
        QMessageBox::StandardButton answer;
        answer = saveImageWhenClosing(no, QMessageBox::No, 0);
        if (answer == QMessageBox::Cancel) {
            return false;
        }
    }

    sio->uninstallDevice(no + DISK_BASE_CDEVIC);
    if (img) {
        delete img;
        diskWidgets[no]->showAsEmpty(respeqtSettings->hideHappyMode(), respeqtSettings->hideChipMode(), respeqtSettings->hideNextImage(), respeqtSettings->hideOSBMode(), respeqtSettings->hideToolDisk());
        respeqtSettings->unmountImage(no);
        updateRecentFileActions();
        deviceStatusChanged(no + DISK_BASE_CDEVIC);
        qDebug() << "!n" << tr("Unmounted disk %1").arg(no + 1);
    }
    return true;
}

int MainWindow::containingDiskSlot(const QPoint &point)
{
    int i;
    QPoint distance = centralWidget()->geometry().topLeft();
    for (i=0; i < DISK_COUNT; i++) {    //
        QRect rect = diskWidgets[i]->geometry().translated(distance);
        if (rect.contains(point)) {
            break;
        }
    }
    if (i > DISK_COUNT-1) {   //
        i = -1;
    }
    return i;
}

int MainWindow::firstEmptyDiskSlot(int startFrom, bool createOne)
{
    int i;
    for (i = startFrom; i < DISK_COUNT; i++) {  //
        if (!sio->getDevice(DISK_BASE_CDEVIC + i)) {
            break;
        }
    }
    if (i > DISK_COUNT-1) {   //
        if (createOne) {
            i = DISK_COUNT-1;
        } else {
            i = -1;
        }
    }
    emit newSlot(i);        //
    return i;
}

void MainWindow::bootExe(const QString &fileName)
{
    SioDevice *old = sio->getDevice(DISK_BASE_CDEVIC);
    AutoBoot loader(sio, old);
    AutoBootDialog dlg(this);

    bool highSpeed =    respeqtSettings->useHighSpeedExeLoader() &&
                        (respeqtSettings->serialPortHandshakingMethod() != HANDSHAKE_SOFTWARE);

    if (!loader.open(fileName, highSpeed)) {
        return;
    }

    sio->uninstallDevice(DISK_BASE_CDEVIC);
    sio->installDevice(DISK_BASE_CDEVIC, &loader);
    connect(&loader, SIGNAL(booterStarted()), &dlg, SLOT(booterStarted()));
    connect(&loader, SIGNAL(booterLoaded()), &dlg, SLOT(booterLoaded()));
    connect(&loader, SIGNAL(blockRead(int, int)), &dlg, SLOT(blockRead(int, int)));
    connect(&loader, SIGNAL(loaderDone()), &dlg, SLOT(loaderDone()));
    connect(&dlg, SIGNAL(keepOpen()), this, SLOT(keepBootExeOpen()));

    dlg.exec();

    sio->uninstallDevice(DISK_BASE_CDEVIC);
    if (old) {
        sio->installDevice(DISK_BASE_CDEVIC, old);
        sio->getDevice(DISK_BASE_CDEVIC);
    }
    if(!g_exefileName.isEmpty()) bootExe(g_exefileName);
}
// Make boot executable dialog persistant until it's manually closed //
void MainWindow::keepBootExeOpen()
{
    bootExe(g_exefileName);
}

void MainWindow::bootExeTriggered(const QString &fileName)
{
    QString path = respeqtSettings->lastRclDir();
    g_exefileName = path + "/" + fileName;
    if (!g_exefileName.isEmpty()) {
        respeqtSettings->setLastExeDir(QFileInfo(g_exefileName).absolutePath());
        bootExe(g_exefileName);
    }
}

void MainWindow::mountFileWithDefaultProtection(int no, const QString &fileName)
{
    // If fileName was passed from RCL it is an 8.1 name, so we need to find
    // the full PC name in order to validate it.  //
    QString atariFileName, atariLongName, path;

    g_rclFileName = fileName;
    atariFileName = fileName;

    if(atariFileName.left(1) == "*") {
        atariLongName = atariFileName.mid(1);
        QString path = respeqtSettings->lastRclDir();
        if(atariLongName == "") {
            sio->port()->writeDataNak();
            return;
        } else {
            atariFileName =  path + "/" + atariLongName;
        }
    }

    const RespeqtSettings::ImageSettings* imgSetting = respeqtSettings->getImageSettingsFromName(atariFileName);
    bool prot = (imgSetting!=nullptr) && imgSetting->isWriteProtected;
    mountFile(no, atariFileName, prot);
}

void MainWindow::mountFile(int no, const QString &fileName, bool /*prot*/)
{
    SimpleDiskImage *disk = nullptr;
    bool isDir = false;
    bool ask   = true;

    if (fileName.isEmpty()) {
        if(g_rclFileName.left(1) == "*") emit fileMounted(false);
        return;
    }

    FileTypes::FileType type = FileTypes::getFileType(fileName);

    if (type == FileTypes::Dir) {
        disk = new FolderImage(sio);
        isDir = true;
    } else {
        disk = installDiskImage(no);
    }

    if (disk) {
        auto oldDisk = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
        Board *board = oldDisk != nullptr ? oldDisk->getBoardInfo() : nullptr;
        if(g_rclFileName.left(1) == "*") ask = false;
        if (!disk->open(fileName, type) || !ejectImage(no, ask) ) {
            respeqtSettings->unmountImage(no);
            delete disk;
            if (board != nullptr) {
                delete board;
            }
            if(g_rclFileName.left(1) == "*") emit fileMounted(false);  //
            return;
        }
        else if (board != nullptr) {
            disk->setBoardInfo(board);
            delete board;
        }

        sio->installDevice(DISK_BASE_CDEVIC + no, disk);

        try {
            auto pclink = dynamic_cast<PCLINK*>(sio->getDevice(PCLINK_CDEVIC));
            if(pclink != nullptr && (isDir || pclink->hasLink(no+1)))
            {
                sio->uninstallDevice(PCLINK_CDEVIC);
                if(isDir)
                {
                    pclink->setLink(no+1,QDir::toNativeSeparators(fileName).toLatin1());
                }
                else
                {
                    pclink->resetLink(no+1);
                }
                sio->installDevice(PCLINK_CDEVIC,pclink);
            }
        } catch(std::bad_cast& e)
        {
            qDebug() << "!e " << tr("Bad cast for PCLINK");
        }

        diskWidgets[no]->updateFromImage(disk, respeqtSettings->hideHappyMode(), respeqtSettings->hideChipMode(), respeqtSettings->hideNextImage(), respeqtSettings->hideOSBMode(), respeqtSettings->hideToolDisk());

        respeqtSettings->mountImage(no, fileName, disk->isReadOnly());
        updateRecentFileActions();
        connect(disk, SIGNAL(statusChanged(int)), this, SLOT(deviceStatusChanged(int)), Qt::QueuedConnection);
        deviceStatusChanged(DISK_BASE_CDEVIC + no);

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

        if(g_rclFileName.left(1) == "*") emit fileMounted(true);  //
    }
}

SimpleDiskImage *MainWindow::installDiskImage(int no)
{
    eHARDWARE emulationLevel = respeqtSettings->firmwareType(no);
    QString name = respeqtSettings->firmwareName(emulationLevel);
    QString path = respeqtSettings->firmwarePath(emulationLevel);
    if ((emulationLevel != SIO_EMULATION) && (!firmwareAvailable(no, name, path))) {
        emulationLevel = SIO_EMULATION;
    }
    if (emulationLevel != SIO_EMULATION) {
        Rom *rom = m_romProvider->RegisterRom(name, path);
        FirmwareDiskImage *disk = new FirmwareDiskImage(sio, emulationLevel, name, rom, 288, respeqtSettings->firmwarePowerOnWithDisk(no));
        disk->setDisplayTransmission(respeqtSettings->displayTransmission());
        disk->setSpyMode(respeqtSettings->isSpyMode());
        disk->setTrackLayout(respeqtSettings->isTrackLayout());
        disk->setDisassembleUploadedCode(respeqtSettings->disassembleUploadedCode());
        disk->setTranslatorAutomaticDetection(respeqtSettings->translatorAutomaticDetection());
        disk->setTranslatorDiskImagePath(respeqtSettings->translatorDiskImagePath());
        disk->setToolDiskImagePath(respeqtSettings->toolDiskImagePath());
        disk->setActivateChipModeWithTool(respeqtSettings->activateChipModeWithTool());
        disk->setActivateHappyModeWithTool(respeqtSettings->activateHappyModeWithTool());
        disk->SetDisplayDriveHead(respeqtSettings->displayDriveHead());
        disk->SetDisplayFdcCommands(respeqtSettings->displayFdcCommands());
        disk->SetDisplayIndexPulse(respeqtSettings->displayIndexPulse());
        disk->SetDisplayMotorOnOff(respeqtSettings->displayMotorOnOff());
        disk->SetDisplayIDAddressMarks(respeqtSettings->displayIDAddressMarks());
        disk->SetDisplayTrackInformation(respeqtSettings->displayTrackInformation());
        disk->SetDisplayCpuInstructions(respeqtSettings->displayCpuInstructions());
        disk->SetTraceFilename(respeqtSettings->traceFilename());
        return disk;
    } else {
        SimpleDiskImage *disk = new SimpleDiskImage(sio);
        disk->setDisplayTransmission(respeqtSettings->displayTransmission());
        disk->setSpyMode(respeqtSettings->isSpyMode());
        disk->setTrackLayout(respeqtSettings->isTrackLayout());
        disk->setDisassembleUploadedCode(respeqtSettings->disassembleUploadedCode());
        disk->setTranslatorAutomaticDetection(respeqtSettings->translatorAutomaticDetection());
        disk->setTranslatorDiskImagePath(respeqtSettings->translatorDiskImagePath());
        disk->setToolDiskImagePath(respeqtSettings->toolDiskImagePath());
        disk->setActivateChipModeWithTool(respeqtSettings->activateChipModeWithTool());
        disk->setActivateHappyModeWithTool(respeqtSettings->activateHappyModeWithTool());
        return disk;
    }
}

bool MainWindow::firmwareAvailable(int no, QString &name, QString path)
{
    QString diskName = tr("Disk %1").arg(++no & 0x0F);
    if (path.isEmpty()) {
        qWarning() << "!w" << tr("[%1] No firmware ROM file selected for '%2' emulation. Please, check settings in menu Tools>Options>Firmware emulation. Using SIO emulation.")
                      .arg(diskName)
                      .arg(name);
        return false;
    }
    auto firmwareFile = new QFile(path);
    if (!firmwareFile->open(QFile::ReadOnly)) {
        delete firmwareFile;
        qWarning() << "!w" << tr("[%1] Firmware '%2' not found for '%3' emulation. Please, check settings in menu Tools>Options>Firmware emulation. Using SIO emulation.")
                      .arg(diskName)
                      .arg(path)
                      .arg(name);
        return false;
    }
    firmwareFile->close();
    delete firmwareFile;
    return true;
}

void MainWindow::mountDiskImage(int no)
{
    QString dir;
// Always mount from "last image dir" //
//    if (diskWidgets[no].fileNameLabel->text().isEmpty()) {
        dir = respeqtSettings->lastDiskImageDir();
//    } else {
//        dir = QFileInfo(diskWidgets[no].fileNameLabel->text()).absolutePath();
//    }
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open a disk image"),
                                                    dir,
                                                    tr(
                                                    "All Atari disk images (*.atr *.xfd *.atx *.pro *.scp);;"
                                                    "SIO2PC ATR images (*.atr);;"
                                                    "XFormer XFD images (*.xfd);;"
                                                    "ATX images (*.atx);;"
                                                    "Pro images (*.pro);;"
                                                    "SuperCard Pro images (*.scp);;"
                                                    "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    respeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
    mountFileWithDefaultProtection(no, fileName);
}

void MainWindow::mountFolderImage(int no)
{
    QString dir;
    // Always mount from "last folder dir" //
    dir = respeqtSettings->lastFolderImageDir();
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Open a folder image"), dir);
    fileName = QDir::fromNativeSeparators(fileName);    //
    if (fileName.isEmpty()) {
        return;
    }
    respeqtSettings->setLastFolderImageDir(fileName);
    mountFileWithDefaultProtection(no, fileName);
}

void MainWindow::loadNextSide(int no)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    mountFileWithDefaultProtection(no, img->getNextSideFilename());
}

void MainWindow::toggleHappy(int no, bool enabled)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    img->setHappyMode(enabled);
}

void MainWindow::toggleChip(int no, bool open)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    img->setChipMode(open);
    updateHighSpeed();
}

void MainWindow::toggleOSB(int no, bool open)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    img->setOSBMode(open);
}

void MainWindow::toggleToolDisk(int no, bool enabled)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    img->setToolDiskMode(enabled);
    updateHighSpeed();
}

void MainWindow::toggleWriteProtection(int no, bool protectionEnabled)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    img->setReadOnly(protectionEnabled);
    respeqtSettings->setMountedImageProtection(no, protectionEnabled);
}

void MainWindow::openEditor(int no)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    if (img->editDialog()) {
        img->editDialog()->close();
    } else {
        auto dlg = new DiskEditDialog();
        dlg->go(img);
        dlg->show();
    }
}

QMessageBox::StandardButton MainWindow::saveImageWhenClosing(int no, QMessageBox::StandardButton previousAnswer, int number)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));

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
    qApp->removeTranslator(&respeqt_qt_translator);
    qApp->removeTranslator(&respeqt_translator);
    if (respeqtSettings->i18nLanguage().compare("auto") == 0) {
        QString locale = QLocale::system().name();
        respeqt_translator.load(":/translations/i18n/respeqt_" + locale);
        respeqt_qt_translator.load(":/translations/i18n/qt_" + locale);
        qApp->installTranslator(&respeqt_qt_translator);
        qApp->installTranslator(&respeqt_translator);
    } else if (respeqtSettings->i18nLanguage().compare("en") != 0) {
        respeqt_translator.load(":/translations/i18n/respeqt_" + respeqtSettings->i18nLanguage());
        respeqt_qt_translator.load(":/translations/i18n/qt_" + respeqtSettings->i18nLanguage());
        qApp->installTranslator(&respeqt_qt_translator);
        qApp->installTranslator(&respeqt_translator);
    }
}

void MainWindow::saveDisk(int no)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));

    if (img->isUnnamed()) {
        saveDiskAs(no);
    } else {
        img->lock();
        bool saved = img->save();
        img->unlock();
        if (!saved) {
            if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
                .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                saveDiskAs(no);
            }
        }
    }
}
//
void MainWindow::autoCommit(int no, bool st)
{
    if( no < DISK_COUNT )
    {
        if ( (diskWidgets[no]->isAutoSaveEnabled() && st) || (!diskWidgets[no]->isAutoSaveEnabled() && !st) )
                          diskWidgets[no]->triggerAutoSaveClickIfEnabled();
    }
}


void MainWindow::happy(int no, bool st)
{
    if( no < DISK_COUNT )
    {
        if ( (diskWidgets[no]->isHappyEnabled() && st) || (!diskWidgets[no]->isHappyEnabled() && !st) )
                          diskWidgets[no]->triggerHappyClickIfEnabled();
    }
}


void MainWindow::chip(int no, bool st)
{
    if( no < DISK_COUNT )
    {
        if ( (diskWidgets[no]->isChipEnabled() && st) || (!diskWidgets[no]->isChipEnabled() && !st) )
                          diskWidgets[no]->triggerChipClickIfEnabled();
    }
}

void MainWindow::autoSaveDisk(int no)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));

    DriveWidget* widget = diskWidgets[no];

    if (img->isUnnamed()) {
        saveDiskAs(no);
        widget->updateFromImage(img, respeqtSettings->hideHappyMode(), respeqtSettings->hideChipMode(), respeqtSettings->hideNextImage(), respeqtSettings->hideOSBMode(), respeqtSettings->hideToolDisk());
        return;
    }

    bool autoSaveEnabled = widget->isAutoSaveEnabled();

    if (autoSaveEnabled) {
        qDebug() << "!n" << tr("[Disk %1] Auto-commit ON.").arg(no+1);
    } else {
        qDebug() << "!n" << tr("[Disk %1] Auto-commit OFF.").arg(no+1);
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
    }
    widget->updateFromImage(img, respeqtSettings->hideHappyMode(), respeqtSettings->hideChipMode(), respeqtSettings->hideNextImage(), respeqtSettings->hideOSBMode(), respeqtSettings->hideToolDisk());
}
//
void MainWindow::saveDiskAs(int no)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    QString dir, fileName;
    bool saved = false;

    if (img->isUnnamed()) {
        dir = respeqtSettings->lastDiskImageDir();
    } else {
        dir = QFileInfo(img->originalFileName()).absolutePath();
    }

    do {
        fileName = QFileDialog::getSaveFileName(this, tr("Save image as"),
                                 dir,
                                 tr(
                                                    "All Atari disk images (*.atr *.xfd *.atx *.pro *.scp);;"
                                                    "SIO2PC ATR images (*.atr);;"
                                                    "XFormer XFD images (*.xfd);;"
                                                    "ATX images (*.atx);;"
                                                    "Pro images (*.pro);;"
                                                    "SuperCard Pro images (*.scp);;"
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
        respeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
    }
    respeqtSettings->unmountImage(no);
    respeqtSettings->mountImage(no, fileName, img->isReadOnly());
}

void MainWindow::revertDisk(int no)
{
    auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    if (QMessageBox::question(this, tr("Revert to last saved"),
            tr("Do you really want to revert '%1' to its last saved state? You will lose the changes that has been made.")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        img->lock();
        img->reopen();
        img->unlock();
        deviceStatusChanged(DISK_BASE_CDEVIC + no);
    }
}

// Slots for handling actions for devices.
void MainWindow::on_actionMountDisk_triggered(int deviceId) {mountDiskImage(deviceId);}
void MainWindow::on_actionMountFolder_triggered(int deviceId) {mountFolderImage(deviceId);}
void MainWindow::on_actionEject_triggered(int deviceId) {ejectImage(deviceId);}
void MainWindow::on_actionNextSide_triggered(int deviceId) {loadNextSide(deviceId);}
void MainWindow::on_actionToggleHappy_triggered(int deviceId, bool open) {toggleHappy(deviceId, open);}
void MainWindow::on_actionToggleChip_triggered(int deviceId, bool open) {toggleChip(deviceId, open);}
void MainWindow::on_actionToggleOSB_triggered(int deviceId, bool open) {toggleOSB(deviceId, open);}
void MainWindow::on_actionToolDisk_triggered(int deviceId, bool open) {toggleToolDisk(deviceId, open);}
void MainWindow::on_actionWriteProtect_triggered(int deviceId, bool writeProtectEnabled) {toggleWriteProtection(deviceId, writeProtectEnabled);}
void MainWindow::on_actionEditDisk_triggered(int deviceId) {openEditor(deviceId);}
void MainWindow::on_actionSave_triggered(int deviceId) {saveDisk(deviceId);}
//
void MainWindow::on_actionAutoSave_triggered(int deviceId) {autoSaveDisk(deviceId);}
void MainWindow::on_actionSaveAs_triggered(int deviceId) {saveDiskAs(deviceId);}
void MainWindow::on_actionRevert_triggered(int deviceId) {revertDisk(deviceId);}

void MainWindow::on_actionMountRecent_triggered(const QString &fileName) {mountFileWithDefaultProtection(firstEmptyDiskSlot(), fileName);}

void MainWindow::on_actionEjectAll_triggered()
{
    QMessageBox::StandardButton answer = QMessageBox::No;

    int toBeSaved = 0;

    for (int i = 0; i < DISK_COUNT; i++) {  //
        auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
        if (img && img->isModified()) {
            toBeSaved++;
        }
    }

    if (!toBeSaved) {
        for (int i = DISK_COUNT-1; i >= 0; i--) {
            ejectImage(i);
        }
        return;
    }

    bool wasRunning = ui->actionStartEmulation->isChecked();
    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }

    for (int i = DISK_COUNT-1; i >= 0; i--) {
        auto img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
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
    for (int i = DISK_COUNT-1; i >= 0; i--) {
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

    int no = firstEmptyDiskSlot(0, true);

    SimpleDiskImage *disk = installDiskImage(no);
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

    if (!ejectImage(no)) {
        delete disk;
        return;
    }

    sio->installDevice(DISK_BASE_CDEVIC + no, disk);
    deviceStatusChanged(DISK_BASE_CDEVIC + no);
    qDebug() << "!n" << tr("[%1] Mounted '%2' as '%3'.")
            .arg(disk->deviceName())
            .arg(disk->originalFileName())
            .arg(disk->description());
}

void MainWindow::on_actionOpenSession_triggered()
{
    QString dir = respeqtSettings->lastSessionDir();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open session"),
                                 dir,
                                 tr(
                                         "RespeQt sessions (*.respeqt);;"
                                         "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
// First eject existing images, then mount session images and restore mainwindow position and size //
    MainWindow::on_actionEjectAll_triggered();

    respeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());
    g_sessionFile = QFileInfo(fileName).fileName();
    g_sessionFilePath = QFileInfo(fileName).absolutePath();

// Pass Session file name, path and MainWindow title to RespeQtSettings //
    respeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
    respeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    respeqtSettings->loadSessionFromFile(fileName);

    setWindowTitle(g_mainWindowTitle + tr(" -- Session: ") + g_sessionFile);
    setGeometry(respeqtSettings->lastHorizontalPos(), respeqtSettings->lastVerticalPos(), respeqtSettings->lastWidth() , respeqtSettings->lastHeight());

    for (int i = 0; i < DISK_COUNT; i++) {  //
        RespeqtSettings::ImageSettings is;
        is = respeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }
    g_D9DOVisible =  respeqtSettings->D9DOVisible();
    on_actionHideShowDrives_triggered();
    setSession();
}
void MainWindow::on_actionSaveSession_triggered()
{
    QString dir = respeqtSettings->lastSessionDir();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save session as"),
                                 dir,
                                 tr(
                                         "RespeQt sessions (*.respeqt);;"
                                         "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    respeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());

// Save mainwindow position and size to session file //
    if (respeqtSettings->saveWindowsPos()) {
        respeqtSettings->setLastHorizontalPos(geometry().x());
        respeqtSettings->setLastVerticalPos(geometry().y());
        respeqtSettings->setLastWidth(geometry().width());
        respeqtSettings->setLastHeight(geometry().height());
    }
    respeqtSettings->saveSessionToFile(fileName);
}

void MainWindow::on_actionBootExe_triggered()
{
    QString dir = respeqtSettings->lastExeDir();
    g_exefileName = QFileDialog::getOpenFileName(this, tr("Open executable"),
                                 dir,
                                 tr(
                                         "Atari executables (*.xex *.com *.exe);;"
                                         "All files (*)"));

    if (!g_exefileName.isEmpty()) {
        respeqtSettings->setLastExeDir(QFileInfo(g_exefileName).absolutePath());
        bootExe(g_exefileName);
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
                                                    respeqtSettings->lastCasDir(),
                                                    tr(
                                                    "CAS images (*.cas);;"
                                                    "All files (*)"));

    if (fileName.isEmpty()) {
        return;
    }
    respeqtSettings->setLastCasDir(QFileInfo(fileName).absolutePath());

    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
        sio->wait();
        qApp->processEvents();
    }

    auto dlg = new CassetteDialog(this, fileName);
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

void MainWindow::on_actionBootOption_triggered()
{
    QString folderPath = respeqtSettings->mountedImageSetting(0).fileName;
    BootOptionsDialog bod(folderPath, this);
    bod.exec();
}
