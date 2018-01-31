#include "bufferedpaintwidget.h"

#include <QPainter>
#include "logdisplaydialog.h"

namespace Printers
{
    BufferedPaintWidget::BufferedPaintWidget(QWidget *parent)
        : QWidget(parent)
    {
        mCanvas.setBoundingRect(geometry());
    }

    void BufferedPaintWidget::paintEvent(QPaintEvent *)
    {
        QPainter painter;
        painter.begin(this);
        painter.setViewport(mCanvas.boundingRect());
        painter.fillRect(mCanvas.boundingRect(), QColor("yellow"));
        painter.drawPicture(0, 0, mCanvas);
        qDebug()<<"!n Paint Event "<<mCanvas.boundingRect();
        painter.end();
    }

    void BufferedPaintWidget::resizeEvent(QResizeEvent *event)
    {
        mCanvas.setBoundingRect(QRect(QPoint(0, 0), event->size()));
        update();
    }

    void BufferedPaintWidget::updateCanvas(QPicture &newCanvas)
    {
        mCanvas = newCanvas;
        QRect r = newCanvas.boundingRect();
        resize(r.width(), r.height());
        QPainter painter;
        painter.begin(this);
        painter.setViewport(mCanvas.boundingRect());
        painter.fillRect(mCanvas.boundingRect(), QColor("yellow"));
        painter.drawPicture(0, 0, mCanvas);
        qDebug()<<"!n Paint Event "<<mCanvas.boundingRect();
        painter.end();
    }

}
