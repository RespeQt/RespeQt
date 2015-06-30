#ifndef FOLDERIMAGE_H
#define FOLDERIMAGE_H

#include <QDir>
#include "diskimage.h"

class AtariFile
{
public:
    bool exists;
    QFileInfo original;
    QString atariName;
    QString atariExt;
    QString longName;
    int lastSector;
    quint64 pos;
    int sectPass;
};

class FolderImage : public SimpleDiskImage
{
    Q_OBJECT

protected:
    QDir dir;
    bool mReadOnly;
    void buildDirectory();
    AtariFile atariFiles[64];
    int atariFileNo;             // 

public:
    FolderImage(SioWorker *worker): SimpleDiskImage(worker) {}
    ~FolderImage();

    void close();
    bool open(const QString &fileName, FileTypes::FileType /* type */);
    bool readSector(quint16 sector, QByteArray &data);
    bool writeSector(quint16 sector, const QByteArray &data);
    bool format(quint16 aSectorCount, quint16 aSectorSize);
    QString longName (QString &lastMountedFolder, QString &atariFileName);   // 
    QString description() const {return tr("Folder image");}
};
    extern FolderImage *folderImage;

#endif // FOLDERIMAGE_H
