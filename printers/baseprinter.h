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
    const QString &typeName() const { return mTypeName; }
    virtual bool requiresNativePrinter() const { return false; }

    virtual void handleCommand(quint8 command, quint16 aux);
    virtual bool handleBuffer(QByteArray &buffer, int len) = 0;

    virtual const QChar translateAtascii(const char b);

    // create a printer object of specified type
    static BasePrinter *createPrinter(int type, SioWorker *worker);

    static const int NUM_KNOWN_PRINTERS = 3;

    static const int TEXTPRINTER = 1;
    static const int ATARI1027 = 2;
    static const int ATARI1020 = 3;
    static const int NECP6 = -1;
    static const int EPSONFX80 = -2;
    static const int ATARI1029 = -3;

protected:
    int mTypeId;
    QString mTypeName;

    Atascii mAtascii;


    bool mPrinting;

private:
    int m_lastOperation;
};

#endif // BASEPRINTER_H
