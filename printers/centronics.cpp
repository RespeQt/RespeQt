#include "centronics.h"

namespace Printers
{
    Centronics::Centronics(SioWorkerPtr sio)
        : BasePrinter(sio)
    {}

    const QChar Centronics::translateAtascii(const unsigned char b) const
    {
        if (static_cast<unsigned char>(b) == 155) // Translate EOL to CR
        {
            return QChar(13);
        }
        return BasePrinter::translateAtascii(b);
    }
}
