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
        virtual void paintEvent(QPaintEvent *event);
        virtual void newLine();
        virtual void newPage();

    private:
        Ui::WindowOutput *ui;
        QPicture *mPicture;
    };
}
#endif // WINDOWOUTPUT_H
