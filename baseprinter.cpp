#include "baseprinter.h"
#include "textprinter.h"
#include "atari1027.h"

BasePrinter::~BasePrinter()
{
    if (mTypeName != NULL)
    {
        delete mTypeName;
        mTypeName = NULL;
    }
}

const QChar &BasePrinter::translateAtascii(const char b)
{
    return mAtascii(b);
}

BasePrinter *BasePrinter::createPrinter(int type, SioWorker *worker)
{
    switch (type)
    {
        case TEXTPRINTER:
            return new TextPrinter(worker);
        case ATARI1027:
            return new Atari1027(worker);
        default:
            throw new std::invalid_argument("Unknown printer type");
    }
    return NULL;
}
