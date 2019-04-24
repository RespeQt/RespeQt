#include "rawoutput.h"
#include "cups/cups.h"

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
        rawPrinterName = "HP_Color_LaserJet_MFP_M277dw";
        return true;
    }

    bool RawOutput::beginOutput()
    {
        QByteArray temp = rawPrinterName.toLocal8Bit();
        mJobId = cupsCreateJob(CUPS_HTTP_DEFAULT, temp.data(), "RespeQt", 0, Q_NULLPTR);
        if (mJobId > 0)
        {
            // Job created, now start first and last document.
            cupsStartDocument(CUPS_HTTP_DEFAULT, temp.data(), mJobId, "RespeQt", CUPS_FORMAT_RAW, 1);
        } else {
            // Error
            return false;
        }
        return true;
    }

    bool RawOutput::endOutput()
    {
        QByteArray temp = rawPrinterName.toLocal8Bit();
        cupsFinishDocument(CUPS_HTTP_DEFAULT, temp.data());

        return true;
    }

    bool RawOutput::sendBuffer(const QByteArray &b, unsigned int len)
    {
        // CUPS direct print
        cupsWriteRequestData(CUPS_HTTP_DEFAULT, b.data(), static_cast<size_t>(len));
        return false;

    }
}
