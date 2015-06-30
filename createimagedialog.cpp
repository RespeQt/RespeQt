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
    
    connect(m_ui->sectorsSpin, SIGNAL(valueChanged(int)), SLOT(recalculate()));
    connect(m_ui->densityCombo, SIGNAL(currentIndexChanged(int)), SLOT(recalculate()));
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

void CreateImageDialog::on_stdEnhancedButton_toggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(1040);
        m_ui->densityCombo->setCurrentIndex(0);
        m_ui->geometryWidget->setEnabled(false);
    }
}

void CreateImageDialog::on_stdSingleButton_toggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(720);
        m_ui->densityCombo->setCurrentIndex(0);
        m_ui->geometryWidget->setEnabled(false);
    }
}

void CreateImageDialog::on_stdDoubleButton_toggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(720);
        m_ui->densityCombo->setCurrentIndex(1);
        m_ui->geometryWidget->setEnabled(false);
    }
}

void CreateImageDialog::on_doubleDoubleButton_toggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(1440);
        m_ui->densityCombo->setCurrentIndex(1);
        m_ui->geometryWidget->setEnabled(false);
    }
}

void CreateImageDialog::on_customButton_toggled(bool checked)
{
    if (checked) {
        m_ui->geometryWidget->setEnabled(true);
    }
}

void CreateImageDialog::on_harddiskButton_toggled(bool checked)
{
    if (checked) {
        m_ui->sectorsSpin->setValue(65535);
        m_ui->densityCombo->setCurrentIndex(1);
        m_ui->geometryWidget->setEnabled(false);
    }
}
