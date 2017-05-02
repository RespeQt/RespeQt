#ifndef SVGOUTPUT_H
#define SVGOUTPUT_H

#include "nativeoutput.h"

#include <QSvgGenerator>

namespace Printers
{
    class SVGOutput : public NativeOutput
    {
    public:
        SVGOutput();
        virtual ~SVGOutput();
        QSvgGenerator *svg() { return dynamic_cast<QSvgGenerator*>(mDevice); }

        void setFileName(const QString &filename) { svg()->setFileName(filename); }
        virtual void updateBoundingBox();

    };
}
#endif // SVGOUTPUT_H
