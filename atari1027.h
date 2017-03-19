#ifndef ATARI1027_H
#define ATARI1027_H

#include "baseprinter.h"

#include <QFont>
#include <QFontMetrics>
#include <QPrinter>
#include <QRect>

class Atari1027 : public BasePrinter
{
    Q_OBJECT
public:
    Atari1027(SioWorker *worker);

    virtual void handleCommand(quint8 command, quint16 aux);
    void setNativePrinter(QPrinter *printer) { mNativePrinter = printer; }
    const QPrinter *nativePrinter() const { return mNativePrinter; }

private:
    int m_lastOperation;
    bool mInternational;
    bool mFirstESC, mSecondESC;
    QFont mFont;
    int x, y;
    QRect mBoundingBox;
    QFontMetrics mFontMetrics;
    QPrinter *mNativePrinter;

    bool handleBuffer(const QByteArray &buffer);
    bool handleEscapedCodes(const char b);
    bool handlePrintableCodes(const char b);
};

#endif // ATARI1027_H
