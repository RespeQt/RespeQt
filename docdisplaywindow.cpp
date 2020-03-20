/*
 * docdisplaywindow.cpp
 *
 * Copyright 2015 Joseph Zatarski
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "docdisplaywindow.h"
#include "ui_docdisplaywindow.h"
#include "respeqtsettings.h"

#include <QFileDialog>
#include <QPrintDialog>
#include <QPrinter>

DocDisplayWindow::DocDisplayWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DocDisplayWindow)
{
    ui->setupUi(this);
    QFont f;
    f.setFixedPitch(true);
    f.setFamily("monospace");
    ui->docDisplay->setFont(f);
}

DocDisplayWindow::~DocDisplayWindow()
{
    delete ui;
}

void DocDisplayWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void DocDisplayWindow::closeEvent(QCloseEvent *e)
{
    emit closed();
    e->accept();
}

void DocDisplayWindow::print(const QString &text)
{
    QTextCursor c = ui->docDisplay->textCursor();
    c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    ui->docDisplay->setTextCursor(c);
    ui->docDisplay->insertPlainText(text);
}

void DocDisplayWindow::on_actionPrint_triggered()
{
     QPrinter printer;
     auto *dialog = new QPrintDialog(&printer, this);
     if (dialog->exec() != QDialog::Accepted)
         return;
     ui->docDisplay->print(&printer);
}
