/*
 * optionsdialog.cpp
 *
 * Copyright 2015 Joseph Zatarski
 * Copyright 2016 TheMontezuma
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "respeqtsettings.h"

#include <QTranslator>
#include <QDir>

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::OptionsDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    m_ui->setupUi(this);

    m_ui->treeWidget->expandAll();
    itemStandard = m_ui->treeWidget->topLevelItem(0)->child(0);
    itemQt       = m_ui->treeWidget->topLevelItem(0)->child(1);
    itemAtariSio = m_ui->treeWidget->topLevelItem(0)->child(2);
    itemEmulation = m_ui->treeWidget->topLevelItem(1);
    itemI18n = m_ui->treeWidget->topLevelItem(2);

#ifndef Q_OS_LINUX
    m_ui->treeWidget->topLevelItem(0)->removeChild(itemAtariSio);
#endif

    connect(this, SIGNAL(accepted()), this, SLOT(OptionsDialog_accepted()));

    /* Retrieve application settings */
    m_ui->serialPortDeviceNameEdit->setText(respeqtSettings->serialPortName());
    m_ui->serialPortHandshakeCombo->setCurrentIndex(respeqtSettings->serialPortHandshakingMethod());
    m_ui->serialPortWriteDelayCombo->setCurrentIndex(respeqtSettings->serialPortWriteDelay());
    m_ui->serialPortBaudCombo->setCurrentIndex(respeqtSettings->serialPortMaximumSpeed());
    m_ui->serialPortUseDivisorsBox->setChecked(respeqtSettings->serialPortUsePokeyDivisors());
    m_ui->serialPortDivisorEdit->setValue(respeqtSettings->serialPortPokeyDivisor());
    m_ui->serialPortCompErrDelayBox->setValue(respeqtSettings->serialPortCompErrDelay());
    m_ui->QtSerialPortDeviceNameEdit->setText(respeqtSettings->QtSerialPortName());
    m_ui->QtSerialPortHandshakeCombo->setCurrentIndex(respeqtSettings->QtSerialPortHandshakingMethod());
    m_ui->QtSerialPortWriteDelayCombo->setCurrentIndex(respeqtSettings->QtSerialPortWriteDelay());
    m_ui->QtSerialPortBaudCombo->setCurrentIndex(respeqtSettings->QtSerialPortMaximumSpeed());
    m_ui->QtSerialPortUseDivisorsBox->setChecked(respeqtSettings->QtSerialPortUsePokeyDivisors());
    m_ui->QtSerialPortDivisorEdit->setValue(respeqtSettings->QtSerialPortPokeyDivisor());
    m_ui->QtSerialPortCompErrDelayBox->setValue(respeqtSettings->QtSerialPortCompErrDelay());
    m_ui->atariSioDriverNameEdit->setText(respeqtSettings->atariSioDriverName());
    m_ui->atariSioHandshakingMethodCombo->setCurrentIndex(respeqtSettings->atariSioHandshakingMethod());
    m_ui->emulationHighSpeedExeLoaderBox->setChecked(respeqtSettings->useHighSpeedExeLoader());
    m_ui->emulationUseCustomCasBaudBox->setChecked(respeqtSettings->useCustomCasBaud());
    m_ui->emulationCustomCasBaudSpin->setValue(respeqtSettings->customCasBaud());
    m_ui->minimizeToTrayBox->setChecked(respeqtSettings->minimizeToTray());
    m_ui->saveWinPosBox->setChecked(respeqtSettings->saveWindowsPos());
    m_ui->saveDiskVisBox->setChecked(respeqtSettings->saveDiskVis());
    m_ui->filterUscore->setChecked(respeqtSettings->filterUnderscore());
    m_ui->capitalLettersPCLINK->setChecked(respeqtSettings->capitalLettersInPCLINK());
    m_ui->useLargerFont->setChecked(respeqtSettings->useLargeFont());
    m_ui->enableShade->setChecked(respeqtSettings->enableShade());

    switch (respeqtSettings->backend()) {
        case SERIAL_BACKEND_STANDARD:
            itemStandard->setCheckState(0, Qt::Checked);
            itemQt->setCheckState(0, Qt::Unchecked);
            itemAtariSio->setCheckState(0, Qt::Unchecked);
            m_ui->treeWidget->setCurrentItem(itemStandard);
            break;
        case SERIAL_BACKEND_QT:
            itemStandard->setCheckState(0, Qt::Unchecked);
            itemQt->setCheckState(0, Qt::Checked);
            itemAtariSio->setCheckState(0, Qt::Unchecked);
            m_ui->treeWidget->setCurrentItem(itemQt);
            break;
        case SERIAL_BACKEND_SIO_DRIVER:
            itemStandard->setCheckState(0, Qt::Unchecked);
            itemQt->setCheckState(0, Qt::Unchecked);
            itemAtariSio->setCheckState(0, Qt::Checked);
            m_ui->treeWidget->setCurrentItem(itemAtariSio);
            break;
    }
    m_ui->serialPortBox->setCheckState(itemStandard->checkState(0));
    m_ui->QtSerialPortBox->setCheckState(itemQt->checkState(0));
    m_ui->atariSioBox->setCheckState(itemAtariSio->checkState(0));
    
    /* list available translations */
    QTranslator local_translator;
    m_ui->i18nLanguageCombo->clear();
    m_ui->i18nLanguageCombo->addItem(tr("Automatic"), "auto");
    if (respeqtSettings->i18nLanguage().compare("auto") == 0)
      m_ui->i18nLanguageCombo->setCurrentIndex(0);
    m_ui->i18nLanguageCombo->addItem(QT_TR_NOOP("English"), "en");
    if (respeqtSettings->i18nLanguage().compare("en") == 0)
      m_ui->i18nLanguageCombo->setCurrentIndex(1);
    QDir dir(":/translations/i18n/");
    QStringList filters;
    filters << "respeqt_*.qm";
    dir.setNameFilters(filters);
    for (int i = 0; i < dir.entryList().size(); ++i) {
        local_translator.load(":/translations/i18n/" + dir.entryList()[i]);
        m_ui->i18nLanguageCombo->addItem(local_translator.translate("OptionsDialog", "English"), dir.entryList()[i].replace("respeqt_", "").replace(".qm", ""));
        if (dir.entryList()[i].replace("respeqt_", "").replace(".qm", "").compare(respeqtSettings->i18nLanguage()) == 0) {
            m_ui->i18nLanguageCombo->setCurrentIndex(i+2);
        }
    }

    bool software_handshake = (respeqtSettings->serialPortHandshakingMethod()==HANDSHAKE_SOFTWARE);
    m_ui->serialPortWriteDelayLabel->setVisible(software_handshake);
    m_ui->serialPortWriteDelayCombo->setVisible(software_handshake);
    m_ui->serialPortBaudLabel->setVisible(!software_handshake);
    m_ui->serialPortBaudCombo->setVisible(!software_handshake);
    m_ui->serialPortUseDivisorsBox->setVisible(!software_handshake);
    m_ui->serialPortDivisorLabel->setVisible(!software_handshake);
    m_ui->serialPortDivisorEdit->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayLabel->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayBox->setVisible(!software_handshake);

    bool qt_software_handshake = (respeqtSettings->QtSerialPortHandshakingMethod()==HANDSHAKE_SOFTWARE);
    m_ui->QtSerialPortWriteDelayLabel->setVisible(qt_software_handshake);
    m_ui->QtSerialPortWriteDelayCombo->setVisible(qt_software_handshake);
    m_ui->QtSerialPortBaudLabel->setVisible(!qt_software_handshake);
    m_ui->QtSerialPortBaudCombo->setVisible(!qt_software_handshake);
    m_ui->QtSerialPortUseDivisorsBox->setVisible(!qt_software_handshake);
    m_ui->QtSerialPortDivisorLabel->setVisible(!qt_software_handshake);
    m_ui->QtSerialPortDivisorEdit->setVisible(!qt_software_handshake);
    m_ui->QtSerialPortCompErrDelayLabel->setVisible(!qt_software_handshake);
    m_ui->QtSerialPortCompErrDelayBox->setVisible(!qt_software_handshake);

    if(    ((SERIAL_BACKEND_STANDARD == respeqtSettings->backend()) && software_handshake) ||
           ((SERIAL_BACKEND_QT == respeqtSettings->backend()) && qt_software_handshake))
    {
        m_ui->emulationHighSpeedExeLoaderBox->setVisible(false);
    }
    else
    {
        m_ui->emulationHighSpeedExeLoaderBox->setVisible(true);
    }
}

