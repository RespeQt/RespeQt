#include "baseprinter.h"

BasePrinter::~BasePrinter()
{
    if (mTypeName != NULL)
    {
        delete mTypeName;
        mTypeName = NULL;
    }

}
