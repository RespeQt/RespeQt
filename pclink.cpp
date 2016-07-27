/*
 * pclink.cpp
 *
 * This file contains code from the SIO2BSD project by KMK (drac030)
 */

#include <QtDebug>
#include <dirent.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>

#include "pclink.h"

# define SDX_MAXLEN 16777215L

/* SDX required attribute mask */
# define RA_PROTECT     0x01
# define RA_HIDDEN      0x02
# define RA_ARCHIVED	0x04
# define RA_SUBDIR      0x08
# define RA_NO_PROTECT	0x10
# define RA_NO_HIDDEN	0x20
# define RA_NO_ARCHIVED	0x40
# define RA_NO_SUBDIR	0x80

/* SDX set attribute mask */
# define SA_PROTECT     0x01
# define SA_UNPROTECT	0x10
# define SA_HIDE        0x02
# define SA_UNHIDE      0x20
# define SA_ARCHIVE     0x04
# define SA_UNARCHIVE	0x40
# define SA_SUBDIR      0x08	/* illegal mode */
# define SA_UNSUBDIR	0x80	/* illegal mode */

# define DEVICE_LABEL	".PCLINK.VOLUME.LABEL"

# define PCL_MAX_FNO	0x14
# define PCL_MAX_DIR_LENGTH 1023

/* Atari SIO status block */
typedef struct
{
    uchar stat;
    uchar err;
    uchar tmot;
    uchar none;
} STATUS;

typedef	struct			/* PCLink parameter buffer */
{
    uchar fno;		    /* function number */
    uchar handle;		/* file handle */
    uchar f1,f2,f3,f4;	/* general-purpose bytes */
    uchar f5,f6;		/* more general-purpose bytes */
    uchar fmode;		/* fmode */
    uchar fatr1;		/* fatr1 */
    uchar fatr2;		/* fatr2 */
    uchar name[12];		/* name */
    uchar names[12];	/* names */
    uchar path[65];		/* path */
} PARBUF;

typedef struct
{
    STATUS status;		/* the 4-byte status block */
    int on;			    /* PCLink mount flag */
    char dirname[PCL_MAX_DIR_LENGTH+1];	/* PCLink root directory path */
    uchar cwd[65];		/* PCLink current working dir, relative to the above */
    PARBUF parbuf;		/* PCLink parameter buffer */
} DEVICE;

typedef struct
{
    union
    {
        FILE *file;
        DIR *dir;
    } fps;

    DIRENTRY *dir_cache;	/* used only for directories */

    uchar devno;
    uchar cunit;
    uchar fpmode;
    uchar fatr1;
    uchar fatr2;
    uchar t1,t2,t3;
    uchar d1,d2,d3;
    struct stat fpstat;
    char fpname[12];
    long fppos;
    long fpread;
    int eof;
    char pathname[PCL_MAX_DIR_LENGTH+1];
} IODESC;

typedef struct
{
    uchar handle;
    uchar dirbuf[23];
} PCLDBF;

static bool D = false; // extended debug
static ulong upper_dir = 0; // in PCLink dirs accept lowercase characters only
static IODESC iodesc[16];
static DEVICE device[16];	/* 1 PCLINK device with support for 15 units */
static PCLDBF pcl_dbf;

static const char *fun[] =
{
    "FREAD", "FWRITE", "FSEEK", "FTELL", "FLEN", "(none)", "FNEXT", "FCLOSE",
    "INIT", "FOPEN", "FFIRST", "RENAME", "REMOVE", "CHMOD", "MKDIR", "RMDIR",
    "CHDIR", "GETCWD", "SETBOOT", "DFREE", "CHVOL"
};

/*************************************************************************/

PCLINK::PCLINK(SioWorker *worker)
    :SioDevice(worker)
{
    do_pclink_init(1);
}

/*************************************************************************/

PCLINK::~PCLINK()
{
}

/*************************************************************************/

void PCLINK::handleCommand(quint8 command, quint16 aux)
{
    uchar caux1 = aux & 0xFF;
    uchar caux2 = (aux >> 8) & 0xFF;
    
    uchar cunit = caux2 & 0x0F;	/* PCLink ignores DUNIT */

    if(D) qDebug() << "!n" << tr("PCLINK Command=[$%1] aux1=$%2 aux2=$%3 cunit=$%4").arg(command,0,16)
                                                                          .arg(caux1,0,16)
                                                                          .arg(caux2,0,16)
                                                                          .arg(cunit,0,16);

    /* cunit == 0 is init during warm reset */
    if ((cunit == 0) || device[cunit].on)
    {
        switch (command)
        {
            case 'P':
            {
                qDebug() << "!n" << tr("[%1] P").arg(deviceName());
                do_pclink(PCLINK_CDEVIC, command, caux1, caux2);
                break;
            }

            case 'R':
            {
                qDebug() << "!n" << tr("[%1] R").arg(deviceName());
                do_pclink(PCLINK_CDEVIC, command, caux1, caux2);
                break;
            }

            case 'S':	/* status */
            {
                sio->port()->writeCommandAck();

                QByteArray status(4, 0);
                status[0] = device[cunit].status.stat;
                status[1] = device[cunit].status.err;
                status[2] = device[cunit].status.tmot;
                status[3] = device[cunit].status.none;

                sio->port()->writeComplete();
                sio->port()->writeDataFrame(status);

                qDebug() << "!n" << tr("[%1] Get status for [%2]").arg(deviceName()).arg(cunit);
                break;
            }

            case '?':	/* send hi-speed index */
            {
                sio->port()->writeCommandAck();
                sio->port()->writeComplete();
                QByteArray speed(1, 0);
                speed[0] = sio->port()->speedByte();
                sio->port()->writeDataFrame(speed);
                qDebug() << "!n" << tr("[%1] Speed poll").arg(deviceName());
                break;
            }

            default:
            {
                sio->port()->writeCommandNak();
                qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                               .arg(deviceName())
                               .arg(command, 2, 16, QChar('0'))
                               .arg(aux, 4, 16, QChar('0'));
                break;
            }
        }
    }
}

/*************************************************************************/

bool PCLINK::hasLink(int no)
{
    if(no<1 || no>15)return false;

    return (device[no].on == 1);
}

/*************************************************************************/

void PCLINK::setLink(int no, const char* fileName)
{
    if(no<1 || no>15)return;

    fps_close(no);
    memset(&device[no].parbuf, 0, sizeof(PARBUF));

    strncpy(device[no].dirname,fileName,PCL_MAX_DIR_LENGTH);
    device[no].dirname[PCL_MAX_DIR_LENGTH]=0;
    device[no].cwd[0]=0;
    device[no].on = 1;

    if(D) qDebug() << "!n" << tr("PCLINK[%1] Mount %2").arg(no).arg(fileName);
}

/*************************************************************************/

void PCLINK::swapLinks(int from, int to)
{
    if(from<1 || from>15 || to<1 || to>15)return;

    char tmp_dir_name[PCL_MAX_DIR_LENGTH];
    if(hasLink(from))
    {
        strncpy(tmp_dir_name, device[from].dirname, PCL_MAX_DIR_LENGTH);
        resetLink(from);
        if(hasLink(to))
        {
            setLink(from,device[to].dirname);
            resetLink(to);
        }
        setLink(to, tmp_dir_name);
    }
    else if(hasLink(to))
    {
        setLink(from,device[to].dirname);
        resetLink(to);
    }
}

/*************************************************************************/

void PCLINK::resetLink(int no)
{
    if(no<1 || no>15)return;

    fps_close(no);
    memset(&device[no].parbuf, 0, sizeof(PARBUF));

    device[no].on = 0;
    device[no].dirname[0]=0;
    device[no].cwd[0]=0;

    if(D) qDebug() << "!n" << tr("PCLINK[%1] Unmount").arg(no);
}

/*************************************************************************/

void PCLINK::do_pclink_init(int force)
{
    uchar handle;

    if (force == 0)
    {
        if(D) qDebug() << "!n" << tr("closing all files");
    }

    for (handle = 0; handle < 16; handle++)
    {
        if (force)
            iodesc[handle].fps.file = NULL;
        fps_close(handle);
        memset(&device[handle].parbuf, 0, sizeof(PARBUF));
    }
}

/*************************************************************************/

/* Command: DDEVIC+DUNIT-1 = $6f, DAUX1 = parbuf size, DAUX2 = %vvvvuuuu
 * where: v - protocol version number (0), u - unit number
 */
