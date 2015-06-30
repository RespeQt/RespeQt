#include "aspeqtsettings.h"
#include "serialport.h"
#include "mainwindow.h"    //

AspeqtSettings::AspeqtSettings()
{
    mSettings = new QSettings();

    mIsFirstTime = mSettings->value("FirstTime", true).toBool();
    mSettings->setValue("FirstTime", false);

    // Set Window Position/Size defaults //
    mMainX = mSettings->value("MainX", 20).toInt();
    mMainY = mSettings->value("MainY", 40).toInt();
    mMainW = mSettings->value("MainW", 688).toInt();
    mMainH = mSettings->value("MainH", 426).toInt();
    mMiniX = mSettings->value("MiniX", 8).toInt();
    mMiniY = mSettings->value("MiniY", 30).toInt();
    mPrtX = mSettings->value("PrtX", 25).toInt();
    mPrtY = mSettings->value("PrtY", 45).toInt();
    mPrtW = mSettings->value("PrtW", 600).toInt();
    mPrtH = mSettings->value("PrtH", 486).toInt();

    /* Standard serial port backend */
    mSerialPortName = mSettings->value("SerialPortName", StandardSerialPortBackend::defaultPortName()).toString();
    mSerialPortHandshakingMethod = mSettings->value("HandshakingMethod", 0).toInt();
    mSerialPortMaximumSpeed = mSettings->value("MaximumSerialPortSpeed", 2).toInt();
    mSerialPortUsePokeyDivisors = mSettings->value("SerialPortUsePokeyDivisors", false).toBool();
    mSerialPortPokeyDivisor = mSettings->value("SerialPortPokeyDivisor", 6).toInt();

    mUseHighSpeedExeLoader = mSettings->value("UseHighSpeedExeLoader", false).toBool();

    mAtariSioDriverName = mSettings->value("AtariSioDriverName", AtariSioBackend::defaultPortName()).toString();
    mAtariSioHandshakingMethod = mSettings->value("AtariSioHandshakingMethod", 0).toInt();

    mBackend = mSettings->value("Backend", 0).toInt();

    mUseCustomCasBaud = mSettings->value("UseCustomCasBaud", false).toBool();
    mCustomCasBaud = mSettings->value("CustomCasBaud", 875).toInt();

    int i;

    mSettings->beginReadArray("MountedImageSettings");

    for (i = 0; i < 15; i++) {      //
        mSettings->setArrayIndex(i);
        mMountedImageSettings[i].fileName = mSettings->value("FileName", QString()).toString();
        mMountedImageSettings[i].isWriteProtected = mSettings->value("IsWriteProtected", false).toBool();
    }
    mSettings->endArray();

    mSettings->beginReadArray("RecentImageSettings");
    for (i = 0; i < 10; i++) {
        mSettings->setArrayIndex(i);
        mRecentImageSettings[i].fileName = mSettings->value("FileName", QString()).toString();
        mRecentImageSettings[i].isWriteProtected = mSettings->value("IsWriteProtected", false).toBool();
    }
    mSettings->endArray();

    mLastDiskImageDir = mSettings->value("LastDiskImageDir", "").toString();
    mLastFolderImageDir = mSettings->value("LastFolderImageDir", "").toString();
    mLastSessionDir = mSettings->value("LastSessionDir", "").toString();
    mLastExeDir = mSettings->value("LastExeDir", "").toString();
    mLastExtractDir = mSettings->value("LastExtractDir", "").toString();
    mLastPrinterTextDir = mSettings->value("LastPrinterTextDir", "").toString();
    mLastCasDir = mSettings->value("LastCasDir", "").toString();
    
    mI18nLanguage = mSettings->value("I18nLanguage", "auto").toString();

    mMinimizeToTray = mSettings->value("MinimizeToTray", false).toBool();
    msaveWindowsPos = mSettings->value("SaveWindowsPosSize", true).toBool();
    mFilterUnderscore = mSettings->value("FilterUnderscore", true).toBool();
    msaveDiskVis = mSettings->value("SaveDiskVisibility", true).toBool();
    mdVis = mSettings->value("D9DOVisible",true).toBool();
    if (mMainW < 688 && mdVis) mMainW = 688;
    if (mMainH < 426 && mdVis) mMainH = 426;
    mUseLargeFont = mSettings->value("UseLargeFont", false).toBool();
    mExplorerOnTop = mSettings->value("ExplorerOnTop", false).toBool();
    mEnableShade = mSettings->value("EnableShadeByDefault", true).toBool();

}

