#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QWidget>

namespace Ui {
class InfoWidget;
}

class InfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InfoWidget(QWidget *parent = 0);
    ~InfoWidget();

private:
    Ui::InfoWidget *ui;
};

#endif // INFOWIDGET_H
