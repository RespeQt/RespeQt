#include "diskeditdialog.h"
#include "ui_diskeditdialog.h"

#include <QUrl>
#include <QFileDialog>
#include <QMessageBox>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QTextEdit>
#include <QPrinter>
#include <QDebug>

#include "aspeqtsettings.h"
#include "miscutils.h"

/* MyModel */

MyModel::MyModel(QObject *parent)
    :QAbstractTableModel(parent)
{
    fileSystem = 0;
    tempDirs = new QStringList();
}

MyModel::~MyModel()
{
    while (!tempDirs->isEmpty()) {
        deltree(QDir::temp().absoluteFilePath(tempDirs->last()));
        tempDirs->removeLast();
    }
    delete tempDirs;
    if (fileSystem) {
        delete fileSystem;
    }
}

Qt::ItemFlags MyModel::flags(const QModelIndex &index) const
{
    if (index.column() == 1 || index.column() == 2) {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
    } else if (index.column() == 4) {
        if (fileSystem->fileSystemCode() == 5) {
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
        } else {
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        }
    } else {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
}

QVariant MyModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= rowCount()) {
        return QVariant();
    }
    if (index.column() == 1 && role == Qt::DecorationRole) {
        if ((entries.at(index.row()).attributes & AtariDirEntry::Directory) != 0) {
            return QPixmap(":/icons/silk-icons/icons/folder.png");
        } else {
            return QPixmap(":/icons/silk-icons/icons/page_white.png");
        }
    }

    if (role == Qt::TextAlignmentRole && (index.column() == 3 || index.column() == 0)) {
        return (int)(Qt::AlignRight | Qt::AlignVCenter);
    }

    QDateTime time = entries.at(index.row()).dateTime;

    if (index.column() == 4 && !time.isValid()) {
        if (role == Qt::TextAlignmentRole) {
            return (int)(Qt::AlignCenter);
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        QVariant x;
        return x;
    }

    switch (index.column()) {
    case 0:
        return entries.at(index.row()).no;
        break;
    case 1:
        return entries.at(index.row()).baseName();
        break;
    case 2:
        return entries.at(index.row()).suffix();
        break;
    case 3:
        if (entries.at(index.row()).size >= 0) {
            return entries.at(index.row()).size;
        } else {
            return QVariant();
        }
        break;
    case 4:
        if (time.isValid()) {
            return time;
        }
        if (role == Qt::DisplayRole) {
            return QString("n/a");
        }
        return QVariant();
        break;
    case 5:
        return entries.at(index.row()).attributeNames();
        break;
    default:
        return QVariant();
        break;
    }
    return QVariant();
}

bool MyModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }
    switch (index.column()) {
    case 1: {
        QString s = value.toString();
        if (s.isEmpty()) {
            return false;
        }
        s = s.toUpper();
        if (s.count() > 8) {
            s.resize(8);
        }
        foreach (QChar c, s) {
            if ((c > 0x7f) || (!c.isLetterOrNumber() && c != '_')) {
                return false;
            }
        }
        while (s.count() < 8) {
            s.append(' ');
        }
        s.append(entries.at(index.row()).suffix());
        while (s.count() < 11) {
            s.append(' ');
        }
        if (s.toLatin1() == entries.at(index.row()).atariName) {
            return false;
        }
        if (fileSystem->rename(entries.at(index.row()), s.toLatin1())) {
            AtariDirEntry entry = entries.at(index.row());
            entry.atariName = s.toLatin1();
            entries[index.row()] = entry;
            emit dataChanged(index, index);
            return true;
        } else {
            return false;
        }
        break;
    }
    case 2: {
        QString s = value.toString();
        if (s.isEmpty()) {
            return false;
        }
        s = s.toUpper();
        if (s.count() > 3) {
            s.resize(3);
        }
        foreach (QChar c, s) {
            if ((c > 0x7f) || (!c.isLetterOrNumber() && c != '_')) {
                return false;
            }
        }
        while (s.count() < 3) {
            s.append(' ');
        }
        QString b = entries.at(index.row()).baseName();
        while (b.count() < 8) {
            b.append(' ');
        }
        s = b + s;
        if (s.toLatin1() == entries.at(index.row()).atariName) {
            return false;
        }
        if (fileSystem->rename(entries.at(index.row()), s.toLatin1())) {
            AtariDirEntry entry = entries.at(index.row());
            entry.atariName = s.toLatin1();
            entries[index.row()] = entry;
            emit dataChanged(index, index);
            return true;
        } else {
            return false;
        }
        break;
    }
    case 4:
        return false;
        break;
    }
    return false;
}

