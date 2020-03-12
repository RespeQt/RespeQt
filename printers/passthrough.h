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

        virtual bool handleBuffer(const QByteArray &buffer, const unsigned int len) override;
        virtual void setupFont() override;
        virtual void setupOutput() override;

        static QString typeName()
        {
            return tr("Passthrough");
        }

    };
}
#endif // PASSTHROUGH_H
