#include "rawoutput.h"
#include "respeqtsettings.h"
#include <winspool.h>
#include <QComboBox>

namespace Printers {

    RawOutput::RawOutput()
        :NativeOutput()
    {}

    RawOutput::~RawOutput()
    {}

    void RawOutput::updateBoundingBox()
    {}

    void RawOutput::newPage(bool /*linefeed*/)
    {}

    bool RawOutput::setupOutput()
    {
        rawPrinterName = respeqtSettings->rawPrinterName();
        return true;
    }

    bool RawOutput::beginOutput()
    {
        wchar_t *temp = new wchar_t[256];
        wchar_t *temp2 = new wchar_t[256];
        DOC_INFO_1 di1;
        DWORD needed;
        QString type, docname;

        rawPrinterName.toWCharArray(temp);
        temp[rawPrinterName.length()] = 0;

        OpenPrinter(temp, &mJob, nullptr);

        // To get the driver version, we need to get the printer driver info.
        // First call is the call to get the needed bytes, second for the actual info.
        GetPrinterDriver(mJob, nullptr, 2, nullptr, 0, &needed);
        std::vector<char> buffer(needed);
        GetPrinterDriver(mJob, nullptr, 2, reinterpret_cast<LPBYTE>(&buffer[0]), needed, &needed);
        needed = reinterpret_cast<DRIVER_INFO_2*>(&buffer[0])->cVersion;

        type = needed >= 4 ? "XPS_PASS" : "RAW";
        docname = "RespeQt";
        type.toWCharArray(temp);
        temp[type.length()] = 0;
        docname.toWCharArray(temp2);
        temp2[docname.length()] = 0;

        di1.pDatatype = temp;
        di1.pDocName = temp2;
        di1.pOutputFile = nullptr;

        StartDocPrinter(mJob, 1, reinterpret_cast<LPBYTE>(&di1));
        StartPagePrinter(mJob);

        delete[] temp;
        delete[] temp2;
        return true;
    }

    bool RawOutput::endOutput()
    {       
        EndPagePrinter(mJob);
        EndDocPrinter(mJob);
        ClosePrinter(mJob);
        mJob = nullptr;
        return true;
    }

    bool RawOutput::sendBuffer(const QByteArray &b, unsigned int len)
    {
        DWORD dwWritten;
        WritePrinter(mJob, reinterpret_cast<LPVOID>(const_cast<char*>(b.data())), len, &dwWritten);
        return true;
    }

    void RawOutput::setupRawPrinters(QComboBox *list)
    {
        PRINTER_INFO_1 *pinfo;
        DWORD needed, returned;

        list->clear();
        list->addItem(QObject::tr("Select raw printer"), QVariant(-1));

        EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 1, nullptr, 0, &needed, &returned);
        if (needed == 0)
            return;

        std::vector<char> buffer(needed);
        if (!EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 1,
                     reinterpret_cast<LPBYTE>(&buffer[0]), buffer.size(), &needed, &returned))
        {
            return;
        }
        pinfo = reinterpret_cast<PRINTER_INFO_1*>(&buffer[0]);
        for(DWORD i = 0; i < returned; i++)
        {
            list->addItem(QString::fromWCharArray(pinfo[i].pName), QVariant(static_cast<uint>(i)));
        }
    }
}
