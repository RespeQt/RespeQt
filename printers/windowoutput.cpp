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

    void WindowOutput::newLine()
    {
        QFontMetrics metrics(*mFont);
        NativeOutput::x = mBoundingBox.left();
        if (NativeOutput::y + metrics.height() > mBoundingBox.bottom())
        {
            NativeOutput::y += metrics.lineSpacing();
            NativeOutput::x = 0;
        }
        ui->page->resize(width(), NativeOutput::y);
    }
}
