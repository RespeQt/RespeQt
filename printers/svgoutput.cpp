#include "svgoutput.h"
#include "mainwindow.h"
#include <cmath>
#include <QFileDialog>

namespace Printers {
    SVGOutput::SVGOutput()
        :NativeOutput()
    {
        mDevice = QSvgGeneratorPtr::create();
        mBoundingBox = QRectF(0, 0, 2000, 1000000);
    }

    SVGOutput::~SVGOutput()
    = default;

    void SVGOutput::updateBoundingBox()
    {
        QFontMetrics metrics(*mFont);
        mX = static_cast<int>(trunc(mBoundingBox.left()));
        mY = static_cast<int>(trunc(mBoundingBox.top() + metrics.lineSpacing()));
    }

    void SVGOutput::newPage(bool /*linefeed*/)
    {}

    bool SVGOutput::setupOutput()
    {
        QString fileName = QFileDialog::getSaveFileName(MainWindow::getInstance(), QObject::tr("Save SVG"), "", QObject::tr("SVG (*.svg)"));
        if (fileName.count() > 0)
        {
            setFileName(fileName);
            return true;
        }

        return false;
    }
}
