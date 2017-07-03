#ifndef WINDOWOUTPUT_H
#define WINDOWOUTPUT_H

#include "nativeoutput.h"

#include <QFrame>
#include <QPicture>

namespace Ui {
class WindowOutput;
}

namespace Printers {

    class WindowOutput : public QFrame, public NativeOutput
    {
        Q_OBJECT

    public:
        explicit WindowOutput(QWidget *parent = 0);
        ~WindowOutput();
        virtual void newLine(bool linefeed = false);
        virtual void newPage(bool linefeed = false);

    protected:
        virtual void updateBoundingBox();

    private:
        Ui::WindowOutput *ui;
    };
}
#endif // WINDOWOUTPUT_H
