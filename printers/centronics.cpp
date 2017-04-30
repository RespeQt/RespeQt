#include "centronics.h"

Centronics::Centronics(SioWorker *sio)
    : BasePrinter(sio)
{}

const QChar Centronics::translateAtascii(const char b)
{
    if ((unsigned char)b == 155) // Translate EOL to CR
    {
        return QChar(13);
    }
    return BasePrinter::translateAtascii(b);
}
