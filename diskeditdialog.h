#ifndef DISKEDITDIALOG_H
#define DISKEDITDIALOG_H

#include <QtWidgets/QMainWindow>

namespace Ui {
    class DiskEditDialog;
}

#include <QComboBox>
#include <QAbstractTableModel>
#include <QMimeData>
#include <QItemSelection>

#include "atarifilesystem.h"

class MyModel: public QAbstractTableModel
{
    Q_OBJECT

public:
    MyModel(QObject *parent);
    ~MyModel();
    QList <AtariDirEntry> entries;
    AtariFileSystem *fileSystem;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data (const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    void deleteFiles(QModelIndexList indexes);
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount (const QModelIndex & parent = QModelIndex()) const;
    int columnCount (const QModelIndex & parent = QModelIndex()) const;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    void setDirectory(int row);
    void toParent();
    void setRoot();
    void setFileSystem(AtariFileSystem *aFileSystem);
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    QStringList mimeTypes() const;
    void insertFiles(QStringList names);
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    inline bool isRoot() const {return dirs.count() == 1;}
    inline QString currentPath() const {return m_currentPath;}
private:
    QString m_currentPath;
    QStringList paths;
    QStringList *tempDirs;

    QList <quint16> dirs;
};

class DiskEditDialog : public QMainWindow {
    Q_OBJECT

public:
    DiskEditDialog(QWidget *parent = 0);
    ~DiskEditDialog();
    void go(SimpleDiskImage *image, int fileSystem = -1);

protected:
    MyModel *model;
    void changeEvent(QEvent *e);

private:
    Ui::DiskEditDialog *m_ui;
    SimpleDiskImage *m_disk;
    QComboBox *m_fileSystemBox;

private slots:
    void on_actionAddFiles_triggered();
    void on_actionDeleteSelectedFiles_triggered();
    void on_actionTextConversion_triggered();
    void on_actionExtractFiles_triggered();
    void on_actionToParent_triggered();
    void on_actionPrint_triggered();        // 
    void on_aView_doubleClicked(QModelIndex index);
    void onTopChanged();

    void fileSystemChanged(int index);
    void currentChanged (const QModelIndex & current, const QModelIndex & previous);
    void selectionChanged (const QItemSelection & selected, const QItemSelection & deselected);
};

#endif // DISKEDITDIALOG_H
