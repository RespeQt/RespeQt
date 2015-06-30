#include "textprinterwindow.h"
#include "ui_textprinterwindow.h"
#include "aspeqtsettings.h"

#include <QFileDialog>
#include <QPrintDialog>
#include <QPrinter>

// Includes, Globals and various additional class declarations // 
#include <QString>
#include <QFontComboBox>
#include <QMessageBox>

int effAtasciiFont = 0;
int effFontSize = 0;
bool showAscii = true;
bool showAtascii = true;
int fontSize = 9;
QString atasciiFont("Atari Classic Chunky");


TextPrinterWindow::TextPrinterWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TextPrinterWindow)
{
    ui->setupUi(this);
    QFont f;
    f.setFixedPitch(true);
    f.setFamily("monospace");
    ui->printerTextEditASCII->setFont(f);

    // Set Initial Font for ATASCII text Window // 
    QFont a;
    a.setFixedPitch(true);
    a.setFamily(atasciiFont);
    a.setPointSize(fontSize);
    ui->printerTextEdit->setFont(a);
    ui->atasciiFontName->setText(atasciiFont + " - " + QString("%1").arg(fontSize));
    effAtasciiFont = 1;

    connect(ui->asciiFontName, SIGNAL(currentFontChanged(QFont)), this, SLOT(asciiFontChanged(QFont)));

}

TextPrinterWindow::~TextPrinterWindow()
{
    delete ui;
}

void TextPrinterWindow::changeEvent(QEvent *e)
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

void TextPrinterWindow::closeEvent(QCloseEvent *e)
{
    // Save Current TexPrinterWindow Position and size // 
    if (aspeqtSettings->saveWindowsPos()) {
        aspeqtSettings->setLastPrtHorizontalPos(TextPrinterWindow::geometry().x());
        aspeqtSettings->setLastPrtVerticalPos(TextPrinterWindow::geometry().y());
        aspeqtSettings->setLastPrtWidth(TextPrinterWindow::geometry().width());
        aspeqtSettings->setLastPrtHeight(TextPrinterWindow::geometry().height());
    }
    emit closed();
    e->accept();
}
 void TextPrinterWindow::print(const QString &text)
{
     // DO both ASCII and ATASCII windows   // 
    QTextCursor c = ui->printerTextEdit->textCursor();
    c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    ui->printerTextEdit->setTextCursor(c);
    ui->printerTextEdit->insertPlainText(text);

    int n = text.size();
    QByteArray textASCII;
    textASCII.append(text);

    // Disable ATASCII Inverse Video for ASCII window // 
    for (int x = 0; x <= n-1; ++x){
        int byte = textASCII[x];
        if (byte < 0){
            textASCII[x] = byte ^ 0x80;
        }
    }
    c = ui->printerTextEditASCII->textCursor();
    c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
    ui->printerTextEditASCII->setTextCursor(c);
    ui->printerTextEditASCII->insertPlainText(textASCII);
}

void TextPrinterWindow::on_actionWord_wrap_triggered()
{
    if (ui->actionWord_wrap->isChecked()) {
        ui->printerTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        // 
        ui->printerTextEditASCII->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    } else {
        ui->printerTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
        // 
        ui->printerTextEditASCII->setLineWrapMode(QPlainTextEdit::NoWrap);

    }
}

void TextPrinterWindow::on_actionClear_triggered()
{
    ui->printerTextEdit->clear();
    // 
    ui->printerTextEditASCII->clear();
    ui->actionStrip_Line_Numbers->setEnabled(true);
}

// Toggle ATASCII fonts // 
void TextPrinterWindow::on_actionAtasciiFont_triggered()
{
    ++effAtasciiFont;
    if (effAtasciiFont == 4){
        effAtasciiFont = 1;
    }
    switch (effAtasciiFont) {
            case 1 :
                atasciiFont = "Atari Classic Chunky";
                break;
            case 2 :
                atasciiFont = "Atari Classic Smooth";
                break;
            case 3 :
                atasciiFont = "Atari Classic Extrasmooth";
    }
    QFont(a);
    a.setFamily(atasciiFont);
    a.setPointSize(fontSize);
    ui->printerTextEdit->setFont(a);
    ui->atasciiFontName->setText(atasciiFont + " - " + QString("%1").arg(fontSize));
}