void PCLINK::do_pclink(uchar devno, uchar ccom, uchar caux1, uchar caux2)
{
    uchar fno, ob[7], handle;
    ushort cunit = caux2 & 0x0f, parsize;
    ulong faux;
    struct stat sb;
    struct dirent *dp;
    static uchar old_ccom = 0;

    parsize = caux1 ? caux1 : 256;

    if (caux2 & 0xf0)	/* protocol version number must be 0 */
    {
        sio->port()->writeCommandNak();
        return;
    }

    if (parsize > sizeof(PARBUF))	/* and not more than fits in parbuf */
    {
        sio->port()->writeCommandNak();
        return;
    }

    if (ccom == 'P')
    {
        PARBUF pbuf;

        sio->port()->writeCommandAck(); /* ack the command */

        memset(&pbuf, 0, sizeof(PARBUF));

        QByteArray data = sio->port()->readDataFrame(parsize);

        device[cunit].status.stat &= ~0x02;

        sio->port()->writeDataAck(); /* ack the received block */

        if (data.length() != parsize)
        {
            device[cunit].status.stat |= 0x02;
            device[cunit].status.err = 143;
            goto complete;
        }
        else
        {
            memcpy(&pbuf, data.constData(), parsize);
        }

        device[cunit].status.stat &= ~0x04;

        if (memcmp(&pbuf, &device[cunit].parbuf, sizeof(PARBUF)) == 0)
        {
            /* this is a retry of P-block. Most commands don't like that */
            if ((pbuf.fno != 0x00) && (pbuf.fno != 0x01) && (pbuf.fno != 0x03) \
                && (pbuf.fno != 0x04) && (pbuf.fno != 0x06) && \
                    (pbuf.fno != 0x11) && (pbuf.fno != 0x13))
            {
                if(D) qDebug() << "!n" << tr("PARBLK retry, ignored");
                goto complete;
            }
        }

        memcpy(&device[cunit].parbuf, &pbuf, sizeof(PARBUF));
    }

    fno = device[cunit].parbuf.fno;
    faux = device[cunit].parbuf.f1 + device[cunit].parbuf.f2 * 256 + \
        device[cunit].parbuf.f3 * 65536;

    if (fno < (PCL_MAX_FNO+1))
    {
        if(D) qDebug() << "!n" << tr("%1 (fno $%02)").arg(fun[fno]).arg(fno,0,16);
    }

    handle = device[cunit].parbuf.handle;

    if (fno == 0x00)	/* FREAD */
    {
        uchar *mem;
        ulong blk_size = (faux & 0x0000FFFFL), buffer;

        if (ccom == 'P')
        {
            if ((handle > 15) || (iodesc[handle].fps.file == NULL))
            {
                if(D) qDebug() << "!n" << tr("bad handle 1 %1").arg(handle);
                device[cunit].status.err = 134;	/* bad file handle */
                goto complete;
            }

            if (blk_size == 0)
            {
                if(D) qDebug() << "!n" << tr("bad size $0000 (0)");
                device[cunit].status.err = 176;
                set_status_size(cunit, 0);
                goto complete;
            }

            device[cunit].status.err = 1;
            iodesc[handle].eof = 0;

            buffer = iodesc[handle].fpstat.st_size - iodesc[handle].fppos;

            if (buffer < blk_size)
            {
                blk_size = buffer;
                device[cunit].parbuf.f1 = (buffer & 0x00ff);
                device[cunit].parbuf.f2 = (buffer & 0xff00) >> 8;
                iodesc[handle].eof = 1;
                if (blk_size == 0)
                    device[cunit].status.err = 136;
            }
            if(D) qDebug() << "!n" << tr("size $%1 (%2), buffer $%3 (%4)").arg(blk_size,0,16)
                                                                    .arg(blk_size)
                                                                    .arg(blk_size,0,16)
                                                                    .arg(blk_size);
            set_status_size(cunit, (ushort)blk_size);
            goto complete;
        }

        if ((ccom == 'R') && (old_ccom == 'R'))
        {
            sio->port()->writeCommandNak();
            if(D) qDebug() << "!n" << tr("serial communication error, abort");
            return;
        }

        sio->port()->writeCommandAck();	/* ack the command */

        if(D) qDebug() << "!n" << tr("handle %1").arg(handle);

        mem = (uchar*)malloc(blk_size + 1);

        if ((device[cunit].status.err == 1))
        {
            iodesc[handle].fpread = blk_size;

            if (iodesc[handle].fpmode & 0x10)
            {
                ulong rdata;
                int eof_sig;

                rdata = dir_read(mem, blk_size, handle, &eof_sig);

                if (rdata != blk_size)
                {
                    if(D) qDebug() << "!n" << tr("FREAD: cannot read %1 bytes from dir").arg(blk_size);
                    if (eof_sig)
                    {
                        iodesc[handle].fpread = rdata;
                        device[cunit].status.err = 136;
                    }
                    else
                    {
                        iodesc[handle].fpread = 0;
                        device[cunit].status.err = 255;
                    }
                }
            }
            else
            {
                if (fseek(iodesc[handle].fps.file, iodesc[handle].fppos, SEEK_SET))
                {
                    if(D) qDebug() << "!n" << tr("FREAD: cannot seek to $%1 (%2)").arg(iodesc[handle].fppos,0,16).arg(iodesc[handle].fppos);
                    device[cunit].status.err = 166;
                }
                else
                {
                    long fdata = fread(mem, sizeof(char), blk_size, iodesc[handle].fps.file);

                    if ((ulong)fdata != blk_size)
                    {
                        if(D) qDebug() << "!n" << tr("FREAD: cannot read %1 bytes from file").arg(blk_size);
                        if (feof(iodesc[handle].fps.file))
                        {
                            iodesc[handle].fpread = fdata;
                            device[cunit].status.err = 136;
                        }
                        else
                        {
                            iodesc[handle].fpread = 0;
                            device[cunit].status.err = 255;
                        }
                    }
                }
            }
        }

        iodesc[handle].fppos += iodesc[handle].fpread;

        if (device[cunit].status.err == 1)
        {
            if (iodesc[handle].eof)
                device[cunit].status.err = 136;
            else if (iodesc[handle].fppos == iodesc[handle].fpstat.st_size)
                device[cunit].status.err = 3;
        }

        set_status_size(cunit, iodesc[handle].fpread);

        if(D) qDebug() << "!n" << tr("FREAD: send $%1 (%2), status $%3").arg(blk_size,0,16).arg(blk_size).arg(device[cunit].status.err);

        QByteArray data((const char *)mem,(int)blk_size);
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);

        free(mem);

        goto exit;
    }

    if (fno == 0x01)	/* FWRITE */
    {
        uchar *mem;
        ulong blk_size = (faux & 0x0000FFFFL);

        if (ccom == 'P')
        {
            if ((handle > 15) || (iodesc[handle].fps.file == NULL))
            {
                if(D) qDebug() << "!n" << tr("bad handle 2 %1").arg(handle);
                device[cunit].status.err = 134;	/* bad file handle */
                goto complete;
            }

            if (blk_size == 0)
            {
                if(D) qDebug() << "!n" << tr("bad size $0000 (0)");
                device[cunit].status.err = 176;
                set_status_size(cunit, 0);
                goto complete;
            }

            device[cunit].status.err = 1;

            if(D) qDebug() << "!n" << tr("size $%1 (%2)").arg(blk_size,0,16).arg(blk_size);
            set_status_size(cunit, (ushort)blk_size);
            goto complete;
        }

        if ((ccom == 'R') && (old_ccom == 'R'))
        {
            sio->port()->writeCommandNak();
            if(D) qDebug() << "!n" << tr("serial communication error, abort");
            return;
        }

        sio->port()->writeCommandAck();	/* ack the command */

        if(D) qDebug() << "!n" << tr("handle %1").arg(handle);

        if ((iodesc[handle].fpmode & 0x10) == 0)
        {
            if (fseek(iodesc[handle].fps.file, iodesc[handle].fppos, SEEK_SET))
            {
                if(D) qDebug() << "!n" << tr("FWRITE: cannot seek to $%1 (%2)").arg(0,16,iodesc[handle].fppos).arg(iodesc[handle].fppos);
                device[cunit].status.err = 166;
            }
        }

        mem = (uchar*)malloc(blk_size + 1);

        QByteArray data = sio->port()->readDataFrame(blk_size);

        sio->port()->writeDataAck(); 	/* ack the block of data */

        if (data.length() != blk_size)
        {
            if(D) qDebug() << "!n" << tr("FWRITE: block CRC mismatch");
            device[cunit].status.err = 143;
            free(mem);
            goto complete;
        }
        else
        {
            memcpy(mem, data.constData(), blk_size);
        }

        if (device[cunit].status.err == 1)
        {
            long rdata;

            iodesc[handle].fpread = blk_size;

            if (iodesc[handle].fpmode & 0x10)
            {
                /* ignore raw dir writes */
            }
            else
            {
                rdata = fwrite(mem, sizeof(char), blk_size, iodesc[handle].fps.file);

                if ((ulong)rdata != blk_size)
                {
                    if(D) qDebug() << "!n" << tr("FWRITE: cannot write %1 bytes to file").arg(blk_size);
                    iodesc[handle].fpread = rdata;
                    device[cunit].status.err = 255;
                }
            }
        }

        iodesc[handle].fppos += iodesc[handle].fpread;

        set_status_size(cunit, iodesc[handle].fpread);

        if(D) qDebug() << "!n" << tr("FWRITE: received $%1 (%2), status $%3").arg(blk_size,0,16).arg(blk_size).arg(device[cunit].status.err,0,16);

        free(mem);
        goto complete;
    }

    if (fno == 0x02)	/* FSEEK */
    {
        ulong newpos = faux;

        if ((handle > 15) || (iodesc[handle].fps.file == NULL))
        {
            if(D) qDebug() << "!n" << tr("bad handle 3 %1").arg(handle);
            device[cunit].status.err = 134;	/* bad file handle */
            goto complete;
        }

        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            if(D) qDebug() << "!n" << tr("bad exec");
            device[cunit].status.err = 176;
            goto complete;
        }

        device[cunit].status.err = 1;

        if(D) qDebug() << "!n" << tr("handle %1, newpos $%2 (%3)").arg(handle).arg(newpos,0,16).arg(newpos);

        if (iodesc[handle].fpmode & 0x08)
            iodesc[handle].fppos = newpos;
        else
        {
            if (newpos <= (ulong)iodesc[handle].fpstat.st_size)
                iodesc[handle].fppos = newpos;
            else
                device[cunit].status.err = 166;
        }

        goto complete;
    }

    if ((fno == 0x03) || (fno == 0x04))	/* FTELL/FLEN */
    {
        ulong outval = 0;
        uchar out[3];

        if (ccom == 'P')
        {
            if ((handle > 15) || (iodesc[handle].fps.file == NULL))
            {
                if(D) qDebug() << "!n" << tr("bad handle 4 %1").arg(handle);
                device[cunit].status.err = 134;	/* bad file handle */
                goto complete;
            }

            device[cunit].status.err = 1;

            if(D) qDebug() << "!n" << tr("device $%1").arg(cunit,0,16);
            goto complete;
        }

        sio->port()->writeCommandAck();	/* ack the command */

        if (fno == 0x03)
            outval = iodesc[handle].fppos;
        else
            outval = iodesc[handle].fpstat.st_size;

        if(D) qDebug() << "!n" << tr("handle %1, send $%2 (%3)").arg(handle).arg(outval,0,16).arg(outval);

        out[0] = (uchar)(outval & 0x000000ffL);
        out[1] = (uchar)((outval & 0x0000ff00L) >> 8);
        out[2] = (uchar)((outval & 0x00ff0000L) >> 16);

        QByteArray data((const char *)out,sizeof(out));
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);

        goto exit;
    }

    if (fno == 0x06)	/* FNEXT */
    {
        if (ccom == 'P')
        {
            device[cunit].status.err = 1;

            if(D) qDebug() << "!n" << tr("device $%1").arg(cunit,0,16);
            goto complete;
        }

        if ((ccom == 'R') && (old_ccom == 'R'))
        {
            sio->port()->writeCommandNak();
            if(D) qDebug() << "!n" << tr("serial communication error, abort");
            return;
        }

        sio->port()->writeCommandAck();	/* ack the command */

        memset(pcl_dbf.dirbuf, 0, sizeof(pcl_dbf.dirbuf));

        if ((handle > 15) || (iodesc[handle].fps.file == NULL))
        {
            if(D) qDebug() << "!n" << tr("bad handle 5 %1").arg(handle);
            device[cunit].status.err = 134;	/* bad file handle */
        }
        else
        {
            int eof_flg, match = 0;

            if(D) qDebug() << "!n" << tr("handle %1").arg(handle);

            do
            {
                struct stat ts;

                memset(&ts, 0, sizeof(ts));
                memset(pcl_dbf.dirbuf, 0, sizeof(pcl_dbf.dirbuf));
                iodesc[handle].fppos += dir_read(pcl_dbf.dirbuf, sizeof(pcl_dbf.dirbuf), handle, &eof_flg);

                if(D) qDebug() << "!n" << tr("eof_flg %1").arg(eof_flg);

                if (!eof_flg)
                {
                    /* fake stat to satisfy match_dos_names() */
                    if ((pcl_dbf.dirbuf[0] & 0x01) == 0)
                        ts.st_mode |= S_IWUSR;
                    if (pcl_dbf.dirbuf[0] & 0x20)
                        ts.st_mode |= S_IFDIR;
                    else
                        ts.st_mode |= S_IFREG;

                    match = !match_dos_names((char *)pcl_dbf.dirbuf+6, iodesc[handle].fpname, iodesc[handle].fatr1, &ts);
                }

            } while (!eof_flg && !match);

            if (eof_flg)
            {
                if(D) qDebug() << "!n" << tr("FNEXT: EOF");
                device[cunit].status.err = 136;
            }
            else if (iodesc[handle].fppos == iodesc[handle].fpstat.st_size)
                device[cunit].status.err = 3;
        }

        /* avoid the 4th execution stage */
        pcl_dbf.handle = device[cunit].status.err;

        if(D) qDebug() << "!n" << tr("FNEXT: status %1, send $%2 $%3%4 $%5%6%7 %8%9%10%11%12%13%14%15%16%17%18 %19-%20-%21 %22:%23:%24")
            .arg(pcl_dbf.handle)
            .arg(pcl_dbf.dirbuf[0],0,16)
            .arg(pcl_dbf.dirbuf[2],0,16).arg(pcl_dbf.dirbuf[1],0,16)
            .arg(pcl_dbf.dirbuf[5]).arg(pcl_dbf.dirbuf[4]).arg(pcl_dbf.dirbuf[3])
            .arg(pcl_dbf.dirbuf[6]).arg(pcl_dbf.dirbuf[7]).arg(pcl_dbf.dirbuf[8]).arg(pcl_dbf.dirbuf[9])
            .arg(pcl_dbf.dirbuf[10]).arg(pcl_dbf.dirbuf[11]).arg(pcl_dbf.dirbuf[12]).arg(pcl_dbf.dirbuf[13])
            .arg(pcl_dbf.dirbuf[14]).arg(pcl_dbf.dirbuf[15]).arg(pcl_dbf.dirbuf[16])
            .arg(pcl_dbf.dirbuf[17]).arg(pcl_dbf.dirbuf[18]).arg(pcl_dbf.dirbuf[19])
            .arg(pcl_dbf.dirbuf[20]).arg(pcl_dbf.dirbuf[21]).arg(pcl_dbf.dirbuf[22]);

        QByteArray data((const char *)&pcl_dbf,sizeof(pcl_dbf));
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);

        goto exit;
    }

    if (fno == 0x07)	/* FCLOSE */
    {
        uchar fpmode;
        time_t mtime;
        char pathname[PCL_MAX_DIR_LENGTH+1];

        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            device[cunit].status.err = 176;
            if(D) qDebug() << "!n" << tr("bad exec");
            goto complete;
        }

        if ((handle > 15) || (iodesc[handle].fps.file == NULL))
        {
            if(D) qDebug() << "!n" << tr("bad handle 6 %1").arg(handle);
            device[cunit].status.err = 134;	/* bad file handle */
            goto complete;
        }

        if(D) qDebug() << "!n" << tr("handle %1").arg(handle);

        device[cunit].status.err = 1;

        fpmode = iodesc[handle].fpmode;
        mtime = iodesc[handle].fpstat.st_mtime;

        strcpy(pathname, iodesc[handle].pathname);

        fps_close(handle);	/* this clears out iodesc[handle] */

        if (mtime && (fpmode & 0x08))
        {
            utimbuf ub;
            ub.actime = mtime;
            ub.modtime = mtime;
            utime(pathname,&ub);
        }
        goto complete;
    }

    if (fno == 0x08)	/* INIT */
    {
        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            device[cunit].status.err = 176;
            if(D) qDebug() << "!n" << tr("bad exec");
            goto complete;
        }

        do_pclink_init(0);

        device[cunit].parbuf.handle = 0xff;
        device[cunit].status.none = PCLINK_CDEVIC;
        device[cunit].status.err = 1;
        goto complete;
    }

    if ((fno == 0x09) || (fno == 0x0a))	/* FOPEN/FFIRST */
    {
        if (ccom == 'P')
        {
            if(D) qDebug() << "!n" << tr("mode: $%1, atr1: $%2, atr2: $%3, path: '%4', name: '%5'")
                .arg(device[cunit].parbuf.fmode,0,16).arg(device[cunit].parbuf.fatr1,0,16)
                .arg(device[cunit].parbuf.fatr2,0,16).arg(QString((const char*)device[cunit].parbuf.path))
                .arg(QString((const char*)device[cunit].parbuf.name));

            device[cunit].status.err = 1;

            if (fno == 0x0a)
                device[cunit].parbuf.fmode |= 0x10;
            goto complete;
        }
        else	/* ccom not 'P', execution stage */
        {
            DIR *dh;
            uchar i;
            long sl;
            struct stat tempstat;
            char newpath[PCL_MAX_DIR_LENGTH+1], raw_name[12];

            if ((ccom == 'R') && (old_ccom == 'R'))
            {
                sio->port()->writeCommandNak();
                if(D) qDebug() << "!n" << tr("serial communication error, abort");
                return;
            }

            sio->port()->writeCommandAck();	/* ack the command */

            memset(raw_name, 0, sizeof(raw_name));
            memcpy(raw_name, device[cunit].parbuf.name, 8+3);

            if (((device[cunit].parbuf.fmode & 0x0c) == 0) || \
                ((device[cunit].parbuf.fmode & 0x18) == 0x18))
            {
                if(D) qDebug() << "!n" << tr("unsupported fmode ($%1)").arg(device[cunit].parbuf.fmode);
                device[cunit].status.err = 146;
                goto complete_fopen;
            }

            create_user_path(cunit, newpath);

            if (!validate_user_path(device[cunit].dirname, newpath))
            {
                if(D) qDebug() << "!n" << tr("invalid path 1 '%1'").arg(newpath);
                device[cunit].status.err = 150;
                goto complete_fopen;
            }

            if(D) qDebug() << "!n" << tr("local path '%1'").arg(newpath);

            for (i = 0; i < 16; i++)
            {
                if (iodesc[i].fps.file == NULL)
                    break;
            }
            if (i > 15)
            {
                if(D) qDebug() << "!n" << tr("FOPEN: too many channels open");
                device[cunit].status.err = 161;
                goto complete_fopen;
            }

            if (stat(newpath, &tempstat) < 0)
            {
                if(D) qDebug() << "!n" << tr("FOPEN: cannot stat '%1'").arg(newpath);
                device[cunit].status.err = 150;
                goto complete_fopen;
            }

            dh = opendir(newpath);

            if(D) qDebug() << "!n" << tr("OPEN DIR");

            if (device[cunit].parbuf.fmode & 0x10)
            {
                iodesc[i].fps.dir = dh;
                memcpy(&sb, &tempstat, sizeof(sb));
            }
            else
            {
                if(D) qDebug() << "!n" << tr(" ! fmode & 0x10");
                while ((dp = readdir(dh)) != NULL)
                {
                    if(D) qDebug() << "!n" << tr("CHECK DOS NAME");

                    if (check_dos_name(newpath, dp, &sb))
                        continue;

                    /* convert 8+3 to NNNNNNNNXXX */
                    ugefina(dp->d_name, raw_name);

                    /* match */
                    if (match_dos_names(raw_name, \
                        (char *)device[cunit].parbuf.name, \
                            device[cunit].parbuf.fatr1, &sb) == 0)
                        break;
                }

                sl = strlen(newpath);
                if (sl && (newpath[sl-1] != '/'))
                    strcat(newpath, "/");

                if (dp)
                {
                    strcat(newpath, dp->d_name);
                    ugefina(dp->d_name, raw_name);
                    if ((device[cunit].parbuf.fmode & 0x0c) == 0x08)
                        sb.st_mtime = timestamp2mtime(&device[cunit].parbuf.f1);
                }
                else
                {
                    if ((device[cunit].parbuf.fmode & 0x0c) == 0x04)
                    {
                        if(D) qDebug() << "!n" << tr("FOPEN: file not found");
                        device[cunit].status.err = 170;
                        closedir(dh);
                        dp = NULL;
                        goto complete_fopen;
                    }
                    else
                    {
                        char name83[12];

                        if(D) qDebug() << "!n" << tr("FOPEN: creating file");

                        uexpand(device[cunit].parbuf.name, name83);

                        if (validate_dos_name(name83))
                        {
                            if(D) qDebug() << "!n" << tr("FOPEN: bad filename '%1'").arg(name83);
                            device[cunit].status.err = 165; /* bad filename */
                            goto complete_fopen;
                        }

                        strcat(newpath, name83);
                        ugefina(name83, raw_name);

                        memset(&sb, 0, sizeof(struct stat));
                        sb.st_mode = S_IFREG|S_IRUSR|S_IWUSR;

                        sb.st_mtime = timestamp2mtime(&device[cunit].parbuf.f1);
                    }
                }

                if(D) qDebug() << "!n" << tr("FOPEN: full local path '%1'").arg(newpath);

                if (stat(newpath, &tempstat) < 0)
                {
                    if ((device[cunit].parbuf.fmode & 0x0c) == 0x04)
                    {
                        if(D) qDebug() << "!n" << tr("FOPEN: cannot stat '%1'").arg(newpath);
                        device[cunit].status.err = 170;
                        goto complete_fopen;
                    }
                }
                else
                {
                    if (device[cunit].parbuf.fmode & 0x08)
                    {
                        if ((tempstat.st_mode & S_IWUSR) == 0)
                        {
                            if(D) qDebug() << "!n" << tr("FOPEN: '%1' is read-only").arg(newpath);
                            device[cunit].status.err = 151;
                            goto complete_fopen;
                        }
                    }
                }

                if ((device[cunit].parbuf.fmode & 0x0d) == 0x04)
                    iodesc[i].fps.file = fopen(newpath, "rb");
                else if ((device[cunit].parbuf.fmode & 0x0d) == 0x08)
                {
                    iodesc[i].fps.file = fopen(newpath, "wb");
                    if (iodesc[i].fps.file)
                        sb.st_size = 0;
                }
                else if ((device[cunit].parbuf.fmode & 0x0d) == 0x09)
                {
                    iodesc[i].fps.file = fopen(newpath, "rb+");
                    if (iodesc[i].fps.file)
                        fseek(iodesc[i].fps.file, sb.st_size, SEEK_SET);
                }
                else if ((device[cunit].parbuf.fmode & 0x0d) == 0x0c)
                    iodesc[i].fps.file = fopen(newpath, "rb+");

                closedir(dh);
                dp = NULL;
            }

            if (iodesc[i].fps.file == NULL)
            {
                if(D) qDebug() << "!n" << tr("FOPEN: cannot open '%1', %2 (%3)").arg(newpath).arg(strerror(errno)).arg(errno);
                if (device[cunit].parbuf.fmode & 0x04)
                    device[cunit].status.err = 170;
                else
                    device[cunit].status.err = 151;
                goto complete_fopen;
            }

            handle = device[cunit].parbuf.handle = i;

            iodesc[handle].devno = devno;
            iodesc[handle].cunit = cunit;
            iodesc[handle].fpmode = device[cunit].parbuf.fmode;
            iodesc[handle].fatr1 = device[cunit].parbuf.fatr1;
            iodesc[handle].fatr2 = device[cunit].parbuf.fatr2;
            iodesc[handle].t1 = device[cunit].parbuf.f1;
            iodesc[handle].t2 = device[cunit].parbuf.f2;
            iodesc[handle].t3 = device[cunit].parbuf.f3;
            iodesc[handle].d1 = device[cunit].parbuf.f4;
            iodesc[handle].d2 = device[cunit].parbuf.f5;
            iodesc[handle].d3 = device[cunit].parbuf.f6;
            iodesc[handle].fppos = 0L;
            strcpy(iodesc[handle].pathname, newpath);
            memcpy((void *)&iodesc[handle].fpstat, (void *)&sb, sizeof(struct stat));
            if (iodesc[handle].fpmode & 0x10)
                memcpy(iodesc[handle].fpname, device[cunit].parbuf.name, sizeof(iodesc[i].fpname));
            else
                memcpy(iodesc[handle].fpname, raw_name, sizeof(iodesc[handle].fpname));

            iodesc[handle].fpstat.st_size = get_file_len(handle);

            if ((iodesc[handle].fpmode & 0x1d) == 0x09)
                iodesc[handle].fppos = iodesc[handle].fpstat.st_size;

            memset(pcl_dbf.dirbuf, 0, sizeof(pcl_dbf.dirbuf));

            if ((handle > 15) || (iodesc[handle].fps.file == NULL))
            {
                if(D) qDebug() << "!n" << tr("FOPEN: bad handle 7 %1").arg(handle);
                device[cunit].status.err = 134;	/* bad file handle */
                pcl_dbf.handle = 134;
            }
            else
            {
                pcl_dbf.handle = handle;

                unix_time_2_sdx(&iodesc[handle].fpstat.st_mtime, ob);

                if(D) qDebug() << "!n" << tr("FOPEN: %1 handle %2").arg((iodesc[handle].fpmode & 0x08) ? "write" : "read").arg(handle);

                memset(pcl_dbf.dirbuf, 0, sizeof(pcl_dbf.dirbuf));

                if (iodesc[handle].fpmode & 0x10)
                {
                    int eof_sig;

                    iodesc[handle].dir_cache = cache_dir(handle);
                    iodesc[handle].fppos += dir_read(pcl_dbf.dirbuf, sizeof(pcl_dbf.dirbuf), handle, &eof_sig);

                    if (eof_sig)
                    {
                        if(D) qDebug() << "!n" << tr("FOPEN: dir EOF?");
                        device[cunit].status.err = 136;
                    }
                    else if (iodesc[handle].fppos == iodesc[handle].fpstat.st_size)
                            device[cunit].status.err = 3;
                }
                else
                {
                    int x;
                    ulong dlen = iodesc[handle].fpstat.st_size;

                    memset(pcl_dbf.dirbuf+6, 0x20, 11);
                    pcl_dbf.dirbuf[3] = (uchar)(dlen & 0x000000ffL);
                    pcl_dbf.dirbuf[4] = (uchar)((dlen & 0x0000ff00L) >> 8);
                    pcl_dbf.dirbuf[5] = (uchar)((dlen & 0x00ff0000L) >> 16);
                    memcpy(pcl_dbf.dirbuf+17, ob, 6);

                    pcl_dbf.dirbuf[0] = 0x08;

                    if ((iodesc[handle].fpstat.st_mode & S_IWUSR) == 0)
                        pcl_dbf.dirbuf[0] |= 0x01;	/* protected */
                    if (S_ISDIR(iodesc[handle].fpstat.st_mode))
                        pcl_dbf.dirbuf[0] |= 0x20;	/* directory */

                    x = 0;
                    while (iodesc[handle].fpname[x] && (x < 11))
                    {
                        pcl_dbf.dirbuf[6+x] = iodesc[handle].fpname[x];
                        x++;
                    }
                }

                if(D) qDebug() << "!n" << tr("FOPEN: send %1, send $%2 $%3%4 $%5%6%7 %8%9%10%11%12%13%14%15%16%17%18 %19-%20-%21 %22:%23:%24")
                    .arg(pcl_dbf.handle)
                    .arg(pcl_dbf.dirbuf[0],0,16)
                    .arg(pcl_dbf.dirbuf[2],0,16).arg(pcl_dbf.dirbuf[1],0,16)
                    .arg(pcl_dbf.dirbuf[5]).arg(pcl_dbf.dirbuf[4]).arg(pcl_dbf.dirbuf[3])
                    .arg(pcl_dbf.dirbuf[6]).arg(pcl_dbf.dirbuf[7]).arg(pcl_dbf.dirbuf[8]).arg(pcl_dbf.dirbuf[9])
                    .arg(pcl_dbf.dirbuf[10]).arg(pcl_dbf.dirbuf[11]).arg(pcl_dbf.dirbuf[12]).arg(pcl_dbf.dirbuf[13])
                    .arg(pcl_dbf.dirbuf[14]).arg(pcl_dbf.dirbuf[15]).arg(pcl_dbf.dirbuf[16])
                    .arg(pcl_dbf.dirbuf[17]).arg(pcl_dbf.dirbuf[18]).arg(pcl_dbf.dirbuf[19])
                    .arg(pcl_dbf.dirbuf[20]).arg(pcl_dbf.dirbuf[21]).arg(pcl_dbf.dirbuf[22]);
            }

complete_fopen:

            QByteArray data((const char *)&pcl_dbf,sizeof(pcl_dbf));
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(data);

            goto exit;
        }
    }

    if (fno == 0x0b)	/* RENAME/RENDIR */
    {
        char newpath[PCL_MAX_DIR_LENGTH+1];
        DIR *renamedir;
        ulong fcnt = 0;

        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            device[cunit].status.err = 176;
            if(D) qDebug() << "!n" << tr("bad exec");
            goto complete;
        }

        create_user_path(cunit, newpath);

        if (!validate_user_path(device[cunit].dirname, newpath))
        {
            if(D) qDebug() << "!n" << tr("invalid path 2 '%1'").arg(newpath);
            device[cunit].status.err = 150;
            goto complete;
        }

        renamedir = opendir(newpath);

        if (renamedir == NULL)
        {
            if(D) qDebug() << "!n" << tr("cannot open dir '%1'").arg(newpath);
            device[cunit].status.err = 255;
            goto complete;
        }

        if(D) qDebug() << "!n" << tr("local path '%1', fatr1 $%2").arg(newpath)
            .arg(device[cunit].parbuf.fatr1 | RA_NO_PROTECT,0,16);

        device[cunit].status.err = 1;

        while ((dp = readdir(renamedir)) != NULL)
        {
            char raw_name[12];

            if (check_dos_name(newpath, dp, &sb))
                continue;

            /* convert 8+3 to NNNNNNNNXXX */
            ugefina(dp->d_name, raw_name);

            /* match */
            if (match_dos_names(raw_name, (char *)device[cunit].parbuf.name, \
                device[cunit].parbuf.fatr1 | RA_NO_PROTECT, &sb) == 0)
            {
                char xpath[PCL_MAX_DIR_LENGTH+1], xpath2[PCL_MAX_DIR_LENGTH+1], newname[16];
                uchar names[12];
                struct stat dummy;
                ushort x;

                fcnt++;

                strcpy(xpath, newpath);
                strcat(xpath, "/");
                strcat(xpath, dp->d_name);

                memcpy(names, device[cunit].parbuf.names, 12);

                for (x = 0; x < 12; x++)
                {
                    if (names[x] == '?')
                        names[x] = raw_name[x];
                }

                uexpand(names, newname);

                strcpy(xpath2, newpath);
                strcat(xpath2, "/");
                strcat(xpath2, newname);

                if(D) qDebug() << "!n" << tr("RENAME: renaming '%1' -> '%2'").arg(dp->d_name).arg(newname);

                if (stat(xpath2, &dummy) == 0)
                {
                    if(D) qDebug() << "!n" << tr("RENAME: '%1' already exists").arg(xpath2);
                    device[cunit].status.err = 151;
                    break;
                }

                if (rename(xpath, xpath2))
                {
                    if(D) qDebug() << "!n" << tr("RENAME: %1").arg(strerror(errno));
                    device[cunit].status.err = 255;
                }
            }
        }

        closedir(renamedir);

        if ((fcnt == 0) && (device[cunit].status.err == 1))
            device[cunit].status.err = 170;
        goto complete;
    }

    if (fno == 0x0c)	/* REMOVE */
    {
        char newpath[PCL_MAX_DIR_LENGTH+1];
        DIR *deldir;
        ulong delcnt = 0;

        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            device[cunit].status.err = 176;
            if(D) qDebug() << "!n" << tr("bad exec");
            goto complete;
        }

        create_user_path(cunit, newpath);

        if (!validate_user_path(device[cunit].dirname, newpath))
        {
            if(D) qDebug() << "!n" << tr("invalid path 3 '%1'").arg(newpath);
            device[cunit].status.err = 150;
            goto complete;
        }

        if(D) qDebug() << "!n" << tr("local path '%1'").arg(newpath);

        deldir = opendir(newpath);

        if (deldir == NULL)
        {
            if(D) qDebug() << "!n" << tr("cannot open dir '%1'").arg(newpath);
            device[cunit].status.err = 255;
            goto complete;
        }

        device[cunit].status.err = 1;

        while ((dp = readdir(deldir)) != NULL)
        {
            char raw_name[12];

            if (check_dos_name(newpath, dp, &sb))
                continue;

            /* convert 8+3 to NNNNNNNNXXX */
            ugefina(dp->d_name, raw_name);

            /* match */
            if (match_dos_names(raw_name, (char *)device[cunit].parbuf.name, \
                RA_NO_PROTECT | RA_NO_SUBDIR | RA_NO_HIDDEN, &sb) == 0)
            {
                char xpath[PCL_MAX_DIR_LENGTH+1];

                strcpy(xpath, newpath);
                strcat(xpath, "/");
                strcat(xpath, dp->d_name);

                if (!S_ISDIR(sb.st_mode))
                {
                    if(D) qDebug() << "!n" << tr("REMOVE: delete '%1'").arg(xpath);
                    if (unlink(xpath))
                    {
                        if(D) qDebug() << "!n" << tr("REMOVE: cannot delete '%1'").arg(xpath);
                        device[cunit].status.err = 255;
                    }
                    delcnt++;
                }
            }
        }
        closedir(deldir);
        if (delcnt == 0)
            device[cunit].status.err = 170;
        goto complete;
    }

    if (fno == 0x0d)	/* CHMOD */
    {
        char newpath[PCL_MAX_DIR_LENGTH+1];
        DIR *chmdir;
        ulong fcnt = 0;
        uchar fatr2 = device[cunit].parbuf.fatr2;

        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            device[cunit].status.err = 176;
            if(D) qDebug() << "!n" << tr("bad exec");
            goto complete;
        }

        if (fatr2 & (SA_SUBDIR | SA_UNSUBDIR))
        {
            if(D) qDebug() << "!n" << tr("illegal fatr2 $%1").arg(fatr2);
            device[cunit].status.err = 146;
            goto complete;
        }

        create_user_path(cunit, newpath);

        if (!validate_user_path(device[cunit].dirname, newpath))
        {
            if(D) qDebug() << "!n" << tr("invalid path 4 '%1'").arg(newpath);
            device[cunit].status.err = 150;
            goto complete;
        }

        if(D) qDebug() << "!n" << tr("local path '%1', fatr1 $%2 fatr2 $%3").arg(newpath)
                .arg(device[cunit].parbuf.fatr1,0,16).arg(fatr2);

        chmdir = opendir(newpath);

        if (chmdir == NULL)
        {
            if(D) qDebug() << "!n" << tr("CHMOD: cannot open dir '%1'").arg(newpath);
            device[cunit].status.err = 255;
            goto complete;
        }


        device[cunit].status.err = 1;

        while ((dp = readdir(chmdir)) != NULL)
        {
            char raw_name[12];

            if (check_dos_name(newpath, dp, &sb))
                continue;

            /* convert 8+3 to NNNNNNNNXXX */
            ugefina(dp->d_name, raw_name);

            /* match */
            if (match_dos_names(raw_name, (char *)device[cunit].parbuf.name, \
                device[cunit].parbuf.fatr1, &sb) == 0)
            {
                char xpath[PCL_MAX_DIR_LENGTH+1];
                mode_t newmode = sb.st_mode;

                strcpy(xpath, newpath);
                strcat(xpath, "/");
                strcat(xpath, dp->d_name);
                if(D) qDebug() << "!n" << tr("CHMOD: change atrs in '%1'").arg(xpath);

                /* On Unix, ignore Hidden and Archive bits */
                if (fatr2 & SA_UNPROTECT)
                    newmode |= S_IWUSR;
                if (fatr2 & SA_PROTECT)
                    newmode &= ~S_IWUSR;
                if (chmod(xpath, newmode))
                {
                    if(D) qDebug() << "!n" << tr("CHMOD: failed on '%1'").arg(xpath);
                    device[cunit].status.err |= 255;
                }
                fcnt++;
            }
        }
        closedir(chmdir);
        if (fcnt == 0)
            device[cunit].status.err = 170;
        goto complete;
    }

    if (fno == 0x0e)	/* MKDIR - warning, fatr2 is bogus */
    {
        char newpath[PCL_MAX_DIR_LENGTH+1], fname[12];
        uchar dt[6];
        struct stat dummy;

        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            device[cunit].status.err = 176;
            if(D) qDebug() << "!n" << tr("bad exec");
            goto complete;
        }

        create_user_path(cunit, newpath);

        if (!validate_user_path(device[cunit].dirname, newpath))
        {
            if(D) qDebug() << "!n" << tr("invalid path 5 '%1'").arg(newpath);
            device[cunit].status.err = 150;
            goto complete;
        }

        uexpand(device[cunit].parbuf.name, fname);

        if (validate_dos_name(fname))
        {
            if(D) qDebug() << "!n" << tr("bad dir name '%1'").arg(fname);
            device[cunit].status.err = 165;
            goto complete;
        }

        strcat(newpath, "/");
        strcat(newpath, fname);

        memcpy(dt, &device[cunit].parbuf.f1, sizeof(dt));

        if(D) qDebug() << "!n" << tr("making dir '%1', time %2-%3-%4 %5:%6:%7").arg(newpath)
            .arg(dt[0]).arg(dt[1]).arg(dt[2]).arg(dt[3]).arg(dt[4]).arg(dt[5]);

        if (stat(newpath, &dummy) == 0)
        {
            if(D) qDebug() << "!n" << tr("MKDIR: '%1' already exists").arg(newpath);
            device[cunit].status.err = 151;
            goto complete;
        }

