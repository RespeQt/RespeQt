#ifndef RAWOUTPUT_H
#define RAWOUTPUT_H

#include "nativeoutput.h"
#include <QObject>

#if defined(Q_OS_WIN)
#include <windows.h>
#elif defined (Q_OS_MAC) || defined(Q_OS_LINUX)
#include <cups/cups.h>
#endif

class QComboBox;

namespace Printers
{
    class RawOutput : public NativeOutput
    {
        public:
            RawOutput();
            virtual ~RawOutput();

            virtual bool beginOutput() override;
            virtual bool endOutput() override;
            virtual void updateBoundingBox() override;
            virtual void newPage(bool linefeed = false) override;
            virtual bool setupOutput() override;
            bool sendBuffer(const QByteArray &b, unsigned int len);

            static QString typeName()
            {
                return QObject::tr("Raw output");
            }

            static void setupRawPrinters(QComboBox *list);

        protected:
            QString rawPrinterName;
#if defined(Q_OS_WIN)
            HANDLE mJob;
#elif defined(Q_OS_MAC) || defined(Q_OS_LINUX)
            int mJobId;
            cups_dest_t *mDest;
            cups_dinfo_t *mInfo;
            http_t *mHttp;
#endif
    };
}
#endif // RAWOUTPUT_H
