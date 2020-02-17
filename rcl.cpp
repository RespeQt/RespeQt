/*
 * rcl.cpp
 *
 * Copyright 2015, 2017 Joseph Zatarski
 * Copyright 2016, 2017 TheMontezuma
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#ifdef Q_WS_WIN
#include "windows.h"
#endif

#include "rcl.h"
#include "respeqtsettings.h"
#include "mainwindow.h"


#include <QDateTime>
#include <QtDebug>
#include <QDesktopServices>
#include <QUrl>

char RCl::g_rclSlotNo;

// RespeQt Client ()

void RCl::handleCommand(quint8 command, quint16 aux)
{
    QByteArray  data(5, 0);
    QByteArray  fdata(21, 0);
    QByteArray  ddata(255, 0);
    QDateTime dateTime = QDateTime::currentDateTime();

    switch (command) {

    case 0x91 : // list rcl folder(up to 250 files)
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }
        quint8 offset = (aux  % 256);
        quint8 index  = 0;
        QString pth = respeqtSettings->lastRclDir();
        QDir dir(pth);
        QStringList filters;
        filters << "*.atr" << "*.xfd" << "*.atx" << "*.pro";
        dir.setNameFilters(filters);
        QFileInfoList list = dir.entryInfoList();

        if(offset == 0) {
            QByteArray fn  = ("Path: " + pth).toUtf8();
            for(int n = 0; n < fn.length() && n < 37; n++)
                ddata[index++] = fn[n] & 0xff;
            ddata[index++] = 155;
        }

        for (quint8 i = offset; i < list.size() && i < 250;  ++i) {
            QFileInfo fileInfo = list.at(i);
            QString dosfilname = fileInfo.fileName();
            QString atarifilname = toAtariFileName(dosfilname);
            QByteArray fn  = (" "+ atarifilname).toUtf8();
            if(index + fn.length() < 252) {
                for(int n = 0; n < fn.length(); n++)
                    ddata[index++] = fn[n] & 0xff;
                ddata[index++] = 155;
                ddata[254] =  0x00;
            } else  {
                ddata[254] =  i;
                break;
            }
        }

        for(int n = index; n < 253 ; n++) ddata[index++] = 0x00;
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(ddata);
        return;

    }
        break;

    case 0x92 :   // get slots filename
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        qint8 deviceNo;
        deviceNo =  (aux  % 256);
        deviceNo  = (deviceNo > 9) ? (deviceNo -16) :deviceNo;

        if (deviceNo >= 0x0 && deviceNo <= 0xF ) {
            SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo -1 +  DISK_BASE_CDEVIC));
            QString  filename = "";
            if (img) {
                int i = -1;
                i = img->originalFileName().lastIndexOf("/");
                if ((i != -1) || (img->originalFileName().mid(0, 14) == "Untitled image"))
                    filename = " " + img->originalFileName().right(img->originalFileName().size() - ++i);
            }


            QByteArray fn = filename.toUtf8();
            for(int i=0; i < 22; i++)
                fdata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;

            sio->port()->writeComplete();
            sio->port()->writeDataFrame(fdata);
            return;
        }
        sio->port()->writeDataNak();
    }

        break;
      case 0x93 :   // Send Date/Time
         {
            if (!sio->port()->writeCommandAck()) {
                return;
            }

            data[0] = static_cast<char>(dateTime.date().day());
            data[1] = static_cast<char>(dateTime.date().month());
            data[2] = static_cast<char>(dateTime.date().year() % 100);
            data[3] = static_cast<char>(dateTime.time().hour());
            data[4] = static_cast<char>(dateTime.time().minute());
            data[5] = static_cast<char>(dateTime.time().second());
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
            swapDisk1 = static_cast<qint8>(aux / 256);
            swapDisk2 = static_cast<qint8>(aux % 256);
            if (swapDisk1 > 25)
                swapDisk1 -= 16;
            if (swapDisk2 > 25)
                swapDisk2 -= 16;
            if (swapDisk1 > 0 && swapDisk1 <= 15
                && swapDisk2 > 0 && swapDisk2 <= 15
                && swapDisk1 != swapDisk2)
            {
                sio->swapDevices(
                    static_cast<quint8>(swapDisk1 + DISK_BASE_CDEVIC - 1),
                    static_cast<quint8>(swapDisk2 + DISK_BASE_CDEVIC - 1));
                respeqtSettings->swapImages(swapDisk1 - 1, swapDisk2 - 1);
                qDebug() << "!n" << tr("[%1] Swapped disk %2 with disk %3.")
                                .arg(deviceName())
                                .arg(swapDisk2 + 1)
                                .arg(swapDisk1 + 1);
            } else {
                sio->port()->writeCommandNak();
                qDebug() << "!e" << tr("[%1] Invalid swap request for drives: (%2)-(%3).")
                           .arg(deviceName())
                           .arg(swapDisk2)
                           .arg(swapDisk1);
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
          unmountDisk = static_cast<qint8>(aux /256);
          if (unmountDisk == -6)
              unmountDisk = 0;        // All drives
          if (unmountDisk > 25)
              unmountDisk -= 16;       // Drive 10-15
          if (unmountDisk >= 0 && unmountDisk <= 15) {
              if (unmountDisk == 0) {
              // Eject All disks
                  int toBeSaved = 0;
                  for (int i = 0; i <= 14; i++) {    //
                      SimpleDiskImage *img = qobject_cast <SimpleDiskImage*>
                            (sio->getDevice(static_cast<quint8>(i + DISK_BASE_CDEVIC)));
                      if (img && img->isModified()) {
                          toBeSaved++;
                      }
                  }
                  if (!toBeSaved) {
                      for (int i = 14; i >= 0; i--) {
                          SimpleDiskImage *img = qobject_cast <SimpleDiskImage*>
                                  (sio->getDevice(static_cast<quint8>(i + DISK_BASE_CDEVIC)));
                          sio->uninstallDevice(static_cast<quint8>(i + DISK_BASE_CDEVIC));
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
                  SimpleDiskImage *img = qobject_cast <SimpleDiskImage*>
                          (sio->getDevice(static_cast<quint8>(unmountDisk - 1 + DISK_BASE_CDEVIC)));

                  if (img && img->isModified()) {
                      sio->port()->writeCommandNak();
                      qDebug() << "!e" << tr("[%1] Can not remotely unmount disk %2 due to pending changes.")
                                .arg(deviceName())
                                .arg(unmountDisk);
                  } else {
                      sio->uninstallDevice(static_cast<quint8>(unmountDisk - 1 + DISK_BASE_CDEVIC));
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
              data = sio->port()->readDataFrame(static_cast<uint>(len));
#ifndef QT_NO_DEBUG
              sio->writeSnapshotDataFrame(data);
#endif
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
                  QFile file(respeqtSettings->lastRclDir() + "/" + imageFileName);
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
                      fileData[2] = static_cast<char>(0x80);
                      fileData[3] = static_cast<char>(0x16);
                      fileData[4] = static_cast<char>(0x80);
                      }
                      break;
                    case 2 :        // Enhanced Density
                      {
                      fileSize = 133120;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = static_cast<char>(0x80);
                      fileData[3] = static_cast<char>(0x20);
                      fileData[4] = static_cast<char>(0x80);
                      }
                      break;
                    case 3 :        // Double Density
                      {
                      fileSize = 183936;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = static_cast<char>(0xE8);
                      fileData[3] = static_cast<char>(0x2C);
                      fileData[4] = static_cast<char>(0x00);
                      fileData[5] = static_cast<char>(0x01);
                      }
                      break;
                    case 4 :        // Double Sided, Double Density
                      {
                      fileSize = 368256;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = static_cast<char>(0xE8);
                      fileData[3] = static_cast<char>(0x59);
                      fileData[4] = static_cast<char>(0x00);
                      fileData[5] = static_cast<char>(0x01);
                      }
                      break;
                    case 5 :        // Double Density Hard Disk
                      {
                      fileSize = 16776576;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = static_cast<char>(0xD8);
                      fileData[3] = static_cast<char>(0xFF);
                      fileData[4] = static_cast<char>(0x00);
                      fileData[5] = static_cast<char>(0x01);
                      fileData[6] = static_cast<char>(0x0F);
                      }
                      break;
                    case 6 :        // Quad Density Hard Disk
                      {
                      fileSize = 33553576;
                      fileData.resize(fileSize+16);
                      fileData.fill(0);
                      fileData[2] = static_cast<char>(0xE0);
                      fileData[3] = static_cast<char>(0xFF);
                      fileData[4] = static_cast<char>(0x00);
                      fileData[5] = static_cast<char>(0x02);
                      fileData[6] = static_cast<char>(0x1F);
                      }
                      break;
                  }
                  fileData[0] = static_cast<char>(0x96);
                  fileData[1] = static_cast<char>(0x02);
                  file.write(fileData);
                  fileData.clear();
                  file.close();

              } // Cmd 0x97 -- Create new image file first

              sio->port()->writeDataAck();

              imageFileName = "*" + toDosFileName(imageFileName);

              // Ask the MainWindow for the next available slot number
              emit findNewSlot(0, true);

          } else {

              // Return the last mounted drive number
              QByteArray data(1,0);
              data[0] = g_rclSlotNo;
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
        bool commitOnOff;
        commitDisk = aux % 256 - 1;
        commitOnOff = (aux/256)?false:true;

        if (commitDisk > 9) commitDisk -= 16;
        if (commitDisk != -7 && (commitDisk <0 || commitDisk > 14)) {
            sio->port()->writeCommandNak();
            return;
        }

        // All disks or a given disk
        if (commitDisk == -7) {
            for (int i = 0; i < 15; i++) {
                emit toggleAutoCommit(i, commitOnOff);
            }
        } else {
            emit toggleAutoCommit(commitDisk, commitOnOff);
        }

        sio->port()->writeComplete();
    }
        break;

     case 0x99 :   // save disks
     {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        int diskSaved = 0;
        qint8 deviceNo;
        deviceNo = aux /256;

        if (deviceNo == -6) deviceNo = 0;        // All drives
        if (deviceNo > 9) deviceNo -= 16;        // Drive 10-15
        if (deviceNo >= 0 and deviceNo <= 15) {
            if (deviceNo == 0) {
                // Eject All disks

                for (int i = 0; i <= 14; i++) {    //
                    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
                    if (img && img->isModified() && !img->isUnnamed()) {
                        img->save();
                        qDebug() << "!n" << tr("[%1] Saved disk %2")
                                    .arg(deviceName())
                                    .arg(i + 1);
                        diskSaved++;
                    }
                }

            } else {
                // Single Disk save
                SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo - 1 + DISK_BASE_CDEVIC));

                if (img && img->isModified() && !img->isUnnamed()) {
                    img->save();
                    qDebug() << "!n" << tr("[%1] Saved disk %2")
                                .arg(deviceName())
                                .arg(deviceNo);
                    diskSaved++;
                }
            }
            if (!diskSaved)
              sio->port()->writeCommandNak();

        } else {
            sio->port()->writeCommandNak();
            qDebug() << "!e" << tr("[%1] Invalid drive number: %2 for remote unmount")
                        .arg(deviceName())
                        .arg(deviceNo);
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
void RCl::gotNewSlot(int slot)
{
   g_rclSlotNo = static_cast<char>(slot);

   // Ask the MainWindow to mount the file
   emit mountFile(slot, imageFileName);
}

void RCl::fileMounted(bool mounted)
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



QString RCl::toAtariFileName(QString dosFileName) {
    QString name = "";
    QString ext  = "";
    QString filename = dosFileName.toUpper();
    int t = filename.lastIndexOf(".");
    if(t > 0){
        name = filename.left(t);
        ext = filename.right(filename.length() - t -1);
    }
    name.remove(QRegExp("[^A-Z0-9_]"));
    name = (name.length() > 8)?name.left(5)+"$"+name.right(2): name;
    ext.remove(QRegExp("[^A-Z0-9_]"));
    return name + "." +ext;
}


QString RCl::toDosFileName(QString atariFileName)
{
    QString pth = respeqtSettings->lastRclDir();
    QDir dir(pth);
    QStringList filters;
    filters << "*.atr" << "*.xfd" << "*.atx" << "*.pro";
    dir.setNameFilters(filters);
    QFileInfoList list = dir.entryInfoList();
    for (quint8 i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        QString dosFilename = fileInfo.fileName();
        if(toAtariFileName(dosFilename) == atariFileName)
            return dosFilename;

     }
     return "";
}



