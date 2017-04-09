#ifndef NECP6_H
#define NECP6_H

#include "sioworker.h"
#include "nativeprintersupport.h"

class NecP6 : public NativePrinterSupport
{
public:
    NecP6(SioWorker *sio);

    bool handleBuffer(QByteArray &buffer, int len);
};

#endif // NECP6_H
