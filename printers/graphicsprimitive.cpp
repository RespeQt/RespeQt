#include "logdisplaydialog.h"
#include "graphicsprimitive.h"

namespace Printers
{
    GraphicsPrimitive::GraphicsPrimitive(const int srcX, const int srcY, const int color):
        mSrcX(srcX),
        mSrcY(srcY),
        mColor(color)
    {
    }

    GraphicsPrimitive::~GraphicsPrimitive() = default;

    GraphicsDrawLine::GraphicsDrawLine(const int srcX, const int srcY, const int color, const int destX, const int destY, const int lineType)
        :GraphicsPrimitive(srcX, srcY, color),
          mDestX(destX),
          mDestY(destY),
          mLineType(lineType)
    {
    }

    GraphicsDrawLine::~GraphicsDrawLine() = default;

    void GraphicsDrawLine::execute(QGraphicsScene &graphicsScene)
    {
        QColor color(0, 0, 0);
        switch(mColor & 0x03) {
        case 0:
            break;
        case 1:
            color = QColor(0, 0, 255);
            break;
        case 2:
            color = QColor(0, 255, 0);
            break;
        case 3:
            color = QColor(255, 0, 0);
            break;
        }
        QPen pen(color);
        if (mLineType == 0) {
            pen.setStyle(Qt::SolidLine);
        } else {
            pen.setStyle(Qt::CustomDashLine);
            QVector<qreal> pattern;
            pattern << 1 << (mLineType & 0x0F);
            pen.setDashPattern(pattern);
        }
        pen.setWidth(1);
        graphicsScene.addLine(mSrcX, -mSrcY, mDestX, -mDestY, pen);
    }

    GraphicsDrawText::GraphicsDrawText(const int srcX, const int srcY, const int color, const int orientation, const int scale, QString text)
        :GraphicsPrimitive(srcX, srcY, color),
          mOrientation(orientation),
          mScale(scale),
          mText(text)
    {
    }

    GraphicsDrawText::~GraphicsDrawText() = default;

    void GraphicsDrawText::execute(QGraphicsScene &graphicsScene)
    {
        qDebug() << "!n" << tr("*** Drawing text at (%1,%2): %3")
                    .arg(mSrcX)
                    .arg(mSrcY)
                    .arg(mText);
        graphicsScene.addSimpleText(mText);
    }

    GraphicsClearPane::GraphicsClearPane()
        :GraphicsPrimitive(0, 0, 0)
    {
    }

    GraphicsClearPane::~GraphicsClearPane() = default;

    void GraphicsClearPane::execute(QGraphicsScene &graphicsScene)
    {
        graphicsScene.clear();
    }
}
