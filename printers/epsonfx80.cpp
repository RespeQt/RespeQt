#include "epsonfx80.h"

EpsonFX80::EpsonFX80(SioWorker *sio)
    : NativePrinterSupport(sio)
{
    mTypeId = EPSONFX80;
    mTypeName = "Epson FX-80";
}

bool EpsonFX80::handleBuffer(QByteArray &/*buffer*/, int /*len*/)
{
    return true;
}
