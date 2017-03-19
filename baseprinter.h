#ifndef BASEPRINTER_H
#define BASEPRINTER_H

#include "sioworker.h"
#include <QByteArray>

class BasePrinter : public SioDevice
{
    Q_OBJECT
public:
    BasePrinter(SioWorker *worker) : SioDevice(worker),
        mRequiresNativePrinter(false) {}
    virtual ~BasePrinter();

    int typeId() const { return mTypeId; }
    const QString *typeName() const { return mTypeName; }
    bool requiresNativePrinter() const { return mRequiresNativePrinter; }

    virtual void handleCommand(quint8 /*command*/, quint16 /*aux*/) {}

protected:
    int mTypeId;
    QString *mTypeName;
    bool mRequiresNativePrinter;

private:
    int m_lastOperation;
};

#endif // BASEPRINTER_H
