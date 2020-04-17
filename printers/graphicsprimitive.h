#ifndef GRAPHICSPRIMITIVE_H
#define GRAPHICSPRIMITIVE_H

#include <QFont>
#include <QGraphicsTextItem>
#include <QGraphicsScene>

using QPainterPtr = QSharedPointer<QPainter>;

namespace Printers
{
    class GraphicsPrimitive : public QObject
    {
        Q_OBJECT

    public:
        GraphicsPrimitive();
        virtual ~GraphicsPrimitive();

        virtual void execute(QGraphicsScene &graphicsScene) = 0;
        virtual void execute(QPainterPtr painter) = 0;

        static QString typeName()
        {
            return "GraphicsPrimitive";
        }

    protected:
        QPoint mPoint;
        int mColor;
    };

    class GraphicsClearPane : public GraphicsPrimitive
    {
        Q_OBJECT

    public:
        GraphicsClearPane();
        virtual ~GraphicsClearPane();

        virtual void execute(QGraphicsScene &graphicsScene) override;
        virtual void execute(QPainterPtr painter) override;

        static QString typeName()
        {
            return "GraphicsClearPane";
        }
    };

    class GraphicsSetPoint : public GraphicsPrimitive
    {
        Q_OBJECT

    public:
        GraphicsSetPoint(const QPoint point, const QPen);
        virtual ~GraphicsSetPoint();

        virtual void execute(QGraphicsScene &graphicsScene) = 0;
        virtual void execute(QPainterPtr painter) = 0;

        static QString typeName()
        {
            return "GraphicsSetPoint";
        }

    protected:
        QPoint mPoint;
        QPen mPen;
    };

    class GraphicsDrawLine : public GraphicsSetPoint
    {
        Q_OBJECT

    public:
        GraphicsDrawLine(const QPoint srcPoint, const QPen pen, const QPoint destPoint);
        virtual ~GraphicsDrawLine();

        virtual void execute(QGraphicsScene &graphicsScene) override;
        virtual void execute(QPainterPtr painter) override;

        static QString typeName()
        {
            return "GraphicsDrawLine";
        }

    protected:
        QPoint mDestPoint;
    };

    class GraphicsDrawText : public GraphicsSetPoint
    {
        Q_OBJECT

    public:
        GraphicsDrawText(const QPoint point, const QPen pen, const int orientation, const QFont font, QString text);
        virtual ~GraphicsDrawText();

        virtual void execute(QGraphicsScene &graphicsScene) override;
        virtual void execute(QPainterPtr painter) override;

        static QString typeName()
        {
            return "GraphicsDrawText";
        }

    protected:
        int mOrientation;
        QFont mFont;
        QString mText;

        QPoint computeTextCoordinates();
    };
}
#endif // GRAPHICSPRIMITIVE_H