AspeqtSettings::~AspeqtSettings()
{
    delete mSettings;
}
// Get session file name from Mainwindow //
void AspeqtSettings::setSessionFile(const QString &g_sessionFile, const QString &g_sessionFilePath)
{
    mSessionFileName = g_sessionFile;
    mSessionFilePath = g_sessionFilePath;
}

// Save all session related settings, so that a session could be fully restored later //
void AspeqtSettings::saveSessionToFile(const QString &fileName)
{
    extern bool g_miniMode;
    QSettings s(fileName, QSettings::IniFormat);

    s.beginGroup("AspeQt");
        s.setValue("Backend", mBackend);
        s.setValue("AtariSioDriverName", mAtariSioDriverName);
        s.setValue("AtariSioHandshakingMethod", mAtariSioHandshakingMethod);
        s.setValue("SerialPortName", mSerialPortName);
        s.setValue("HandshakingMethod", mSerialPortHandshakingMethod);
        s.setValue("MaximumSerialPortSpeed", mSerialPortMaximumSpeed);
        s.setValue("SerialPortUsePokeyDivisors", mSerialPortUsePokeyDivisors);
        s.setValue("SerialPortPokeyDivisor", mSerialPortPokeyDivisor);
        s.setValue("UseHighSpeedExeLoader", mUseHighSpeedExeLoader);
        s.setValue("CustomCasBaud", mCustomCasBaud);
        s.setValue("UseCustomCasBaud", mUseCustomCasBaud);
        s.setValue("I18nLanguage", mI18nLanguage);
        s.setValue("SaveWindowsPosSize", msaveWindowsPos);
        s.setValue("SaveDiskVisibility", msaveDiskVis);
        s.setValue("D9DOVisible", mdVis);
        if (g_miniMode) {
            s.setValue("MiniX", mMiniX);
            s.setValue("MiniY", mMiniY);
        } else {
            s.setValue("MainX", mMainX);
            s.setValue("MainY", mMainY);
            s.setValue("MainW", mMainW);
            s.setValue("MainH", mMainH);
        }
        s.setValue("PrtX", mPrtX);
        s.setValue("PrtY", mPrtY);
        s.setValue("PrtW", mPrtW);
        s.setValue("PrtH", mPrtH);
        s.setValue("FilterUnderscore", mFilterUnderscore);
        s.setValue("UseLargeFont", mUseLargeFont);
        s.setValue("ExplorerOnTop", mExplorerOnTop);
        s.setValue("EnableShadeByDefault", mEnableShade);
    s.endGroup();
//
    s.beginWriteArray("MountedImageSettings");
    for (int i = 0; i < 15; i++) {                      //
        ImageSettings is = mMountedImageSettings[i];
        s.setArrayIndex(i);
        s.setValue("FileName", is.fileName);
        s.setValue("IsWriteProtected", is.isWriteProtected);
    }
    s.endArray();
}
// Get all session related settings, so that a session could be fully restored //
 void AspeqtSettings::loadSessionFromFile(const QString &fileName)
{
    QSettings s(fileName, QSettings::IniFormat);
    s.beginGroup("AspeQt");
        mBackend = s.value("Backend", 0).toInt();
        mAtariSioDriverName = s.value("AtariSioDriverName", AtariSioBackend::defaultPortName()).toString();
        mAtariSioHandshakingMethod = s.value("AtariSioHandshakingMethod", 0).toInt();
        mSerialPortName = s.value("SerialPortName", StandardSerialPortBackend::defaultPortName()).toString();
        mSerialPortHandshakingMethod = s.value("HandshakingMethod", 0).toInt();
        mSerialPortMaximumSpeed = s.value("MaximumSerialPortSpeed", 2).toInt();
        mSerialPortUsePokeyDivisors = s.value("SerialPortUsePokeyDivisors", false).toBool();
        mSerialPortPokeyDivisor = s.value("SerialPortPokeyDivisor", 6).toInt();
        mUseHighSpeedExeLoader = s.value("UseHighSpeedExeLoader", false).toBool();
        mCustomCasBaud = s.value("CustomCasBaud", 875).toInt();
        mUseCustomCasBaud = s.value("UseCustomCasBaud", false).toBool();
        mI18nLanguage = s.value("I18nLanguage").toString();
        msaveWindowsPos = s.value("SaveWindowsPosSize", true).toBool();
        msaveDiskVis = s.value("SaveDiskVisibility", true).toBool();
        mdVis = s.value("D9DOVisible", true).toBool();
        mMainX = s.value("MainX", 20).toInt();
        mMainY = s.value("MainY", 40).toInt();
        mMainW = s.value("MainW", 688).toInt();
        mMainH = s.value("MainH", 426).toInt();
        if (mMainW < 688 && mdVis) mMainW = 688;
        if (mMainH < 426 && mdVis) mMainH = 426;
        mMiniX = s.value("MiniX", 8).toInt();
        mMiniY = s.value("MiniY", 30).toInt();
        mPrtX = s.value("PrtX", 20).toInt();
        mPrtY = s.value("PrtY", 40).toInt();
        mPrtW = s.value("PrtW", 600).toInt();
        mPrtH = s.value("PrtH", 486).toInt();
        mFilterUnderscore = s.value("FilterUnderscore", true).toBool();
        mUseLargeFont = s.value("UseLargeFont", false).toBool();
        mExplorerOnTop = s.value("ExplorerOnTop", false).toBool();
        mEnableShade = s.value("EnableShadeByDefault", true).toBool();
    s.endGroup();
 //
    s.beginReadArray("MountedImageSettings");
    for (int i = 0; i < 15; i++) {              //
        s.setArrayIndex(i);
        setMountedImageSetting(i, s.value("FileName", "").toString(), s.value("IsWriteProtected", false).toBool());
    }
    s.endArray();
}
// Get MainWindow title from MainWindow  //
void AspeqtSettings::setMainWindowTitle(const QString &g_mainWindowTitle)
{
    mMainWindowTitle = g_mainWindowTitle;
}

