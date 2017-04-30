#include "nativeprinter.h"

NativePrinter::NativePrinter()
{
    QPrinter *printer = new QPrinter();
    mDevice = printer;
    mBoundingBox = printer->pageRect();
}

void NativePrinter::updateBoundingBox()
{
    if (mFont)
    {
        QFontMetrics metrics(*mFont);
        mBoundingBox = printer()->pageRect();
        x = mBoundingBox.left();
        y = mBoundingBox.top() + metrics.lineSpacing();
    }
}
