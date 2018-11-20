#ifndef BASEPRINTER_H
#define BASEPRINTER_H

#include "sioworker.h"
#include "atascii.h"
#include "nativeoutput.h"

#include <QByteArray>
#include <QPainter>
#include <QPrinter>
#include <QRect>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>

namespace Printers
{
    class BasePrinter : public SioDevice
    {
        Q_OBJECT
    public:
        BasePrinter(SioWorker *worker);
        virtual ~BasePrinter();

        int typeId() const { return mTypeId; }
        const QString &typeName() const { return mTypeName; }

        virtual void handleCommand(quint8 command, quint16 aux);
        virtual bool handleBuffer(QByteArray &buffer, int len) = 0;

        virtual const QChar translateAtascii(const char b);

        NativeOutput *output() const { return mOutput; }
        void setOutput(NativeOutput *output);

        // create a printer object of specified type
        static BasePrinter *createPrinter(int type, SioWorker *worker);

        static const int NUM_KNOWN_PRINTERS = 2;

        static const int ATARI1027 = 1;
        static const int ATARI1020 = 2;
        static const int ATARI1029 = 3;
        static const int ESCP = -1;

    protected:
        // This should be static methods, because they are called
        // from the constructor
        virtual void setupFont() {}
        virtual void setupOutput();

        int mTypeId;
        QString mTypeName;
        Atascii mAtascii;
        NativeOutput *mOutput;

    private:
        int m_lastOperation;

    };
}
#endif // BASEPRINTER_H
