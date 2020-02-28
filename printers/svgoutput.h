#ifndef SVGOUTPUT_H
#define SVGOUTPUT_H

#include "nativeoutput.h"
#include <memory>
#include <QSvgGenerator>
#include <QSharedPointer>

using QSvgGeneratorPtr = QSharedPointer<QSvgGenerator>;

namespace Printers
{
    class SVGOutput : public NativeOutput
    {
    public:
        SVGOutput();
        virtual ~SVGOutput();
        QSvgGeneratorPtr svg() {
            return qSharedPointerDynamicCast<QSvgGenerator>(mDevice);
        }

        void setFileName(const QString &filename) { svg()->setFileName(filename); }
        virtual void updateBoundingBox();
        virtual void newPage(bool linefeed = false);

        virtual bool setupOutput();

        static QString typeName()
        {
            return "SVG";
        }

    };
}
#endif // SVGOUTPUT_H
