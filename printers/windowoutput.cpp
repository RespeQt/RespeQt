#include "windowoutput.h"
#include "ui_windowoutput.h"
#include "logdisplaydialog.h"

namespace Printers {
    WindowOutput::WindowOutput(QWidget *parent) :
        QFrame(parent),
        NativeOutput(),
        ui(new Ui::WindowOutput)
    {
        ui->setupUi(this);
        mDevice = ui->page->canvas();
    }

    WindowOutput::~WindowOutput()
    {
        close();
        delete ui;
    }

    void WindowOutput::newLine(bool linefeed)
    {
        QFontMetrics metrics(*mFont);
        if (!linefeed)
            mX = mBoundingBox.left();
        if (mY + metrics.height() > mBoundingBox.bottom())
        {
            mY += metrics.lineSpacing();
            mX = 0;
        }
        ui->page->resize(QFrame::width(), mY);
    }

    void WindowOutput::newPage(bool /*linefeed*/)
    {}

    void WindowOutput::updateBoundingBox()
    {
        QFontMetrics metrics(*mFont);
        mBoundingBox = QRectF(0, 0, mDevice->width(), mDevice->height());
        mX = mBoundingBox.left();
        mY = mBoundingBox.top() + metrics.lineSpacing();
    }

}