#if defined(Q_OS_LINUX) || defined(Q_OS_OSX)
        if (mkdir(newpath, S_IRWXU|S_IRWXG|S_IRWXO))
#else
        if (mkdir(newpath))
#endif
        {
            if(D) qDebug() << "!n" << tr("MKDIR: cannot make dir '%1'").arg(newpath);
            device[cunit].status.err = 255;
        }
        else
        {
            time_t mtime = timestamp2mtime(dt);
            device[cunit].status.err = 1;
            if (mtime)
            {
                utimbuf ub;
                ub.actime = mtime;
                ub.modtime = mtime;
                utime(newpath,&ub);
            }
        }
        goto complete;
    }

    if (fno == 0x0f)	/* RMDIR */
    {
        char newpath[PCL_MAX_DIR_LENGTH+1], fname[12];

        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            device[cunit].status.err = 176;
            if(D) qDebug() << "!n" << tr("bad exec");
            goto complete;
        }

        create_user_path(cunit, newpath);

        if (!validate_user_path(device[cunit].dirname, newpath))
        {
            if(D) qDebug() << "!n" << tr("invalid path 6 '%1'").arg(newpath);
            device[cunit].status.err = 150;
            goto complete;
        }

        uexpand(device[cunit].parbuf.name, fname);

        if (validate_dos_name(fname))
        {
            if(D) qDebug() << "!n" << tr("bad dir name '%1'").arg(fname);
            device[cunit].status.err = 165;
            goto complete;
        }

        strcat(newpath, "/");
        strcat(newpath, fname);

        if (stat(newpath, &sb) < 0)
        {
            if(D) qDebug() << "!n" << tr("cannot stat '%1'").arg(newpath);
            device[cunit].status.err = 170;
            goto complete;
        }

