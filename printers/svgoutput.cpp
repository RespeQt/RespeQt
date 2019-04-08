#include "svgoutput.h"
#include <math.h>

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
        mX = static_cast<int>(trunc(mBoundingBox.left()));
        mY = static_cast<int>(trunc(mBoundingBox.top() + metrics.lineSpacing()));
    }

    void SVGOutput::newPage(bool /*linefeed*/)
    {}
}
