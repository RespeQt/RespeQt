#ifndef ATARIPRINTER_H
#define ATARIPRINTER_H

#include "nativeprintersupport.h"
#include "atasciiinternational.h"
#include "sioworker.h"

class AtariPrinter : public NativePrinterSupport
{
    Q_OBJECT
public:
    AtariPrinter(SioWorker *worker);

    bool internationalMode() const { return mInternational; }
    void setInternationalMode(bool internationalMode) { mInternational = internationalMode; }

    virtual const QChar translateAtascii(const char b);
protected:
    bool mInternational;
    AtasciiInternational mAtasciiInternational;
};

#endif // ATARIPRINTER_H
