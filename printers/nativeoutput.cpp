
#include "nativeoutput.h"
#include "logdisplaydialog.h"

NativeOutput::NativeOutput():
    mPainter(NULL),
    mDevice(NULL),
    mFont(NULL),
    x(0), y(0)
{
}

NativeOutput::~NativeOutput()
{
    endOutput();
}

void NativeOutput::beginOutput() {
    mPainter = new QPainter();
    mPainter->begin(mDevice);
    setFont(mFont);
    updateBoundingBox();
}

void NativeOutput::updateBoundingBox()
{
    QFontMetrics metrics(*mFont);
    mBoundingBox = QRect(0, 0, mDevice->width(), mDevice->height());
    x = mBoundingBox.left();
    y = mBoundingBox.top() + metrics.lineSpacing();
}

void NativeOutput::endOutput() {
    if (mPainter)
    {
        mPainter->end();
    }
}

void NativeOutput::setFont(QFont *font)
{
    if (font != mFont)
    {
        delete mFont;
        mFont = font;
    }
    if (mFont && mPainter)
    {
        mPainter->setFont(*mFont);
    }
}

void NativeOutput::printChar(const QChar &c)
{
    QFontMetrics metrics(*mFont);
    if (metrics.width(c) + x > mBoundingBox.right()) { // Char has to go on next line
        newLine();
    }
    mPainter->drawText(x, y + metrics.height(), c);
    x += metrics.width(c);
}

void NativeOutput::printString(const QString &s)
{
    QString::const_iterator cit;
    for(cit = s.cbegin(); cit != s.cend(); ++cit)
    {
        printChar(*cit);
    }
}

void NativeOutput::drawLine(const QPointF &p1, const QPointF &p2)
{
    mPainter->drawLine(p1, p2);
}

void NativeOutput::setPen(const QColor &color)
{
    mPainter->setPen(color);
}

void NativeOutput::setPen(const QPen &pen)
{
    mPainter->setPen(pen);
}

void NativeOutput::setPen(Qt::PenStyle style)
{
    mPainter->setPen(style);
}

void NativeOutput::newLine()
{
    QFontMetrics metrics(*mFont);
    x = mBoundingBox.left();
    if (y + metrics.height() > mBoundingBox.bottom())
    {
        newPage();
        qDebug()<<"!n"<<"newPage";
        y = mBoundingBox.top();
    } else {
        y += metrics.lineSpacing();
    }
}

void NativeOutput::newPage()
{}

void NativeOutput::translate(const QPointF &offset)
{
    if (mPainter)
    {
        mPainter->translate(offset);
    }
}

void NativeOutput::setWindow(QRect const&window)
{
    if (mPainter)
    {
        mPainter->setWindow(window);
    }
}
