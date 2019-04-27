#ifndef RAWOUTPUT_H
#define RAWOUTPUT_H

#include "nativeoutput.h"
#include <QObject>

#if defined(Q_OS_WIN)
#include "windows.h"
#endif

class QComboBox;

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

            static void setupRawPrinters(QComboBox *);

        protected:
            QString rawPrinterName;
#if defined(Q_OS_WIN)
            HANDLE mJob;
#else
            int mJobId;
#endif
    };
}
#endif // RAWOUTPUT_H