// Toggle Atari Output Font Size in both ATASCII and ASCII Windows // 
void TextPrinterWindow::on_actionFont_Size_triggered()
{
    ++effFontSize;
    if (effFontSize == 4){
        effFontSize = 1;
    }
    switch (effFontSize) {
            case 1 :
                fontSize = 6;
                break;
            case 2 :
                fontSize = 9;
                break;
            case 3 :
                fontSize = 12;
    }
    QFont(a);
    a.setFamily(atasciiFont);
    a.setPointSize(fontSize);
    ui->printerTextEdit->setFont(a);
    ui->atasciiFontName->setText(atasciiFont + " - " + QString("%1").arg(fontSize));
    QFont (f);
    f.setFamily(ui->asciiFontName->currentFont().toString());
    f.setPointSize(fontSize);
    ui->printerTextEditASCII->setFont(f);

}
// Change ASCII text window font    // 
void TextPrinterWindow::asciiFontChanged(const QFont &)
{
    ui->printerTextEditASCII->setFont(ui->asciiFontName->currentFont());
}

// Hide/Show Ascii text window  // 
void TextPrinterWindow::on_actionHideShow_Ascii_triggered()
{
    if (showAscii) {
        ui->printerTextEditASCII->hide();
        ui->asciiFontName->hide();
        showAscii = false;
    }
    else {
        ui->printerTextEditASCII->show();
        ui->asciiFontName->show();
        showAscii = true;
    }
}
// Hide/Show Atascii text window  // 
void TextPrinterWindow::on_actionHideShow_Atascii_triggered()
{
    if (showAtascii) {
        ui->printerTextEdit->hide();
        ui->atasciiFontName->hide();
        showAtascii = false;
    }
    else {
        ui->printerTextEdit->show();
        ui->atasciiFontName->show();
        showAtascii = true;
    }
}
// Send to Printer Action   // 
void TextPrinterWindow::on_actionPrint_triggered()
{
    QPrinter printer;
    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    if (dialog->exec() != QDialog::Accepted)
    return;

    ui->printerTextEdit->print(&printer);
}

void TextPrinterWindow::on_actionSave_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save printer text output"), aspeqtSettings->lastPrinterTextDir(),
                                                    tr("Text files (*.txt);;All files (*)"), 0);
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastPrinterTextDir(QFileInfo(fileName).absolutePath());
    QFile file(fileName);
    file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text);
    file.write(ui->printerTextEdit->toPlainText().toLatin1());
}
 void TextPrinterWindow::on_actionStrip_Line_Numbers_triggered()
{
     bool number;
     bool lineNumberFound = false;

     QString plainTextEditContents = ui->printerTextEdit->toPlainText();
     QStringList lines = plainTextEditContents.split("\n");

     for (int i = 0; i < lines.size(); ++i) {
        int x = lines.at(i).indexOf(" ");
        if (x > 0) {
        lines.at(i).mid(1,x-1).toInt(&number);
            if (number) {
                lineNumberFound = true;
                break;
            }
        }
     }
     if (!lineNumberFound) {
         QMessageBox::information (this, tr("Stripping Line Numbers.."),tr("The text does not seem to contain any line numbers!"),QMessageBox::Ok);
     } else {
         ui->printerTextEdit->clear();
         ui->printerTextEditASCII->clear();

         for (int i = 0; i < lines.size(); ++i) {
             int x = lines.at(i).indexOf(" ");
             if (x > 0) {
             lines.at(i).mid(1,x-1).toInt(&number);
                 if (!number) {
                     x = -1;
                 }
             }
             QTextCursor c = ui->printerTextEdit->textCursor();
             c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
             ui->printerTextEdit->setTextCursor(c);
             ui->printerTextEdit->insertPlainText(lines.at(i).mid(x+1)+"\n");

             c = ui->printerTextEditASCII->textCursor();
             c.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
             ui->printerTextEditASCII->setTextCursor(c);
             ui->printerTextEditASCII->insertPlainText(lines.at(i).mid(x+1)+"\n");
         }
//         ui->actionStrip_Line_Numbers->setEnabled(false);
     }
 }