void MyModel::deleteFiles(QModelIndexList indexes)
{
    QList <int> l;

    QList <AtariDirEntry> selectedEntries;
    foreach (QModelIndex i, indexes) {
        if (i.isValid() && i.column() == 0) {
            selectedEntries.append(entries.at(i.row()));
            l.append(i.row());
        }
    }

    fileSystem->deleteRecursive(selectedEntries);

    emit layoutAboutToBeChanged();
    while (!l.isEmpty()) {
        int i = l.last();
        entries.removeAt(i);
        l.removeLast();
    }

    emit layoutChanged();
}

QVariant MyModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) {
        return QVariant();
    }
    switch (section) {
    case 0:
        return tr("No");
        break;
    case 1:
        return tr("Name");
        break;
    case 2:
        return tr("Extension");
        break;
    case 3:
        return tr("Size");
        break;
    case 4:
        return tr("Time");
        break;
    case 5:
        return tr("Notes");
        break;
    default:
        return QVariant();
        break;
    }
}

int MyModel::rowCount (const QModelIndex & /*parent*/) const
{
    return entries.count();
}

int MyModel::columnCount (const QModelIndex & /*parent*/) const
{
    return 6;
}

void MyModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column > 5) {
        return;
    }
    emit layoutAboutToBeChanged();
    switch (column) {
        case 0:
            if (order == Qt::AscendingOrder) {
                qStableSort(entries.begin(), entries.end(), atariDirEntryNoLessThan);
            } else {
                qStableSort(entries.begin(), entries.end(), atariDirEntryNoGreaterThan);
            }
            break;
        case 1:
            if (order == Qt::AscendingOrder) {
                qStableSort(entries.begin(), entries.end(), atariDirEntryNameLessThan);
            } else {
                qStableSort(entries.begin(), entries.end(), atariDirEntryNameGreaterThan);
            }
            break;
        case 2:
            if (order == Qt::AscendingOrder) {
                qStableSort(entries.begin(), entries.end(), atariDirEntryExtensionLessThan);
            } else {
                qStableSort(entries.begin(), entries.end(), atariDirEntryExtensionGreaterThan);
            }
            break;
        case 3:
            if (order == Qt::AscendingOrder) {
                qStableSort(entries.begin(), entries.end(), atariDirEntrySizeLessThan);
            } else {
                qStableSort(entries.begin(), entries.end(), atariDirEntrySizeGreaterThan);
            }
            break;
        case 4:
            if (order == Qt::AscendingOrder) {
                qStableSort(entries.begin(), entries.end(), atariDirEntryDateLessThan);
            } else {
                qStableSort(entries.begin(), entries.end(), atariDirEntryDateGreaterThan);
            }
            break;
        case 5:
            if (order == Qt::AscendingOrder) {
                qStableSort(entries.begin(), entries.end(), atariDirEntryNotesLessThan);
            } else {
                qStableSort(entries.begin(), entries.end(), atariDirEntryNotesGreaterThan);
            }
            break;
    }
    emit layoutChanged();
}