bool AspeqtSettings::isFirstTime()
{
    return mIsFirstTime;
}

QString AspeqtSettings::serialPortName()
{
    return mSerialPortName;
}

void AspeqtSettings::setSerialPortName(const QString &name)
{
    mSerialPortName = name;
    if(mSessionFileName == "") mSettings->setValue("SerialPortName", mSerialPortName);
}

QString AspeqtSettings::atariSioDriverName()
{
    return mAtariSioDriverName;
}

void AspeqtSettings::setAtariSioDriverName(const QString &name)
{    
    mAtariSioDriverName = name;
    if(mSessionFileName == "") mSettings->setValue("AtariSioDriverName", mAtariSioDriverName);
}

int AspeqtSettings::atariSioHandshakingMethod()
{
    return mAtariSioHandshakingMethod;
}

void AspeqtSettings::setAtariSioHandshakingMethod(int method)
{    
    mAtariSioHandshakingMethod = method;
    if(mSessionFileName == "") mSettings->setValue("AtariSioHandshakingMethod", mAtariSioHandshakingMethod);
}

int AspeqtSettings::serialPortMaximumSpeed()
{
    return mSerialPortMaximumSpeed;
}

void AspeqtSettings::setSerialPortMaximumSpeed(int speed)
{    
    mSerialPortMaximumSpeed = speed;
    if(mSessionFileName == "") mSettings->setValue("MaximumSerialPortSpeed", mSerialPortMaximumSpeed);
}

bool AspeqtSettings::serialPortUsePokeyDivisors()
{
    return mSerialPortUsePokeyDivisors;
}

void AspeqtSettings::setSerialPortUsePokeyDivisors(bool use)
{    
    mSerialPortUsePokeyDivisors = use;
    if(mSessionFileName == "") mSettings->setValue("SerialPortUsePokeyDivisors", mSerialPortUsePokeyDivisors);
}

int AspeqtSettings::serialPortPokeyDivisor()
{
    return mSerialPortPokeyDivisor;
}

void AspeqtSettings::setSerialPortPokeyDivisor(int divisor)
{  
    mSerialPortPokeyDivisor = divisor;
    if(mSessionFileName == "") mSettings->setValue("SerialPortPokeyDivisor", mSerialPortPokeyDivisor);
}

