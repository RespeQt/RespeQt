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
    itemAtariSio = m_ui->treeWidget->topLevelItem(0)->child(1);
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
    m_ui->atariSioDriverNameEdit->setText(respeqtSettings->atariSioDriverName());
    m_ui->atariSioHandshakingMethodCombo->setCurrentIndex(respeqtSettings->atariSioHandshakingMethod());
    m_ui->emulationHighSpeedExeLoaderBox->setChecked(respeqtSettings->useHighSpeedExeLoader());
    m_ui->emulationUseCustomCasBaudBox->setChecked(respeqtSettings->useCustomCasBaud());
    m_ui->emulationCustomCasBaudSpin->setValue(respeqtSettings->customCasBaud());
    m_ui->minimizeToTrayBox->setChecked(respeqtSettings->minimizeToTray());
    m_ui->saveWinPosBox->setChecked(respeqtSettings->saveWindowsPos());
    m_ui->saveDiskVisBox->setChecked(respeqtSettings->saveDiskVis());
    m_ui->filterUscore->setChecked(respeqtSettings->filterUnderscore());
    m_ui->useLargerFont->setChecked(respeqtSettings->useLargeFont());
    m_ui->enableShade->setChecked(respeqtSettings->enableShade());

    switch (respeqtSettings->backend()) {
        case 0:
            itemStandard->setCheckState(0, Qt::Checked);
            itemAtariSio->setCheckState(0, Qt::Unchecked);
            m_ui->treeWidget->setCurrentItem(itemStandard);
            break;
        case 1:
            itemStandard->setCheckState(0, Qt::Unchecked);
            itemAtariSio->setCheckState(0, Qt::Checked);
            m_ui->treeWidget->setCurrentItem(itemAtariSio);
            break;
    }
    m_ui->serialPortBox->setCheckState(itemStandard->checkState(0));
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
    m_ui->emulationHighSpeedExeLoaderBox->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayLabel->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayBox->setVisible(!software_handshake);
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
    respeqtSettings->setAtariSioDriverName(m_ui->atariSioDriverNameEdit->text());
    respeqtSettings->setAtariSioHandshakingMethod(m_ui->atariSioHandshakingMethodCombo->currentIndex());
    respeqtSettings->setUseHighSpeedExeLoader(m_ui->emulationHighSpeedExeLoaderBox->isChecked());
    respeqtSettings->setUseCustomCasBaud(m_ui->emulationUseCustomCasBaudBox->isChecked());
    respeqtSettings->setCustomCasBaud(m_ui->emulationCustomCasBaudSpin->value());
    respeqtSettings->setMinimizeToTray(m_ui->minimizeToTrayBox->isChecked());
    respeqtSettings->setsaveWindowsPos(m_ui->saveWinPosBox->isChecked());
    respeqtSettings->setsaveDiskVis(m_ui->saveDiskVisBox->isChecked());
    respeqtSettings->setfilterUnderscore(m_ui->filterUscore->isChecked());
    respeqtSettings->setUseLargeFont(m_ui->useLargerFont->isChecked());
    respeqtSettings->setEnableShade(m_ui->enableShade->isChecked());

    int backend = 0;
    if (itemAtariSio->checkState(0) == Qt::Checked) {
        backend = 1;
    }

    respeqtSettings->setBackend(backend);

    respeqtSettings->setI18nLanguage(m_ui->i18nLanguageCombo->itemData(m_ui->i18nLanguageCombo->currentIndex()).toString());
}

void OptionsDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/)
{
    if (current == itemStandard) {
        m_ui->stackedWidget->setCurrentIndex(0);
    } else if (current == itemAtariSio) {
        m_ui->stackedWidget->setCurrentIndex(1);
    } else if (current == itemEmulation) {
        m_ui->stackedWidget->setCurrentIndex(2);
    } else if (current == itemI18n) {
	m_ui->stackedWidget->setCurrentIndex(3);
    }
}

void OptionsDialog::on_treeWidget_itemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (item->checkState(0) == Qt::Checked) {
        if (item != itemStandard) {
            itemStandard->setCheckState(0, Qt::Unchecked);
        }
        if (item != itemAtariSio) {
            itemAtariSio->setCheckState(0, Qt::Unchecked);
        }
    } else if ((itemStandard->checkState(0) == Qt::Unchecked) &&
               (itemAtariSio->checkState(0) == Qt::Unchecked)) {
        item->setCheckState(0, Qt::Checked);
    }
    m_ui->serialPortBox->setCheckState(itemStandard->checkState(0));
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
    m_ui->emulationHighSpeedExeLoaderBox->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayLabel->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayBox->setVisible(!software_handshake);
}

void OptionsDialog::on_serialPortUseDivisorsBox_toggled(bool checked)
{
    m_ui->serialPortBaudLabel->setEnabled(!checked);
    m_ui->serialPortBaudCombo->setEnabled(!checked);
    m_ui->serialPortDivisorLabel->setEnabled(checked);
    m_ui->serialPortDivisorEdit->setEnabled(checked);
}

void OptionsDialog::on_useEmulationCustomCasBaudBox_toggled(bool checked)
{
    m_ui->emulationCustomCasBaudSpin->setEnabled(checked);
}
