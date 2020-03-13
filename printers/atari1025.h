#ifndef ATARI1025_H
#define ATARI1025_H

#include "atariprinter.h"

#include <QFont>
#include <QFontMetrics>
#include <QPrinter>
#include <QRect>

namespace Printers
{
    class Atari1025 : public AtariPrinter
    {
        Q_OBJECT
    public:
        Atari1025(SioWorkerPtr worker);

        virtual bool handleBuffer(const QByteArray &buffer, const unsigned int len) override;
        virtual void setupFont() override;

        static QString typeName()
        {
            return "Atari 1025";
        }

    protected:
        // set char count per inch
        void setCharsPI(float chars);
        //get char count per inch
        float charsPI() const;
        // set char count per line
        void setLineChars(unsigned char chars);
        // get char count per line
        unsigned char lineChars() const;
        // set lines per inch
        void setLinesPI(unsigned char lines);
        // get lines per inch
        unsigned char linesPI() const;

    private:
        bool mESC;

        bool handleEscapedCodes(const unsigned char b);
        bool handlePrintableCodes(const unsigned char b);
        float mCPI;
        unsigned char mLineChars;
        unsigned char mLPI;
    };
}
#endif // ATARI1025_H
