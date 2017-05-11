#ifndef BUFFEREDPAINTWIDGET_H
#define BUFFEREDPAINTWIDGET_H

#include <QWidget>
#include <QPicture>
#include <QResizeEvent>
#include <QPaintEvent>

namespace Printers
{
    class BufferedPaintWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit BufferedPaintWidget(QWidget *parent = 0);
        virtual ~BufferedPaintWidget();
        virtual void paintEvent(QPaintEvent *event);
        virtual void resizeEvent(QResizeEvent *event);
        QPicture *canvas() { return mCanvas; }

    private:
        QPicture *mCanvas;

    };
}
#endif // BUFFEREDPAINTWIDGET_H
