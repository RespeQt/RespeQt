#ifndef NATIVEPRINTER_H
#define NATIVEPRINTER_H

#include "nativeoutput.h"
#include <memory>
#include <QPrinter>
#include <QSharedPointer>

using QPrinterPtr = QSharedPointer<QPrinter>;
namespace Printers
{
    class NativePrinter : public NativeOutput
    {
    public:
        NativePrinter();
        inline QPrinterPtr printer() const {
            return qSharedPointerDynamicCast<QPrinter>(mDevice);
        }
        virtual void newPage(bool linefeed = false) override;
        virtual bool setupOutput() override;

        static QString typeName()
        {
            return "NativePrinter";
        }

    protected:
        virtual void updateBoundingBox() override;
    };
}
#endif // NATIVEPRINTER_H
