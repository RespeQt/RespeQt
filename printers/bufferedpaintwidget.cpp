#include "bufferedpaintwidget.h"

#include <QPainter>
#include "logdisplaydialog.h"

namespace Printers
{
    BufferedPaintWidget::BufferedPaintWidget(QWidget *parent)
        : QWidget(parent),
          mCanvas(NULL)
    {
        mCanvas = new QPicture();
        mCanvas->setBoundingRect(geometry());
    }

    BufferedPaintWidget::~BufferedPaintWidget()
    {
        delete mCanvas;
    }

    void BufferedPaintWidget::paintEvent(QPaintEvent *)
    {
        QPainter painter;
        painter.begin(this);
        painter.setViewport(mCanvas->boundingRect());
        painter.fillRect(mCanvas->boundingRect(), QColor("yellow"));
        painter.drawPicture(0, 0, *mCanvas);
        painter.end();
    }

    void BufferedPaintWidget::resizeEvent(QResizeEvent *event)
    {
        mCanvas->setBoundingRect(QRect(QPoint(0, 0), event->size()));
        update();
    }
}
