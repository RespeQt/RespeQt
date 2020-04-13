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
//#include "mainwindow.h"
#include "Emulator.h"

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
    mSerialPortDTRControlEnable = mSettings->value("DTRControlEnable", false).toBool();
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
    mRclDir = mSettings->value("LastRclDir","").toString();
    mMinimizeToTray = mSettings->value("MinimizeToTray", false).toBool();
    msaveWindowsPos = mSettings->value("SaveWindowsPosSize", true).toBool();
    mFilterUnderscore = mSettings->value("FilterUnderscore", true).toBool();
    mUseCapitalLettersInPCLINK = mSettings->value("CapitalLettersInPCLINK", false).toBool();
    mUseURLSubmit = mSettings->value("URLSubmit", false).toBool();
    mSpyMode = mSettings->value("SpyMode", false).toBool();
    mCommandName = mSettings->value("CommandName", false).toBool();
    mTrackLayout = mSettings->value("TrackLayout", false).toBool();
    msaveDiskVis = mSettings->value("SaveDiskVisibility", true).toBool();
    mdVis = mSettings->value("D9DOVisible",true).toBool();
    if (mMainW < 688 && mdVis) mMainW = 688;
    if (mMainH < 426 && mdVis) mMainH = 426;
    mUseLargeFont = mSettings->value("UseLargeFont", false).toBool();
    mExplorerOnTop = mSettings->value("ExplorerOnTop", false).toBool();
    mEnableShade = mSettings->value("EnableShadeByDefault", true).toBool();

    // Printer specific settings
    mAtariFixedFontName = mSettings->value("AtariFixedFontFamily", "Courier").toString();
    mSettings->beginReadArray("ConnectedPrinterSettings");
    for(i = 0; i < PRINTER_COUNT; i++)
    {
        mSettings->setArrayIndex(i);
        mPrinterSettings[i].printerName = mSettings->value("PrinterName", "").toString();
        mPrinterSettings[i].outputName = mSettings->value("OutputName", "").toString();
    }
    mSettings->endArray();
    mPrinterSpyMode = mSettings->value("PrinterSpyMode", false).toBool();

    m810Firmware = mSettings->value("Atari810Firmware", "").toString();
    m810ChipFirmware = mSettings->value("Atari810ChipFirmware", "").toString();
    m810HappyFirmware = mSettings->value("Atari810HappyFirmware", "").toString();
    m1050Firmware = mSettings->value("Atari1050Firmware", "").toString();
    m1050ArchiverFirmware = mSettings->value("Atari1050ArchiverFirmware", "").toString();
    m1050HappyFirmware = mSettings->value("Atari1050HappyFirmware", "").toString();
    m1050SpeedyFirmware = mSettings->value("Atari1050SpeedyFirmware", "").toString();
    m1050TurboFirmware = mSettings->value("Atari1050TurboFirmware", "").toString();
    m1050DuplicatorFirmware = mSettings->value("Atari1050DuplicatorFirmware", "").toString();

    mD1EmulationType = (eHARDWARE)mSettings->value("D1EmulationType", 0).toInt();
    mD2EmulationType = (eHARDWARE)mSettings->value("D2EmulationType", 0).toInt();
    mD3EmulationType = (eHARDWARE)mSettings->value("D3EmulationType", 0).toInt();
    mD4EmulationType = (eHARDWARE)mSettings->value("D4EmulationType", 0).toInt();
    mDisplayTransmission = mSettings->value("DisplayTransmission", false).toBool();
    mDisplayDriveHead = mSettings->value("DisplayDriveHead", false).toBool();
    mDisplayFdcCommands = mSettings->value("DisplayFdcCommands", false).toBool();
    mDisplayIndexPulse = mSettings->value("DisplayIndexPulse", false).toBool();
    mDisplayMotorOnOff = mSettings->value("DisplayMotorOnOff", false).toBool();
    mDisplayIDAddressMarks = mSettings->value("DisplayIDAddressMarks", false).toBool();
    mDisplayTrackInformation = mSettings->value("DisplayTrackInformation", false).toBool();
    mDisassembleUploadedCode = mSettings->value("DisassembleUploadedCode", false).toBool();
    mTranslatorAutomaticDetection = mSettings->value("TranslatorAutomaticDetection", false).toBool();
    mTranslatorDiskImagePath = mSettings->value("TranslatorDiskImagePath", "").toString();
    mSioAutoReconnect = mSettings->value("SioAutoReconnect", false).toBool();
    mHideChipMode = mSettings->value("HideChipMode", false).toBool();
    mHideHappyMode = mSettings->value("HideHappyMode", false).toBool();
    mHideNextImage = mSettings->value("HideNextImage", false).toBool();
    mHideOSBMode = mSettings->value("HideOSBMode", false).toBool();
    mHideToolDisk = mSettings->value("HideToolDisk", false).toBool();
    mToolDiskImagePath = mSettings->value("ToolDiskImagePath", "").toString();
    mActivateChipModeWithTool = mSettings->value("ActivateChipModeWithTool", false).toBool();
    mActivateHappyModeWithTool = mSettings->value("ActivateHappyModeWithTool", false).toBool();
    mDisplayCpuInstructions = mSettings->value("DisplayCpuInstructions", false).toBool();
    mTraceFilename = mSettings->value("TraceFilename", "").toString();
    mD1PowerOnWithDiskInserted = mSettings->value("D1PowerOnWithDiskInserted", false).toBool();
    mD2PowerOnWithDiskInserted = mSettings->value("D2PowerOnWithDiskInserted", false).toBool();
    mD3PowerOnWithDiskInserted = mSettings->value("D3PowerOnWithDiskInserted", false).toBool();
    mD4PowerOnWithDiskInserted = mSettings->value("D4PowerOnWithDiskInserted", false).toBool();


