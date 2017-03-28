#include "baseprinter.h"
#include "textprinter.h"
#include "atari1027.h"

BasePrinter::BasePrinter(SioWorker *worker) : SioDevice(worker),
  mPainter(NULL),
  mFontMetrics(NULL)
{}

BasePrinter::~BasePrinter()
{
    if (mTypeName != NULL)
    {
        delete mTypeName;
        mTypeName = NULL;
    }
    if (requiresNativePrinter())
    {
        endPrint();
    }
}

const QChar &BasePrinter::translateAtascii(const char b)
{
    return mAtascii(b);
}

void BasePrinter::setupPrinter() {
    mNativePrinter = new QPrinter();
    mPainter = new QPainter();
    mPainter -> setFont(mFont);
    mBoundingBox = mNativePrinter -> paperRect();
    x = mBoundingBox.left();
    y = mBoundingBox.top() + mFontMetrics->lineSpacing();
}

void BasePrinter::beginPrint() {
    mPainter->begin(mNativePrinter);
    mPainter->setFont(mFont);
}

void BasePrinter::endPrint() {
    if (mPainter->isActive())
    {
        mPainter->end();
    }
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
