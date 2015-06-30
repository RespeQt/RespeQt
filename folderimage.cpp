#include "folderimage.h"
#include "respeqtsettings.h"

#include <QFileInfoList>
#include <QtDebug>

// CIRCULAR SECTORS USED FOR SERVING FILES FROM FOLDER IMAGES
// ==========================================================
// Circular sectors per file logic utilizes all sectors from 433 to 1023 for a total of 591 sectors.
// Sector number will cycle back to 433 once it hits 1023, and this cycle will repeat until the entire file is read.
// The same pool of sectors are used for every file in the Folder Image.
// First sector number of each file however is selected from a different pool of (369-432) so that they are unique for each file.
// This allows for dynamic calculation of the Atari file number within the code.
// Sector numbers (5, 6, 32-134) are reserved for SpartaDos boot process.

extern QString g_respeQtAppPath;
extern bool g_disablePicoHiSpeed;

FolderImage::~FolderImage()
{
    close();
}

void FolderImage::close()
{
    for (int i = 0; i < 64; i++) {
        atariFiles[i].exists = false;
    }

    return;
}

bool FolderImage::format(quint16, quint16)
{
    return false;
}

// Return the long file name of a short Atari file name from a given (last mounted) Folder Image

QString FolderImage::longName(QString &lastMountedFolder, QString &atariFileName)
{
    if (FolderImage::open(lastMountedFolder, FileTypes::Dir)) {
        for (int i = 0; i < 64; i++) {
            if(atariFiles[i].atariName + "." + atariFiles[i].atariExt == atariFileName)
                return atariFiles[i].longName;
        }
     }
     return NULL;
}
void FolderImage::buildDirectory()
{
    QFileInfoList infos = dir.entryInfoList(QDir::Files,  QDir::Name);
    QFileInfo info;
    QString name, longName;
    QString ext;

    int j = -1, k, i;
    for (i = 0; i < 64; i++) {
        do {
            j++;
            if (j >= infos.count()) {
                atariFiles[i].exists = false;
                break;
            }
            info = infos.at(j);
            longName = info.completeBaseName();
            name = longName.toUpper();
            if(respeqtSettings->filterUnderscore()) {
                name.remove(QRegExp("[^A-Z0-9]"));
            } else {
                name.remove(QRegExp("[^A-Z0-9_]"));
            }
            name = name.left(8);
            if (name.isEmpty()) {
                name = "BADNAME";
            }
            longName += "." + info.suffix();
            ext = info.suffix().toUpper();
            if(respeqtSettings->filterUnderscore()) {
                ext.remove(QRegExp("[^A-Z0-9]"));
            } else {
                ext.remove(QRegExp("[^A-Z0-9_]"));
            }
            ext = ext.left(3);
            QString baseName = name.left(7);

            int l = 2;
            do {
                for (k = 0; k < i; k++) {
                    if (atariFiles[k].atariName == name && atariFiles[k].atariExt == ext) {
                        break;
                    }
                }
                if (k < i) {
                    name = QString("%1%2").arg(baseName).arg(l);
                    l++;
                }
            } while (k < i && l < 10000000);
            if (l == 10) {baseName = name.left(6);}
            if (l == 100) {baseName = name.left(5);}
            if (l == 1000) {baseName = name.left(4);}
            if (l == 10000) {baseName = name.left(3);}
            if (l == 100000) {baseName = name.left(2);}
            if (l == 1000000) {baseName = name.left(1);}
            if (l == 10000000) {baseName = "";}
            if (l == 100000000) {
                qWarning() << "!w" << tr("Cannot mirror '%1' in '%2': No suitable Atari name can be found.")
                               .arg(info.fileName())
                               .arg(dir.path());
            }
        } while (k < i);

        if (j >= infos.count()) {
            break;
        }

        atariFiles[i].exists = true;
        atariFiles[i].original = info;
        atariFiles[i].atariName = name;
        atariFiles[i].longName = longName;
        atariFiles[i].atariExt = ext;
        atariFiles[i].lastSector = 0;
        atariFiles[i].pos = 0;
        atariFiles[i].sectPass = 0;

    }

    if (i < infos.count()) {
        qWarning() << "!w" << tr("Cannot mirror %1 of %2 files in '%3': Atari directory is full.")
                       .arg(infos.count() - i)
                       .arg(infos.count())
                       .arg(dir.path());
    }
}

