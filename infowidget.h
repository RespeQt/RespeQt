/*
 * infowidget.h
 *
 * Copyright 2017 blind
 *
 */
 
#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <QFrame>

namespace Ui {
class InfoWidget;
}

class InfoWidget : public QFrame
{
    Q_OBJECT

public:
    explicit InfoWidget(QWidget *parent = 0);
    ~InfoWidget();

private:
    Ui::InfoWidget *ui;
};

#endif // INFOWIDGET_H