#if defined(Q_OS_LINUX) || defined(Q_OS_OSX)
        if (!access(newpath, W_OK))
        {
            if(D) qDebug() << "!n" << tr("'%1' can't be accessed").arg(newpath);
            device[cunit].status.err = 170;
            goto complete;
        }
#endif

        if (!S_ISDIR(sb.st_mode))
        {
            if(D) qDebug() << "!n" << tr("'%1' is not a directory").arg(newpath);
            device[cunit].status.err = 170;
            goto complete;
        }

        if ((sb.st_mode & S_IWUSR) == 0)
        {
            if(D) qDebug() << "!n" << tr("dir '%1' is write-protected").arg(newpath);
            device[cunit].status.err = 170;
            goto complete;
        }

        if(D) qDebug() << "!n" << tr("delete dir '%1'").arg(newpath);

        device[cunit].status.err = 1;

        if (rmdir(newpath))
        {
            if(D) qDebug() << "!n" << tr("RMDIR: cannot del '%1', %2 (%3)").arg(newpath).arg(strerror(errno)).arg(errno);
            if (errno == ENOTEMPTY)
                device[cunit].status.err = 167;
            else
                device[cunit].status.err = 255;
        }
        goto complete;
    }

    if (fno == 0x10)	/* CHDIR */
    {
        ulong i;
        char newpath[PCL_MAX_DIR_LENGTH+1], newwd[PCL_MAX_DIR_LENGTH+1], oldwd[PCL_MAX_DIR_LENGTH+1];

        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            device[cunit].status.err = 176;
            if(D) qDebug() << "!n" << tr("bad exec");
            goto complete;
        }

