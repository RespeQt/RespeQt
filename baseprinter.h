#ifndef BASEPRINTER_H
#define BASEPRINTER_H

#include "sioworker.h"

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
    BasePrinter(SioWorker *worker) : SioDevice(worker),
        mRequiresNativePrinter(false),
        mPainter(NULL),
        mFontMetrics(NULL) {}
    virtual ~BasePrinter();

    int typeId() const { return mTypeId; }
    const QString *typeName() const { return mTypeName; }
    bool requiresNativePrinter() const { return mRequiresNativePrinter; }

    virtual void handleCommand(quint8 /*command*/, quint16 /*aux*/) {}

    virtual QPainter *painter() const { return mPainter; }
    virtual void setPainter(QPainter *painter) { mPainter = painter; }

    // TODO Decide whether needed void setNativePrinter(QPrinter *printer) { mNativePrinter = printer; }
    virtual QPrinter *nativePrinter() const { return mNativePrinter; }

    virtual const QChar &translateAtascii(const char b);

protected:
    int mTypeId;
    QString *mTypeName;
    bool mRequiresNativePrinter;
    QPainter *mPainter;

    QFont mFont;
    int x, y;
    QRect mBoundingBox;
    QFontMetrics *mFontMetrics;
    QPrinter *mNativePrinter;

private:
    int m_lastOperation;
};

#endif // BASEPRINTER_H
