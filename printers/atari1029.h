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
        Atari1029(SioWorker *worker);

        virtual void setupFont();

    private:
        bool mESC;
        bool mElongatedMode;
        uint8_t mGraphicsMode; // TODO Enum?
        uint16_t mGraphicsColumns;

        virtual bool handleBuffer(QByteArray &buffer, int len);
        bool handleEscapedCodes(const char b);
        bool handlePrintableCodes(const char b);
        bool elongatedMode() { return mElongatedMode; }
        void setElongatedMode(bool elongatedMode);
        bool handleGraphicsMode(const char b);
    };
}
#endif // ATARI1029_H
