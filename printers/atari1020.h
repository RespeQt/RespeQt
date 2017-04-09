#ifndef ATARI1020_H
#define ATARI1020_H

#include "atariprinter.h"

class Atari1020 : public AtariPrinter
{
public:
    Atari1020(SioWorker *sio);

    bool handleBuffer(QByteArray &buffer, int len);
    virtual void setupFont();
    virtual void setupPrinter();

protected:
    static const unsigned char BLACK;
    static const unsigned char BLUE;
    static const unsigned char RED;
    static const unsigned char GREEN;

    static std::map<unsigned char, QColor> sColorMapping;
    bool mGraphicsMode, mEsc;
    QPoint mPenPoint;
    bool mPrintText;
    int mTextOrientation;

    bool handlePrintableCodes(const char b);
    bool handleGraphicsMode(QByteArray &buffer, int len, int &i);
    int fetchIntFromBuffer(QByteArray &buffer, int len, int i, int &end);
    void endCommandLine();
    bool drawAxis(bool xAxis, int size, int count);
};

#endif // ATARI1020_H
