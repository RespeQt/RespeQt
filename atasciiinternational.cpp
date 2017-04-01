#include "atasciiinternational.h"
#include "logdisplaydialog.h"

AtasciiInternational::AtasciiInternational()
    :Atascii()
{
    AtasciiInternational::initMapping();
}

void AtasciiInternational::initMapping()
{
    Atascii::initMapping();

    mapping[0x00] = QChar(0x00E1);
    mapping[0x01] = QChar(0x00F9);
    mapping[0x02] = QChar(0x00D1);
    mapping[0x03] = QChar(0x00C9);
    mapping[0x04] = QChar(0x00E7);
    mapping[0x05] = QChar(0x00F4);
    mapping[0x06] = QChar(0x00F2);
    mapping[0x07] = QChar(0x00EC);
    mapping[0x08] = QChar(0x00A3);
    mapping[0x09] = QChar(0x00EF);
    mapping[0x0A] = QChar(0x00FC);
    mapping[0x0B] = QChar(0x00E4);
    mapping[0x0C] = QChar(0x00D6);
    mapping[0x0D] = QChar(0x00FA);
    mapping[0x0E] = QChar(0x00F3);
    mapping[0x0F] = QChar(0x00F6);
    mapping[0x10] = QChar(0x00DC);
    mapping[0x11] = QChar(0x00E2);
    mapping[0x12] = QChar(0x00FB);
    mapping[0x13] = QChar(0x00EE);
    mapping[0x14] = QChar(0x00E9);
    mapping[0x15] = QChar(0x00E8);
    mapping[0x16] = QChar(0x00F1);
    mapping[0x17] = QChar(0x00EA);
    mapping[0x18] = QChar(0x0227);
    mapping[0x19] = QChar(0x00E0);
    mapping[0x1A] = QChar(0x0226);
    mapping[0x1C] = QChar(0x2191);
    mapping[0x1D] = QChar(0x2193);
    mapping[0x1E] = QChar(0x2190);
    mapping[0x1F] = QChar(0x2192);
    mapping[0x60] = QChar(0x0130);
    mapping[0x7B] = QChar(0x00C4);
}
