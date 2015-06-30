#include "autoboot.h"
#include "aspeqtsettings.h"

#include <QtDebug>

AutoBoot::~AutoBoot()
{
    close();
}

void AutoBoot::passToOldHandler(quint8 command, quint16 aux)
{
    if (oldDevice) {
        oldDevice->handleCommand(command, aux);
    } else {
        sio->port()->writeCommandNak();
        qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                       .arg(deviceName())
                       .arg(command, 2, 16, QChar('0'))
                       .arg(aux, 4, 16, QChar('0'));
    }
}

void AutoBoot::handleCommand(quint8 command, quint16 aux)
{

    switch (command) {
        case 0x3F:  // Speed poll
            {
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                sio->port()->writeComplete();
                QByteArray speed(1, 0);
                speed[0] = sio->port()->speedByte();
                sio->port()->writeDataFrame(speed);
                qDebug() << "!n" << tr("[%1] Speed poll.").arg(deviceName());
                break;
            }
        case 0x52:
            {    /* Read sector */
                if (loaded) {
                    passToOldHandler(command, aux);
                    return;
                }
                if (aux >= 1 && aux <= sectorCount) {
                    if (!sio->port()->writeCommandAck()) {
                        return;
                    }
                    if (!started) {
                        emit booterStarted();
                        started = true;
                    }
                    QByteArray data;
                    if (readSector(aux, data)) {
                        sio->port()->writeComplete();
                        sio->port()->writeDataFrame(data);
                        qDebug() << "!n" << tr("[%1] Read sector %2 (%3 bytes).")
                                       .arg(deviceName())
                                       .arg(aux)
                                       .arg(data.size());
                    } else {
                        sio->port()->writeError();
                        qCritical() << "!e" << tr("[%1] Read sector %2 failed.")
                                       .arg(deviceName())
                                       .arg(aux);
                    }
                } else {
                    passToOldHandler(command, aux);
                }
                break;
            }
        case 0x53:
            {    /* Get status */
                if (loaded) {
                    passToOldHandler(command, aux);
                    return;
                }
                if (!sio->port()->writeCommandAck()) {
                    return;
                }

                QByteArray status(4, 0);
                status[0] = 8;
                status[3] = 1;
                sio->port()->writeComplete();
                sio->port()->writeDataFrame(status);
                qDebug() << "!n" << tr("[%1] Get status.")
                               .arg(deviceName());
                break;
            }
        case 0xFD:
            {
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                qDebug() << "!n" << tr("[%1] Atari is jumping to %2.")
                               .arg(deviceName())
                               .arg(aux);
                emit loaderDone();
                sio->port()->writeComplete();
                break;
            }
        case 0xFE:
            {   /* Get chunk */
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                qDebug() << "!n" << tr("[%1] Get chunk %2 (%3 bytes).")
                               .arg(deviceName())
                               .arg(aux)
                               .arg(chunks.at(aux).data.size());
                sio->port()->writeComplete();
                sio->port()->writeDataFrame(chunks.at(aux).data);
                emit blockRead(aux + 1, chunks.count());
                break;
            }
        case 0xFF:
            {   /* Get chunk info */
                if (!sio->port()->writeCommandAck()) {
                    return;
                }
                if (!loaded) {
                    loaded = true;
                    emit booterLoaded();
                }
                QByteArray data;
                data[0] = chunks.at(aux).address % 256;
                data[1] = chunks.at(aux).address / 256;
                data[2] = 1;
                data[3] = chunks.size() != aux + 1;
                data[4] = chunks.at(aux).data.size() % 256;
                data[5] = chunks.at(aux).data.size() / 256;
                qDebug() << "!d" << tr("[%1] Get chunk info %2 (%3 bytes at %4).")
                               .arg(deviceName())
                               .arg(aux)
                               .arg(chunks.at(aux).data.size())
                               .arg(chunks.at(aux).address);
                sio->port()->writeComplete();
                sio->port()->writeDataFrame(data);
                break;
            }
        default:
            passToOldHandler(command, aux);
            return;
            break;
    }
}

