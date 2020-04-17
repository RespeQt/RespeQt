#include "logdisplaydialog.h"
#include "atariprinter.h"
#include "graphicsprimitive.h"

namespace Printers
{
    GraphicsPrimitive::GraphicsPrimitive()
    {
    }

    GraphicsPrimitive::~GraphicsPrimitive() = default;

    GraphicsClearPane::GraphicsClearPane()
        :GraphicsPrimitive()
    {
    }

    GraphicsClearPane::~GraphicsClearPane() = default;

    void GraphicsClearPane::execute(QGraphicsScene &graphicsScene)
    {
        graphicsScene.clear();
    }

    void GraphicsClearPane::execute(QPainterPtr /*painter*/)
    {
    }

    GraphicsSetPoint::GraphicsSetPoint(const QPoint point, const QPen pen):
        mPoint(point),
        mPen(pen)
    {
    }

    GraphicsSetPoint::~GraphicsSetPoint() = default;

    GraphicsDrawLine::GraphicsDrawLine(const QPoint srcPoint, const QPen pen, const QPoint destPoint)
        :GraphicsSetPoint(srcPoint, pen),
          mDestPoint(destPoint)
    {
    }

    GraphicsDrawLine::~GraphicsDrawLine() = default;

    void GraphicsDrawLine::execute(QGraphicsScene &graphicsScene)
    {
        graphicsScene.addLine(mPoint.x(), -mPoint.y(), mDestPoint.x(), -mDestPoint.y(), mPen);
    }

    void GraphicsDrawLine::execute(QPainterPtr painter)
    {
        painter->setPen(mPen);
        painter->drawLine(mPoint.x(), -mPoint.y(), mDestPoint.x(), -mDestPoint.y());
    }

    GraphicsDrawText::GraphicsDrawText(const QPoint point, const QPen pen, const int orientation, const QFont font, QString text)
        :GraphicsSetPoint(point, pen),
          mOrientation(orientation),
          mFont(font),
          mText(text)
    {
    }

    GraphicsDrawText::~GraphicsDrawText() = default;

    void GraphicsDrawText::execute(QGraphicsScene &graphicsScene)
    {
        QGraphicsTextItem *text = graphicsScene.addText(mText, mFont);
        text->setDefaultTextColor(mPen.color());
        text->setRotation(mOrientation);
        QPoint adjusted = computeTextCoordinates();
        text->setPos(adjusted.x(), adjusted.y());
    }

    void GraphicsDrawText::execute(QPainterPtr painter)
    {
        QFont previous = painter->font();
        painter->setFont(mFont);
        painter->setPen(mPen);
        painter->rotate((qreal) mOrientation);
        QPoint adjusted = computeTextCoordinates();
        painter->drawText(adjusted.x(), adjusted.y(), mText);
        painter->rotate(0);
        painter->setFont(previous);
    }

    QPoint GraphicsDrawText::computeTextCoordinates()
    {
        // fix position because (0,0) on Windows/Linux is top left but on the 1020 it is bottom left.
        QPoint adjusted(mPoint.x(), -mPoint.y());
        QFontMetrics metrics(mFont);
        switch (mOrientation) {

        case 0:
            adjusted.setY(adjusted.y() - metrics.height());
            break;

        case 90:
            adjusted.setX(adjusted.x() + metrics.height());
            break;

        case 180:
            adjusted.setY(adjusted.y() + metrics.height());
            break;

        case 270:
            adjusted.setY(adjusted.x() - metrics.height());
            break;
        }

        return adjusted;
    }
}
