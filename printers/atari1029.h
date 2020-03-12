#ifndef ATARI1029_H
#define ATARI1029_H

#include "atariprinter.h"

#include <QFont>
#include <QFontMetrics>
#include <QPrinter>
#include <QRect>

namespace Printers
{
    class Atari1029 : public AtariPrinter
    {
        Q_OBJECT
    public:
        Atari1029(SioWorkerPtr worker);

        virtual void setupFont() override;
        static QString typeName()
        {
            return "Atari 1029";
        }

    private:
        bool mESC;
        bool mElongatedMode;
        uint8_t mGraphicsMode; // TODO Enum?
        uint16_t mGraphicsColumns;

        virtual bool handleBuffer(const QByteArray &buffer, const unsigned int len) override;
        bool handleEscapedCodes(const unsigned char b);
        bool handlePrintableCodes(const unsigned char b);
        bool elongatedMode() { return mElongatedMode; }
        void setElongatedMode(bool elongatedMode);
        bool handleGraphicsMode(const unsigned char b);
    };
}
#endif // ATARI1029_H
