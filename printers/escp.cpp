#include "escp.h"
 #include <utility> 
namespace Printers
{
    Escp::Escp(SioWorkerPtr sio)
        : Centronics(std::move(sio))
    {
        initPrinter();
    }

    void Escp::initPrinter()
    {
        mEsc = false;
    }

    bool Escp::handleBuffer(const QByteArray &/*buffer*/, const unsigned int len)
    {
        for(unsigned int i = 0; i < len; i++)
        {
            //unsigned char b = buffer.at(i);
        }
        return true;
    }

    void Escp::handlePrintableCodes(unsigned char /*b*/)
    {

    }

    void Escp::handleEscapableCodes(unsigned char /*b*/)
    {

    }

}

