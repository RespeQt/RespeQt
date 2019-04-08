#include "nativeprinter.h"

namespace Printers
{
    NativePrinter::NativePrinter()
        :NativeOutput()
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
            mX = static_cast<int>(trunc(mBoundingBox.left()));
            mY = static_cast<int>(trunc(mBoundingBox.top())) + metrics.lineSpacing();
        }
    }

    void NativePrinter::newPage(bool)
    {}

}
