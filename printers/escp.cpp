#include "escp.h"

namespace Printers
{
    Escp::Escp(SioWorker *sio)
        : Centronics(sio)
    {
        mTypeId = ESCP;
        mTypeName = "ESC/P";
        initPrinter();
    }

    void Escp::initPrinter()
    {
        mEsc = false;
    }

    bool Escp::handleBuffer(QByteArray &buffer, int len)
    {
        for(int i = 0; i < len; i++)
        {
            unsigned char b = buffer.at(i);
        }
        return true;
    }

    void Escp::handlePrintableCodes(unsigned char b)
    {

    }

    void Escp::handleEscapableCodes(unsigned char b)
    {

    }

}

