#include "windowoutput.h"
#include "ui_windowoutput.h"
#include "logdisplaydialog.h"

namespace Printers {
    WindowOutput::WindowOutput(QWidget *parent) :
        QFrame(parent),
        NativeOutput(),
        ui(new Ui::WindowOutput),
        mPicture(NULL)
    {
        ui->setupUi(this);
        mPicture = new QPicture();
        mPicture->setBoundingRect(ui->page->geometry());
        mDevice = mPicture;
    }

    WindowOutput::~WindowOutput()
    {
        close();
        delete ui;
    }

    void WindowOutput::paintEvent(QPaintEvent *)
    {
        if (mPicture && mPainter)
        {
            mPainter->drawPicture(0, 0, *mPicture);
            mPainter->drawText(0, 0, "TEST");
        }
    }

    void WindowOutput::newLine()
    {
        NativeOutput::newLine();
        qDebug() << "!n"<<"newLine";
        update();
    }

    void WindowOutput::newPage()
    {
        QRect rect = mPicture->boundingRect();
        QFontMetricsF metrics(*mFont);
        rect.moveBottom(rect.bottom() + metrics.lineSpacing());
        mPicture->setBoundingRect(rect);
        ui->page->resize(rect.width(), rect.height());
    }
}