#ifdef Q_OS_MAC
    mNativeMenu = mSettings->value("NativeMenu", false).toBool();
#endif
    mRawPrinterName = mSettings->value("RawPrinterName", "").toString();
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
        s.setValue("DTRControlEnable", mSerialPortDTRControlEnable);
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
        s.setValue("SpyMode", mSpyMode);
        s.setValue("CommandName", mCommandName);
        s.setValue("TrackLayout", mTrackLayout);
        s.setValue("UseLargeFont", mUseLargeFont);
        s.setValue("ExplorerOnTop", mExplorerOnTop);
        s.setValue("EnableShadeByDefault", mEnableShade);
        s.setValue("PrinterSpyMode", mPrinterSpyMode);
        s.setValue("Atari810Firmware", m810Firmware);
        s.setValue("Atari810ChipFirmware", m810ChipFirmware);
        s.setValue("Atari810HappyFirmware", m810HappyFirmware);
        s.setValue("Atari1050Firmware", m1050Firmware);
        s.setValue("Atari1050ArchiverFirmware", m1050ArchiverFirmware);
        s.setValue("Atari1050HappyFirmware", m1050HappyFirmware);
        s.setValue("Atari1050SpeedyFirmware", m1050SpeedyFirmware);
        s.setValue("Atari1050TurboFirmware", m1050TurboFirmware);
        s.setValue("Atari1050DuplicatorFirmware", m1050DuplicatorFirmware);
        s.setValue("D1EmulationType", mD1EmulationType);
        s.setValue("D2EmulationType", mD2EmulationType);
        s.setValue("D3EmulationType", mD3EmulationType);
        s.setValue("D4EmulationType", mD4EmulationType);
        s.setValue("DisplayTransmission", mDisplayTransmission);
        s.setValue("DisplayFdcCommands", mDisplayFdcCommands);
        s.setValue("DisplayIndexPulse", mDisplayIndexPulse);
        s.setValue("DisplayMotorOnOff", mDisplayMotorOnOff);
        s.setValue("DisplayIDAddressMarks", mDisplayIDAddressMarks);
        s.setValue("DisplayTrackInformation", mDisplayTrackInformation);
        s.setValue("DisassembleUploadedCode", mDisassembleUploadedCode);
        s.setValue("TranslatorAutomaticDetection", mTranslatorAutomaticDetection);
        s.setValue("TranslatorDiskImagePath", mTranslatorDiskImagePath);
        s.setValue("SioAutoReconnect", mSioAutoReconnect);
        s.setValue("HideChipMode", mHideChipMode);
        s.setValue("HideHappyMode", mHideHappyMode);
        s.setValue("HideNextImage", mHideNextImage);
        s.setValue("HideOSBMode", mHideOSBMode);
        s.setValue("HideToolDisk", mHideToolDisk);
        s.setValue("ToolDiskImagePath", mToolDiskImagePath);
        s.setValue("ActivateChipModeWithTool", mActivateChipModeWithTool);
        s.setValue("ActivateHappyModeWithTool", mActivateHappyModeWithTool);
        s.setValue("DisplayCpuInstructions", mDisplayCpuInstructions);
        s.setValue("TraceFilename", mTraceFilename);
        s.setValue("D1PowerOnWithDiskInserted", mD1PowerOnWithDiskInserted);
        s.setValue("D2PowerOnWithDiskInserted", mD2PowerOnWithDiskInserted);
        s.setValue("D3PowerOnWithDiskInserted", mD3PowerOnWithDiskInserted);
        s.setValue("D4PowerOnWithDiskInserted", mD4PowerOnWithDiskInserted);
        s.setValue("RawPrinterName", mRawPrinterName);
        s.setValue("LastRclDir",mRclDir);
