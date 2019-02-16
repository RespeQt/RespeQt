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
        QPicture *temp = new QPicture();
        temp->setBoundingRect(ui->page->geometry());
        mDevice = temp;
        QObject::connect(this, &WindowOutput::newCanvas,
                ui->page, &BufferedPaintWidget::updateCanvas);
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
        mPainter->drawRect(QRect(0, 0, mX, mY));
        mPainter->end();
        QPicture *temp = dynamic_cast<QPicture*>(mDevice);
        temp-> setBoundingRect(QRect(0, 0, mBoundingBox.left(), mY));
        emit newCanvas(*temp);
        mPainter->begin(mDevice);
        mPainter->setFont(*mFont);
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
