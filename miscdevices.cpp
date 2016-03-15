/*
 * miscdevices.cpp
 *
 * Copyright 2015 Joseph Zatarski
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifdef Q_WS_WIN
#include "windows.h"
#endif

#include "miscdevices.h"
#include "respeqtsettings.h"
#include "mainwindow.h"


#include <QDateTime>
#include <QtDebug>

extern char g_aspeclSlotNo;
extern bool g_printerEmu;

// 
bool conversionMsgdisplayedOnce;
QString imageFileName;

void Printer::handleCommand(quint8 command, quint16 aux)
{
    if(g_printerEmu) {  // Ignore printer commands  if Emulation turned OFF)    // 
        switch(command) {
        case 0x53:
            {
                // Get status
                if (!sio->port()->writeCommandAck()) {
                    return;
                }

                QByteArray status(4, 0);
                status[0] = 0;
                status[1] = m_lastOperation;
                status[2] = 3;
                status[3] = 0;
                sio->port()->writeComplete();
                sio->port()->writeDataFrame(status);
                qDebug() << "!n" << tr("[%1] Get status.").arg(deviceName());
                break;
            }
        case 0x57:
            {
                // Write
                int aux2 = aux % 256;
                int len;
                switch (aux2) {
                case 0x4e:
                    // Normal
                    len = 40;
                    break;
                case 0x53:
                    // Sideways
                    len = 29;
                    break;
                case 0x44:
                    // Double width
                    len = 21;
                    break;
                default:
                    sio->port()->writeCommandNak();
                    qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                                   .arg(deviceName())
                                   .arg(command, 2, 16, QChar('0'))
                                   .arg(aux, 4, 16, QChar('0'));
                    return;
                }
                // Display Info message once  // 
                if (!conversionMsgdisplayedOnce) {
                    qDebug() << "!n" << tr("[%1] Converting Inverse Video Characters for ASCII viewing").arg(deviceName()).arg(len);
                    conversionMsgdisplayedOnce = true;
                }
                sio->port()->writeCommandAck();

                QByteArray data = sio->port()->readDataFrame(len);
                if (data.isEmpty()) {
                    qCritical() << "!e" << tr("[%1] Print: data frame failed")
                                  .arg(deviceName());
                    sio->port()->writeDataNak();
                    return;
                }
                sio->port()->writeDataAck();
                qDebug() << "!n" << tr("[%1] Print (%2 chars)").arg(deviceName()).arg(len);
                int n = data.indexOf('\x9b');
                if (n == -1) {
                    n = len;
                }
                data.resize(n);
                data.replace('\n', '\x9b');
                if (n < len) {
                    data.append("\n");
                }
                emit print(QString::fromLatin1(data));
                sio->port()->writeComplete();
                break;
            }
        default:
            sio->port()->writeCommandNak();
            qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                           .arg(deviceName())
                           .arg(command, 2, 16, QChar('0'))
                           .arg(aux, 4, 16, QChar('0'));
        }
    } else {
        qDebug() << "!u" << tr("[%1] ignored").arg(deviceName());
    }
}

//ApeTime device

void ApeTime::handleCommand(quint8 command, quint16 aux)
{
    QByteArray data(5, 0);
    QDateTime dateTime = QDateTime::currentDateTime();

    if (command == 0x93) {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        data[0] = dateTime.date().day();
        data[1] = dateTime.date().month();
        data[2] = dateTime.date().year() % 100;
        data[3] = dateTime.time().hour();
        data[4] = dateTime.time().minute();
        data[5] = dateTime.time().second();

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);

        qDebug() << "!n" << tr("[%1] Read date/time (%2).")
                       .arg(deviceName())
                       .arg(dateTime.toString(Qt::SystemLocaleShortDate));
    } else {
        sio->port()->writeCommandNak();
        qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                       .arg(deviceName())
                       .arg(command, 2, 16, QChar('0'))
                       .arg(aux, 4, 16, QChar('0'));
    }
}

// AspeQt Client ()

void AspeCl::handleCommand(quint8 command, quint16 aux)
{
    QByteArray data(5, 0);
    QDateTime dateTime = QDateTime::currentDateTime();

    switch (command) {
      case 0x93 :   // Send Date/Time
         {
            if (!sio->port()->writeCommandAck()) {
                return;
            }

            data[0] = dateTime.date().day();
            data[1] = dateTime.date().month();
            data[2] = dateTime.date().year() % 100;
            data[3] = dateTime.time().hour();
            data[4] = dateTime.time().minute();
            data[5] = dateTime.time().second();
            qDebug() << "!n" << tr("[%1] Date/time sent to client (%2).")
                       .arg(deviceName())
                       .arg(dateTime.toString(Qt::SystemLocaleShortDate));

            sio->port()->writeComplete();
            sio->port()->writeDataFrame(data);
         }
         break;

      case 0x94 :   // Swap Disks
         {
            if (!sio->port()->writeCommandAck()) {
                return;
            }
            qint8 swapDisk1, swapDisk2;
            swapDisk1 = aux /256 - 1;
            swapDisk2 = aux % 256 - 1;
            if (swapDisk1 > 9) swapDisk1 -= 16;
            if (swapDisk2 > 9) swapDisk2 -= 16;
            if (swapDisk1 >= 0 and swapDisk1 < 15 and swapDisk2 >=0 and swapDisk2 < 15 and swapDisk1 != swapDisk2) {
                sio->swapDevices(swapDisk1 + DISK_BASE_CDEVIC, swapDisk2 + DISK_BASE_CDEVIC);
                respeqtSettings->swapImages(swapDisk1, swapDisk2);
                qDebug() << "!n" << tr("[%1] Swapped disk %2 with disk %3.")
                                .arg(deviceName())
                                .arg(swapDisk2 + 1)
                                .arg(swapDisk1 + 1);
            } else {
                sio->port()->writeCommandNak();
                qDebug() << "!e" << tr("[%1] Invalid swap request for drives: (%2)-(%3).")
                           .arg(deviceName())
                           .arg(swapDisk2 + 1)
                           .arg(swapDisk1 + 1);
            }
            sio->port()->writeComplete();
         }
         break;

      case 0x95 :   // Unmount Disk(s)
       {
          if (!sio->port()->writeCommandAck()) {
              return;
          }
          qint8 unmountDisk;
          unmountDisk = aux /256;
          if (unmountDisk == -6) unmountDisk = 0;        // All drives
          if (unmountDisk > 9) unmountDisk -= 16;       // Drive 10-15
          if (unmountDisk >= 0 and unmountDisk <= 15) {
              if (unmountDisk == 0) {
              // Eject All disks
                  int toBeSaved = 0;
                  for (int i = 0; i <= 14; i++) {    // 
                      SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
                      if (img && img->isModified()) {
                          toBeSaved++;
                      }
                  }
                  if (!toBeSaved) {
                      for (int i = 14; i >= 0; i--) {
                          SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
                          sio->uninstallDevice(i + DISK_BASE_CDEVIC);
                          delete img;
                          respeqtSettings->unmountImage(i);
                          qDebug() << "!n" << tr("[%1] Unmounted disk %2")
                                        .arg(deviceName())
                                        .arg(i + 1);
                      }
                      qDebug() << "!n" << tr("[%1] ALL images were remotely unmounted")
                                .arg(deviceName());
                  } else {
                      sio->port()->writeCommandNak();
                      qDebug() << "!e" << tr("[%1] Can not remotely unmount ALL images due to pending changes.")
                                .arg(deviceName());
                  }
              } else {
                  // Single Disk Eject
                  SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(unmountDisk - 1 + DISK_BASE_CDEVIC));

                  if (img && img->isModified()) {
                      sio->port()->writeCommandNak();
                      qDebug() << "!e" << tr("[%1] Can not remotely unmount disk %2 due to pending changes.")
                                .arg(deviceName())
                                .arg(unmountDisk);
                  } else {
                      sio->uninstallDevice(unmountDisk - 1 + DISK_BASE_CDEVIC);
                      delete img;
                      respeqtSettings->unmountImage(unmountDisk - 1);
                      qDebug() << "!n" << tr("[%1] Remotely unmounted disk %2")
                                  .arg(deviceName())
                                  .arg(unmountDisk);
                  }
                }

              } else {
                  sio->port()->writeCommandNak();
                  qDebug() << "!e" << tr("[%1] Invalid drive number: %2 for remote unmount")
                         .arg(deviceName())
                         .arg(unmountDisk);
            }
            sio->port()->writeComplete();
       }
       break;

      case 0x96 :   // Mount Disk Image
      case 0x97 :   // Create and Mount a new Disk Image
       {
          if (!sio->port()->writeCommandAck()) {
              return;
          }
          // If no Folder Image has ever been mounted abort the command as we won't
          // know which folder to use to remotely create/mount an image file.
          if(respeqtSettings->lastFolderImageDir() == "") {
              qCritical() << "!e" << tr("[%1] RespeQt can't determine the folder where the image file must be created/mounted!")
                            .arg(deviceName());
              qCritical() << "!e" << tr("[%1] Mount a Folder Image at least once before issuing a remote mount command.")
                            .arg(deviceName());
              sio->port()->writeDataNak();
              sio->port()->writeError();
              return;
          }
          // Get the name of the image file
          int len;
          if (command == 0x96) {
            len = 12;
          } else {
            len = 14;
          }
          if (aux == 0) {
              QByteArray data(len, 0);
              data = sio->port()->readDataFrame(len);
              if (data.isEmpty()) {
                  qCritical() << "!e" << tr("[%1] Read data frame failed")
                                .arg(deviceName());
                  sio->port()->writeDataNak();
                  sio->port()->writeError();
                  return;
               }

              imageFileName = data;
              if (command == 0x97) {     // Create new image file first
                  int i, type;
                  bool ok;
                  i = imageFileName.lastIndexOf(".");
                  type = imageFileName.mid(i+1).toInt(&ok, 10);
                  if (ok && (type < 1 || type > 6)) ok = false;
                  if(!ok) {
                      qCritical() << "!e" << tr("[%1] Invalid image file attribute: %2")
                                     .arg(deviceName())
                                     .arg(type);
                      sio->port()->writeDataNak();
                      sio->port()->writeError();
                      return;
                  }
                  imageFileName = imageFileName.left(i);
                  QFile file(respeqtSettings->lastFolderImageDir() + "/" + imageFileName);
                  if (!file.open(QIODevice::WriteOnly)) {
                      qCritical() << "!e" << tr("[%1] Can not create PC File: %2")
                                     .arg(deviceName())
                                     .arg(imageFileName);
                      sio->port()->writeDataNak();
                      sio->port()->writeError();
                      return;
                  }
                  sio->port()->writeDataAck();

                  int fileSize;
                  QByteArray fileData;
                  switch (type){
                    case 1 :        // Single Density
                      {
                      fileSize = 92160;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = 0x80;
                      fileData[3] = 0x16;
                      fileData[4] = 0x80;
                      }
                      break;
                    case 2 :        // Enhanced Density
                      {
                      fileSize = 133120;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = 0x80;
                      fileData[3] = 0x20;
                      fileData[4] = 0x80;
                      }
                      break;
                    case 3 :        // Double Density
                      {
                      fileSize = 183936;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = 0xE8;
                      fileData[3] = 0x2C;
                      fileData[4] = 0x00;
                      fileData[5] = 0x01;
                      }
                      break;
                    case 4 :        // Double Sided, Double Density
                      {
                      fileSize = 368256;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = 0xE8;
                      fileData[3] = 0x59;
                      fileData[4] = 0x00;
                      fileData[5] = 0x01;
                      }
                      break;
                    case 5 :        // Double Density Hard Disk
                      {
                      fileSize = 16776576;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = 0xD8;
                      fileData[3] = 0xFF;
                      fileData[4] = 0x00;
                      fileData[5] = 0x01;
                      fileData[6] = 0x0F;
                      }
                      break;
                    case 6 :        // Quad Density Hard Disk
                      {
                      fileSize = 33553576;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = 0xE0;
                      fileData[3] = 0xFF;
                      fileData[4] = 0x00;
                      fileData[5] = 0x02;
                      fileData[6] = 0x1F;
                      }
                      break;
                  }
                  fileData[0] = 0x96;
                  fileData[1] = 0x02;
                  file.write(fileData);
                  fileData.clear();
                  file.close();

              } // Cmd 0x97 -- Create new image file first

              sio->port()->writeDataAck();

              imageFileName = "*" + imageFileName;

              // Ask the MainWindow for the next available slot number
              emit findNewSlot(0, true);

          } else {

              // Return the last mounted drive number
              QByteArray data(1,0);
              data[0] = g_aspeclSlotNo;
              sio->port()->writeComplete();
              sio->port()->writeDataFrame(data);
          }
       }
       break;

      case 0x98 :   // Auto-Commit toggle
        {
            if (!sio->port()->writeCommandAck()) {
                return;
            }
            qint8 commitDisk;
            commitDisk = aux % 256 - 1;
            if (commitDisk > 9) commitDisk -= 16;

            if (commitDisk != -7 && (commitDisk <0 || commitDisk > 14)) {
                sio->port()->writeCommandNak();
                return;
            }
            // All disks or a given disk
            if (commitDisk == -7) {
                for (int i = 0; i < 15; i++) {
                    emit toggleAutoCommit(i);
                }
            } else {
                emit toggleAutoCommit(commitDisk);
            }

            sio->port()->writeComplete();
        }
        break;

      default :
      // Invalid Command
         sio->port()->writeCommandNak();
         qWarning() << "!e" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                       .arg(deviceName())
                       .arg(command, 2, 16, QChar('0'))
                       .arg(aux, 4, 16, QChar('0'));
    }
}

// Get the next slot number available for mounting a disk image
void AspeCl::gotNewSlot(int slot)
{
   g_aspeclSlotNo = slot;

   // Ask the MainWindow to mount the file
   emit mountFile(slot, imageFileName);
}

void AspeCl::fileMounted(bool mounted)
{
    if (mounted) {
        sio->port()->writeComplete();
        qDebug() << "!n" << tr("[%1] Image %2 mounted")
                   .arg(deviceName())
                    .arg(imageFileName.mid(1,imageFileName.size()-1));
    } else {
       sio->port()->writeDataNak();
    }
}
