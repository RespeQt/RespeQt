/*
 * mainwindow.h
 *
 * Copyright 2015 Joseph Zatarski
 * Copyright 2016 TheMontezuma
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QtDebug>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTranslator>
#include <QSystemTrayIcon>
#include <QTextEdit>

#include "optionsdialog.h"
#include "aboutdialog.h"
#include "createimagedialog.h"
#include "diskeditdialog.h"
#include "serialport.h"
#include "sioworker.h"
#include "textprinterwindow.h"
#include "docdisplaywindow.h"
#include "network.h"
#include "baseprinter.h"
#include "drivewidget.h"
#include "infowidget.h"

namespace Ui
{
    class MainWindow;
}
class DiskWidget
{
public:
    QLabel *fileNameLabel;
    QLabel *imagePropertiesLabel;
    QAction *saveAction;
    QAction *autoSaveAction;        //
    QAction *bootOptionAction;      //
    QAction *saveAsAction;
    QAction *revertAction;
    QAction *mountDiskAction;
    QAction *mountFolderAction;
    QAction *ejectAction;
    QAction *writeProtectAction;
    QAction *editAction;
    QFrame *frame;
};

class PrinterWidgets
{
public:
    QComboBox *printerTypeComboBox;
    QAction *actionSelectAtariPrinter;
    QAction *actionConnectPrinter;
    QAction *actionDisconnectPrinter;
    BasePrinter *printer;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString g_sessionFile;
    QString g_sessionFilePath;
    QString g_mainWindowTitle;
    static MainWindow *getInstance() { return instance; }
    TextPrinterWindow *getTextPrinterWindow() { return textPrinterWindow; }

public slots:
    void show();
    int firstEmptyDiskSlot(int startFrom = 0, bool createOne = true);       //
    void mountFileWithDefaultProtection(int no, const QString &fileName);   //
    void autoCommit(int no);                                                //
    void openRecent();

private:
    int untitledName;
    Ui::MainWindow *ui;
    SioWorker *sio;
    bool shownFirstTime;
    PrinterWidgets printerWidgets[PRINTER_COUNT]; //
    DriveWidget* diskWidgets[DISK_COUNT];    //
    InfoWidget* infoWidget;

    QLabel *speedLabel, *onOffLabel, *prtOnOffLabel, *netLabel, *clearMessagesLabel;  //
    TextPrinterWindow *textPrinterWindow;
    DocDisplayWindow *docDisplayWindow;    //
    QTranslator respeqt_translator, respeqt_qt_translator;
    QSystemTrayIcon trayIcon;
    Qt::WindowFlags oldWindowFlags;
    Qt::WindowStates oldWindowStates;
    QString lastMessage;
    int lastMessageRepeat;
    static MainWindow *instance;
    
    bool isClosing;

    QDialog *logWindow_;


    QList<QAction*> recentFilesActions_;

    void setSession();  //
    void updateRecentFileActions();
    int containingDiskSlot(const QPoint &point);
    void bootExe(const QString &fileName);
    void mountFile(int no, const QString &fileName, bool prot);
    void mountDiskImage(int no);
    void mountFolderImage(int no);
    bool ejectImage(int no, bool ask = true);
    void toggleWriteProtection(int no, bool protectionEnabled);
    void openEditor(int no);
    void saveDisk(int no);
    void saveDiskAs(int no);
    void revertDisk(int no);
    QMessageBox::StandardButton saveImageWhenClosing(int no, QMessageBox::StandardButton previousAnswer, int number);
    void loadTranslators();
    void autoSaveDisk(int no);                                              //
    void setUpPrinterEmulationWidgets(bool enabled);
    void connectPrinter(qint8 no);
    void disconnectPrinter(qint8 no);


    void createDeviceWidgets();

protected:
    void mousePressEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *);
    void dropEvent(QDropEvent *event);
    void closeEvent(QCloseEvent *event);
    void hideEvent(QHideEvent *event);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void resizeEvent(QResizeEvent *);
    bool eventFilter(QObject *obj, QEvent *event);

signals:
    void logMessage(int type, const QString &msg);
    void newSlot (int slot);
    void fileMounted(bool mounted);
    void sendLogText (QString logText);
    void sendLogTextChange (QString logTextChange);
    void setFont(const QFont &font);

public:
    void doLogMessage(int type, const QString &msg);

private slots:
    void on_actionPlaybackCassette_triggered();
    void on_actionShowPrinterTextOutput_triggered();
    void on_actionBootExe_triggered();
    void on_actionSaveSession_triggered();
    void on_actionOpenSession_triggered();
    void on_actionNewImage_triggered();
    void on_actionMountFolder_triggered();
    void on_actionMountDisk_triggered();
    void on_actionEjectAll_triggered();
    void on_actionOptions_triggered();
    void on_actionStartEmulation_triggered();
    void on_actionPrinterEmulation_triggered();
    void on_actionHideShowDrives_triggered();
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionDocumentation_triggered();

    // Device widget events
    void on_actionMountDisk_triggered(int deviceId);
    void on_actionMountFolder_triggered(int deviceId);
    void on_actionEject_triggered(int deviceId);
    void on_actionWriteProtect_triggered(int deviceId, bool writeProtectEnabled);
    void on_actionMountRecent_triggered(const QString &fileName);
    void on_actionEditDisk_triggered(int deviceId);
    void on_actionSave_triggered(int deviceId);
    void on_actionAutoSave_triggered(int deviceId);
    void on_actionSaveAs_triggered(int deviceId);
    void on_actionRevert_triggered(int deviceId);


    void on_actionBootOption_triggered();
    void on_actionToggleMiniMode_triggered();
    void on_actionToggleShade_triggered();
    void on_actionLogWindow_triggered();

    void on_actionConnectPrinter_17_triggered();
    void on_actionConnectPrinter_18_triggered();
    void on_actionConnectPrinter_19_triggered();
    void on_actionConnectPrinter_20_triggered();
    void on_actionDisconnectPrinter_17_triggered();
    void on_actionDisconnectPrinter_18_triggered();
    void on_actionDisconnectPrinter_19_triggered();
    void on_actionDisconnectPrinter_20_triggered();

    void showHideDrives();
    void sioFinished();
    void sioStarted();
    void sioStatusChanged(QString status);
    void textPrinterWindowClosed();
    void docDisplayWindowClosed();
    void deviceStatusChanged(int deviceNo);
    void uiMessage(int t, const QString message);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void keepBootExeOpen();
    void saveWindowGeometry();
    void saveMiniWindowGeometry();
    void logChanged(QString text);
    void changeFonts();
    void on_comboBoxPrinterType_17_currentIndexChanged(int index);
    void on_comboBoxPrinterType_18_currentIndexChanged(int index);
    void on_comboBoxPrinterType_19_currentIndexChanged(int index);
    void on_comboBoxPrinterType_20_currentIndexChanged(int index);
    void changePrinterType(int index, int typeId);
};

#endif // MAINWINDOW_H
