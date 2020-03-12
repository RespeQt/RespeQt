#ifndef ESCP_H
#define ESCP_H

#include "centronics.h"

namespace Printers
{
    class Escp : public Centronics
    {
    public:
        Escp(SioWorkerPtr sio);

        virtual bool handleBuffer(const QByteArray &buffer, const unsigned int len) override;

        static QString typeName()
        {
            return "ESC/P";
        }

    protected:
        bool mEsc; // Escape mode is off/on
        quint16 mMode, mLastMode;
        bool mDeviceControl; // Whether device is activated.
        qreal mCPI;

        void initPrinter();
        void handleEscapableCodes(unsigned char b);
        void handlePrintableCodes(unsigned char b);
        quint16 mode() { return mMode; }
    };
}
#endif // ESCP_H