//		if(D) qDebug() << "!n" << tr(("req. path '%s'\n", device[cunit].parbuf.path);

        create_user_path(cunit, newpath);

        if (!validate_user_path(device[cunit].dirname, newpath))
        {
            if(D) qDebug() << "!n" << tr("invalid path 7 '%1'").arg(newpath);
            device[cunit].status.err = 150;
            goto complete;
        }

        (void)getcwd(oldwd, sizeof(oldwd));

        if (chdir(newpath))
        {
            if(D) qDebug() << "!n" << tr("cannot access '%1', %2").arg(newpath).arg(strerror(errno));
            device[cunit].status.err = 150;
            goto complete;
        }

        (void)getcwd(newwd, sizeof(newwd));

        /* validate_user_path() guarantees that .dirname is part of newwd */
        i = strlen(device[cunit].dirname);
        strcpy((char *)device[cunit].cwd, newwd + i);
        if(D) qDebug() << "!n" << tr("new current dir '%1'").arg((char *)device[cunit].cwd);

        device[cunit].status.err = 1;

        (void)chdir(oldwd);

        goto complete;
    }

    if (fno == 0x11)	/* GETCWD */
    {
        int i;
        uchar tempcwd[65];

        device[cunit].status.err = 1;

        if (ccom == 'P')
        {
            if(D) qDebug() << "!n" << tr("device $1").arg(cunit);
            goto complete;
        }

        sio->port()->writeCommandAck();	/* ack the command */

        tempcwd[0] = 0;

        for (i = 0; device[cunit].cwd[i] && (i < 64); i++)
        {
            uchar a;

            a = toupper(device[cunit].cwd[i]);
            if (a == '/')
                a = '>';
            tempcwd[i] = a;
        }

        tempcwd[i] = 0;

        if(D) qDebug() << "!n" << tr("send '%1'").arg((const char*)tempcwd);

        QByteArray data((const char *)tempcwd,sizeof(tempcwd)-1);
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);

        goto exit;
    }

    if (fno == 0x13)	/* DFREE */
    {
        FILE *vf;
        int x;
        uchar c = 0, volname[8];
        char lpath[PCL_MAX_DIR_LENGTH+1];
        static uchar dfree[65] =
        {
            0x21,		/* data format version */
            0x00, 0x00,	/* main directory ptr */
            0xff, 0xff, 	/* total sectors */
            0xff, 0xff,	/* free sectors */
            0x00,		/* bitmap length */
            0x00, 0x00,	/* bitmap begin */
            0x00, 0x00,	/* filef */
            0x00, 0x00,	/* dirf */
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,	/* volume name */
            0x00,		/* number of tracks */
            0x01,		/* bytes per sector, encoded */
            0x80,		/* version number */
            0x00, 0x02,	/* real bps */
            0x00, 0x00,	/* fmapen */
            0x01,		/* sectors per cluster */
            0x00, 0x00,	/* nr seq and rnd */
            0x00, 0x00,	/* bootp */
            0x00,		/* lock */

            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,
            0		/* CRC */
        };

        device[cunit].status.err = 1;

        if (ccom == 'P')
        {
            if(D) qDebug() << "!n" << tr("device $%1").arg(cunit);
            goto complete;
        }

        sio->port()->writeCommandAck();	/* ack the command */

        memset(dfree + 0x0e, 0x020, 8);

        strcpy(lpath, (char *)device[cunit].dirname);
        strcat(lpath, "/");
        strcat(lpath, DEVICE_LABEL);

        if(D) qDebug() << "!n" << tr("reading '%1'").arg(lpath);

        vf = fopen(lpath, "rb");

        if (vf)
        {
            int r;
            uchar a;

            r = fread(volname, sizeof (uchar), 8, vf);

            fclose(vf);

            for (x = 0; x < r; x++)
            {
                a = volname[x];
                if (a == 0x9b)
                    break;
                dfree[14+x] = a;
            }
        }

        x = 0;
        while (x < 8)
        {
            c |= dfree[14+x];
            x++;
        }

        if (c == 0x20)
        {
            memcpy(dfree + 14, "PCLink  ", 8);
            dfree[21] = cunit + 0x40;
        }

        if(D) qDebug() << "!n" << tr("DFREE: send info (%1 bytes)").arg((int)sizeof(dfree)-1);

        QByteArray data((const char *)dfree,sizeof(dfree)-1);
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);

        goto exit;
    }

    if (fno == 0x14)	/* CHVOL */
    {
        FILE *vf;
        ulong nl;
        char lpath[PCL_MAX_DIR_LENGTH+1];

        device[cunit].status.err = 1;

        if (ccom == 'R')
        {
            sio->port()->writeCommandAck();	/* ack the command */
            device[cunit].status.err = 176;
            if(D) qDebug() << "!n" << tr("bad exec");
            goto complete;
        }

        nl = strlen((char *)device[cunit].parbuf.name);

        if (nl == 0)
        {
            if(D) qDebug() << "!n" << tr("invalid name");
            device[cunit].status.err = 156;
            goto complete;
        }

        strcpy(lpath, device[cunit].dirname);
        strcat(lpath, "/");
        strcat(lpath, DEVICE_LABEL);

        if(D) qDebug() << "!n" << tr("writing '%1'").arg(lpath);

        vf = fopen(lpath, "wb");

        if (vf)
        {
            int x;
            uchar a;

            for (x = 0; x < 8; x++)
            {
                a = device[cunit].parbuf.name[x];
                if (!a || (a == 0x9b))
                    a = 0x20;
                (void)fwrite(&a, sizeof(uchar), 1, vf);
            }
            fclose(vf);
        }
        else
        {
            if(D) qDebug() << "!n" << tr("CHVOL: %1").arg(strerror(errno));
            device[cunit].status.err = 255;
        }
        goto complete;
    }

    if(D) qDebug() << "!n" << tr("fno $%1 not implemented").arg(fno,0,16);
    device[cunit].status.err = 146;

