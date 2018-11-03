//
// disassembly810 class
// (c) 2018 Eric BACHER
//

#include "disassembly810.h"

struct labels {
    unsigned short address;
    char *label;
};

typedef labels LABELS;

static LABELS labels810[] = {
    { (unsigned short)0x0000, (char *)"FCOMMAND" },
    { (unsigned short)0x0001, (char *)"FTRACK" },
    { (unsigned short)0x0002, (char *)"FSECTOR" },
    { (unsigned short)0x0003, (char *)"FDATA" },
    { (unsigned short)0x0380, (char *)"DRA" },
    { (unsigned short)0x0381, (char *)"DDRA" },
    { (unsigned short)0x0382, (char *)"DRB" },
    { (unsigned short)0x0383, (char *)"DDRB" },
    { (unsigned short)0x0384, (char *)"EDGECTRL" },
    { (unsigned short)0x0394, (char *)"RTIMNOIT" },
    { (unsigned short)0x0395, (char *)"RINTFLAG" },
    { (unsigned short)0x0396, (char *)"WR64NO" },
    { (unsigned short)0x0397, (char *)"WR1024NO" },
    { (unsigned short)0x039C, (char *)"RTIMIT" },
    { (unsigned short)0x039E, (char *)"WR64IT" },
    { (unsigned short)0x039F, (char *)"WR1024IT" },
};

disassembly810::disassembly810()
    : Cpu6502(CPU_6502)
{
}

char *disassembly810::GetAddressLabel(unsigned short addr)
{
    for (unsigned int i = 0; i < sizeof(labels810) / sizeof(LABELS); i++) {
        if (labels810[i].address == addr) {
            return labels810[i].label;
        }
    }
    return (char *)0;
}
