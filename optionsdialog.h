/*
 * optionsdialog.h
 *
 * Copyright 2016 TheMontezuma
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtWidgets/QDialog>
#include <QFileDialog>
#include <QTreeWidget>
#include <QtDebug>

#include "serialport.h"

namespace Ui {
    class OptionsDialog;
}

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    OptionsDialog(QWidget *parent = 0);
    ~OptionsDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::OptionsDialog *m_ui;
    QTreeWidgetItem *itemTestSerialPort, *itemPassthrough, *itemAtari1027,
        *itemStandard, *itemAtariSio, *itemEmulation, *itemDiskOptions, *itemDiskOSB, *itemDiskIcons, *itemDiskFavorite, *itemI18n,
        *itemFirmware810Path, *itemFirmware1050Path, *itemFirmwareEmulation, *itemTraceOptions;

    void selectFirmware(QLineEdit *edit, QString title, QString filters);

private slots:
    void on_serialPortComboBox_currentIndexChanged(int index);
    void on_serialPortHandshakeCombo_currentIndexChanged(int index);
    void on_serialPortUseDivisorsBox_toggled(bool checked);
    void on_treeWidget_itemClicked(QTreeWidgetItem* item, int column);
    void on_treeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void OptionsDialog_accepted();
    void on_useEmulationCustomCasBaudBox_toggled(bool checked);
    void on_testFileButton_clicked();
    void on_actionSelect810Firmware_triggered();
    void on_actionSelect810ChipFirmware_triggered();
    void on_actionSelect810HappyFirmware_triggered();
    void on_actionSelect1050Firmware_triggered();
    void on_actionSelect1050ArchiverFirmware_triggered();
    void on_actionSelect1050HappyFirmware_triggered();
    void on_actionSelect1050SpeedyFirmware_triggered();
    void on_actionSelect1050TurboFirmware_triggered();
    void on_actionSelect1050DuplicatorFirmware_triggered();
    void on_actionSelectTranslatorDisk_triggered();
    void on_actionSelectToolDisk_triggered();
    void on_button_atarifixed_clicked();
    void on_buttonRclFolder_clicked();
};

#endif // OPTIONSDIALOG_H