complete:
    sio->port()->writeComplete();

exit:
    old_ccom = ccom;

    return;
}

/*************************************************************************/

void PCLINK::fps_close(int i)
{
    if (iodesc[i].fps.file != NULL)
    {
        if (iodesc[i].fpmode & 0x10)
            closedir(iodesc[i].fps.dir);
        else
            fclose(iodesc[i].fps.file);
    }

    if (iodesc[i].dir_cache != NULL)
    {
        free(iodesc[i].dir_cache);
        iodesc[i].dir_cache = NULL;
    }

    iodesc[i].fps.file = NULL;

    iodesc[i].devno = 0;
    iodesc[i].cunit = 0;
    iodesc[i].fpmode = 0;
    iodesc[i].fatr1 = 0;
    iodesc[i].fatr2 = 0;
    iodesc[i].t1 = 0;
    iodesc[i].t2 = 0;
    iodesc[i].t3 = 0;
    iodesc[i].d1 = 0;
    iodesc[i].d2 = 0;
    iodesc[i].d3 = 0;
    iodesc[i].fpname[0] = 0;
    iodesc[i].fppos = 0;
    iodesc[i].fpread = 0;
    iodesc[i].eof = 0;
    iodesc[i].pathname[0] = 0;
    memset(&iodesc[i].fpstat, 0, sizeof(struct stat));
}