#ifdef Q_OS_MAC
        s.setValue("NativeMenu", mNativeMenu);
#endif
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

    s.beginWriteArray("ConnectedPrinterSettings");
    for (int i = 0; i < PRINTER_COUNT; i++) {
        PrinterSettings ps = mPrinterSettings[i];
        s.setArrayIndex(i);
        s.setValue("PrinterName", ps.printerName);
        s.setValue("OutputName", ps.outputName);
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
        mRclDir = mSettings->value("LastRclDir","").toString();
        mSerialPortDTRControlEnable = s.value("DTRControlEnable", false).toBool();
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
        mSpyMode = s.value("SpyMode", false).toBool();
        mCommandName = s.value("CommandName", false).toBool();
        mTrackLayout = s.value("TrackLayout", false).toBool();
        mUseLargeFont = s.value("UseLargeFont", false).toBool();
        mExplorerOnTop = s.value("ExplorerOnTop", false).toBool();
        mEnableShade = s.value("EnableShadeByDefault", true).toBool();
        m810Firmware = s.value("Atari810Firmware", "").toString();
        m810ChipFirmware = s.value("Atari810ChipFirmware", "").toString();
        m810HappyFirmware = s.value("Atari810HappyFirmware", "").toString();
        m1050Firmware = s.value("Atari1050Firmware", "").toString();
        m1050ArchiverFirmware = s.value("Atari1050ArchiverFirmware", "").toString();
        m1050HappyFirmware = s.value("Atari1050HappyFirmware", "").toString();
        m1050SpeedyFirmware = s.value("Atari1050SpeedyFirmware", "").toString();
        m1050TurboFirmware = s.value("Atari1050TurboFirmware", "").toString();
        m1050DuplicatorFirmware = s.value("Atari1050DuplicatorFirmware", "").toString();
        mD1EmulationType = (eHARDWARE) s.value("D1EmulationType", false).toInt();
        mD2EmulationType = (eHARDWARE) s.value("D2EmulationType", false).toInt();
        mD3EmulationType = (eHARDWARE) s.value("D3EmulationType", false).toInt();
        mD4EmulationType = (eHARDWARE) s.value("D4EmulationType", false).toInt();
        mDisplayTransmission = s.value("DisplayTransmission", false).toBool();
        mDisplayDriveHead = s.value("DisplayDriveHead", false).toBool();
        mDisplayFdcCommands = s.value("DisplayFdcCommands", false).toBool();
        mDisplayIndexPulse = s.value("DisplayIndexPulse", false).toBool();
        mDisplayMotorOnOff = s.value("DisplayMotorOnOff", false).toBool();
        mDisplayIDAddressMarks = s.value("DisplayIDAddressMarks", false).toBool();
        mDisplayTrackInformation = s.value("DisplayTrackInformation", false).toBool();
        mDisassembleUploadedCode = s.value("DisassembleUploadedCode", false).toBool();
        mTranslatorAutomaticDetection = s.value("TranslatorAutomaticDetection", false).toBool();
        mTranslatorDiskImagePath = s.value("TranslatorDiskImagePath", false).toString();
        mSioAutoReconnect = s.value("SioAutoReconnect", false).toBool();
        mHideChipMode = s.value("HideChipMode", false).toBool();
        mHideHappyMode = s.value("HideHappyMode", false).toBool();
        mHideNextImage = s.value("HideNextImage", false).toBool();
        mHideOSBMode = s.value("HideOSBMode", false).toBool();
        mHideToolDisk = s.value("HideToolDisk", false).toBool();
        mToolDiskImagePath = s.value("ToolDiskImagePath", false).toString();
        mActivateChipModeWithTool = s.value("ActivateChipModeWithTool", false).toBool();
        mActivateHappyModeWithTool = s.value("ActivateHappyModeWithTool", false).toBool();
        mDisplayCpuInstructions = s.value("DisplayCpuInstructions", false).toBool();
        mTraceFilename = s.value("TraceFilename", false).toBool();
        mD1PowerOnWithDiskInserted = s.value("D1PowerOnWithDiskInserted", false).toBool();
        mD2PowerOnWithDiskInserted = s.value("D2PowerOnWithDiskInserted", false).toBool();
        mD3PowerOnWithDiskInserted = s.value("D3PowerOnWithDiskInserted", false).toBool();
        mD4PowerOnWithDiskInserted = s.value("D4PowerOnWithDiskInserted", false).toBool();
        mRawPrinterName = s.value("RawPrinterName", "").toString();
        mPrinterSpyMode = s.value("PrinterSpyMode", false).toBool();

#ifdef Q_OS_MAC
        mNativeMenu = s.value("NativeMenu", false).toBool();
#endif
    s.endGroup();
 //
    s.beginReadArray("MountedImageSettings");
    for (int i = 0; i < 15; i++) {              //
        s.setArrayIndex(i);
        setMountedImageSetting(i, s.value("FileName", "").toString(), s.value("IsWriteProtected", false).toBool());
    }
    s.endArray();

    s.beginReadArray("ConnectedPrinterSettings");
    for (int i = 0; i < PRINTER_COUNT; i++) {
        s.setArrayIndex(i);
        setOutputName(i, s.value("OutputName", "").toString());
        setPrinterName(i, s.value("PrinterName", "").toString());
    }   
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

bool RespeqtSettings::serialPortDTRControlEnable()
{
    return mSerialPortDTRControlEnable;
}

void RespeqtSettings::setSerialPortDTRControlEnable(bool use)
{
    mSerialPortDTRControlEnable = use;
    if(mSessionFileName == "") mSettings->setValue("DTRControlEnable", mSerialPortDTRControlEnable);
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

QString RespeqtSettings::lastRclDir()
{
    return mRclDir;
}

void RespeqtSettings::setRclDir(const QString &dir)
{
    mRclDir = dir;
    mSettings->setValue("LastRclDir", mRclDir);
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
    ImageSettings *is = nullptr;
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

bool RespeqtSettings::isSpyMode()
{
    return mSpyMode;
}

void RespeqtSettings::setSpyMode(bool enabled)
{
    mSpyMode = enabled;
    mSettings->setValue("SpyMode", mSpyMode);
}

bool RespeqtSettings::isCommandName()
{
    return mCommandName;
}

void RespeqtSettings::setCommandName(bool enabled)
{
    mCommandName = enabled;
    mSettings->setValue("CommandName", mCommandName);
}

bool RespeqtSettings::isTrackLayout()
{
    return mTrackLayout;
}

void RespeqtSettings::setTrackLayout(bool enabled)
{
    mTrackLayout = enabled;
    mSettings->setValue("TrackLayout", mTrackLayout);
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

void RespeqtSettings::setPrinterName(int no, const QString &printerName)
{
    mPrinterSettings[no].printerName = printerName;
    if(mSessionFileName == "")
        mSettings->setValue(QString("ConnectedPrinterSettings/%1/PrinterName").arg(no+1), printerName);
}

const QString &RespeqtSettings::printerName(int no) const {
    return mPrinterSettings[no].printerName;
}

void RespeqtSettings::setOutputName(int no, const QString &outputName) {
    mPrinterSettings[no].outputName = outputName;
    if(mSessionFileName == "")
        mSettings->setValue(QString("ConnectedPrinterSettings/%1/OutputName").arg(no+1), outputName);
}

const QString &RespeqtSettings::outputName(int no) const {
    return mPrinterSettings[no].outputName;
}

const RespeqtSettings::PrinterSettings &RespeqtSettings::printerSettings(int no) const {
    return mPrinterSettings[no];
}

QString RespeqtSettings::atariFixedFontFamily() {
    return mAtariFixedFontName;
}

void RespeqtSettings::setAtariFixedFontFamily(QString fontFamily) {
    mAtariFixedFontName = fontFamily;
    mSettings->setValue("AtariFixedFontFamily", fontFamily);
}

bool RespeqtSettings::isPrinterSpyMode()
{
    return mPrinterSpyMode;
}

void RespeqtSettings::setPrinterSpyMode(bool enabled)
{
    mPrinterSpyMode = enabled;
    mSettings->setValue("PrinterSpyMode", mPrinterSpyMode);
}

QString RespeqtSettings::atari810Firmware()
{
    return m810Firmware;
}

void RespeqtSettings::setAtari810Firmware(const QString &firmware)
{
    m810Firmware = firmware;
    if(mSessionFileName == "") mSettings->setValue("Atari810Firmware", m810Firmware);
}

QString RespeqtSettings::atari810ChipFirmware()
{
    return m810ChipFirmware;
}

void RespeqtSettings::setAtari810ChipFirmware(const QString &firmware)
{
    m810ChipFirmware = firmware;
    if(mSessionFileName == "") mSettings->setValue("Atari810ChipFirmware", m810ChipFirmware);
}

QString RespeqtSettings::atari810HappyFirmware()
{
    return m810HappyFirmware;
}

void RespeqtSettings::setAtari810HappyFirmware(const QString &firmware)
{
    m810HappyFirmware = firmware;
    if(mSessionFileName == "") mSettings->setValue("Atari810HappyFirmware", m810HappyFirmware);
}

QString RespeqtSettings::atari1050Firmware()
{
    return m1050Firmware;
}

void RespeqtSettings::setAtari1050Firmware(const QString &firmware)
{
    m1050Firmware = firmware;
    if(mSessionFileName == "") mSettings->setValue("Atari1050Firmware", m1050Firmware);
}

QString RespeqtSettings::atari1050ArchiverFirmware()
{
    return m1050ArchiverFirmware;
}

void RespeqtSettings::setAtari1050ArchiverFirmware(const QString &firmware)
{
    m1050ArchiverFirmware = firmware;
    if(mSessionFileName == "") mSettings->setValue("Atari1050ArchiverFirmware", m1050ArchiverFirmware);
}

QString RespeqtSettings::atari1050HappyFirmware()
{
    return m1050HappyFirmware;
}

void RespeqtSettings::setAtari1050HappyFirmware(const QString &firmware)
{
    m1050HappyFirmware = firmware;
    if(mSessionFileName == "") mSettings->setValue("Atari1050HappyFirmware", m1050HappyFirmware);
}

QString RespeqtSettings::atari1050SpeedyFirmware()
{
    return m1050SpeedyFirmware;
}

void RespeqtSettings::setAtari1050SpeedyFirmware(const QString &firmware)
{
    m1050SpeedyFirmware = firmware;
    if(mSessionFileName == "") mSettings->setValue("Atari1050SpeedyFirmware", m1050SpeedyFirmware);
}

QString RespeqtSettings::atari1050TurboFirmware()
{
    return m1050TurboFirmware;
}

void RespeqtSettings::setAtari1050TurboFirmware(const QString &firmware)
{
    m1050TurboFirmware = firmware;
    if(mSessionFileName == "") mSettings->setValue("Atari1050TurboFirmware", m1050TurboFirmware);
}

QString RespeqtSettings::atari1050DuplicatorFirmware()
{
    return m1050DuplicatorFirmware;
}

void RespeqtSettings::setAtari1050DuplicatorFirmware(const QString &firmware)
{
    m1050DuplicatorFirmware = firmware;
    if(mSessionFileName == "") mSettings->setValue("Atari1050DuplicatorFirmware", m1050DuplicatorFirmware);
}

QString RespeqtSettings::firmwarePath(eHARDWARE eHardware)
{
    QString firmwarePath;
    switch (eHardware) {
    case HARDWARE_810:                    firmwarePath += m810Firmware; break;
    case HARDWARE_810_WITH_THE_CHIP:      firmwarePath += m810ChipFirmware; break;
    case HARDWARE_810_WITH_HAPPY:         firmwarePath += m810HappyFirmware; break;
    case HARDWARE_1050:                   firmwarePath += m1050Firmware; break;
    case HARDWARE_1050_WITH_THE_ARCHIVER: firmwarePath += m1050ArchiverFirmware; break;
    case HARDWARE_1050_WITH_HAPPY:        firmwarePath += m1050HappyFirmware; break;
    case HARDWARE_1050_WITH_SPEEDY:       firmwarePath += m1050SpeedyFirmware; break;
    case HARDWARE_1050_WITH_TURBO:        firmwarePath += m1050TurboFirmware; break;
    case HARDWARE_1050_WITH_DUPLICATOR:   firmwarePath += m1050DuplicatorFirmware; break;
    default: break;
    }
    return firmwarePath;
}

QString RespeqtSettings::firmwareName(eHARDWARE eHardware)
{
    QString firmwareName;
    switch (eHardware) {
    case HARDWARE_810:                    firmwareName += "810"; break;
    case HARDWARE_810_WITH_THE_CHIP:      firmwareName += "Chip 810"; break;
    case HARDWARE_810_WITH_HAPPY:         firmwareName += "Happy 810"; break;
    case HARDWARE_1050:                   firmwareName += "1050"; break;
    case HARDWARE_1050_WITH_THE_ARCHIVER: firmwareName += "The Super Archiver 1050"; break;
    case HARDWARE_1050_WITH_HAPPY:        firmwareName += "Happy 1050"; break;
    case HARDWARE_1050_WITH_SPEEDY:       firmwareName += "Speedy 1050"; break;
    case HARDWARE_1050_WITH_TURBO:        firmwareName += "Turbo 1050"; break;
    case HARDWARE_1050_WITH_DUPLICATOR:   firmwareName += "Duplicator 1050"; break;
    default: break;
    }
    return firmwareName;
}

eHARDWARE RespeqtSettings::d1EmulationType()
{
    return mD1EmulationType;
}

void RespeqtSettings::setD1EmulationType(const eHARDWARE type)
{
    mD1EmulationType = type;
    if(mSessionFileName == "") mSettings->setValue("D1EmulationType", mD1EmulationType);
}

eHARDWARE RespeqtSettings::d2EmulationType()
{
    return mD2EmulationType;
}

void RespeqtSettings::setD2EmulationType(const eHARDWARE type)
{
    mD2EmulationType = type;
    if(mSessionFileName == "") mSettings->setValue("D2EmulationType", mD2EmulationType);
}

eHARDWARE RespeqtSettings::d3EmulationType()
{
    return mD3EmulationType;
}

void RespeqtSettings::setD3EmulationType(const eHARDWARE type)
{
    mD3EmulationType = type;
    if(mSessionFileName == "") mSettings->setValue("D3EmulationType", mD3EmulationType);
}

eHARDWARE RespeqtSettings::d4EmulationType()
{
    return mD4EmulationType;
}

void RespeqtSettings::setD4EmulationType(const eHARDWARE type)
{
    mD4EmulationType = type;
    if(mSessionFileName == "") mSettings->setValue("D4EmulationType", mD4EmulationType);
}

eHARDWARE RespeqtSettings::firmwareType(int drive)
{
    eHARDWARE level = SIO_EMULATION;
    switch (drive) {
    case 0: level = mD1EmulationType; break;
    case 1: level = mD2EmulationType; break;
    case 2: level = mD3EmulationType; break;
    case 3: level = mD4EmulationType; break;
    default: break;
    }
    return level;
}

bool RespeqtSettings::displayTransmission()
{
    return mDisplayTransmission;
}

void RespeqtSettings::setDisplayTransmission(bool displayTransmission)
{
    mDisplayTransmission = displayTransmission;
    if(mSessionFileName == "") mSettings->setValue("DisplayTransmission", mDisplayTransmission);
}

bool RespeqtSettings::displayDriveHead()
{
    return mDisplayDriveHead;
}

void RespeqtSettings::setDisplayDriveHead(bool displayDriveHead)
{
    mDisplayDriveHead = displayDriveHead;
    if(mSessionFileName == "") mSettings->setValue("DisplayDriveHead", mDisplayDriveHead);
}

bool RespeqtSettings::displayFdcCommands()
{
    return mDisplayFdcCommands;
}

void RespeqtSettings::setDisplayFdcCommands(bool displayFdcCommands)
{
    mDisplayFdcCommands = displayFdcCommands;
    if(mSessionFileName == "") mSettings->setValue("DisplayFdcCommands", mDisplayFdcCommands);
}

bool RespeqtSettings::displayIndexPulse()
{
    return mDisplayIndexPulse;
}

void RespeqtSettings::setDisplayIndexPulse(bool displayIndexPulse)
{
    mDisplayIndexPulse = displayIndexPulse;
    if(mSessionFileName == "") mSettings->setValue("DisplayIndexPulse", mDisplayIndexPulse);
}

bool RespeqtSettings::displayMotorOnOff()
{
    return mDisplayMotorOnOff;
}

void RespeqtSettings::setDisplayMotorOnOff(bool displayMotorOnOff)
{
    mDisplayMotorOnOff = displayMotorOnOff;
    if(mSessionFileName == "") mSettings->setValue("DisplayMotorOnOff", mDisplayMotorOnOff);
}

bool RespeqtSettings::displayIDAddressMarks()
{
    return mDisplayIDAddressMarks;
}

void RespeqtSettings::setDisplayIDAddressMarks(bool displayIDAddressMarks)
{
    mDisplayIDAddressMarks = displayIDAddressMarks;
    if(mSessionFileName == "") mSettings->setValue("DisplayIDAddressMarks", mDisplayIDAddressMarks);
}

bool RespeqtSettings::displayTrackInformation()
{
    return mDisplayTrackInformation;
}

void RespeqtSettings::setDisplayTrackInformation(bool displayTrackInformation)
{
    mDisplayTrackInformation = displayTrackInformation;
    if(mSessionFileName == "") mSettings->setValue("DisplayTrackInformation", mDisplayTrackInformation);
}

bool RespeqtSettings::disassembleUploadedCode()
{
    return mDisassembleUploadedCode;
}

void RespeqtSettings::setDisassembleUploadedCode(bool disassembleUploadedCode)
{
    mDisassembleUploadedCode = disassembleUploadedCode;
    if(mSessionFileName == "") mSettings->setValue("DisassembleUploadedCode", mDisassembleUploadedCode);
}

bool RespeqtSettings::translatorAutomaticDetection()
{
    return mTranslatorAutomaticDetection;
}

void RespeqtSettings::setTranslatorAutomaticDetection(bool translatorAutomaticDetection)
{
    mTranslatorAutomaticDetection = translatorAutomaticDetection;
    if(mSessionFileName == "") mSettings->setValue("TranslatorAutomaticDetection", mTranslatorAutomaticDetection);
}

bool RespeqtSettings::sioAutoReconnect()
{
    return mSioAutoReconnect;
}

void RespeqtSettings::setSioAutoReconnect(bool sioAutoReconnect)
{
    mSioAutoReconnect = sioAutoReconnect;
    if(mSessionFileName == "") mSettings->setValue("SioAutoReconnect", mSioAutoReconnect);
}

bool RespeqtSettings::hideChipMode()
{
    return mHideChipMode;
}

void RespeqtSettings::setHideChipMode(bool hidden)
{
    mHideChipMode = hidden;
    if(mSessionFileName == "") mSettings->setValue("HideChipMode", mHideChipMode);
}

bool RespeqtSettings::hideHappyMode()
{
    return mHideHappyMode;
}

void RespeqtSettings::setHideHappyMode(bool hidden)
{
    mHideHappyMode = hidden;
    if(mSessionFileName == "") mSettings->setValue("HideHappyMode", mHideHappyMode);
}

bool RespeqtSettings::hideNextImage()
{
    return mHideNextImage;
}

void RespeqtSettings::setHideNextImage(bool hidden)
{
    mHideNextImage = hidden;
    if(mSessionFileName == "") mSettings->setValue("HideNextImage", mHideNextImage);
}

bool RespeqtSettings::hideOSBMode()
{
    return mHideOSBMode;
}

void RespeqtSettings::setHideOSBMode(bool hidden)
{
    mHideOSBMode = hidden;
    if(mSessionFileName == "") mSettings->setValue("HideOSBMode", mHideOSBMode);
}

bool RespeqtSettings::hideToolDisk()
{
    return mHideToolDisk;
}

void RespeqtSettings::setHideToolDisk(bool hidden)
{
    mHideToolDisk = hidden;
    if(mSessionFileName == "") mSettings->setValue("HideToolDisk", mHideToolDisk);
}

QString RespeqtSettings::translatorDiskImagePath()
{
    return mTranslatorDiskImagePath;
}

void RespeqtSettings::setTranslatorDiskImagePath(const QString &diskImage)
{
    mTranslatorDiskImagePath = diskImage;
    if(mSessionFileName == "") mSettings->setValue("TranslatorDiskImagePath", mTranslatorDiskImagePath);
}

QString RespeqtSettings::toolDiskImagePath()
{
    return mToolDiskImagePath;
}

void RespeqtSettings::setToolDiskImagePath(const QString &diskImage)
{
    mToolDiskImagePath = diskImage;
    if(mSessionFileName == "") mSettings->setValue("ToolDiskImagePath", mToolDiskImagePath);
}

bool RespeqtSettings::activateChipModeWithTool()
{
    return mActivateChipModeWithTool;
}

void RespeqtSettings::setActivateChipModeWithTool(bool activate)
{
    mActivateChipModeWithTool = activate;
    if(mSessionFileName == "") mSettings->setValue("ActivateChipModeWithTool", mActivateChipModeWithTool);
}

bool RespeqtSettings::activateHappyModeWithTool()
{
    return mActivateHappyModeWithTool;
}

void RespeqtSettings::setActivateHappyModeWithTool(bool activate)
{
    mActivateHappyModeWithTool = activate;
    if(mSessionFileName == "") mSettings->setValue("ActivateHappyModeWithTool", mActivateHappyModeWithTool);
}

bool RespeqtSettings::displayCpuInstructions()
{
    return mDisplayCpuInstructions;
}

void RespeqtSettings::setDisplayCpuInstructions(bool displayCpuInstructions)
{
    mDisplayCpuInstructions = displayCpuInstructions;
    if(mSessionFileName == "") mSettings->setValue("DisplayCpuInstructions", mDisplayCpuInstructions);
}

QString RespeqtSettings::traceFilename()
{
    return mTraceFilename;
}

void RespeqtSettings::setTraceFilename(const QString &filename)
{
    mTraceFilename = filename;
    if(mSessionFileName == "") mSettings->setValue("TraceFilename", mTraceFilename);
}

bool RespeqtSettings::d1PowerOnWithDiskInserted()
{
    return mD1PowerOnWithDiskInserted;
}

void RespeqtSettings::setD1PowerOnWithDiskInserted(bool d1PowerOnWithDiskInserted)
{
    mD1PowerOnWithDiskInserted = d1PowerOnWithDiskInserted;
    if(mSessionFileName == "") mSettings->setValue("D1PowerOnWithDiskInserted", mD1PowerOnWithDiskInserted);
}

bool RespeqtSettings::d2PowerOnWithDiskInserted()
{
    return mD2PowerOnWithDiskInserted;
}

void RespeqtSettings::setD2PowerOnWithDiskInserted(bool d2PowerOnWithDiskInserted)
{
    mD2PowerOnWithDiskInserted = d2PowerOnWithDiskInserted;
    if(mSessionFileName == "") mSettings->setValue("D2PowerOnWithDiskInserted", mD2PowerOnWithDiskInserted);
}

bool RespeqtSettings::d3PowerOnWithDiskInserted()
{
    return mD3PowerOnWithDiskInserted;
}

void RespeqtSettings::setD3PowerOnWithDiskInserted(bool d3PowerOnWithDiskInserted)
{
    mD3PowerOnWithDiskInserted = d3PowerOnWithDiskInserted;
    if(mSessionFileName == "") mSettings->setValue("D3PowerOnWithDiskInserted", mD3PowerOnWithDiskInserted);
}

bool RespeqtSettings::d4PowerOnWithDiskInserted()
{
    return mD4PowerOnWithDiskInserted;
}

void RespeqtSettings::setD4PowerOnWithDiskInserted(bool d4PowerOnWithDiskInserted)
{
    mD4PowerOnWithDiskInserted = d4PowerOnWithDiskInserted;
    if(mSessionFileName == "") mSettings->setValue("D4PowerOnWithDiskInserted", mD4PowerOnWithDiskInserted);
}

bool RespeqtSettings::firmwarePowerOnWithDisk(int drive)
{
    bool powerOn = false;
    switch (drive) {
    case 0: powerOn = mD1PowerOnWithDiskInserted; break;
    case 1: powerOn = mD2PowerOnWithDiskInserted; break;
    case 2: powerOn = mD3PowerOnWithDiskInserted; break;
    case 3: powerOn = mD4PowerOnWithDiskInserted; break;
    default: break;
    }
    return powerOn;
}

#ifdef Q_OS_MAC
void RespeqtSettings::setNativeMenu(bool nativeMenu)
{
    mNativeMenu = nativeMenu;
    mSettings->setValue("NativeMenu", nativeMenu);
}

bool RespeqtSettings::nativeMenu()
{
    return mNativeMenu;
}
#endif

void RespeqtSettings::setRawPrinterName(const QString &name)
{
    mRawPrinterName = name;
    mSettings->setValue("RawPrinterName", name);
}

QString RespeqtSettings::rawPrinterName() const
{
    return mRawPrinterName;
}
