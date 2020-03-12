#ifndef ATARI1020_H
#define ATARI1020_H

#include "atariprinter.h"

namespace Printers
{
    class Atari1020 : public AtariPrinter
    {
    public:
        Atari1020(SioWorkerPtr sio);

        virtual bool handleBuffer(const QByteArray &buffer, const unsigned int len) override;
        virtual void setupFont() override;
        virtual void setupOutput() override;

        static QString typeName()
        {
            return "Atari 1020";
        }

    protected:
        bool mGraphicsMode, mEsc;
        QPointF mPenPoint;
        bool mPrintText;
        int mTextOrientation;
        int mFontSize;

        bool handlePrintableCodes(const unsigned char b);
        bool handleGraphicsMode(const QByteArray &buffer, const unsigned int len, unsigned int &i);
        int fetchIntFromBuffer(const QByteArray &buffer, const unsigned int len, const unsigned int i, unsigned int &end);
        void endCommandLine();
        bool drawAxis(bool xAxis, int size, int count);
    };
}
#endif // ATARI1020_H
