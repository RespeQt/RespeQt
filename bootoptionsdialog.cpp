#include "bootoptionsdialog.h"
#include "ui_bootoptionsdialog.h"
#include "mainwindow.h"

#include <QTranslator>
#include <QDir>

extern QString g_aspeQtAppPath;
extern bool g_disablePicoHiSpeed;

QString selectedDOS, bootDir;

BootOptionsDialog::BootOptionsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BootOptionsDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    m_ui->setupUi(this);

    connect(m_ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onClick(QAbstractButton*)));
    connect(m_ui->myPicoDOS, SIGNAL(toggled(bool)), this, SLOT(picoDOSToggled()));
    connect(m_ui->disablePicoHiSpeed, SIGNAL(toggled(bool)), this, SLOT(picoHighSpeedToggled()));
}

BootOptionsDialog::~BootOptionsDialog()
{
    delete m_ui;
}

void BootOptionsDialog::changeEvent(QEvent *e)
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
void BootOptionsDialog::onClick(QAbstractButton* button)
{
    if(button->text() == tr("Apply")){
        if(m_ui->atariDOS->isChecked()) selectedDOS = "$bootata";
        if(m_ui->myDOS->isChecked()) selectedDOS = "$bootmyd";
        if(m_ui->dosXL->isChecked()) selectedDOS = "$bootdxl";
        if(m_ui->smartDOS->isChecked()) selectedDOS = "$bootsma";
        if(m_ui->spartaDOS->isChecked()) selectedDOS = "$bootspa";
        if(m_ui->myPicoDOS->isChecked()) selectedDOS = "$bootpic";
        bootDir = g_aspeQtAppPath + "/" + selectedDOS;
        emit giveFolderPath(0);
    }
    return;
}
void BootOptionsDialog::folderPath(QString fPath)
{
    // fPath is received from the MainWindow and it is
    // the path to the PC Folder Image mounted as boot drive

    QDir dir;
    QFile file;
    QStringList filters;
    QStringList allFiles;
    QString fileName;

    // First delete existing boot files in the Folder Image
    // then copy new boot files from the appropriate DOS directory

    dir.setPath(fPath);
    filters << "*dos.sys" << "dup.sys" << "dosxl.sys"
            << "autorun.sys" << "ramdisk.com" << "menu.com"
            << "startup.exc" << "x*.dos" << "startup.bat" << "$*.bin";
    allFiles =  dir.entryList(filters, QDir::Files);
    foreach(fileName, allFiles) {
        file.remove(fPath + "/" + fileName);
    }
    dir.setPath(g_aspeQtAppPath + "/" + selectedDOS);
    allFiles =  dir.entryList(QDir::NoDotAndDotDot | QDir::Files);
    foreach(fileName, allFiles) {
        file.copy(dir.path() + "/" + fileName, fPath + "/" + fileName);
    }
    close();
}

void BootOptionsDialog::picoDOSToggled()
{
    if(m_ui->myPicoDOS->isChecked()) {
        m_ui->disablePicoHiSpeed->setEnabled(true);
        g_disablePicoHiSpeed = false;
    } else {
        m_ui->disablePicoHiSpeed->setEnabled(false);
    }
}

void BootOptionsDialog::picoHighSpeedToggled()
{
    if(m_ui->disablePicoHiSpeed->isChecked())
        g_disablePicoHiSpeed = true;
}
