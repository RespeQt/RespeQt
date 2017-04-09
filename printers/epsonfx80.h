#ifndef EPSONFX80_H
#define EPSONFX80_H

#include "nativeprintersupport.h"

class EpsonFX80 : public NativePrinterSupport
{
public:
    EpsonFX80(SioWorker *sio);

    virtual bool handleBuffer(QByteArray &buffer, int len);
};

#endif // EPSONFX80_H
