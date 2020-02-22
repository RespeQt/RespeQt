#ifndef PASSTHROUGH_H
#define PASSTHROUGH_H

#include "baseprinter.h"

namespace Printers
{
    class Passthrough : public BasePrinter
    {
    public:
        Passthrough(SioWorkerPtr sio);
        virtual ~Passthrough();

        virtual bool handleBuffer(QByteArray &buffer, unsigned int len);
        virtual void setupFont();
        virtual void setupOutput();

        static QString typeName()
        {
            return tr("Passthrough");
        }

    };
}
#endif // PASSTHROUGH_H
