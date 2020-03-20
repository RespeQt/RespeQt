/*
 * createimagedialog.cpp
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "createimagedialog.h"
#include "ui_createimagedialog.h"

CreateImageDialog::CreateImageDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::CreateImageDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    m_ui->setupUi(this);
    
    void (QSpinBox::*valueChangedSignal)(int) = &QSpinBox::valueChanged;
    connect(m_ui->sectorsSpin, valueChangedSignal, this, &CreateImageDialog::recalculate);
    void (QComboBox::*densitySignal)(const QString&) = &QComboBox::currentIndexChanged;
    connect(m_ui->densityCombo, densitySignal, this, &CreateImageDialog::recalculate);
    connect(m_ui->harddiskButton, &QRadioButton::toggled, this, &CreateImageDialog::harddiskToggled);
    connect(m_ui->customButton, &QRadioButton::toggled, this, &CreateImageDialog::customToggled);
    connect(m_ui->doubleDoubleButton, &QRadioButton::toggled, this, &CreateImageDialog::doubleDoubleToggled);
    connect(m_ui->stdDoubleButton, &QRadioButton::toggled, this, &CreateImageDialog::standardDoubleToggled);
    connect(m_ui->stdSingleButton, &QRadioButton::toggled, this, &CreateImageDialog::standardSingleToggled);
    connect(m_ui->stdEnhancedButton, &QRadioButton::toggled, this, &CreateImageDialog::standardEnhancedToggled);
}

CreateImageDialog::~CreateImageDialog()
{
    delete m_ui;
}

void CreateImageDialog::changeEvent(QEvent *e)
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

int CreateImageDialog::sectorCount()
{
    return m_ui->sectorsSpin->value();
}

int CreateImageDialog::sectorSize()
{
    switch (m_ui->densityCombo->currentIndex()) {
        case 0:
            return 128;
        case 1:
            return 256;
        case 2:
            return 512;
        case 3:
            return 8192;
        default:
            return 0;
    }
}

void CreateImageDialog::recalculate()
{
    int sectors = m_ui->sectorsSpin->value();
    int size = sectors * sectorSize();

    if (m_ui->densityCombo->currentIndex() == 1) {
        if (sectors <= 3) {
            size = sectors * 128;
        } else {
            size -= 384;
        }
    }

    int sizeK = (size + 512) / 1024;

    m_ui->capacityLabel->setText(tr("Total image capacity: %1 bytes (%2 K)")
                                 .arg(size)
                                 .arg(sizeK));
}

void CreateImageDialog::standardEnhancedToggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(1040);
        m_ui->densityCombo->setCurrentIndex(0);
        m_ui->geometryWidget->setEnabled(false);
    }
}

void CreateImageDialog::standardSingleToggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(720);
        m_ui->densityCombo->setCurrentIndex(0);
        m_ui->geometryWidget->setEnabled(false);
    }
}

void CreateImageDialog::standardDoubleToggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(720);
        m_ui->densityCombo->setCurrentIndex(1);
        m_ui->geometryWidget->setEnabled(false);
    }
}

void CreateImageDialog::doubleDoubleToggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(1440);
        m_ui->densityCombo->setCurrentIndex(1);
        m_ui->geometryWidget->setEnabled(false);
    }
}

void CreateImageDialog::customToggled(bool checked)
{
    if (checked) {
        m_ui->geometryWidget->setEnabled(true);
    }
}

void CreateImageDialog::harddiskToggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(65535);
        m_ui->densityCombo->setCurrentIndex(1);
        m_ui->geometryWidget->setEnabled(false);
    }
}