bool FolderImage::open(const QString &fileName, FileTypes::FileType /* type */)
{
    if (dir.exists(fileName)) {
        dir.setPath(fileName);

        buildDirectory();

        m_originalFileName = fileName;
        m_geometry.initialize(false, 40, 26, 128);
        m_newGeometry.initialize(m_geometry);
        m_isReadOnly = true;
        m_isModified = false;
        m_isUnmodifiable = true;
        return true;
    } else {
        return false;
    }
}

bool FolderImage::readSector(quint16 sector, QByteArray &data)
{
    /* Boot */

    QFile boot(dir.path() + "/$boot.bin");
    data = QByteArray(128, 0);
    int bootFileSector;

    if (sector == 1) {
         if (!boot.open(QFile::ReadOnly)) {
             data[1] = 0x01;
             data[3] = 0x07;
             data[4] = 0x40;
             data[5] = 0x15;
             data[6] = 0x4c;
             data[7] = 0x14;
             data[8] = 0x07;     // JMP 0x0714
             data[0x14] = 0x38;  // SEC
             data[0x15] = 0x60;  // RTS
         } else {
             data = boot.read(128);
             buildDirectory();
             for(int i=0; i<64; i++) {
                 // AtariDOS, MyDos, SmartDOS  and DosXL
                 if(atariFiles[i].longName.toUpper() == "DOS.SYS") {
                     bootFileSector = 369 + i;
                     data[15] = bootFileSector % 256;
                     data[16] = bootFileSector / 256;
                     break;
                 }
                 // MyPicoDOS
                 if(atariFiles[i].longName.toUpper() == "PICODOS.SYS") {
                     bootFileSector = 369 + i;
                     if(g_disablePicoHiSpeed) {
                         data[15] = 0;
                         QFile boot(dir.path() + "/$boot.bin");
                         QByteArray speed;
                         boot.open(QFile::ReadWrite);
                         boot.seek(15);
                         speed = boot.read(1);
                         speed[0] = '\x30';
                         boot.seek(15);
                         boot.write(speed);
                         boot.close();
                     }
                     data[9] = bootFileSector % 256;
                     data[10] = bootFileSector / 256;
                     // Create the piconame.txt file
                     QFile picoName(dir.path() + "/piconame.txt");
                     picoName.open(QFile::WriteOnly);
                     QByteArray nameLine;
                     nameLine.append(dir.dirName() + '\x9B');
                     picoName.write(nameLine);
                     for(int i=0; i<64; i++){
                     if(atariFiles[i].exists) {
                         if(atariFiles[i].longName != "$boot.bin") {
                                 nameLine.clear();
                                 nameLine.append(atariFiles[i].atariName);
                                 QByteArray space;
                                 int size;
                                 size = atariFiles[i].atariName.size();
                                 for(int j=0; j<=8-size-1; j++) {
                                     space[j] = '\x20';
                                 }
                                 nameLine.append(space);
                                 nameLine.append(atariFiles[i].atariExt);
                                 nameLine.append('\x20');
                                 nameLine.append(atariFiles[i].longName.mid(0, atariFiles[i].longName.indexOf(".", -1)-1));
                                 nameLine.append('\x9B');
                                 picoName.write(nameLine);
                         }
                      } else {
                             picoName.close();
                             break;
                      }
                     }
                     break;
                 }
                 // SpartaDOS, force it to change to AtariDOS format after the boot
                 if(atariFiles[i].longName.toUpper() == "X32.DOS") {
                     QFile x32Dos(dir.path() + "/x32.dos");
                     x32Dos.open(QFile::ReadOnly);
                     QByteArray flag;
                     flag = x32Dos.readAll();
                     if(flag[0] == '\xFF') {
                         flag[0] = '\x00';
                         data[1] = 0x01;
                         data[3] = 0x07;
                         data[4] = 0x40;
                         data[5] = 0x15;
                         data[6] = 0x4c;
                         data[7] = 0x14;
                         data[8] = 0x07;
                         data[0x14] = 0x38;
                         data[0x15] = 0x60;
                     }
                   break;
                 }
             }
         }
         return true;
    }
    if (sector == 2) {
        boot.open(QFile::ReadOnly);
        boot.seek(128);
        data = boot.read(128);
        return true;
    }
    if (sector == 3) {
        boot.open(QFile::ReadOnly);
        boot.seek(256);
        data = boot.read(128);
        return true;
    }
    // SpartaDOS Boot
    if ((sector >= 32 && sector <= 134) ||
        sector == 5 || sector == 6) {
        boot.open(QFile::ReadOnly);
        boot.seek((sector-1)*128);
        data = boot.read(128);
        if(sector == 134) {
            QFile x32Dos(dir.path() + "/x32.dos");
            x32Dos.open(QFile::ReadWrite);
            QByteArray flag;
            flag = x32Dos.readAll();
            if(flag[0] == '\x00') {
                flag[0] = '\xFF';
                x32Dos.seek(0);
                x32Dos.write(flag);
                x32Dos.close();
            }
        }
        return true;
    }

    /* VTOC */
    if (sector == 360) {
        data = QByteArray(128, 0);
        data[0] = 2;
        data[1] = 1010 % 256;
        data[2] = 1010 / 256;
        data[10] = 0x7F;
        for (int i = 11; i < 100; i++) {
            data[i] = 0xff;
        }
        return true;
    }

    /* Directory sectors */
    if (sector >= 361 && sector <=368) {
        if (sector == 361) {
            buildDirectory();
        }
        data.resize(0);
        for (int i = (sector - 361) * 8; i < (sector - 360) * 8; i++) {
            QByteArray entry;
            if (!atariFiles[i].exists) {
                entry = QByteArray(16, 0);
            } else {
                entry = "";
                entry[0] = 0x42;
                QFileInfo info = atariFiles[i].original;;
                int size = (info.size() + 124) / 125;
                if (size > 999) {
                    size = 999;
                }
                entry[1] = size % 256;
                entry[2] = size / 256;
                int first = 369 + i;
                entry[3] = first % 256;
                entry[4] = first / 256;
                entry += atariFiles[i].atariName.toLatin1();
                while (entry.count() < 13) {
                    entry += 32;
                }
                entry += atariFiles[i].atariExt.toLatin1();
                while (entry.count() < 16) {
                    entry += 32;
                }
            }
            data += entry;
        }
        return true;
    }

    /* Data sectors */

    /* First sector of the file */
        int size, next;
        if  (sector >= 369 && sector <= 432) {
            atariFileNo = sector - 369;
            if (!atariFiles[atariFileNo].exists) {
                data = QByteArray(128, 0);
                return true;
            }
            QFile file(atariFiles[atariFileNo].original.absoluteFilePath());
            file.open(QFile::ReadOnly);
            data = file.read(125);
            size = data.size();
            data.resize(128);
            if (file.atEnd()) {
                next = 0;
            }
            else {
                next = 433;
            }
            data[125] = (atariFileNo * 4) | (next / 256);
            data[126] = next % 256;
            data[127] = size;
            return true;
        }

    /* Rest of the file sectors */
        if ((sector >= 433 && sector <= 1023)) {
            QFile file(atariFiles[atariFileNo].original.absoluteFilePath());
            file.open(QFile::ReadOnly);
            atariFiles[atariFileNo].pos = (125+((sector-433)*125))+(atariFiles[atariFileNo].sectPass*73875);
            file.seek(atariFiles[atariFileNo].pos);
            data = file.read(125);
            next = sector + 1;
            if (sector == 1023) {
                next = 433;
                atariFiles[atariFileNo].sectPass += 1;
            }
            size = data.size();
            data.resize(128);
            atariFiles[atariFileNo].lastSector = sector;
            if (file.atEnd()) {
                next = 0;
            }
            data[125] = (atariFileNo * 4) | (next / 256);
            data[126] = next % 256;
            data[127] = size;
            return true;
        }

    /* Any other sector */

        data = QByteArray(128, 0);
        return true;
}

bool FolderImage::writeSector(quint16, const QByteArray &)
{
    return false;
}