int AspeqtSettings::serialPortHandshakingMethod()
{
    return mSerialPortHandshakingMethod;
}

void AspeqtSettings::setSerialPortHandshakingMethod(int method)
{ 
    mSerialPortHandshakingMethod = method;
    if(mSessionFileName == "") mSettings->setValue("HandshakingMethod", mSerialPortHandshakingMethod);
}

int AspeqtSettings::backend()
{
    return mBackend;
}

void AspeqtSettings::setBackend(int backend)
{   
    mBackend = backend;
    if(mSessionFileName == "") mSettings->setValue("Backend", mBackend);
}

bool AspeqtSettings::useHighSpeedExeLoader()
{
    return mUseHighSpeedExeLoader;
}

void AspeqtSettings::setUseHighSpeedExeLoader(bool use)
{   
    mUseHighSpeedExeLoader = use;
    if(mSessionFileName == "") mSettings->setValue("UseHighSpeedExeLoader", mUseHighSpeedExeLoader);
}

bool AspeqtSettings::useCustomCasBaud()
{
    return mUseCustomCasBaud;
}

void AspeqtSettings::setUseCustomCasBaud(bool use)
{   
    mUseCustomCasBaud = use;
    if(mSessionFileName == "") mSettings->setValue("UseCustomCasBaud", mUseCustomCasBaud);
}

int AspeqtSettings::customCasBaud()
{
    return mCustomCasBaud;
}

void AspeqtSettings::setCustomCasBaud(int baud)
{    
    mCustomCasBaud = baud;
    if(mSessionFileName == "") mSettings->setValue("CustomCasBaud", mCustomCasBaud);
}

AspeqtSettings::ImageSettings AspeqtSettings::getImageSettingsFromName(const QString &fileName)
{
    ImageSettings is;
    int i;
    bool found = false;

    for (i = 0; i < 15; i++) {          //
        if (mMountedImageSettings[i].fileName == fileName) {
            is = mMountedImageSettings[i];
            found = true;
            break;
        }
    }
    if (!found) {
        for (i = 0; i < 10; i++) {
            if (mRecentImageSettings[i].fileName == fileName) {
                is = mRecentImageSettings[i];
                found = true;
                break;
            }
        }
    }
    if (!found) {
        is.fileName = fileName;
        is.isWriteProtected = true;
    }
    return is;
}

AspeqtSettings::ImageSettings AspeqtSettings::mountedImageSetting(int no)
{
    return mMountedImageSettings[no];
}

AspeqtSettings::ImageSettings AspeqtSettings::recentImageSetting(int no)
{
    return mRecentImageSettings[no];
}

void AspeqtSettings::setMountedImageSetting(int no, const QString &fileName, bool prot)
{

    mMountedImageSettings[no].fileName = fileName;
    mMountedImageSettings[no].isWriteProtected = prot;
    if(mSessionFileName == "") mSettings->setValue(QString("MountedImageSettings/%1/FileName").arg(no+1), fileName);
    if(mSessionFileName == "") mSettings->setValue(QString("MountedImageSettings/%1/IsWriteProtected").arg(no+1), prot);
}
void AspeqtSettings::mountImage(int no, const QString &fileName, bool prot)
{
    if (fileName.isEmpty()) {
        return;
    }
    int i;
    bool found = false;
    for (i = 0; i < 10; i++) {
        if (mRecentImageSettings[i].fileName == fileName) {
            found = true;
            break;
        }
    }
    if (found) {
        for (int j = i; j < 9; j++) {
            mRecentImageSettings[j] = mRecentImageSettings[j + 1];
        }
        mRecentImageSettings[9].fileName = "";
        writeRecentImageSettings();
    }
    setMountedImageSetting(no, fileName, prot);
}

void AspeqtSettings::unmountImage(int no)
{
    ImageSettings is = mMountedImageSettings[no];

    for (int i = 9; i > 0; i--) {
            mRecentImageSettings[i] = mRecentImageSettings[i - 1];
    }
    mRecentImageSettings[0] = is;
    writeRecentImageSettings();

    setMountedImageSetting(no, "", false);
}