void MyModel::setFileSystem(AtariFileSystem *aFileSystem)
{
    emit layoutAboutToBeChanged();
    if (fileSystem) {
        delete fileSystem;
    }
    fileSystem = aFileSystem;
    if (fileSystem) {
        setRoot();
    } else {
        entries.clear();
    }
    emit layoutChanged();
}

void MyModel::setDirectory(int row)
{
    emit layoutAboutToBeChanged();
    paths.append(entries.at(row).name());
    dirs.append(entries.at(row).firstSector);
    m_currentPath.append(entries.at(row).name() + ">");
    entries = fileSystem->getEntries(entries.at(row).firstSector);
    emit layoutChanged();
}

void MyModel::toParent()
{
    paths.removeLast();
    dirs.removeLast();
    m_currentPath = QString("D%1:").arg(fileSystem->image()->deviceNo() - 0x30);
    foreach (QString s, paths) {
        m_currentPath.append(s + ">");
    }
    emit layoutAboutToBeChanged();
    entries = fileSystem->getEntries(dirs.last());
    emit layoutChanged();
}

void MyModel::setRoot()
{
    emit layoutAboutToBeChanged();
    paths.clear();
    dirs.clear();
    int dir = fileSystem->rootDir();
    entries = fileSystem->getEntries(dir);
    dirs.append(dir);
    m_currentPath = QString("D%1:").arg(fileSystem->image()->deviceNo() - 0x30);
    emit layoutChanged();
}

void MyModel::insertFiles(QStringList names)
{
    if (names.isEmpty()) {
        return;
    }

    int dir = dirs.last();

    emit layoutAboutToBeChanged();
    entries.append(fileSystem->insertRecursive(dir, names));
    emit layoutChanged();
}

bool MyModel::dropMimeData(const QMimeData *data, Qt::DropAction /*action*/, int /*row*/, int /*column*/, const QModelIndex &/*parent*/)
{
    QStringList names;
    if (data->hasUrls()) {
        foreach (QUrl url, data->urls()) {
            QString name = url.toLocalFile();
            if (!name.isEmpty()) {
                names.append(name);
            }
        }
        insertFiles(names);
        return true;
    } else {
        return false;
    }
}

QStringList MyModel::mimeTypes() const
{
    QStringList types;
    types.append("text/uri-list");
    return types;
}

QMimeData* MyModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *data = new QMimeData();
    QList <AtariDirEntry> selectedEntries;
    QList <QUrl> urls;

    while (!tempDirs->isEmpty()) {
        deltree(QDir::temp().absoluteFilePath(tempDirs->last()));
        QDir::temp().rmdir(tempDirs->last());
        tempDirs->removeLast();
    }

    QTemporaryFile temp;
    temp.setFileTemplate(QDir::temp().absoluteFilePath("aspeqt-dir-XXXXXX"));
    temp.open();
    QString tempPath = temp.fileName() + "v";
    temp.close();
    QFileInfo info(tempPath);
    QDir dir = QDir::temp();
    if (!dir.mkdir(info.fileName())) {
        return data;
    }
    tempDirs->append(info.fileName());

    foreach (QModelIndex i, indexes) {
        if (i.isValid() && i.column() == 0) {
            selectedEntries.append(entries.at(i.row()));
            urls.append(QUrl::fromLocalFile(QString(tempPath) + "/" + entries.at(i.row()).niceName()));
        }
    }
    data->setUrls(urls);

    fileSystem->extractRecursive(selectedEntries, QString(tempPath));
    return data;
}

/* DiskEditDialog */

