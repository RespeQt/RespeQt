#ifndef SDXPROTOCOLS_H
#define SDXPROTOCOLS_H

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

class SDXProtocol : public SioDevice
{
    Q_OBJECT

public:
    SDXProtocol(SioWorkerPtr worker);
};

#endif // SDXPROTOCOLS_H
