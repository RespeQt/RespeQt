#include "miscutils.h"

#include <QDir>
#include <QFile>
#include <QDebug>

void deltree(const QString &name)
{
    QFileInfo info(name);

    if (info.isDir()) {
        QDir dir(name);
        QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
        foreach (QFileInfo file, list) {
            deltree(file.absoluteFilePath());
        }
        QString n = dir.dirName();
        dir.cdUp();
        dir.rmdir(n);
    } else {
        QFile::remove(name);
    }
}

/* FileTypes */

FileTypes::FileType FileTypes::getFileType(const QString &fileName)
{
    QByteArray header;
    FileType result = Unknown;

    /* Check if it is a folder */

    if (QFileInfo(fileName).isDir()) {
        return Dir;
    }

    /* Read the file header */
    {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            header = file.read(4);
        }
        while (header.count() < 4) {
            header.append('\x0');
        }
    }

    /* Check if the file is gzipped */

    bool gz = false;

    if ((quint8)header.at(0) == 0x1f && (quint8)header.at(1) == 0x8b ) {
        /* The file is gzipped, read the real header */
        gz = true;
        GzFile file(fileName);
        if (file.open(QFile::ReadOnly)) {
            header = file.read(4);
        } else {
            header = QByteArray(4, 0);
        }
        while (header.count() < 4) {
            header.append('\x0');
        }
    }

    quint8 b0 = header.at(0);
    quint8 b1 = header.at(1);
    quint8 b2 = header.at(2);
    quint8 b3 = header.at(3);

    /* Determine the file type */

    if (b0 == 'A' && b1 == 'T' && b2 == '8' && b3 == 'X') {
        result = Atx;
    } else if (b0 == 'F' && b1 == 'U' && b2 == 'J' && b3 == 'I') {
        result = Cas;
    } else if (b0 == 0x96 && b1 == 0x02) {
        result = Atr;
    } else if (b0 == 0xF9 || b0 == 0xFA) {
        result = Dcm;
    } else if (b0 == 0xFF && b1 == 0xFF) {
        result = Xex;
    } else if (b0 == 0xFD && b1 == 0xFD) {
        result = Scp;
    } else if (b0 == 'D' && b1 == 'I') {
        result = Di;
    } else if (b2 == 'P' && (b3 == '2' || b3 == '3')) {
        result = Pro;
    } else if (fileName.endsWith(".XFD", Qt::CaseInsensitive) || fileName.endsWith(".XFZ", Qt::CaseInsensitive) || fileName.endsWith(".XFD.GZ", Qt::CaseInsensitive)) {
        result = Xfd;
    }

    if (result != Unknown && gz) {
        result = (FileType) (result + 1);
    }

    return result;
}

QString FileTypes::getFileTypeName(FileType type)
{
    switch (type) {
    case Atr:
        return tr("ATR disk image");
    case AtrGz:
        return tr("gzipped ATR disk image");
    case Xfd:
        return tr("XFD disk image");
    case XfdGz:
        return tr("gziped XFD disk image");
    case Dcm:
        return tr("DCM disk image");
    case DcmGz:
        return tr("gzipped DCM disk image");
    case Scp:
        return tr("SCP disk image");
    case ScpGz:
        return tr("gzipped SCP disk image");
    case Di:
        return tr("DI disk image");
    case DiGz:
        return tr("gzipped DI disk image");
    case Pro:
        return tr("PRO disk image");
    case ProGz:
        return tr("gzipped PRO disk image");
    case Atx:
        return tr("VAPI (ATX) disk image");
    case AtxGz:
        return tr("gzipped VAPI (ATX) disk image");
    case Cas:
        return tr("CAS cassette image");
    case CasGz:
        return tr("gzipped CAS cassette image");
    case Xex:
        return tr("Atari executable");
    case XexGz:
        return tr("gzipped Atari executable");
    default:
        return tr("unknown file type");
    }
}

/* GzFile */

GzFile::GzFile(const QString &path)
    :QFile(path)
{
    mPath = path;
    mHandle = 0;
}

GzFile::~GzFile()
{
    close();
}

bool GzFile::open(OpenMode mode)
{
    if (QFile::open(mode)) {
        if((mode & ReadOnly) == ReadOnly) {
            mHandle = gzdopen(handle(), "rb");
        }  else if((mode & ReadWrite) == ReadWrite) {
            mHandle = gzdopen(handle(), "wb+");
        } else if((mode & WriteOnly) == WriteOnly) {
            mHandle = gzdopen(handle(), "wb");
        }

        if(mHandle == NULL) {
            setErrorString(tr("gzdopen() failed."));
            return false;
        }
    } else {
        return false;
    }
    return true;
}

void GzFile::close()
{
    if (mHandle) {
        gzclose(mHandle);
    }
    QFile::close();
    mHandle = NULL;
}

bool GzFile::isSequential() const
{
    return true;
}

bool GzFile::seek(qint64 pos)
{
    bool result = gzseek(mHandle, pos, SEEK_SET) != -1;
    if (!result) {
        setErrorString(tr("gzseek() failed."));
    }
    return result;
}

qint64 GzFile::readData(char *data, qint64 maxSize)
{
    if (mHandle == NULL) {
        return 0;
    }

    return gzread(mHandle, data, maxSize);
}

qint64 GzFile::writeData(const char *data, qint64 maxSize)
{
    if (mHandle == NULL) {
        return 0;
    }

    return gzwrite(mHandle, data, maxSize);
}

bool GzFile::atEnd() const
{
    return gzeof(mHandle);
}
