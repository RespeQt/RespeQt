#include "nativeprinter.h"
#include <QPrintDialog>

namespace Printers
{
    NativePrinter::NativePrinter()
        :NativeOutput()
    {
        QPrinterPtr printer = QPrinterPtr::create();
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

    bool NativePrinter::setupOutput()
    {
        QPrintDialog dialog(printer().data());
        dialog.setOption(QAbstractPrintDialog::PrintSelection, false);
        dialog.setOption(QAbstractPrintDialog::PrintPageRange, false);
        dialog.setOption(QAbstractPrintDialog::PrintCurrentPage, false);
        return dialog.exec() == QDialog::Accepted;
    }
}
