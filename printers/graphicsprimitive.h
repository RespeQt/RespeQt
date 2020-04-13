#ifndef GRAPHICSPRIMITIVE_H
#define GRAPHICSPRIMITIVE_H

#include <QString>
#include <QGraphicsScene>

namespace Printers
{
    class GraphicsPrimitive : public QObject
    {
        Q_OBJECT

    public:
        GraphicsPrimitive(const int srcX, const int srcY, const int color);
        virtual ~GraphicsPrimitive();

        virtual void execute(QGraphicsScene &graphicsScene) = 0;

        static QString typeName()
        {
            return "GraphicsPrimitive";
        }

    protected:
        int mSrcX, mSrcY;
        int mColor;
    };

    class GraphicsDrawLine : public GraphicsPrimitive
    {
        Q_OBJECT

    public:
        GraphicsDrawLine(const int srcX, const int srcY, const int color, const int destX, const int destY, const int lineType);
        virtual ~GraphicsDrawLine();

        virtual void execute(QGraphicsScene &graphicsScene) override;

        static QString typeName()
        {
            return "GraphicsDrawLine";
        }

    protected:
        int mDestX, mDestY;
        int mLineType;
    };

    class GraphicsDrawText : public GraphicsPrimitive
    {
        Q_OBJECT

    public:
        GraphicsDrawText(const int srcX, const int srcY, const int color, const int orientation, const int scale, QString text);
        virtual ~GraphicsDrawText();

        virtual void execute(QGraphicsScene &graphicsScene) override;

        static QString typeName()
        {
            return "GraphicsDrawText";
        }

    protected:
        int mOrientation;
        int mScale;
        QString mText;
    };

    class GraphicsClearPane : public GraphicsPrimitive
    {
        Q_OBJECT

    public:
        GraphicsClearPane();
        virtual ~GraphicsClearPane();

        virtual void execute(QGraphicsScene &graphicsScene) override;

        static QString typeName()
        {
            return "GraphicsClearPane";
        }
    };
}
#endif // GRAPHICSPRIMITIVE_H
