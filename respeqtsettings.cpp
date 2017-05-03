/*
 * respeqtsettings.cpp
 *
 * Copyright 2015 Joseph Zatarski
 * Copyright 2016, 2017 TheMontezuma
 * Copyright 2017 blind
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "respeqtsettings.h"
#include "serialport.h"
#include "mainwindow.h"

RespeqtSettings::RespeqtSettings()
{
    mSettings = new QSettings(); //uses QApplication's info to determine setting to use

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
    /* old rergistry entries contain SERIAL_PORT_LOCATION at the front of the serial port name, so we skip them now */
    if(mSerialPortName.startsWith(SERIAL_PORT_LOCATION,Qt::CaseInsensitive))
    {
        mSerialPortName.remove(0,strlen(SERIAL_PORT_LOCATION));
    }
    mSerialPortHandshakingMethod = mSettings->value("HandshakingMethod", 0).toInt();
    mSerialPortTriggerOnFallingEdge = mSettings->value("FallingEdge", false).toBool();
    mSerialPortWriteDelay = mSettings->value("WriteDelay", 1).toInt();
#ifdef Q_OS_WIN
    mSerialPortCompErrDelay = mSettings->value("CompErrDelay", 300).toInt(); // default is 300us for windows
#else
    mSerialPortCompErrDelay = mSettings->value("CompErrDelay", 800).toInt(); // default value of 800us works OK with FTDI USB on linux/OSX
#endif
    mSerialPortMaximumSpeed = mSettings->value("MaximumSerialPortSpeed", 2).toInt();
    mSerialPortUsePokeyDivisors = mSettings->value("SerialPortUsePokeyDivisors", false).toBool();
    mSerialPortPokeyDivisor = mSettings->value("SerialPortPokeyDivisor", 6).toInt();

    /* AtariSIO backend */
    mAtariSioDriverName = mSettings->value("AtariSioDriverName", AtariSioBackend::defaultPortName()).toString();
    /* old rergistry entries contain SERIAL_PORT_LOCATION at the front of the serial port name, so we skip them now */
    if(mAtariSioDriverName.startsWith(SERIAL_PORT_LOCATION,Qt::CaseInsensitive))
    {
        mAtariSioDriverName.remove(0,strlen(SERIAL_PORT_LOCATION));
    }
    mAtariSioHandshakingMethod = mSettings->value("AtariSioHandshakingMethod", 0).toInt();

    mBackend = mSettings->value("Backend", 0).toInt();
#ifndef QT_NO_DEBUG
    if (mBackend == SERIAL_BACKEND_TEST) {
        mBackend = SERIAL_BACKEND_STANDARD;
    }
#endif

    mUseHighSpeedExeLoader = mSettings->value("UseHighSpeedExeLoader", false).toBool();
    mPrinterEmulation = mSettings->value("PrinterEmulation", true).toBool();

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
    for (i = 0; i < NUM_RECENT_FILES; i++) {
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
    mUseCapitalLettersInPCLINK = mSettings->value("CapitalLettersInPCLINK", false).toBool();
    mUseURLSubmit = mSettings->value("URLSubmit", false).toBool();
    msaveDiskVis = mSettings->value("SaveDiskVisibility", true).toBool();
    mdVis = mSettings->value("D9DOVisible",true).toBool();
    if (mMainW < 688 && mdVis) mMainW = 688;
    if (mMainH < 426 && mdVis) mMainH = 426;
    mUseLargeFont = mSettings->value("UseLargeFont", false).toBool();
    mExplorerOnTop = mSettings->value("ExplorerOnTop", false).toBool();
    mEnableShade = mSettings->value("EnableShadeByDefault", true).toBool();

}

RespeqtSettings::~RespeqtSettings()
{
    delete mSettings;
}
// Get session file name from Mainwindow //
void RespeqtSettings::setSessionFile(const QString &g_sessionFile, const QString &g_sessionFilePath)
{
    mSessionFileName = g_sessionFile;
    mSessionFilePath = g_sessionFilePath;
}

