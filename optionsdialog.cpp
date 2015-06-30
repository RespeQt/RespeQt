#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "aspeqtsettings.h"

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
    m_ui->serialPortDeviceNameEdit->setText(aspeqtSettings->serialPortName());
    m_ui->serialPortHandshakeCombo->setCurrentIndex(aspeqtSettings->serialPortHandshakingMethod());
    m_ui->serialPortBaudCombo->setCurrentIndex(aspeqtSettings->serialPortMaximumSpeed());
    m_ui->serialPortUseDivisorsBox->setChecked(aspeqtSettings->serialPortUsePokeyDivisors());
    m_ui->serialPortDivisorEdit->setValue(aspeqtSettings->serialPortPokeyDivisor());
    m_ui->atariSioDriverNameEdit->setText(aspeqtSettings->atariSioDriverName());
    m_ui->atariSioHandshakingMethodCombo->setCurrentIndex(aspeqtSettings->atariSioHandshakingMethod());
    m_ui->emulationHighSpeedExeLoaderBox->setChecked(aspeqtSettings->useHighSpeedExeLoader());
    m_ui->emulationUseCustomCasBaudBox->setChecked(aspeqtSettings->useCustomCasBaud());
    m_ui->emulationCustomCasBaudSpin->setValue(aspeqtSettings->customCasBaud());
    m_ui->minimizeToTrayBox->setChecked(aspeqtSettings->minimizeToTray());    
    m_ui->saveWinPosBox->setChecked(aspeqtSettings->saveWindowsPos());
    m_ui->saveDiskVisBox->setChecked(aspeqtSettings->saveDiskVis());
    m_ui->filterUscore->setChecked(aspeqtSettings->filterUnderscore());
    m_ui->useLargerFont->setChecked(aspeqtSettings->useLargeFont());
    m_ui->enableShade->setChecked(aspeqtSettings->enableShade());

    switch (aspeqtSettings->backend()) {
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
    if (aspeqtSettings->i18nLanguage().compare("auto") == 0)
      m_ui->i18nLanguageCombo->setCurrentIndex(0);
    m_ui->i18nLanguageCombo->addItem(QT_TR_NOOP("English"), "en");
    if (aspeqtSettings->i18nLanguage().compare("en") == 0)
      m_ui->i18nLanguageCombo->setCurrentIndex(1);
    QDir dir(":/translations/i18n/");
    QStringList filters;
    filters << "aspeqt_*.qm";
    dir.setNameFilters(filters);
    for (int i = 0; i < dir.entryList().size(); ++i) {
        local_translator.load(":/translations/i18n/" + dir.entryList()[i]);
    m_ui->i18nLanguageCombo->addItem(local_translator.translate("OptionsDialog", "English"), dir.entryList()[i].mid(7).replace(".qm", ""));
	if (dir.entryList()[i].mid(7).replace(".qm", "").compare(aspeqtSettings->i18nLanguage()) == 0) {
        m_ui->i18nLanguageCombo->setCurrentIndex(i+2);
	}
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
    aspeqtSettings->setSerialPortName(m_ui->serialPortDeviceNameEdit->text());
    aspeqtSettings->setSerialPortHandshakingMethod(m_ui->serialPortHandshakeCombo->currentIndex());
    aspeqtSettings->setSerialPortMaximumSpeed(m_ui->serialPortBaudCombo->currentIndex());
    aspeqtSettings->setSerialPortUsePokeyDivisors(m_ui->serialPortUseDivisorsBox->isChecked());
    aspeqtSettings->setSerialPortPokeyDivisor(m_ui->serialPortDivisorEdit->value());
    aspeqtSettings->setAtariSioDriverName(m_ui->atariSioDriverNameEdit->text());
    aspeqtSettings->setAtariSioHandshakingMethod(m_ui->atariSioHandshakingMethodCombo->currentIndex());
    aspeqtSettings->setUseHighSpeedExeLoader(m_ui->emulationHighSpeedExeLoaderBox->isChecked());
    aspeqtSettings->setUseCustomCasBaud(m_ui->emulationUseCustomCasBaudBox->isChecked());
    aspeqtSettings->setCustomCasBaud(m_ui->emulationCustomCasBaudSpin->value());
    aspeqtSettings->setMinimizeToTray(m_ui->minimizeToTrayBox->isChecked());
    aspeqtSettings->setsaveWindowsPos(m_ui->saveWinPosBox->isChecked());
    aspeqtSettings->setsaveDiskVis(m_ui->saveDiskVisBox->isChecked());
    aspeqtSettings->setfilterUnderscore(m_ui->filterUscore->isChecked());
    aspeqtSettings->setUseLargeFont(m_ui->useLargerFont->isChecked());
    aspeqtSettings->setEnableShade(m_ui->enableShade->isChecked());

    int backend = 0;
    if (itemAtariSio->checkState(0) == Qt::Checked) {
        backend = 1;
    }

    aspeqtSettings->setBackend(backend);

    aspeqtSettings->setI18nLanguage(m_ui->i18nLanguageCombo->itemData(m_ui->i18nLanguageCombo->currentIndex()).toString());
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
