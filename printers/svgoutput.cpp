#include "svgoutput.h"

namespace Printers {
    SVGOutput::SVGOutput()
        :NativeOutput()
    {
        mDevice = new QSvgGenerator();
        mBoundingBox = QRectF(0, 0, 2000, 1000000);
    }

    SVGOutput::~SVGOutput()
    {}

    void SVGOutput::updateBoundingBox()
    {
        QFontMetrics metrics(*mFont);
        mX = mBoundingBox.left();
        mY = mBoundingBox.top() + metrics.lineSpacing();
    }

    void SVGOutput::newPage(bool linefeed)
    {}

}
