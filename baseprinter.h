#ifndef BASEPRINTER_H
#define BASEPRINTER_H

#include "sioworker.h"
#include "atascii.h"

#include <QByteArray>
#include <QPainter>
#include <QPrinter>
#include <QRect>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

class BasePrinter : public SioDevice
{
    Q_OBJECT
public:
    BasePrinter(SioWorker *worker);
    virtual ~BasePrinter();

    int typeId() const { return mTypeId; }
    const QString *typeName() const { return mTypeName; }
    virtual bool requiresNativePrinter() const { return false; }

    virtual void handleCommand(quint8 command, quint16 aux);
    virtual bool handleBuffer(QByteArray &buffer, int len) = 0;

    virtual QPrinter *nativePrinter() const { return mNativePrinter; }

    virtual void beginPrint();
    virtual void endPrint();

    virtual const QChar translateAtascii(const char b);

    // create a printer object of specified type
    static BasePrinter *createPrinter(int type, SioWorker *worker);

protected:
    int mTypeId;
    QString *mTypeName;
    QPainter *mPainter;

    QFont mFont;
    int x, y;
    QRect mBoundingBox;
    QFontMetrics *mFontMetrics;
    QPrinter *mNativePrinter;

    Atascii mAtascii;

    static const int TEXTPRINTER = 1;
    static const int ATARI1027 = 2;

    bool mPrinting;

    void setupFont() {}
    void setupPrinter();


private:
    int m_lastOperation;
};

#endif // BASEPRINTER_H