void AspeqtSettings::swapImages(int no1, int no2)
{    
    ImageSettings is1 = mountedImageSetting(no1);
    ImageSettings is2 = mountedImageSetting(no2);
    setMountedImageSetting(no1, is2.fileName, is2.isWriteProtected);
    setMountedImageSetting(no2, is1.fileName, is1.isWriteProtected);
}
// Save drive visibility status //
bool AspeqtSettings::saveDiskVis()
{
    return msaveDiskVis;
}

void AspeqtSettings::setsaveDiskVis(bool saveDvis)
{
    msaveDiskVis = saveDvis;
    if(mSessionFileName == "") mSettings->setValue("SaveDiskVisibility", msaveDiskVis);
}

// Drive visibility status //
bool AspeqtSettings::D9DOVisible()
{
    return mdVis;
}

void AspeqtSettings::setD9DOVisible(bool dVis)
{
    mdVis = dVis;
    if(mSessionFileName == "") mSettings->setValue("D9DOVisible", mdVis);
}
// Shade Mode Enable //
bool AspeqtSettings::enableShade()
{
    return mEnableShade;
}

void AspeqtSettings::setEnableShade(bool shade)
{
    mEnableShade = shade;
    if(mSessionFileName == "") mSettings->setValue("EnableShadeByDefault", mEnableShade);
}

// Use Large Font //
bool AspeqtSettings::useLargeFont()
{
    return mUseLargeFont;
}

void AspeqtSettings::setUseLargeFont(bool largeFont)
{
    mUseLargeFont = largeFont;
    if(mSessionFileName == "") mSettings->setValue("UseLargeFont", mUseLargeFont);
}

// Explorer Window always on top
bool AspeqtSettings::explorerOnTop()
{
    return mExplorerOnTop;
}

void AspeqtSettings::setExplorerOnTop(bool expOnTop)
{
    mExplorerOnTop = expOnTop;
    if(mSessionFileName =="") mSettings->setValue("ExplorerOnTop", mExplorerOnTop);
}

// Save/return last main window position/size option //
bool AspeqtSettings::saveWindowsPos()
{
    return msaveWindowsPos;
}

void AspeqtSettings::setsaveWindowsPos(bool saveMwp)
{    
    msaveWindowsPos = saveMwp;
    if(mSessionFileName == "") mSettings->setValue("SaveWindowsPosSize", msaveWindowsPos);
}
// Last main window position/size (No Session File) //

int AspeqtSettings::lastHorizontalPos()
{
    return mMainX;
}

void AspeqtSettings::setLastHorizontalPos(int lastHpos)
{    
    mMainX = lastHpos;
    if(mSessionFileName == "") mSettings->setValue("MainX", mMainX);
}
int AspeqtSettings::lastVerticalPos()
{
    return mMainY;
}

void AspeqtSettings::setLastVerticalPos(int lastVpos)
{    
    mMainY = lastVpos;
    if(mSessionFileName == "") mSettings->setValue("MainY", mMainY);
}
int AspeqtSettings::lastWidth()
{
    return mMainW;
}

void AspeqtSettings::setLastWidth(int lastW)
{    
    mMainW = lastW;
    if(mSessionFileName == "") mSettings->setValue("MainW", mMainW);
}
int AspeqtSettings::lastHeight()
{
    return mMainH;
}

void AspeqtSettings::setLastHeight(int lastH)
{
    mMainH = lastH;
    if(mSessionFileName == "") mSettings->setValue("MainH", mMainH);
}
// Last mini window position (No Session File) //

int AspeqtSettings::lastMiniHorizontalPos()
{
    return mMiniX;
}

void AspeqtSettings::setLastMiniHorizontalPos(int lastMHpos)
{
    mMiniX = lastMHpos;
    if(mSessionFileName == "") mSettings->setValue("MiniX", mMiniX);
}
int AspeqtSettings::lastMiniVerticalPos()
{
    return mMiniY;
}

void AspeqtSettings::setLastMiniVerticalPos(int lastMVpos)
{
    mMiniY = lastMVpos;
    if(mSessionFileName == "") mSettings->setValue("MiniY", mMiniY);
}

// Last print window position/size (No Session File) //

int AspeqtSettings::lastPrtHorizontalPos()
{
    return mPrtX;
}

void AspeqtSettings::setLastPrtHorizontalPos(int lastPrtHpos)
{
    mPrtX = lastPrtHpos;
    if(mSessionFileName == "") mSettings->setValue("PrtX", mPrtX);
}
int AspeqtSettings::lastPrtVerticalPos()
{
    return mPrtY;
}

