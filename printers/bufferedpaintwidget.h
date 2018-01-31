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
        virtual void paintEvent(QPaintEvent *event);
        virtual void resizeEvent(QResizeEvent *event);
        QPicture &canvas() { return mCanvas; }

    public slots:
        void updateCanvas(QPicture &newCanvas);

    private:
        QPicture mCanvas;

    };
}
#endif // BUFFEREDPAINTWIDGET_H
