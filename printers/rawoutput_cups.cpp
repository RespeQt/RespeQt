#include "rawoutput.h"
#include "respeqtsettings.h"
#include <cups/cups.h>
#include <QComboBox>

namespace Printers {
    RawOutput::RawOutput()
        :NativeOutput()
    {}

    RawOutput::~RawOutput()
    {
        if (mDest != Q_NULLPTR)
            delete mDest;
    }

    void RawOutput::updateBoundingBox()
    {}

    void RawOutput::newPage(bool /*linefeed*/)
    {}

    bool RawOutput::beginOutput()
    {
        if (cupsCreateDestJob(mHttp, mDest, mInfo, &mJobId, "RespeQt", 0, Q_NULLPTR) == IPP_STATUS_OK)
        {
            // Job created, now start first and last document.
            bool return_ = cupsStartDestDocument(mHttp, mDest, mInfo, mJobId, "RespeQt", CUPS_FORMAT_TEXT, 0, Q_NULLPTR, 1) == HTTP_STATUS_CONTINUE;
            if (!return_)
                qDebug() << "!n" << cupsLastErrorString();
            return return_;
        } else {
            // Error
            qDebug() << "!n"<< cupsLastErrorString();
            return false;
        }
    }

    bool RawOutput::endOutput()
    {
        QByteArray temp = rawPrinterName.toLocal8Bit();
        cupsFinishDestDocument(mHttp, mDest, mInfo);

        cupsFreeDestInfo(mInfo);
        mDest = Q_NULLPTR;
#if defined(Q_OS_MAC)
        delete mDest;
#elif defined(Q_OS_LINUX)
        cupsFreeDests(1, mDest);
#endif
        mDest = Q_NULLPTR;
        httpClose(mHttp);

        return true;
    }

    bool RawOutput::sendBuffer(const QByteArray &b, unsigned int len)
    {
        // CUPS direct print
        bool return_ = cupsWriteRequestData(mHttp, b.data(), static_cast<size_t>(len)) != HTTP_STATUS_ERROR;
        if (!return_)
            qDebug()<<"!n "<< cupsLastErrorString();
        return return_;
    }

    typedef struct {
        int num_dests;
        cups_dest_t *dests;
    } my_user_data_t;

    int my_dest_cb(my_user_data_t *user_data, unsigned int flags, cups_dest_t *dest)
    {
        if (flags & CUPS_DEST_FLAGS_REMOVED)
        {
            user_data->num_dests = cupsRemoveDest(dest->name, dest->instance, user_data->num_dests, &(user_data->dests));
        } else {
            user_data->num_dests = cupsCopyDest(dest, user_data->num_dests, &(user_data->dests));
        }
        return 1;
    }

    bool RawOutput::setupOutput()
    {
        my_user_data_t user_data = { 0, Q_NULLPTR };
        cups_ptype_t type = CUPS_PRINTER_LOCAL, mask = CUPS_PRINTER_LOCAL;
        char *resource = new char[256];

        rawPrinterName = respeqtSettings->rawPrinterName();

        if (!cupsEnumDests(CUPS_DEST_FLAGS_NONE, 5000, Q_NULLPTR, type, mask,
            reinterpret_cast<cups_dest_cb_t>(my_dest_cb), &user_data))
        {
            cupsFreeDests(user_data.num_dests, user_data.dests);
            return false;
        }

        mDest = Q_NULLPTR;
        for(int i = 0; i < user_data.num_dests; i++)
        {
            cups_dest_t *dest = reinterpret_cast<cups_dest_t*>(&user_data.dests[i]);
            if (rawPrinterName == QString::fromLocal8Bit(dest->name))
            {
#if defined(Q_OS_MAC)
                mDest = new cups_dest_t();
                memcpy(mDest, dest, sizeof(*dest));
#elif defined(Q_OS_LINUX)
                cupsCopyDest(dest, 1, &mDest);
#endif
            }
        }
        if (mDest != Q_NULLPTR)
        {
            mHttp = cupsConnectDest(mDest, CUPS_DEST_FLAGS_NONE,
                                           30000, Q_NULLPTR, resource,
                                           sizeof(resource), Q_NULLPTR, Q_NULLPTR);
            mInfo = cupsCopyDestInfo(mHttp, mDest);
        }
        cupsFreeDests(user_data.num_dests, user_data.dests);

        return mDest != Q_NULLPTR && mHttp != Q_NULLPTR;
    }

    void RawOutput::setupRawPrinters(QComboBox *list)
    {
        my_user_data_t user_data = { 0, Q_NULLPTR };
        cups_ptype_t type = CUPS_PRINTER_LOCAL, mask = CUPS_PRINTER_LOCAL;

        list->clear();
        list->addItem(QObject::tr("Select raw printer"), QVariant(-1));

        // Get all the Cups printers.
        if (!cupsEnumDests(CUPS_DEST_FLAGS_NONE, 5000, Q_NULLPTR, type, mask,
            reinterpret_cast<cups_dest_cb_t>(my_dest_cb), &user_data))
        {
            cupsFreeDests(user_data.num_dests, user_data.dests);
            return;
        }

        for(int i = 0; i <  user_data.num_dests; i++)
        {
            cups_dest_t *dest = reinterpret_cast<cups_dest_t*>(&user_data.dests[i]);

            list->addItem(QString::fromLocal8Bit(dest->name), i);
        }
        cupsFreeDests(user_data.num_dests, user_data.dests);
    }
}