void AspeqtSettings::setLastPrtVerticalPos(int lastPrtVpos)
{
    mPrtY = lastPrtVpos;
    if(mSessionFileName == "") mSettings->setValue("PrtY", mPrtY);
}
int AspeqtSettings::lastPrtWidth()
{
    return mPrtW;
}

void AspeqtSettings::setLastPrtWidth(int lastPrtW)
{
    mPrtW = lastPrtW;
    if(mSessionFileName == "") mSettings->setValue("PrtW", mPrtW);
}
int AspeqtSettings::lastPrtHeight()
{
    return mPrtH;
}

void AspeqtSettings::setLastPrtHeight(int lastPrtH)
{
    mPrtH = lastPrtH;
    if(mSessionFileName == "") mSettings->setValue("PrtH", mPrtH);
}
QString AspeqtSettings::lastDiskImageDir()
{
    return mLastDiskImageDir;
}

void AspeqtSettings::setLastDiskImageDir(const QString &dir)
{
    mLastDiskImageDir = dir;
    mSettings->setValue("LastDiskImageDir", mLastDiskImageDir);
}

QString AspeqtSettings::lastFolderImageDir()
{
    return mLastFolderImageDir;
}

void AspeqtSettings::setLastFolderImageDir(const QString &dir)
{
    mLastFolderImageDir = dir;
    mSettings->setValue("LastFolderImageDir", mLastFolderImageDir);
}

QString AspeqtSettings::lastSessionDir()
{
    return mLastSessionDir;
}

void AspeqtSettings::setLastSessionDir(const QString &dir)
{
    mLastSessionDir = dir;
//    mSettings->setValue("LastSessionDir", mLastFolderImageDir);  //
    mSettings->setValue("LastSessionDir", mLastSessionDir);
}

QString AspeqtSettings::lastExeDir()
{
    return mLastExeDir;
}

void AspeqtSettings::setLastExeDir(const QString &dir)
{
    mLastExeDir = dir;
    mSettings->setValue("LastExeDir", mLastExeDir);
}

QString AspeqtSettings::lastExtractDir()
{
    return mLastExtractDir;
}

void AspeqtSettings::setLastExtractDir(const QString &dir)
{
    mLastExtractDir = dir;
    mSettings->setValue("LastExtractDir", mLastExeDir);
}

QString AspeqtSettings::lastPrinterTextDir()
{
    return mLastPrinterTextDir;
}

void AspeqtSettings::setLastPrinterTextDir(const QString &dir)
{
    mLastPrinterTextDir = dir;
    mSettings->setValue("LastPrinterTextDir", mLastPrinterTextDir);
}

QString AspeqtSettings::lastCasDir()
{
    return mLastCasDir;
}

void AspeqtSettings::setLastCasDir(const QString &dir)
{
    mLastCasDir = dir;
    mSettings->setValue("LastCasDir", mLastCasDir);
}

QString AspeqtSettings::i18nLanguage()
{
    return mI18nLanguage;
}

void AspeqtSettings::setI18nLanguage(const QString &lang)
{

    mI18nLanguage = lang;
    if(mSessionFileName == "") mSettings->setValue("I18nLanguage", mI18nLanguage);
}

bool AspeqtSettings::minimizeToTray()
{
    return mMinimizeToTray;
}

void AspeqtSettings::setMinimizeToTray(bool tray)
{
    mMinimizeToTray = tray;
    mSettings->setValue("MinimizeToTray", mMinimizeToTray);
}

bool AspeqtSettings::filterUnderscore()
{
    return mFilterUnderscore;
}

void AspeqtSettings::setfilterUnderscore(bool filter)
{
    mFilterUnderscore = filter;
    mSettings->setValue("FilterUnderscore", mFilterUnderscore);
}

void AspeqtSettings::writeRecentImageSettings()
{
    mSettings->beginWriteArray("RecentImageSettings");
    for (int i = 0; i < 10; i++) {
        mSettings->setArrayIndex(i);
        mSettings->setValue("FileName", mRecentImageSettings[i].fileName);
        mSettings->setValue("IsWriteProtected", mRecentImageSettings[i].isWriteProtected);
    }
    mSettings->endArray();
}
