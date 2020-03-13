#ifndef ATARI1027_H
#define ATARI1027_H

#include "atariprinter.h"

#include <QFont>
#include <QFontMetrics>
#include <QPrinter>
#include <QRect>

namespace Printers
{
    class Atari1027 : public AtariPrinter
    {
        Q_OBJECT
    public:
        Atari1027(SioWorkerPtr worker);

        virtual bool handleBuffer(const QByteArray &buffer, const unsigned int len) override;
        virtual void setupFont() override;

        static QString typeName()
        {
            return "Atari 1027";
        }

    private:
        bool mESC;

        bool handleEscapedCodes(const unsigned char b);
        bool handlePrintableCodes(const unsigned char b);
    };
}
#endif // ATARI1027_H
