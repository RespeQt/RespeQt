#ifndef RAWOUTPUT_H
#define RAWOUTPUT_H

#include "nativeoutput.h"
#include <QObject>

namespace Printers
{
    class RawOutput : public NativeOutput
    {
        public:
            RawOutput();
            virtual ~RawOutput();

            virtual bool beginOutput();
            virtual bool endOutput();
            virtual void updateBoundingBox();
            virtual void newPage(bool linefeed = false);
            virtual bool setupOutput();
            bool sendBuffer(const QByteArray &b, unsigned int len);

            static QString typeName()
            {
                return QObject::tr("Raw output");
            }

        protected:
#if defined(Q_OS_WIN)
#else
            int mJobId;
            QString rawPrinterName;
#endif
    };
}
#endif // RAWOUTPUT_H