DiskEditDialog::DiskEditDialog(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::DiskEditDialog)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    m_fileSystemBox = new QComboBox(this);
    m_fileSystemBox->addItem(tr("No file system"));
    m_fileSystemBox->addItem(tr("Atari Dos 1.0"));
    m_fileSystemBox->addItem(tr("Atari Dos 2.0"));
    m_fileSystemBox->addItem(tr("Atari Dos 2.5"));
    m_fileSystemBox->addItem(tr("MyDos"));
    m_fileSystemBox->addItem(tr("SpartaDos"));
    m_ui->statusbar->addPermanentWidget(m_fileSystemBox);

    model = new MyModel(this);
    model->setFileSystem(0);
    m_ui->aView->setModel(model);

    connect(m_fileSystemBox, SIGNAL(currentIndexChanged(int)), SLOT(fileSystemChanged(int)));
    connect(m_ui->aView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), SLOT(currentChanged(QModelIndex,QModelIndex)));
    connect(m_ui->aView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(selectionChanged(QItemSelection,QItemSelection)));
    connect(m_ui->onTopBox, SIGNAL(stateChanged(int)), SLOT(onTopChanged()));
    m_ui->aView->viewport()->setAcceptDrops(true);
    if(aspeqtSettings->explorerOnTop()) {
            m_ui->onTopBox->setChecked(true);
            setWindowFlags(Qt::WindowStaysOnTopHint);
    }
}

DiskEditDialog::~DiskEditDialog()
{
    m_disk->setEditDialog(0);
    m_disk->unlock();
    delete m_ui;
}

void DiskEditDialog::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void DiskEditDialog::go(SimpleDiskImage *image, int fileSystem)
{
    m_disk = image;
    m_disk->lock();
    m_disk->setEditDialog(this);
    if (fileSystem < 0) {
        fileSystem = m_disk->defaultFileSystem();
    }
    if (fileSystem != m_fileSystemBox->currentIndex()) {
        m_fileSystemBox->setCurrentIndex(fileSystem);
    } else {
        fileSystemChanged(fileSystem);
    }
}

void DiskEditDialog::fileSystemChanged(int index)
{
    AtariFileSystem *a = 0;
    switch (index) {
    case 1:
        a = new Dos10FileSystem(m_disk);
        break;
    case 2:
        a = new Dos20FileSystem(m_disk);
        break;
    case 3:
        a = new Dos25FileSystem(m_disk);
        break;
    case 4:
        a = new MyDosFileSystem(m_disk);
        break;
    case 5:
        a = new SpartaDosFileSystem(m_disk);
        break;
    }

    model->setFileSystem(a);
    m_ui->aView->resizeColumnToContents(0);
    m_ui->aView->resizeColumnToContents(1);
    m_ui->aView->resizeColumnToContents(2);
    m_ui->aView->resizeColumnToContents(3);
    m_ui->aView->resizeColumnToContents(4);
    m_ui->aView->resizeColumnToContents(5);

    if (!a) {
        m_ui->aView->setEnabled(false);
        m_ui->actionAddFiles->setEnabled(false);
        m_ui->actionExtractFiles->setEnabled(false);
        m_ui->actionTextConversion->setEnabled(false);
    } else {
        m_ui->aView->setEnabled(true);
        m_ui->actionAddFiles->setEnabled(true);
        m_ui->actionTextConversion->setEnabled(true);
    }

    setWindowTitle(tr("AspeQt - Exploring %1").arg(model->currentPath()));
    m_ui->actionToParent->setEnabled(false);
}

void DiskEditDialog::currentChanged(const QModelIndex &/*current*/, const QModelIndex &/*previous*/)
{

}

void DiskEditDialog::selectionChanged(const QItemSelection &/*selected*/, const QItemSelection &/*deselected*/)
{
    bool enabled = !m_ui->aView->selectionModel()->selectedIndexes().isEmpty();
    m_ui->actionExtractFiles->setEnabled(enabled);
    m_ui->actionDeleteSelectedFiles->setEnabled(enabled);
}

void DiskEditDialog::on_aView_doubleClicked(QModelIndex index)
{
    if (model->entries.at(index.row()).attributes & AtariDirEntry::Directory) {
        model->setDirectory(index.row());
        m_ui->aView->resizeColumnToContents(0);
        m_ui->aView->resizeColumnToContents(1);
        m_ui->aView->resizeColumnToContents(2);
        m_ui->aView->resizeColumnToContents(3);
        m_ui->aView->resizeColumnToContents(4);
        m_ui->aView->resizeColumnToContents(5);
        setWindowTitle(tr("AspeQt - Exploring %1").arg(model->currentPath()));
        m_ui->actionToParent->setEnabled(true);
    }
}

