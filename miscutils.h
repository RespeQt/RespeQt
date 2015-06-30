#ifndef MISCUTILS_H
#define MISCUTILS_H

#include <QString>
#include <QFile>
#include "zlib.h"

void deltree(const QString &name);

class GzFile : public QFile
{
    Q_OBJECT

public:
    GzFile(const QString& path);
    ~GzFile();

    bool open(OpenMode mode);
    void close();
    bool seek(qint64 pos);
    bool isSequential () const;
    bool atEnd() const;

protected:
    gzFile mHandle;
    QString mPath;
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);
};

class FileTypes : public QObject
{
    Q_OBJECT

public:
    enum FileType {
        Unknown,
        Dir,
        Atr, AtrGz,
        Xfd, XfdGz,
        Dcm, DcmGz,
        Scp, ScpGz,
        Di, DiGz,
        Pro, ProGz,
        Atx, AtxGz,
        Cas, CasGz,
        Xex, XexGz
    };
    static FileType getFileType(const QString &fileName);
    static QString getFileTypeName(FileType type);
};


#endif // MISCUTILS_H