bool AutoBoot::readExecutable(const QString &fileName)
{
    QFile file(fileName);

    if (!file.open(QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open file '%1': %2")
                       .arg(fileName)
                       .arg(file.errorString());
        return false;
    }

    int start, end;

    QByteArray data;

    /* Read the $FFFF header */
    data = file.read(2);
    if (data.size() < 2) {
        QString error;
        if (file.atEnd()) {
            error = tr("Unexpected end of file, needed %1 more").arg(2 - data.size());
        } else {
            error = file.errorString();
        }
        qCritical() << "!e" << tr("Cannot read from file '%1': %2.")
                       .arg(fileName)
                       .arg(error);
        return false;
    }
    start = (quint8) data.at(0) + (quint8) data.at(1) * 256;
    if (start != 0xffff) {
        QString error;
        qCritical() << "!e" << tr("Cannot load file '%1': The file doesn't seem to be an Atari DOS executable.")
                       .arg(fileName)
                       .arg(error);
        return false;
    }

    /* Read the start address */
    data = file.read(2);
    if (data.size() < 2) {
        QString error;
        if (file.atEnd()) {
            error = tr("Unexpected end of file, needed %1 more").arg(2 - data.size());
        } else {
            error = file.errorString();
        }
        qCritical() << "!e" << tr("Cannot read from file '%1': %2.").arg(fileName).arg(error);
        return false;
    }
    start = (quint8) data.at(0) + (quint8) data.at(1) * 256;

    do {
        /* Read the end address */
        data = file.read(2);

        /* Check consistency */
        if (data.size() < 2) {
            if (file.atEnd()) {
                qCritical() << "!e" << tr("The executable '%1' is broken: Unexpected end of file, needed %2 more.")
                               .arg(fileName)
                               .arg(2 - data.size());
                if (chunks.count() == 0) {
                    return false;
                } else {
                    break;
                }
            } else {
                qCritical() << "!e" << tr("Cannot read from file '%1': %2.")
                               .arg(fileName)
                               .arg(file.errorString());
                return false;
            }
        }
        end = (quint8) data.at(0) + (quint8) data.at(1) * 256;

        qDebug() << "!d" << "Exe segment" << start << ".." << end;

        if (end < start) {
            qWarning() << tr("The executable '%1' is broken: The end address is less than the start address.")
                          .arg(fileName);
            break;
        }

        /* Read the chunk */
        int size = end - start + 1;
        data = file.read(size);
        if (data.size() < size) {
            if (file.atEnd()) {
                qWarning() << "!w" << tr("The executable '%1' is broken: Unexpected end of file, needed %2 more.")
                               .arg(fileName)
                               .arg(size - data.size());
            } else {
                qWarning() << "!w" << tr("Cannot read from file '%1': %2.")
                               .arg(fileName)
                               .arg(file.errorString());
                return false;
            }
        }

        int maxChunkSize = 1024;
        for (int i = 0; i < data.size(); i += maxChunkSize) {
            AtariExeChunk ch;
            ch.data = data.mid(i, maxChunkSize);
            ch.address = start;
            start += maxChunkSize;
            chunks.append(ch);
        }

        /* Exit if done */
        if (file.atEnd()) {
            break;
        }

        /* Read the next start address */
        data = file.read(2);
        if (data.size() < 2) {
            if (file.atEnd()) {
                qWarning() << "!w" << tr("The executable '%1' is broken: Unexpected end of file, needed %2 more.")
                               .arg(fileName)
                               .arg(2 - data.size());
                break;
            } else {
                qCritical() << "!e" << tr("Cannot read from file '%1': %2.")
                               .arg(fileName)
                               .arg(file.errorString());
                return false;
            }
        }
        start = (quint8) data.at(0) + (quint8) data.at(1) * 256;

        /* Skip if it's a $FFFF */
        if (start == 0xffff) {
            data = file.read(2);
            if (data.size() < 2) {
                if (file.atEnd()) {
                    qWarning() << "!w" << tr("The executable '%1' is broken: Unexpected end of file, needed %2 more.")
                                   .arg(fileName)
                                   .arg(2 - data.size());
                    break;
                } else {
                    qCritical() << "!e" << tr("Cannot read from file '%1': %2.")
                                   .arg(fileName)
                                   .arg(file.errorString());
                    return false;
                }
            }
            start = (quint8) data.at(0) + (quint8) data.at(1) * 256;
        }
    } while (true);
    return true;
}

bool AutoBoot::open(const QString &fileName, bool highSpeed)
{
    close();
    QFile boot;

    if (highSpeed) {
        boot.setFileName(":/binaries/atari/autoboot/autoboot-hispeed.bin");
    } else {
        boot.setFileName(":/binaries/atari/autoboot/autoboot.bin");
    }

    if (!boot.open(QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open the boot loader: %1")
                       .arg(boot.errorString());
        return false;
    }
    bootSectors = boot.readAll();
    boot.close();
    sectorCount = (boot.size() + 127) * 128;

    chunks = QList <AtariExeChunk> ();

    return readExecutable(fileName);
}

void AutoBoot::close()
{
}

bool AutoBoot::readSector(quint16 sector, QByteArray &data)
{
    data = bootSectors.mid((sector - 1) * 128, 128);
    data.resize(128);
    return true;
}

QString AutoBoot::deviceName()
{
    return "AutoBoot";
}
