/*
 * respeqtsettings.h
 *
 * Copyright 2015 Joseph Zatarski
 * Copyright 2016, 2017 TheMontezuma
 * Copyright 2017 blind
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef RESPEQTSETTINGS_H
#define RESPEQTSETTINGS_H

#include <QSettings>


#define NUM_RECENT_FILES 10

class RespeqtSettings
{
public:
    class ImageSettings {
    public:
        QString fileName;
        bool isWriteProtected;
    };

    RespeqtSettings();
    ~RespeqtSettings();

    bool isFirstTime();

    QString serialPortName();
    void setSerialPortName(const QString &name);

    int serialPortHandshakingMethod();
    void setSerialPortHandshakingMethod(int method);

    bool serialPortTriggerOnFallingEdge();
    void setSerialPortTriggerOnFallingEdge(bool use);

    int serialPortMaximumSpeed();
    void setSerialPortMaximumSpeed(int speed);

    bool serialPortUsePokeyDivisors();
    void setSerialPortUsePokeyDivisors(bool use);

    int serialPortPokeyDivisor();
    void setSerialPortPokeyDivisor(int divisor);

    int serialPortWriteDelay();
    void setSerialPortWriteDelay(int delay);

    int serialPortCompErrDelay();
    void setSerialPortCompErrDelay(int delay);

    QString atariSioDriverName();
    void setAtariSioDriverName(const QString &name);

    int atariSioHandshakingMethod();
    void setAtariSioHandshakingMethod(int method);

    int backend();
    void setBackend(int backend);
#ifndef QT_NO_DEBUG
    QString testFile() const { return mTestFile; }
    void setTestFile(const QString testFile) { mTestFile = testFile; }
#endif

    bool useHighSpeedExeLoader();
    void setUseHighSpeedExeLoader(bool use);

    bool printerEmulation();
    void setPrinterEmulation(bool status);

    bool useCustomCasBaud();
    void setUseCustomCasBaud(bool use);

    int customCasBaud();
    void setCustomCasBaud(int baud);

    const ImageSettings* getImageSettingsFromName(const QString &fileName);

    const ImageSettings& mountedImageSetting(int no);

    void setMountedImageSetting(int no, const QString &fileName, bool prot);
    void setMountedImageProtection(int no, bool prot);
    const ImageSettings& recentImageSetting(int no);

    void mountImage(int no, const QString &fileName, bool prot);

    void unmountImage(int no);

    void swapImages(int no1, int no2);

    QString lastDiskImageDir();
    void setLastDiskImageDir(const QString &dir);

    QString lastFolderImageDir();
    void setLastFolderImageDir(const QString &dir);

    QString lastSessionDir();
    void setLastSessionDir(const QString &dir);

    QString lastExeDir();
    void setLastExeDir(const QString &dir);

    QString lastExtractDir();
    void setLastExtractDir(const QString &dir);

    QString lastPrinterTextDir();
    void setLastPrinterTextDir(const QString &dir);

    QString lastCasDir();
    void setLastCasDir(const QString &dir);

    // Set and restore last mainwindow position and size //
    int lastVerticalPos();
    void setLastVerticalPos(int lastVpos);

    int lastHorizontalPos();
    void setLastHorizontalPos(int lastHpos);

    int lastWidth();
    void setLastWidth(int lastW);

    int lastHeight();
    void setLastHeight(int lastH);

    // Set and restore last mini-window position //
    int lastMiniVerticalPos();
    void setLastMiniVerticalPos(int lastMVpos);

    int lastMiniHorizontalPos();
    void setLastMiniHorizontalPos(int lastMHpos);

    // Set and restore last printwindow position and size //
    int lastPrtVerticalPos();
    void setLastPrtVerticalPos(int lastVpos);

    int lastPrtHorizontalPos();
    void setLastPrtHorizontalPos(int lastHpos);

    int lastPrtWidth();
    void setLastPrtWidth(int lastW);

    int lastPrtHeight();
    void setLastPrtHeight(int lastH);

    QString i18nLanguage();
    void setI18nLanguage(const QString &$lang);

    bool minimizeToTray();
    void setMinimizeToTray(bool tray);

// Save window positions and sizes option //
    bool saveWindowsPos();
    void setsaveWindowsPos(bool saveMwp);

// Save drive visibility option //
    bool saveDiskVis();
    void setsaveDiskVis(bool saveDvis);

// To pass session file name/path  //
    void setSessionFile(const QString &g_sessionFile, const QString &g_sessionFilePath);

// To manipulate session files  //
    void saveSessionToFile(const QString &fileName);
    void loadSessionFromFile(const QString &fileName);

// To manipulate Main Window Title for Session file names    //
    void setMainWindowTitle(const QString &g_mainWindowTitle);

// Hide/Show drives D9-DO   //
    bool D9DOVisible();
    void setD9DOVisible(bool dVis);

// Filter special characters from file names in Folder Images
    bool filterUnderscore();
    void setfilterUnderscore(bool filter);

// CAPITAL letters in file names for PCLINK
    bool capitalLettersInPCLINK();
    void setCapitalLettersInPCLINK(bool caps);

// URL Submit feature
    bool isURLSubmitEnabled();
    void setURLSubmit(bool enabled);

// Enable Shade Mode //
    bool enableShade();
    void setEnableShade(bool shade);

// Use Large Font //
    bool useLargeFont();
    void setUseLargeFont(bool largeFont);

// Explorer Window On Top
    bool explorerOnTop();
    void setExplorerOnTop(bool expOnTop);

private:
    QSettings *mSettings;

    void writeRecentImageSettings();

    bool mIsFirstTime;

// To pass values from Mainwindow //
    int mMainX;
    int mMainY;
    int mMainW;
    int mMainH;
    int mMiniX;
    int mMiniY;
    int mPrtX;
    int mPrtY;
    int mPrtW;
    int mPrtH;

    bool msaveWindowsPos;
    bool msaveDiskVis;
    bool mdVis;
    QString mSessionFileName;
    QString mSessionFilePath;
    QString mMainWindowTitle;
//
    QString mSerialPortName;
    int mSerialPortHandshakingMethod;
    bool mSerialPortTriggerOnFallingEdge;
    int mSerialPortWriteDelay;
    int mSerialPortCompErrDelay;
    int mSerialPortMaximumSpeed;
    bool mSerialPortUsePokeyDivisors;
    int mSerialPortPokeyDivisor;

    bool mUseHighSpeedExeLoader;
    bool mPrinterEmulation;

    QString mAtariSioDriverName;
    int mAtariSioHandshakingMethod;

    int mBackend;
#ifndef QT_NO_DEBUG
    QString mTestFile;
#endif

    bool mUseCustomCasBaud;
    int mCustomCasBaud;

    ImageSettings mMountedImageSettings[16];    //

    ImageSettings mRecentImageSettings[NUM_RECENT_FILES];
    QString mLastDiskImageDir;
    QString mLastFolderImageDir;
    QString mLastSessionDir;
    QString mLastExeDir;
    QString mLastExtractDir;
    QString mLastPrinterTextDir;
    QString mLastCasDir;
    
    QString mI18nLanguage;

    bool mMinimizeToTray;
    bool mFilterUnderscore;
    bool mUseCapitalLettersInPCLINK;
    bool mUseURLSubmit;
    bool mUseLargeFont;
    bool mExplorerOnTop;
    bool mEnableShade;
};

extern RespeqtSettings *respeqtSettings;

#endif // RESPEQTSETTINGS_H
