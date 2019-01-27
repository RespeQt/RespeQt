//
// disassembly1050 class
// (c) 2018 Eric BACHER
//

#include "disassembly1050.h"

struct labels {
    unsigned short address;
    char *label;
};

typedef labels LABELS;

static LABELS labels1050[] = {
    { (unsigned short)0x0400, (char *)"FCOMMAND" },
    { (unsigned short)0x0401, (char *)"FTRACK" },
    { (unsigned short)0x0402, (char *)"FSECTOR" },
    { (unsigned short)0x0403, (char *)"FDATA" },
    { (unsigned short)0x0280, (char *)"DRA" },
    { (unsigned short)0x0281, (char *)"DDRA" },
    { (unsigned short)0x0282, (char *)"DRB" },
    { (unsigned short)0x0283, (char *)"DDRB" },
    { (unsigned short)0x0284, (char *)"EDGECTRL" },
    { (unsigned short)0x0294, (char *)"RTIMNOIT" },
    { (unsigned short)0x0295, (char *)"RINTFLAG" },
    { (unsigned short)0x0296, (char *)"WR64NO" },
    { (unsigned short)0x0297, (char *)"WR1024NO" },
    { (unsigned short)0x029C, (char *)"RTIMIT" },
    { (unsigned short)0x029E, (char *)"WR64IT" },
    { (unsigned short)0x029F, (char *)"WR1024IT" },
};

disassembly1050::disassembly1050()
    : Cpu6502(CPU_6502)
{
}

char *disassembly1050::GetAddressLabel(unsigned short addr)
{
    for (unsigned int i = 0; i < sizeof(labels1050) / sizeof(LABELS); i++) {
        if (labels1050[i].address == addr) {
            return labels1050[i].label;
        }
    }
    return (char *)0;
}
