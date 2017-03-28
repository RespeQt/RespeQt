#include "drivewidget.h"
#include "ui_drivewidget.h"
#include "diskimage.h"

DriveWidget::DriveWidget(int driveNum, QWidget *parent)
   : QFrame(parent)
   , ui(new Ui::DriveWidget)
   , driveNo_(driveNum)
{
    ui->setupUi(this);
}

DriveWidget::~DriveWidget()
{
    delete ui;
}


static void FormatStatusTip(QAction* action, QString& driveNum)
{
    const QString& tip = action->statusTip().arg(driveNum);
    action->setStatusTip(tip);
}

void DriveWidget::setup()
{
    QString driveTxt;
    if(driveNo_ < 9 ) {
        driveTxt = QString("%1").arg(driveNo_+1);
        ui->driveLabel->setText(driveTxt);
    } else {
        driveTxt = QString("%1").arg((char)((char)(driveNo_-9)+'J'));
    }

    ui->driveLabel->setText(QString("%1:").arg(driveTxt));

    // Fixup status tips (Note: not all of these have an %1 in the status tip
    FormatStatusTip(ui->actionBootOption, driveTxt);
    FormatStatusTip(ui->actionSave, driveTxt);
    FormatStatusTip(ui->actionAutoSave, driveTxt);
    FormatStatusTip(ui->actionSaveAs, driveTxt);
    FormatStatusTip(ui->actionRevert, driveTxt);
    FormatStatusTip(ui->actionMountDisk, driveTxt);
    FormatStatusTip(ui->actionMountFolder, driveTxt);
    FormatStatusTip(ui->actionEject, driveTxt);
    FormatStatusTip(ui->actionWriteProtect, driveTxt);
    FormatStatusTip(ui->actionEditDisk, driveTxt);

    // Add actions to context menu
    if(driveNo_ == 0) insertAction(0, ui->actionBootOption );
    insertAction(0, ui->actionSave);
    insertAction(0, ui->actionAutoSave);       //
    insertAction(0, ui->actionSaveAs);
    insertAction(0, ui->actionRevert);
    insertAction(0, ui->actionMountDisk);
    insertAction(0, ui->actionMountFolder);
    insertAction(0, ui->actionEject);
    insertAction(0, ui->actionWriteProtect);
    insertAction(0, ui->actionEditDisk);

    // Connect widget actions to buttons
    ui->buttonMountDisk->setDefaultAction(ui->actionMountDisk);
    ui->buttonMountFolder->setDefaultAction(ui->actionMountFolder);
    ui->buttonEject->setDefaultAction(ui->actionEject);
    ui->buttonSave->setDefaultAction(ui->actionSave);
    ui->autoSave->setDefaultAction(ui->actionAutoSave);
    ui->buttonEditDisk->setDefaultAction(ui->actionEditDisk);

}

void DriveWidget::updateFromImage(SimpleDiskImage *diskImage)
{
    if (diskImage == NULL)
    {
        showAsEmpty();
        return;
    }

    const QString &fileName = diskImage->originalFileName();
    ui->labelFileName->setText(fileName);
    ui->labelImageProperties->setText(diskImage->description());
    ui->actionEject->setEnabled(true);

    bool enableEdit = diskImage->editDialog() != NULL;
    ui->actionEditDisk->setChecked(enableEdit);


    // Update save/revert
    bool modified = diskImage->isModified();
    ui->actionSave->setEnabled(!modified);
    ui->actionRevert->setEnabled(!modified);

    // This was not set in default image mounted!
    ui->actionWriteProtect->setChecked(diskImage->isReadOnly());
    ui->actionWriteProtect->setEnabled(!diskImage->isUnmodifiable());
}


void DriveWidget::triggerAutoSaveClickIfEnabled()
{
    if(ui->autoSave->isEnabled()) ui->autoSave->click();
}


void DriveWidget::showAsEmpty()
{
    ui->actionSave->setEnabled(false);
    ui->labelFileName->clear();
    ui->labelImageProperties->clear();
    ui->actionEject->setEnabled(false);
    ui->actionRevert->setEnabled(false);
    ui->actionSaveAs->setEnabled(false);
    ui->actionEditDisk->setEnabled(false);
    ui->actionEditDisk->setChecked(false);
    ui->actionAutoSave->setEnabled(false);
    ui->actionAutoSave->setChecked(false);
    if(driveNo_ == 0)
        ui->actionBootOption->setEnabled(false);

    QString empty = "";
    setLabelToolTips(empty, empty, empty);
}

void DriveWidget::showAsFolderMounted(const QString &fileName, const QString &description, bool editEnabled)
{
    // code dupe
    ui->labelFileName->setText(fileName);
    ui->labelImageProperties->setText(description);
    ui->actionEject->setEnabled(true);
    ui->actionEditDisk->setChecked(editEnabled);

    ui->labelFileName->setStyleSheet("color: rgb(54, 168, 164); font-weight: bold");
    ui->actionEditDisk->setEnabled(true);

    ui->actionSaveAs->setEnabled(false);
    ui->actionSave->setEnabled(false);
    ui->actionAutoSave->setEnabled(false);
    ui->actionRevert->setEnabled(false);

    if(driveNo_ == 0) ui->actionBootOption->setEnabled(true);
}

void DriveWidget::showAsImageMounted(const QString &fileName, const QString &description, bool editEnabled, bool enableSave)
{
    ui->labelFileName->setText(fileName);
    ui->labelImageProperties->setText(description);
    ui->actionEject->setEnabled(true);
    ui->actionEditDisk->setChecked(editEnabled);


    ui->labelFileName->setStyleSheet("color: rgb(0, 0, 0); font-weight: normal");  //

    ui->actionEditDisk->setEnabled(true);

    ui->actionSaveAs->setEnabled(true);
    ui->actionSave->setEnabled(false);

    ui->actionAutoSave->setEnabled(false);
    ui->actionRevert->setEnabled(false);

    ui->actionAutoSave->setEnabled(true);

    if(driveNo_ == 0) ui->actionBootOption->setEnabled(false);

    ui->actionSave->setEnabled(enableSave);
    ui->actionRevert->setEnabled(enableSave);
}

bool DriveWidget::isAutoSaveEnabled()
{
    return ui->actionAutoSave->isChecked();
}


void DriveWidget::setLabelToolTips(const QString &one, const QString &two, const QString &three)
{
    ui->labelFileName->setToolTip(one);
    ui->labelFileName->setStatusTip(two);
    ui->labelImageProperties->setToolTip(three);
}


void DriveWidget::setFont(const QFont& font) {
    ui->labelFileName->setFont(font);
}

void DriveWidget::on_actionMountFolder_triggered()  { emit actionMountFolder(driveNo_); }
void DriveWidget::on_actionMountDisk_triggered()    { emit actionMountDisk(driveNo_); }
void DriveWidget::on_actionEject_triggered()        { emit actionEject(driveNo_); }
void DriveWidget::on_actionWriteProtect_toggled(bool state) { emit actionWriteProtect(driveNo_, state); }
void DriveWidget::on_actionEditDisk_triggered()     { emit actionEditDisk(driveNo_); }
void DriveWidget::on_actionSave_triggered()         { emit actionSave(driveNo_); }
void DriveWidget::on_actionRevert_triggered()       { emit actionRevert(driveNo_); }
void DriveWidget::on_actionSaveAs_triggered()       { emit actionSaveAs(driveNo_); }
void DriveWidget::on_actionAutoSave_toggled(bool state) { emit actionAutoSave(driveNo_, state); }
void DriveWidget::on_actionBootOption_triggered()   { emit actionBootOptions(driveNo_); }