OptionsDialog::~OptionsDialog()
{
    delete m_ui;
}

void OptionsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void OptionsDialog::OptionsDialog_accepted()
{
    respeqtSettings->setSerialPortName(m_ui->serialPortDeviceNameEdit->text());
    respeqtSettings->setSerialPortHandshakingMethod(m_ui->serialPortHandshakeCombo->currentIndex());
    respeqtSettings->setSerialPortWriteDelay(m_ui->serialPortWriteDelayCombo->currentIndex());
    respeqtSettings->setSerialPortCompErrDelay(m_ui->serialPortCompErrDelayBox->value());
    respeqtSettings->setSerialPortMaximumSpeed(m_ui->serialPortBaudCombo->currentIndex());
    respeqtSettings->setSerialPortUsePokeyDivisors(m_ui->serialPortUseDivisorsBox->isChecked());
    respeqtSettings->setSerialPortPokeyDivisor(m_ui->serialPortDivisorEdit->value());
    respeqtSettings->setQtSerialPortName(m_ui->QtSerialPortDeviceNameEdit->text());
    respeqtSettings->setQtSerialPortHandshakingMethod(m_ui->QtSerialPortHandshakeCombo->currentIndex());
    respeqtSettings->setQtSerialPortWriteDelay(m_ui->QtSerialPortWriteDelayCombo->currentIndex());
    respeqtSettings->setQtSerialPortCompErrDelay(m_ui->QtSerialPortCompErrDelayBox->value());
    respeqtSettings->setQtSerialPortMaximumSpeed(m_ui->QtSerialPortBaudCombo->currentIndex());
    respeqtSettings->setQtSerialPortUsePokeyDivisors(m_ui->QtSerialPortUseDivisorsBox->isChecked());
    respeqtSettings->setQtSerialPortPokeyDivisor(m_ui->QtSerialPortDivisorEdit->value());
    respeqtSettings->setAtariSioDriverName(m_ui->atariSioDriverNameEdit->text());
    respeqtSettings->setAtariSioHandshakingMethod(m_ui->atariSioHandshakingMethodCombo->currentIndex());
    respeqtSettings->setUseHighSpeedExeLoader(m_ui->emulationHighSpeedExeLoaderBox->isChecked());
    respeqtSettings->setUseCustomCasBaud(m_ui->emulationUseCustomCasBaudBox->isChecked());
    respeqtSettings->setCustomCasBaud(m_ui->emulationCustomCasBaudSpin->value());
    respeqtSettings->setMinimizeToTray(m_ui->minimizeToTrayBox->isChecked());
    respeqtSettings->setsaveWindowsPos(m_ui->saveWinPosBox->isChecked());
    respeqtSettings->setsaveDiskVis(m_ui->saveDiskVisBox->isChecked());
    respeqtSettings->setfilterUnderscore(m_ui->filterUscore->isChecked());
    respeqtSettings->setCapitalLettersInPCLINK(m_ui->capitalLettersPCLINK->isChecked());
    respeqtSettings->setUseLargeFont(m_ui->useLargerFont->isChecked());
    respeqtSettings->setEnableShade(m_ui->enableShade->isChecked());

    int backend = SERIAL_BACKEND_STANDARD;
    if (itemQt->checkState(0) == Qt::Checked)
    {
        backend = SERIAL_BACKEND_QT;
    }
    else if (itemAtariSio->checkState(0) == Qt::Checked)
    {
        backend = SERIAL_BACKEND_SIO_DRIVER;
    }

    respeqtSettings->setBackend(backend);

    respeqtSettings->setI18nLanguage(m_ui->i18nLanguageCombo->itemData(m_ui->i18nLanguageCombo->currentIndex()).toString());
}

void OptionsDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/)
{
    if (current == itemStandard) {
        m_ui->stackedWidget->setCurrentIndex(0);
    } else if (current == itemQt) {
        m_ui->stackedWidget->setCurrentIndex(1);
    } else if (current == itemAtariSio) {
        m_ui->stackedWidget->setCurrentIndex(2);
    } else if (current == itemEmulation) {
        m_ui->stackedWidget->setCurrentIndex(3);
    } else if (current == itemI18n) {
    m_ui->stackedWidget->setCurrentIndex(4);
    }
}

void OptionsDialog::on_treeWidget_itemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (item->checkState(0) == Qt::Checked)
    {
        if (item == itemStandard)
        {
            m_ui->emulationHighSpeedExeLoaderBox->setVisible(HANDSHAKE_SOFTWARE != m_ui->serialPortHandshakeCombo->currentIndex());
        }
        else
        {
            itemStandard->setCheckState(0, Qt::Unchecked);
        }
        if (item == itemQt)
        {
            m_ui->emulationHighSpeedExeLoaderBox->setVisible(HANDSHAKE_SOFTWARE != m_ui->QtSerialPortHandshakeCombo->currentIndex());
        }
        else
        {
            itemQt->setCheckState(0, Qt::Unchecked);
        }
        if (item == itemAtariSio)
        {
            m_ui->emulationHighSpeedExeLoaderBox->setVisible(true);
        }
        else
        {
            itemAtariSio->setCheckState(0, Qt::Unchecked);
        }
    }
    else if ((itemStandard->checkState(0) == Qt::Unchecked) &&
            (itemQt->checkState(0) == Qt::Unchecked) &&
            (itemAtariSio->checkState(0) == Qt::Unchecked))
    {
        item->setCheckState(0, Qt::Checked);
    }
    m_ui->serialPortBox->setCheckState(itemStandard->checkState(0));
    m_ui->QtSerialPortBox->setCheckState(itemQt->checkState(0));
    m_ui->atariSioBox->setCheckState(itemAtariSio->checkState(0));
}

