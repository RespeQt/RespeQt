/*
 * pclink.h
 *
 * This file contains code from the SIO2BSD project by KMK (drac030)
 */

#ifndef PCLINK_H
#define PCLINK_H

#include "sioworker.h"

# ifndef uchar
#  define uchar unsigned char
# endif

# ifndef ushort
#  define ushort unsigned short
# endif

# ifndef ulong
#  define ulong unsigned long
# endif

typedef struct
{
    uchar status;
    uchar map_l, map_h;
    uchar len_l, len_m, len_h;
    char fname[11];
    uchar stamp[6];
} DIRENTRY;

class PCLINK: public SioDevice
{
    Q_OBJECT

public:
    PCLINK(SioWorker *worker);
    ~PCLINK();
    void handleCommand(quint8 command, quint16 aux);
    // links are numbered from 1 to 15
    bool hasLink(int no);
    void setLink(int no, const char* fileName);
    void swapLinks(int from, int to);
    void resetLink(int no);
private:
    void do_pclink_init(int force);
    void do_pclink(uchar devno, uchar ccom, uchar caux1, uchar caux2);
    void fps_close(int i);
    void set_status_size(uchar cunit, ushort size);
    ulong dir_read(uchar *mem, ulong blk_size, uchar handle, int *eof_sig);
    int match_dos_names(char *name, char *mask, uchar fatr1, struct stat *sb);
    void create_user_path(uchar cunit, char *newpath);
    int validate_user_path(char *defwd, char *newpath);
    int check_dos_name(char *newpath, struct dirent *dp, struct stat *sb);
    void ugefina(char *src, char *out);
    time_t timestamp2mtime(uchar *stamp);
    void uexpand(uchar *rawname, char *name83);
    int validate_dos_name(char *fname);
    ulong get_file_len(uchar handle);
    void unix_time_2_sdx(time_t *todp, uchar *ob);
    DIRENTRY * cache_dir(uchar handle);
    void path_copy(uchar *dst, uchar *src);
    void path2unix(uchar *out, uchar *path);
    long dos_2_term(uchar c);
    long validate_fn(uchar *name, int len);
    int ispathsep(uchar c);
    long dos_2_allowed(uchar c);
};

#endif // PCLINK_H
