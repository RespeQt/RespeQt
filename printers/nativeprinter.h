#ifndef NATIVEPRINTER_H
#define NATIVEPRINTER_H

#include "nativeoutput.h"

#include <QPrinter>

class NativePrinter : public NativeOutput
{
public:
    NativePrinter();
    inline QPrinter *printer() const {
        return dynamic_cast<QPrinter*>(mDevice);
    }

protected:
    virtual void updateBoundingBox();
};

#endif // NATIVEPRINTER_H