void DiskEditDialog::on_actionToParent_triggered()
{
    model->toParent();
    m_ui->aView->resizeColumnToContents(0);
    m_ui->aView->resizeColumnToContents(1);
    m_ui->aView->resizeColumnToContents(2);
    m_ui->aView->resizeColumnToContents(3);
    m_ui->aView->resizeColumnToContents(4);
    m_ui->aView->resizeColumnToContents(5);
    setWindowTitle(tr("AspeQt - Exploring %1").arg(model->currentPath()));
    m_ui->actionToParent->setEnabled(!model->isRoot());
}

void DiskEditDialog::on_actionExtractFiles_triggered()
{
    QModelIndexList indexes = m_ui->aView->selectionModel()->selectedRows();
    if (indexes.isEmpty()) {
        return;
    }

    QString target = QFileDialog::getExistingDirectory(this, tr("Extract files"), aspeqtSettings->lastExtractDir());

    if (target.isEmpty()) {
        return;
    }

    aspeqtSettings->setLastExtractDir(target);

    QList <AtariDirEntry> selectedEntries;
    foreach (QModelIndex i, indexes) {
        if (i.isValid() && i.column() == 0) {
            selectedEntries.append(model->entries.at(i.row()));
        }
    }
    model->fileSystem->extractRecursive(selectedEntries, target);
}

void DiskEditDialog::on_actionTextConversion_triggered()
{
    if (m_ui->actionTextConversion->isChecked()) {
        m_ui->actionTextConversion->setToolTip(tr("Text conversion is on"));
        m_ui->actionTextConversion->setStatusTip(tr("Text conversion is on"));
        model->fileSystem->setTextConversion(true);
    } else {
        m_ui->actionTextConversion->setToolTip(tr("Text conversion is off"));
        m_ui->actionTextConversion->setStatusTip(tr("Text conversion is off"));
        model->fileSystem->setTextConversion(false);
    }
}

void DiskEditDialog::on_actionDeleteSelectedFiles_triggered()
{
    if (QMessageBox::question(this, tr("Confirmation"), tr("Are you sure you want to delete selected files?"), QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    QModelIndexList indexes = m_ui->aView->selectionModel()->selectedRows();
    model->deleteFiles(indexes);
    m_ui->aView->selectionModel()->clearSelection();
}

void DiskEditDialog::on_actionAddFiles_triggered()
{
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Add files"), aspeqtSettings->lastExtractDir());
    if (files.empty()) {
        return;
    }
    model->insertFiles(files);
}

// 
void DiskEditDialog::on_actionPrint_triggered()
{

    QPrinter printer;

    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    if (dialog->exec() != QDialog::Accepted)
    return;

    QTextEdit dirlist;
    QString fname, ext;
    int size;
    dirlist.clear();
    dirlist.append("Name\tExt\tSize");
    dirlist.append("========\t===\t====");

    for (int row = 0; row < model->entries.size(); row++) {
        fname =  model->entries.at(row).atariName.left(8);
        ext = model->entries.at(row).atariName.right(3);
        size = model->entries.at(row).size;
        dirlist.append(fname + "\t" + ext + "\t" + QString("%1").arg(size));
    }

    dirlist.print(&printer);
}
void DiskEditDialog::onTopChanged()
{
    if(m_ui->onTopBox->isChecked())
    {
       setWindowFlags(Qt::WindowStaysOnTopHint);
       aspeqtSettings->setExplorerOnTop(true);
    } else {
       setWindowFlags(Qt::WindowStaysOnBottomHint);
       aspeqtSettings->setExplorerOnTop(false);
    }
    show();
}
