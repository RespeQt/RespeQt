/*
 * aboutdialog.cpp - sets up the about dialog
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent, QString version) :
    QDialog(parent),
    m_ui(new Ui::AboutDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    m_ui->setupUi(this);

    m_ui->versionLabel->setText(tr("version %1").arg(version));
    m_ui->textBrowser->setSource(*new QUrl(tr("qrc:/documentation/about.html")));

    //connect(this, SIGNAL(accepted()), this, SLOT(AboutDialog_accepted()));
}

AboutDialog::~AboutDialog()
{
    delete m_ui;
}

void AboutDialog::changeEvent(QEvent *e)
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
