#include "atariprinter.h"

AtariPrinter::AtariPrinter(SioWorker *worker)
    :BasePrinter(worker),
    mInternational(false)
{}

const QChar AtariPrinter::translateAtascii(const char b)
{
    if (internationalMode()) {
        return mAtasciiInternational(b);
    }
    return BasePrinter::translateAtascii(b);
}
