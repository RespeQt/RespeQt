#ifndef BASEPRINTER_H
#define BASEPRINTER_H

#include "sioworker.h"

#include <QByteArray>
#include <QPainter>

class BasePrinter : public SioDevice
{
    Q_OBJECT
public:
    BasePrinter(SioWorker *worker) : SioDevice(worker),
        mRequiresNativePrinter(false),
        mPainter(NULL) {}
    virtual ~BasePrinter();

    int typeId() const { return mTypeId; }
    const QString *typeName() const { return mTypeName; }
    bool requiresNativePrinter() const { return mRequiresNativePrinter; }

    virtual void handleCommand(quint8 /*command*/, quint16 /*aux*/) {}

    const QPainter *painter() { return mPainter; }
    void setPainter(QPainter *painter) { mPainter = painter; }

    virtual const QChar &translateAtascii(const char b);

protected:
    int mTypeId;
    QString *mTypeName;
    bool mRequiresNativePrinter;
    QPainter *mPainter;

private:
    int m_lastOperation;
};

#endif // BASEPRINTER_H