// Save all session related settings, so that a session could be fully restored later //
void RespeqtSettings::saveSessionToFile(const QString &fileName)
{
    extern bool g_miniMode;
    QSettings s(fileName, QSettings::IniFormat);

#ifndef QT_NO_DEBUG
    if (mBackend == SERIAL_BACKEND_TEST) {
        mBackend = SERIAL_BACKEND_STANDARD;
    }
#endif

    s.beginGroup("RespeQt");
        s.setValue("Backend", mBackend);
        s.setValue("AtariSioDriverName", mAtariSioDriverName);
        s.setValue("AtariSioHandshakingMethod", mAtariSioHandshakingMethod);
        s.setValue("SerialPortName", mSerialPortName);
        s.setValue("HandshakingMethod", mSerialPortHandshakingMethod);
        s.setValue("FallingEdge", mSerialPortTriggerOnFallingEdge);
        s.setValue("WriteDelay", mSerialPortWriteDelay);
        s.setValue("CompErrDelay", mSerialPortCompErrDelay);
        s.setValue("MaximumSerialPortSpeed", mSerialPortMaximumSpeed);
        s.setValue("SerialPortUsePokeyDivisors", mSerialPortUsePokeyDivisors);
        s.setValue("SerialPortPokeyDivisor", mSerialPortPokeyDivisor);
        s.setValue("UseHighSpeedExeLoader", mUseHighSpeedExeLoader);
        s.setValue("PrinterEmulation", mPrinterEmulation);
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
        s.setValue("CapitalLettersInPCLINK", mUseCapitalLettersInPCLINK);
        s.setValue("URLSubmit", mUseURLSubmit);
        s.setValue("UseLargeFont", mUseLargeFont);
        s.setValue("ExplorerOnTop", mExplorerOnTop);
        s.setValue("EnableShadeByDefault", mEnableShade);
    s.endGroup();
//
    s.beginWriteArray("MountedImageSettings");
    for (int i = 0; i < 15; i++) {                      //
        ImageSettings& is = mMountedImageSettings[i];
        s.setArrayIndex(i);
        s.setValue("FileName", is.fileName);
        s.setValue("IsWriteProtected", is.isWriteProtected);
    }
    s.endArray();
}
// Get all session related settings, so that a session could be fully restored //
 void RespeqtSettings::loadSessionFromFile(const QString &fileName)
{
    QSettings s(fileName, QSettings::IniFormat);
    s.beginGroup("RespeQt");
        mBackend = s.value("Backend", 0).toInt();
        mAtariSioDriverName = s.value("AtariSioDriverName", AtariSioBackend::defaultPortName()).toString();
        mAtariSioHandshakingMethod = s.value("AtariSioHandshakingMethod", 0).toInt();
        mSerialPortName = s.value("SerialPortName", StandardSerialPortBackend::defaultPortName()).toString();
        mSerialPortHandshakingMethod = s.value("HandshakingMethod", 0).toInt();
        mSerialPortTriggerOnFallingEdge = s.value("FallingEdge", false).toBool();
        mSerialPortWriteDelay = s.value("WriteDelay", 1).toInt();
        mSerialPortCompErrDelay = s.value("CompErrDelay", 1).toInt();
        mSerialPortMaximumSpeed = s.value("MaximumSerialPortSpeed", 2).toInt();
        mSerialPortUsePokeyDivisors = s.value("SerialPortUsePokeyDivisors", false).toBool();
        mSerialPortPokeyDivisor = s.value("SerialPortPokeyDivisor", 6).toInt();
        mUseHighSpeedExeLoader = s.value("UseHighSpeedExeLoader", false).toBool();
        mPrinterEmulation = s.value("PrinterEmulation", true).toBool();
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
        mUseCapitalLettersInPCLINK = s.value("CapitalLettersInPCLINK", false).toBool();
        mUseURLSubmit = s.value("URLSubmit", false).toBool();
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
void RespeqtSettings::setMainWindowTitle(const QString &g_mainWindowTitle)
{
    mMainWindowTitle = g_mainWindowTitle;
}

bool RespeqtSettings::isFirstTime()
{
    return mIsFirstTime;
}

QString RespeqtSettings::serialPortName()
{
    return mSerialPortName;
}

void RespeqtSettings::setSerialPortName(const QString &name)
{
    mSerialPortName = name;
    if(mSessionFileName == "") mSettings->setValue("SerialPortName", mSerialPortName);
}

int RespeqtSettings::serialPortMaximumSpeed()
{
    return mSerialPortMaximumSpeed;
}

void RespeqtSettings::setSerialPortMaximumSpeed(int speed)
{
    mSerialPortMaximumSpeed = speed;
    if(mSessionFileName == "") mSettings->setValue("MaximumSerialPortSpeed", mSerialPortMaximumSpeed);
}

bool RespeqtSettings::serialPortUsePokeyDivisors()
{
    return mSerialPortUsePokeyDivisors;
}

void RespeqtSettings::setSerialPortUsePokeyDivisors(bool use)
{
    mSerialPortUsePokeyDivisors = use;
    if(mSessionFileName == "") mSettings->setValue("SerialPortUsePokeyDivisors", mSerialPortUsePokeyDivisors);
}

int RespeqtSettings::serialPortPokeyDivisor()
{
    return mSerialPortPokeyDivisor;
}

void RespeqtSettings::setSerialPortPokeyDivisor(int divisor)
{
    mSerialPortPokeyDivisor = divisor;
    if(mSessionFileName == "") mSettings->setValue("SerialPortPokeyDivisor", mSerialPortPokeyDivisor);
}

int RespeqtSettings::serialPortHandshakingMethod()
{
    return mSerialPortHandshakingMethod;
}

void RespeqtSettings::setSerialPortHandshakingMethod(int method)
{
    mSerialPortHandshakingMethod = method;
    if(mSessionFileName == "") mSettings->setValue("HandshakingMethod", mSerialPortHandshakingMethod);
}

bool RespeqtSettings::serialPortTriggerOnFallingEdge()
{
    return mSerialPortTriggerOnFallingEdge;
}

void RespeqtSettings::setSerialPortTriggerOnFallingEdge(bool use)
{
    mSerialPortTriggerOnFallingEdge = use;
    if(mSessionFileName == "") mSettings->setValue("FallingEdge", mSerialPortTriggerOnFallingEdge);
}

int RespeqtSettings::serialPortWriteDelay()
{
    return mSerialPortWriteDelay;
}

void RespeqtSettings::setSerialPortWriteDelay(int delay)
{
    mSerialPortWriteDelay = delay;
    if(mSessionFileName == "") mSettings->setValue("WriteDelay", mSerialPortWriteDelay);
}

int RespeqtSettings::serialPortCompErrDelay()
{
    return mSerialPortCompErrDelay;
}

void RespeqtSettings::setSerialPortCompErrDelay(int delay)
{
    mSerialPortCompErrDelay = delay;
    if(mSessionFileName == "") mSettings->setValue("CompErrDelay", mSerialPortCompErrDelay);
}

QString RespeqtSettings::atariSioDriverName()
{
    return mAtariSioDriverName;
}

void RespeqtSettings::setAtariSioDriverName(const QString &name)
{    
    mAtariSioDriverName = name;
    if(mSessionFileName == "") mSettings->setValue("AtariSioDriverName", mAtariSioDriverName);
}

int RespeqtSettings::atariSioHandshakingMethod()
{
    return mAtariSioHandshakingMethod;
}

void RespeqtSettings::setAtariSioHandshakingMethod(int method)
{    
    mAtariSioHandshakingMethod = method;
    if(mSessionFileName == "") mSettings->setValue("AtariSioHandshakingMethod", mAtariSioHandshakingMethod);
}

int RespeqtSettings::backend()
{
    return mBackend;
}

void RespeqtSettings::setBackend(int backend)
{   
    mBackend = backend;
    if(mSessionFileName == "") mSettings->setValue("Backend", mBackend);
}

bool RespeqtSettings::useHighSpeedExeLoader()
{
    return mUseHighSpeedExeLoader;
}

void RespeqtSettings::setUseHighSpeedExeLoader(bool use)
{   
    mUseHighSpeedExeLoader = use;
    if(mSessionFileName == "") mSettings->setValue("UseHighSpeedExeLoader", mUseHighSpeedExeLoader);
}

bool RespeqtSettings::printerEmulation()
{
    return mPrinterEmulation;
}

void RespeqtSettings::setPrinterEmulation(bool status)
{
    mPrinterEmulation = status;
    if(mSessionFileName == "") mSettings->setValue("PrinterEmulation", mPrinterEmulation);
}

bool RespeqtSettings::useCustomCasBaud()
{
    return mUseCustomCasBaud;
}

void RespeqtSettings::setUseCustomCasBaud(bool use)
{   
    mUseCustomCasBaud = use;
    if(mSessionFileName == "") mSettings->setValue("UseCustomCasBaud", mUseCustomCasBaud);
}

int RespeqtSettings::customCasBaud()
{
    return mCustomCasBaud;
}

void RespeqtSettings::setCustomCasBaud(int baud)
{    
    mCustomCasBaud = baud;
    if(mSessionFileName == "") mSettings->setValue("CustomCasBaud", mCustomCasBaud);
}

const RespeqtSettings::ImageSettings* RespeqtSettings::getImageSettingsFromName(const QString &fileName)
{
    ImageSettings *is = NULL;
    int i;
    bool found = false;

    for (i = 0; i < 15; i++) {          //
        if (mMountedImageSettings[i].fileName == fileName) {
            is = &mMountedImageSettings[i];
            found = true;
            break;
        }
    }
    if (!found) {
        for (i = 0; i < NUM_RECENT_FILES; i++) {
            if (mRecentImageSettings[i].fileName == fileName) {
                is = &mRecentImageSettings[i];
                found = true;
                break;
            }
        }
    }
    return is;
}

const RespeqtSettings::ImageSettings& RespeqtSettings::mountedImageSetting(int no)
{
    return mMountedImageSettings[no];
}

const RespeqtSettings::ImageSettings& RespeqtSettings::recentImageSetting(int no)
{
    return mRecentImageSettings[no];
}

void RespeqtSettings::setMountedImageProtection(int no, bool prot)
{
    mMountedImageSettings[no].isWriteProtected = prot;
    if(mSessionFileName == "") mSettings->setValue(QString("MountedImageSettings/%1/IsWriteProtected").arg(no+1), prot);
}

void RespeqtSettings::setMountedImageSetting(int no, const QString &fileName, bool prot)
{

    mMountedImageSettings[no].fileName = fileName;
    mMountedImageSettings[no].isWriteProtected = prot;
    if(mSessionFileName == "") mSettings->setValue(QString("MountedImageSettings/%1/FileName").arg(no+1), fileName);
    if(mSessionFileName == "") mSettings->setValue(QString("MountedImageSettings/%1/IsWriteProtected").arg(no+1), prot);
}

void RespeqtSettings::mountImage(int no, const QString &fileName, bool prot)
{
    if (fileName.isEmpty()) {
        return;
    }
    int i;
    bool found = false;
    for (i = 0; i < NUM_RECENT_FILES; i++) {
        if (mRecentImageSettings[i].fileName == fileName) {
            found = true;
            break;
        }
    }
    if (found) {
        for (int j = i; j < (NUM_RECENT_FILES-1); j++) {
            mRecentImageSettings[j] = mRecentImageSettings[j + 1];
        }
        mRecentImageSettings[(NUM_RECENT_FILES-1)].fileName = "";
        writeRecentImageSettings();
    }
    setMountedImageSetting(no, fileName, prot);
}

void RespeqtSettings::unmountImage(int no)
{
    ImageSettings is = mMountedImageSettings[no];

    for (int i = (NUM_RECENT_FILES-1); i > 0; i--) {
            mRecentImageSettings[i] = mRecentImageSettings[i - 1];
    }
    mRecentImageSettings[0] = is;
    writeRecentImageSettings();

    setMountedImageSetting(no, "", false);
}

void RespeqtSettings::swapImages(int no1, int no2)
{    
    ImageSettings is1 = mountedImageSetting(no1);
    ImageSettings is2 = mountedImageSetting(no2);
    setMountedImageSetting(no1, is2.fileName, is2.isWriteProtected);
    setMountedImageSetting(no2, is1.fileName, is1.isWriteProtected);
}
// Save drive visibility status //
bool RespeqtSettings::saveDiskVis()
{
    return msaveDiskVis;
}

void RespeqtSettings::setsaveDiskVis(bool saveDvis)
{
    msaveDiskVis = saveDvis;
    if(mSessionFileName == "") mSettings->setValue("SaveDiskVisibility", msaveDiskVis);
}

// Drive visibility status //
bool RespeqtSettings::D9DOVisible()
{
    return mdVis;
}

void RespeqtSettings::setD9DOVisible(bool dVis)
{
    mdVis = dVis;
    if(mSessionFileName == "") mSettings->setValue("D9DOVisible", mdVis);
}
// Shade Mode Enable //
bool RespeqtSettings::enableShade()
{
    return mEnableShade;
}

void RespeqtSettings::setEnableShade(bool shade)
{
    mEnableShade = shade;
    if(mSessionFileName == "") mSettings->setValue("EnableShadeByDefault", mEnableShade);
}

// Use Large Font //
bool RespeqtSettings::useLargeFont()
{
    return mUseLargeFont;
}

void RespeqtSettings::setUseLargeFont(bool largeFont)
{
    mUseLargeFont = largeFont;
    if(mSessionFileName == "") mSettings->setValue("UseLargeFont", mUseLargeFont);
}

// Explorer Window always on top
bool RespeqtSettings::explorerOnTop()
{
    return mExplorerOnTop;
}

void RespeqtSettings::setExplorerOnTop(bool expOnTop)
{
    mExplorerOnTop = expOnTop;
    if(mSessionFileName =="") mSettings->setValue("ExplorerOnTop", mExplorerOnTop);
}

// Save/return last main window position/size option //
bool RespeqtSettings::saveWindowsPos()
{
    return msaveWindowsPos;
}

void RespeqtSettings::setsaveWindowsPos(bool saveMwp)
{    
    msaveWindowsPos = saveMwp;
    if(mSessionFileName == "") mSettings->setValue("SaveWindowsPosSize", msaveWindowsPos);
}
// Last main window position/size (No Session File) //

int RespeqtSettings::lastHorizontalPos()
{
    return mMainX;
}

void RespeqtSettings::setLastHorizontalPos(int lastHpos)
{    
    mMainX = lastHpos;
    if(mSessionFileName == "") mSettings->setValue("MainX", mMainX);
}
int RespeqtSettings::lastVerticalPos()
{
    return mMainY;
}

void RespeqtSettings::setLastVerticalPos(int lastVpos)
{    
    mMainY = lastVpos;
    if(mSessionFileName == "") mSettings->setValue("MainY", mMainY);
}
int RespeqtSettings::lastWidth()
{
    return mMainW;
}

void RespeqtSettings::setLastWidth(int lastW)
{    
    mMainW = lastW;
    if(mSessionFileName == "") mSettings->setValue("MainW", mMainW);
}
int RespeqtSettings::lastHeight()
{
    return mMainH;
}

void RespeqtSettings::setLastHeight(int lastH)
{
    mMainH = lastH;
    if(mSessionFileName == "") mSettings->setValue("MainH", mMainH);
}
// Last mini window position (No Session File) //

int RespeqtSettings::lastMiniHorizontalPos()
{
    return mMiniX;
}

void RespeqtSettings::setLastMiniHorizontalPos(int lastMHpos)
{
    mMiniX = lastMHpos;
    if(mSessionFileName == "") mSettings->setValue("MiniX", mMiniX);
}
int RespeqtSettings::lastMiniVerticalPos()
{
    return mMiniY;
}

void RespeqtSettings::setLastMiniVerticalPos(int lastMVpos)
{
    mMiniY = lastMVpos;
    if(mSessionFileName == "") mSettings->setValue("MiniY", mMiniY);
}

// Last print window position/size (No Session File) //

int RespeqtSettings::lastPrtHorizontalPos()
{
    return mPrtX;
}

void RespeqtSettings::setLastPrtHorizontalPos(int lastPrtHpos)
{
    mPrtX = lastPrtHpos;
    if(mSessionFileName == "") mSettings->setValue("PrtX", mPrtX);
}
int RespeqtSettings::lastPrtVerticalPos()
{
    return mPrtY;
}

void RespeqtSettings::setLastPrtVerticalPos(int lastPrtVpos)
{
    mPrtY = lastPrtVpos;
    if(mSessionFileName == "") mSettings->setValue("PrtY", mPrtY);
}
int RespeqtSettings::lastPrtWidth()
{
    return mPrtW;
}

void RespeqtSettings::setLastPrtWidth(int lastPrtW)
{
    mPrtW = lastPrtW;
    if(mSessionFileName == "") mSettings->setValue("PrtW", mPrtW);
}
int RespeqtSettings::lastPrtHeight()
{
    return mPrtH;
}

void RespeqtSettings::setLastPrtHeight(int lastPrtH)
{
    mPrtH = lastPrtH;
    if(mSessionFileName == "") mSettings->setValue("PrtH", mPrtH);
}
QString RespeqtSettings::lastDiskImageDir()
{
    return mLastDiskImageDir;
}

void RespeqtSettings::setLastDiskImageDir(const QString &dir)
{
    mLastDiskImageDir = dir;
    mSettings->setValue("LastDiskImageDir", mLastDiskImageDir);
}

QString RespeqtSettings::lastFolderImageDir()
{
    return mLastFolderImageDir;
}

void RespeqtSettings::setLastFolderImageDir(const QString &dir)
{
    mLastFolderImageDir = dir;
    mSettings->setValue("LastFolderImageDir", mLastFolderImageDir);
}

QString RespeqtSettings::lastSessionDir()
{
    return mLastSessionDir;
}

void RespeqtSettings::setLastSessionDir(const QString &dir)
{
    mLastSessionDir = dir;
//    mSettings->setValue("LastSessionDir", mLastFolderImageDir);  //
    mSettings->setValue("LastSessionDir", mLastSessionDir);
}

QString RespeqtSettings::lastExeDir()
{
    return mLastExeDir;
}

void RespeqtSettings::setLastExeDir(const QString &dir)
{
    mLastExeDir = dir;
    mSettings->setValue("LastExeDir", mLastExeDir);
}

QString RespeqtSettings::lastExtractDir()
{
    return mLastExtractDir;
}

void RespeqtSettings::setLastExtractDir(const QString &dir)
{
    mLastExtractDir = dir;
    mSettings->setValue("LastExtractDir", mLastExeDir);
}

QString RespeqtSettings::lastPrinterTextDir()
{
    return mLastPrinterTextDir;
}

void RespeqtSettings::setLastPrinterTextDir(const QString &dir)
{
    mLastPrinterTextDir = dir;
    mSettings->setValue("LastPrinterTextDir", mLastPrinterTextDir);
}

QString RespeqtSettings::lastCasDir()
{
    return mLastCasDir;
}

void RespeqtSettings::setLastCasDir(const QString &dir)
{
    mLastCasDir = dir;
    mSettings->setValue("LastCasDir", mLastCasDir);
}

QString RespeqtSettings::i18nLanguage()
{
    return mI18nLanguage;
}

void RespeqtSettings::setI18nLanguage(const QString &lang)
{

    mI18nLanguage = lang;
    if(mSessionFileName == "") mSettings->setValue("I18nLanguage", mI18nLanguage);
}

bool RespeqtSettings::minimizeToTray()
{
    return mMinimizeToTray;
}

void RespeqtSettings::setMinimizeToTray(bool tray)
{
    mMinimizeToTray = tray;
    mSettings->setValue("MinimizeToTray", mMinimizeToTray);
}

bool RespeqtSettings::filterUnderscore()
{
    return mFilterUnderscore;
}

void RespeqtSettings::setfilterUnderscore(bool filter)
{
    mFilterUnderscore = filter;
    mSettings->setValue("FilterUnderscore", mFilterUnderscore);
}

bool RespeqtSettings::capitalLettersInPCLINK()
{
    return mUseCapitalLettersInPCLINK;
}

void RespeqtSettings::setCapitalLettersInPCLINK(bool caps)
{
    mUseCapitalLettersInPCLINK = caps;
    mSettings->setValue("CapitalLettersInPCLINK", mUseCapitalLettersInPCLINK);
}

bool RespeqtSettings::isURLSubmitEnabled()
{
    return mUseURLSubmit;
}

void RespeqtSettings::setURLSubmit(bool enabled)
{
    mUseURLSubmit = enabled;
    mSettings->setValue("URLSubmit", mUseURLSubmit);
}

void RespeqtSettings::writeRecentImageSettings()
{
    mSettings->beginWriteArray("RecentImageSettings");
    for (int i = 0; i < NUM_RECENT_FILES; i++) {
        mSettings->setArrayIndex(i);
        mSettings->setValue("FileName", mRecentImageSettings[i].fileName);
        mSettings->setValue("IsWriteProtected", mRecentImageSettings[i].isWriteProtected);
    }
    mSettings->endArray();
}
