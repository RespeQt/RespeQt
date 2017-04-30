#include "escp.h"

Escp::Escp(SioWorker *sio)
    : Centronics(sio)
{
    mTypeId = ESCP;
    mTypeName = "ESC/P";
}

bool Escp::handleBuffer(QByteArray &/*buffer*/, int /*len*/)
{
    return true;
}
