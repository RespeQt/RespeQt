/*
 * infowidget.cpp
 *
 * Copyright 2017 blind
 *
 */
 
#include "infowidget.h"
#include "ui_infowidget.h"

InfoWidget::InfoWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::InfoWidget)
{
    ui->setupUi(this);
}

InfoWidget::~InfoWidget()
{
    delete ui;
}
