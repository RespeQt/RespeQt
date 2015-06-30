#ifndef ASPEQTSETTINGS_H
#define ASPEQTSETTINGS_H

#include <QSettings>

class AspeqtSettings
{
public:
    class ImageSettings {
    public:
        QString fileName;
        bool isWriteProtected;
    };

    AspeqtSettings();
    ~AspeqtSettings();

    bool isFirstTime();

    QString serialPortName();
    void setSerialPortName(const QString &name);

    int serialPortHandshakingMethod();
    void setSerialPortHandshakingMethod(int method);

    int serialPortMaximumSpeed();
    void setSerialPortMaximumSpeed(int speed);

    bool serialPortUsePokeyDivisors();
    void setSerialPortUsePokeyDivisors(bool use);

    int serialPortPokeyDivisor();
    void setSerialPortPokeyDivisor(int divisor);

    bool useHighSpeedExeLoader();
    void setUseHighSpeedExeLoader(bool use);

    QString atariSioDriverName();
    void setAtariSioDriverName(const QString &name);

    int atariSioHandshakingMethod();
    void setAtariSioHandshakingMethod(int method);

    int backend();
    void setBackend(int backend);

    bool useCustomCasBaud();
    void setUseCustomCasBaud(bool use);

    int customCasBaud();
    void setCustomCasBaud(int baud);

    ImageSettings getImageSettingsFromName(const QString &fileName);

    ImageSettings mountedImageSetting(int no);

    void setMountedImageSetting(int no, const QString &fileName, bool prot);
    ImageSettings recentImageSetting(int no);

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
    int mSerialPortMaximumSpeed;
    bool mSerialPortUsePokeyDivisors;
    int mSerialPortPokeyDivisor;

    bool mUseHighSpeedExeLoader;

    QString mAtariSioDriverName;
    int mAtariSioHandshakingMethod;

    int mBackend;

    bool mUseCustomCasBaud;
    int mCustomCasBaud;

    ImageSettings mMountedImageSettings[16];    //

    ImageSettings mRecentImageSettings[10];
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
    bool mUseLargeFont;
    bool mExplorerOnTop;
    bool mEnableShade;
};

extern AspeqtSettings *aspeqtSettings;

#endif // ASPEQTSETTINGS_H
