/*
 * diskimagepro.h
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifndef DISKIMAGEPRO_H
#define DISKIMAGEPRO_H

#include "diskimage.h"

class DiskImagePro : public SimpleDiskImage
{
    Q_OBJECT

public:
    DiskImagePro(SioWorker *worker): SimpleDiskImage(worker) {}
    ~DiskImagePro();

    void close();
    bool open(const QString &fileName, FileTypes::FileType /* type */);
    bool seekToSector(quint16 sector);
    bool readSector(quint16 sector, QByteArray &data);
    bool writeSector(quint16 sector, const QByteArray &data);
    void getStatus(QByteArray &status);
    bool format(const DiskGeometry& );
protected:
    QFile *sourceFile;
    quint8 count[1040];
    quint8 wd1772status;
};

#endif // DISKIMAGEPRO_H
