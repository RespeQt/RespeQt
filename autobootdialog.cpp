#include "autobootdialog.h"
#include "ui_autobootdialog.h"
#include "mainwindow.h"

#include <QTime>
#include <QDebug>

extern QString g_exefileName;
bool reload;

AutoBootDialog::AutoBootDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutoBootDialog)
{
    ui->setupUi(this);
    ui->progressBar->setVisible(false);
    connect(ui->reloadButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onClick(QAbstractButton*)));
    connect(ui->reloadButton, SIGNAL(clicked()), this, SLOT(reloadExe));
    reload = false;
    ui->progressBar->setVisible(true);
}

AutoBootDialog::~AutoBootDialog()
{
    delete ui;
}

void AutoBootDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void AutoBootDialog::closeEvent(QCloseEvent *)
{
    if (!reload) g_exefileName = "";
}

void AutoBootDialog::booterStarted()
{
    ui->label->setText(tr("Atari is loading the booter."));
}

void AutoBootDialog::booterLoaded()
{
    ui->label->setText(tr("Atari is loading the program.\n\nFor some programs you may have to close this dialog manually when the program starts."));
}

void AutoBootDialog::blockRead(int current, int all)
{
    ui->progressBar->setMaximum(all);
    ui->progressBar->setValue(current);
}

void AutoBootDialog::loaderDone()
{
    accept();
}
void AutoBootDialog::onClick(QAbstractButton *button)
{
    if(button->text() == "Cancel") {
        g_exefileName="";
        return;
    }
}

void AutoBootDialog::reloadExe()
{
    reload = true;
    close();
}
