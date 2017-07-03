#ifndef NATIVEPRINTER_H
#define NATIVEPRINTER_H

#include "nativeoutput.h"

#include <QPrinter>

namespace Printers
{
    class NativePrinter : public NativeOutput
    {
    public:
        NativePrinter();
        inline QPrinter *printer() const {
            return dynamic_cast<QPrinter*>(mDevice);
        }
        virtual void newPage(bool linefeed = false);

    protected:
        virtual void updateBoundingBox();
    };
}
#endif // NATIVEPRINTER_H
