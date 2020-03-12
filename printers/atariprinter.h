#ifndef ATARIPRINTER_H
#define ATARIPRINTER_H

#include "baseprinter.h"
#include "atasciiinternational.h"
#include "sioworker.h"

namespace Printers
{
    class AtariPrinter : public BasePrinter
    {
        Q_OBJECT
    public:
        AtariPrinter(SioWorkerPtr worker);

        bool internationalMode() const { return mInternational; }
        void setInternationalMode(bool internationalMode) { mInternational = internationalMode; }

        virtual const QChar translateAtascii(const unsigned char b) const override;
    protected:
        bool mInternational;
        AtasciiInternational mAtasciiInternational;
    };
}
#endif // ATARIPRINTER_H
