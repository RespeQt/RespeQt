#include "atariprinter.h"

namespace Printers
{
    AtariPrinter::AtariPrinter(SioWorkerPtr worker)
        : BasePrinter(worker),
        mInternational(false)
    {}

    const QChar AtariPrinter::translateAtascii(const unsigned char b) const
    {
        if (internationalMode()) {
            return mAtasciiInternational(b);
        }
        return BasePrinter::translateAtascii(b);
    }
}
