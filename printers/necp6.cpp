#include "necp6.h"

NecP6::NecP6(SioWorker *sio)
    : NativePrinterSupport(sio)
{
    mTypeId = NECP6;
    mTypeName = QString("Nec P6");
}

bool NecP6::handleBuffer(QByteArray & /*buffer*/, int /*len*/)
{
    return true;
}
