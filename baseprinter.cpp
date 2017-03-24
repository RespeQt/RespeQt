#include "baseprinter.h"

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
    QChar result;

    result = QChar::fromLatin1(b);

    return result;
}