void OptionsDialog::on_serialPortHandshakeCombo_currentIndexChanged(int index)
{
    bool software_handshake = (index==HANDSHAKE_SOFTWARE);
    m_ui->serialPortWriteDelayLabel->setVisible(software_handshake);
    m_ui->serialPortWriteDelayCombo->setVisible(software_handshake);
    m_ui->serialPortBaudLabel->setVisible(!software_handshake);
    m_ui->serialPortBaudCombo->setVisible(!software_handshake);
    m_ui->serialPortUseDivisorsBox->setVisible(!software_handshake);
    m_ui->serialPortDivisorLabel->setVisible(!software_handshake);
    m_ui->serialPortDivisorEdit->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayLabel->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayBox->setVisible(!software_handshake);
    if(itemStandard->checkState((0)) == Qt::Checked)
    {
        m_ui->emulationHighSpeedExeLoaderBox->setVisible(!software_handshake);
    }
}

void OptionsDialog::on_QtSerialPortHandshakeCombo_currentIndexChanged(int index)
{
    bool software_handshake = (index==HANDSHAKE_SOFTWARE);
    m_ui->QtSerialPortWriteDelayLabel->setVisible(software_handshake);
    m_ui->QtSerialPortWriteDelayCombo->setVisible(software_handshake);
    m_ui->QtSerialPortBaudLabel->setVisible(!software_handshake);
    m_ui->QtSerialPortBaudCombo->setVisible(!software_handshake);
    m_ui->QtSerialPortUseDivisorsBox->setVisible(!software_handshake);
    m_ui->QtSerialPortDivisorLabel->setVisible(!software_handshake);
    m_ui->QtSerialPortDivisorEdit->setVisible(!software_handshake);
    m_ui->QtSerialPortCompErrDelayLabel->setVisible(!software_handshake);
    m_ui->QtSerialPortCompErrDelayBox->setVisible(!software_handshake);
    if(itemQt->checkState((0)) == Qt::Checked)
    {
        m_ui->emulationHighSpeedExeLoaderBox->setVisible(!software_handshake);
    }
}

void OptionsDialog::on_serialPortUseDivisorsBox_toggled(bool checked)
{
    m_ui->serialPortBaudLabel->setEnabled(!checked);
    m_ui->serialPortBaudCombo->setEnabled(!checked);
    m_ui->serialPortDivisorLabel->setEnabled(checked);
    m_ui->serialPortDivisorEdit->setEnabled(checked);
}

void OptionsDialog::on_QtSerialPortUseDivisorsBox_toggled(bool checked)
{
    m_ui->QtSerialPortBaudLabel->setEnabled(!checked);
    m_ui->QtSerialPortBaudCombo->setEnabled(!checked);
    m_ui->QtSerialPortDivisorLabel->setEnabled(checked);
    m_ui->QtSerialPortDivisorEdit->setEnabled(checked);
}


void OptionsDialog::on_useEmulationCustomCasBaudBox_toggled(bool checked)
{
    m_ui->emulationCustomCasBaudSpin->setEnabled(checked);
}
