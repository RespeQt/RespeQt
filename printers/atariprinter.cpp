#include "atariprinter.h"
 #include <utility> 
namespace Printers
{
    AtariPrinter::AtariPrinter(SioWorkerPtr worker)
        : BasePrinter(std::move(worker)),
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
