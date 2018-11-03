//
// Rom class
// (c) 2016 Eric BACHER
//

#ifndef ROM_HPP
#define ROM_HPP 1

#include <QString>
#include <QByteArray>
#include "Chip.hpp"

struct ROM_SYMBOLS {
    char name[16];
    unsigned short address;
    unsigned short address2;
    unsigned char *displayed;
    struct ROM_SYMBOLS *next;
};

// This class emulates a ROM chip.
class Rom : public Chip {

private:

    // rom binary
    QString					m_romName;
    QString					m_romFilename;
    unsigned char			*m_firmware;
    int						m_length;
    Rom						*m_next;

    // rom symbols
    QString					m_symFilename;
    struct ROM_SYMBOLS		*m_symbols;
    struct ROM_SYMBOLS		*m_lastSkip;

protected:

    unsigned char			*LoadBinary(QString filename, int *size);
    struct ROM_SYMBOLS		*LoadSymbolFile(QString filename);

public:

    // constructor and destructor
                            Rom(QString romName);
    virtual					~Rom();

    // load files
    virtual bool			LoadRomFile(QString filename);

    // getters
    virtual QString			GetRomName() { return m_romName; }
    virtual Rom				*GetNext() { return m_next; }
    virtual int             GetLength() { return m_length; }

    // read/write Rom addresses
    virtual unsigned char	ReadRegister(unsigned short addr);
    virtual void			WriteRegister(unsigned short addr, unsigned char val);

    // get label for a given address
    virtual char			*GetAddressLabel(unsigned short addr);
	virtual bool			IsAddressSkipped(unsigned short addr);
    virtual bool			IsCommandActive(unsigned char command, unsigned short aux);
};

#endif