/*************************************************************************/

void PCLINK::set_status_size(uchar cunit, ushort size)
{
    device[cunit].status.tmot = (size & 0x00ff);
    device[cunit].status.none = (size & 0xff00) >> 8;
}

/*************************************************************************/

ulong PCLINK::dir_read(uchar *mem, ulong blk_size, uchar handle, int *eof_sig)
{
    uchar *db = (uchar *)iodesc[handle].dir_cache;
    ulong dirlen = iodesc[handle].fpstat.st_size, newblk;

    eof_sig[0] = 0;

    newblk = dirlen - iodesc[handle].fppos;

    if (newblk < blk_size)
    {
        blk_size = newblk;
        eof_sig[0] = 1;
    }

    if (blk_size)
        memcpy(mem, db+iodesc[handle].fppos, blk_size);

    return blk_size;
}

/*************************************************************************/

int PCLINK::match_dos_names(char *name, char *mask, uchar fatr1, struct stat *sb)
{
    if(D) qDebug() << "!n" << tr("match: %1%2%3%4%5%6%7%8%9%10%11 with %12%13%14%15%16%17%18%19%20%21%22: ")
    .arg(name[0]).arg(name[1]).arg(name[2]).arg(name[3]).arg(name[4]).arg(name[5]).arg(name[6]).arg(name[7])
    .arg(name[8]).arg(name[9]).arg(name[10])
    .arg(mask[0]).arg(mask[1]).arg(mask[2]).arg(mask[3]).arg(mask[4]).arg(mask[5]).arg(mask[6]).arg(mask[7])
    .arg(mask[8]).arg(mask[9]).arg(mask[10]);

    ushort i;

    for (i = 0; i < 11; i++)
    {
        if (mask[i] != '?')
            if (toupper((uchar)name[i]) != toupper((uchar)mask[i]))
            {
                if(D) qDebug() << "!n" << tr("no match");
                return 1;
            }
    }

    /* There are no such attributes in Unix */
    fatr1 &= ~(RA_NO_HIDDEN|RA_NO_ARCHIVED);

    /* Now check the attributes */
    if (fatr1 & (RA_HIDDEN | RA_ARCHIVED))
    {
        if(D) qDebug() << "!n" << tr("atr mismatch: not HIDDEN or ARCHIVED");
        return 1;
    }

    if (fatr1 & RA_PROTECT)
    {
        if (sb->st_mode & S_IWUSR)
        {
            if(D) qDebug() << "!n" << tr("atr mismatch: not PROTECTED");
            return 1;
        }
    }

    if (fatr1 & RA_NO_PROTECT)
    {
        if ((sb->st_mode & S_IWUSR) == 0)
        {
            if(D) qDebug() << "!n" << tr("atr mismatch: not UNPROTECTED");
            return 1;
        }
    }

    if (fatr1 & RA_SUBDIR)
    {
        if (!S_ISDIR(sb->st_mode))
        {
            if(D) qDebug() << "!n" << tr("atr mismatch: not SUBDIR");
            return 1;
        }
    }

    if (fatr1 & RA_NO_SUBDIR)
    {
        if (S_ISDIR(sb->st_mode))
        {
            if(D) qDebug() << "!n" << tr("atr mismatch: not FILE");
            return 1;
        }
    }

    if(D) qDebug() << "!n" << tr("match");
    return 0;
}

/*************************************************************************/

void PCLINK::create_user_path(uchar cunit, char *newpath)
{
    long sl, cwdo = 0;
    uchar lpath[128], upath[128];

    strcpy(newpath, device[cunit].dirname);

    /* this is user-requested new path */
    path_copy(lpath, device[cunit].parbuf.path);
    path2unix(upath, lpath);

    if (upath[0] != '/')
    {
        sl = strlen(newpath);
        if (sl && (newpath[sl-1] != '/'))
            strcat(newpath, "/");
        if (device[cunit].cwd[0] == '/')
            cwdo++;
        strcat(newpath, (char *)device[cunit].cwd + cwdo);
        sl = strlen(newpath);
        if (sl && (newpath[sl-1] != '/'))
            strcat(newpath, "/");
    }
    strcat(newpath, (char *)upath);
    sl = strlen(newpath);
    if (sl && (newpath[sl-1] == '/'))
        newpath[sl-1] = 0;
}

/*************************************************************************/

/* defwd - a mounted directory
 * newpath - ?
 */
int PCLINK::validate_user_path(char *defwd, char *newpath)
{
    char *d, oldwd[PCL_MAX_DIR_LENGTH+1], newwd[PCL_MAX_DIR_LENGTH+1];

    (void)getcwd(oldwd, sizeof(oldwd));
    if (chdir(newpath) < 0)
        return 0;
    (void)getcwd(newwd, sizeof(newwd));
    (void)chdir(oldwd);

    d = strstr(newwd, defwd);

    if (d == NULL)
        return 0;
    if (d != newwd)
        return 0;

    return 1;
}

/*************************************************************************/

int PCLINK::check_dos_name(char *newpath, struct dirent *dp, struct stat *sb)
{
    char temp_fspec[PCL_MAX_DIR_LENGTH+1], fname[256];

    strcpy(fname, dp->d_name);

    if(D) qDebug() << "!n" << tr("%1: got fname '%2'").arg(__extension__ __FUNCTION__).arg(fname);

    if (validate_dos_name(fname))
        return 1;

    /* stat() the file (fetches the length) */
    sprintf(temp_fspec, "%s/%s", newpath, fname);

    if(D) qDebug() << "!n" << tr("%1: stat '%2'").arg(__extension__ __FUNCTION__).arg(fname);

    if (stat(temp_fspec, sb))
        return 1;

    if (!S_ISREG(sb->st_mode) && !S_ISDIR(sb->st_mode))
        return 1;

#if defined(Q_OS_LINUX) || defined(Q_OS_OSX)
    if (sb->st_mode == S_IFLNK) {
        if (D) qDebug() << "!n" << tr("'%1': is a symlink").arg(temp_fspec);
    }

    if (access(temp_fspec, R_OK) < 0)	{	/* belongs to us? */
        if (D) qDebug() << "!n" << tr("'%1': can't be accessed").arg(temp_fspec);
        if (D) qDebug() << "!n" << tr("access error code %1").arg(errno);
        return 1;
    }
#endif

    if ((sb->st_mode & S_IRUSR) == 0)	/* unreadable? */
        return 1;

    if (S_ISDIR(sb->st_mode) && ((sb->st_mode & S_IXUSR) == 0))
        return 1;

    return 0;
}

/*************************************************************************/

void PCLINK::ugefina(char *src, char *out)
{
    char *dot;
    ushort i;

    memset(out, 0x20, 8+3);

    dot = strchr(src, '.');

    if (dot)
    {
        i = 1;
        while (dot[i] && (i < 4))
        {
            out[i+7] = toupper((uchar)dot[i]);
            i++;
        }
    }

    i = 0;
    while ((src[i] != '.') && !dos_2_term(src[i]) && (i < 8))
    {
        out[i] = toupper((uchar)src[i]);
        i++;
    }
}

/*************************************************************************/

time_t PCLINK::timestamp2mtime(uchar *stamp)
{
    struct tm sdx_tm;

    memset(&sdx_tm, 0, sizeof(struct tm));

    sdx_tm.tm_sec = stamp[5];
    sdx_tm.tm_min = stamp[4];
    sdx_tm.tm_hour = stamp[3];
    sdx_tm.tm_mday = stamp[0];
    sdx_tm.tm_mon = stamp[1];
    sdx_tm.tm_year = stamp[2];

    if ((sdx_tm.tm_mday == 0) || (sdx_tm.tm_mon == 0))
        return 0;

    if (sdx_tm.tm_mon)
        sdx_tm.tm_mon--;

    if (sdx_tm.tm_year < 80)
        sdx_tm.tm_year += 2000;
    else
        sdx_tm.tm_year += 1900;

    sdx_tm.tm_year -= 1900;

    return mktime(&sdx_tm);
}

/*************************************************************************/

void PCLINK::uexpand(uchar *rawname, char *name83)
{
    ushort x, y;
    uchar t;

    name83[0] = 0;

    for (x = 0; x < 8; x++)
    {
        t = rawname[x];
        if (t && (t != 0x20))
            name83[x] = upper_dir ? toupper(t) : tolower(t);
        else
            break;
    }

    y = 8;

    if (rawname[y] && (rawname[y] != 0x20))
    {
        name83[x] = '.';
        x++;

        while ((y < 11) && rawname[y] && (rawname[y] != 0x20))
        {
            name83[x] = upper_dir ? toupper(rawname[y]) : tolower(rawname[y]);
            x++;
            y++;
        }
    }

    name83[x] = 0;
}

/*************************************************************************/

int PCLINK::validate_dos_name(char *fname)
{
    char *dot = strchr(fname, '.');
    long valid_fn, valid_xx;

    if ((dot == NULL) && (strlen(fname) > 8))
        return 1;
    if (dot)
    {
        long dd = strlen(dot);

        if (dd > 4)
            return 1;
        if ((dot - fname) > 8)
            return 1;
        if ((dot == fname) && (dd == 1))
            return 1;
        if ((dd == 2) && (dot[1] == '.'))
            return 1;
        if ((dd == 3) && ((dot[1] == '.') || (dot[2] == '.')))
            return 1;
        if ((dd == 4) && ((dot[1] == '.') || (dot[2] == '.') || (dot[3] == '.')))
            return 1;
    }

    valid_fn = validate_fn((uchar *)fname, 8);
    if (dot != NULL)
        valid_xx = validate_fn((uchar *)(dot + 1), 3);
    else
        valid_xx = 1;

    if (!valid_fn || !valid_xx)
        return 1;

    return 0;
}

/*************************************************************************/

ulong PCLINK::get_file_len(uchar handle)
{
    ulong filelen;
    struct dirent *dp;
    struct stat sb;

    if (iodesc[handle].fpmode & 0x10)	/* directory */
    {
        rewinddir(iodesc[handle].fps.dir);
        filelen = sizeof(DIRENTRY);

        while ((dp = readdir(iodesc[handle].fps.dir)) != NULL)
        {
            if (check_dos_name(iodesc[handle].pathname, dp, &sb))
                continue;
            filelen += sizeof(DIRENTRY);
        }
        rewinddir(iodesc[handle].fps.dir);
    }
    else
        filelen = iodesc[handle].fpstat.st_size;

    if (filelen > SDX_MAXLEN)
        filelen = SDX_MAXLEN;

    return filelen;
}

/*************************************************************************/

void PCLINK::unix_time_2_sdx(time_t *todp, uchar *ob)
{
    struct tm *t;
    uchar yy;

    memset(ob, 0, 6);

    if (*todp == 0)
        return;

    t = localtime(todp);

    yy = t->tm_year;
    while (yy >= 100)
        yy-=100;

    ob[0] = t->tm_mday;
    ob[1] = t->tm_mon + 1;
    ob[2] = yy;
    ob[3] = t->tm_hour;
    ob[4] = t->tm_min;
    ob[5] = t->tm_sec;
}

/*************************************************************************/

DIRENTRY * PCLINK::cache_dir(uchar handle)
{
    char *bs, *cwd;
    uchar dirnode = 0x00;
    ushort node;
    ulong dlen, flen, sl, dirlen = iodesc[handle].fpstat.st_size;
    DIRENTRY *dbuf, *dir;
    struct dirent *dp;
    struct stat sb;

    if (iodesc[handle].dir_cache != NULL)
    {
        if(D) qDebug() << "!n" << tr("Internal error: dir_cache should be NULL!");
        free(iodesc[handle].dir_cache);
        iodesc[handle].dir_cache = NULL;
    }

    dir = dbuf = (DIRENTRY*)malloc(dirlen + sizeof(DIRENTRY));
    memset(dbuf, 0, dirlen + sizeof(DIRENTRY));

    dir->status = 0x28;
    dir->map_l = 0x00;			/* low 11 bits: file number, high 5 bits: dir number */
    dir->map_h = dirnode;
    dir->len_l = dirlen & 0x000000ffL;
    dir->len_m = (dirlen & 0x0000ff00L) >> 8;
    dir->len_h = (dirlen & 0x00ff0000L) >> 16;

    memset(dir->fname, 0x20, 11);

    sl = strlen(device[iodesc[handle].cunit].dirname);

    cwd = iodesc[handle].pathname + sl;

    bs = strrchr(cwd, '/');

    if (bs == NULL)
        memcpy(dir->fname, "MAIN", 4);
    else
    {
        char *cp = cwd;

        ugefina(bs+1, (char *)dir->fname);

        node = 0;

        while (cp <= bs)
        {
            if (*cp == '/')
                dirnode++;
            cp++;
        }

        dir->map_h = (dirnode & 0x1f) << 3;
    }

    unix_time_2_sdx(&iodesc[handle].fpstat.st_mtime, dir->stamp);

    dir++;
    flen = sizeof(DIRENTRY);

    node = 1;

    while ((dp = readdir(iodesc[handle].fps.dir)) != NULL)
    {
        ushort map;

        if (check_dos_name(iodesc[handle].pathname, dp, &sb))
            continue;

        dlen = sb.st_size;
        if (dlen > SDX_MAXLEN)
            dlen = SDX_MAXLEN;

        dir->status = (sb.st_mode & S_IWUSR) ? 0x08 : 0x09;

        if (S_ISDIR(sb.st_mode))
        {
            dir->status |= 0x20;		/* directory */
            dlen = sizeof(DIRENTRY);
        }

        map = dirnode << 11;
        map |= (node & 0x07ff);

        dir->map_l = map & 0x00ff;
        dir->map_h = ((map & 0xff00) >> 8);
        dir->len_l = dlen & 0x000000ffL;
        dir->len_m = (dlen & 0x0000ff00L) >> 8;
        dir->len_h = (dlen & 0x00ff0000L) >> 16;

        ugefina(dp->d_name, (char *)dir->fname);

        unix_time_2_sdx(&sb.st_mtime, dir->stamp);

        node++;
        dir++;
        flen += sizeof(DIRENTRY);

        if (flen >= dirlen)
            break;
    }

    return dbuf;
}

/*************************************************************************/

void PCLINK::path_copy(uchar *dst, uchar *src)
{
    uchar a;

    while (*src)
    {
        a = *src;
        if (ispathsep(a))
        {
            while (ispathsep(*src))
                src++;
            src--;
        }
        *dst = a;
        src++;
        dst++;
    }

    *dst = 0;
}

/*************************************************************************/

// from SPARTA DOS file system to unix file system
void PCLINK::path2unix(uchar *out, uchar *path)
{
    int i, y = 0;

    for (i = 0; path[i] && (i < 64); i++)
    {
        char a;

        a = upper_dir ? toupper(path[i]) : tolower(path[i]);

        if (ispathsep(a))
            a = '/';
        else if (a == '<')
        {
            a = '.';
            out[y++] = '.';
        }
        out[y++] = a;
    }

    if (y && (out[y-1] != '/'))
        out[y++] = '/';

    out[y] = 0;
}

/*************************************************************************/

long PCLINK::dos_2_term(uchar c)
{
    return ((c == 0) || (c == 0x20));
}

long PCLINK::validate_fn(uchar *name, int len)
{
    int x;

    for (x = 0; x < len; x++)
    {
        if (dos_2_term(name[x]))
            return (x != 0);
        if (name[x] == '.')
            return 1;
        if (!dos_2_allowed(name[x]))
            return 0;
    }

    return 1;
}

/*************************************************************************/

/* is it a Sparta Dos path separator? */
int PCLINK::ispathsep(uchar c)
{
    return ((c == '>') || (c == '\\'));
}

/*************************************************************************/

long PCLINK::dos_2_allowed(uchar c)
{
#ifdef Q_OS_LINUX
    if (upper_dir)
        return (isupper(c) || isdigit(c) || (c == '_') || (c == '@'));

    return (islower(c) || isdigit(c) || (c == '_') || (c == '@'));
#else
    return (isalpha(c) || isdigit(c) || (c == '_') || (c == '@'));
#endif
}
