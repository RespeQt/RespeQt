//
// Rom class
// (c) 2016 Eric BACHER
//

#include <QFile>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "Rom.hpp"

Rom::Rom(QString romName)
{
    m_romName = romName;
    m_firmware = nullptr;
    m_symbols = nullptr;
}

Rom::~Rom()
{
	free(m_firmware);
    m_firmware = nullptr;
	struct ROM_SYMBOLS *head = m_symbols;
	while (head) {
		struct ROM_SYMBOLS *next = head->next;
        if (head->displayed) {
            free(head->displayed);
            head->displayed = nullptr;
        }
		free(head);
		head = next;
	}
    m_symbols = nullptr;
}

unsigned char *Rom::LoadBinary(QString filename, int *size)
{
    QFile *firmwareFile = new QFile(filename);
    if (!firmwareFile->open(QFile::ReadOnly)) {
        delete firmwareFile;
        return nullptr;
    }
    QByteArray array = firmwareFile->readAll();
    firmwareFile->close();
    delete firmwareFile;
    *size = array.size();
    unsigned char *content = (unsigned char *)malloc(*size);
    memcpy(content, array.data(), *size);
    return content;
}

/*
 * syntax:
 *   ; comment
 *   label1=$1234 ; another comment
 *   label2=$AB ; zero-page address
 *   #skip=$1022-$1028 ; do not disassemble instructions if address is between $1022 and $1028 (inclusive)
 *   #command=$50 ; disassembly active when command $50 is received and executed
 *   #command=$57 ; disassembly active when command $57 is received and executed
 *   #frame=$57-$0001 ; disassembly active when command $57 is received and executed with $0001 in aux
 */
struct ROM_SYMBOLS *Rom::LoadSymbolFile(QString filename)
{
    struct ROM_SYMBOLS *head = nullptr;
    QFile *symbolFile = new QFile(filename);
    if (!symbolFile->open(QFile::ReadOnly | QFile::Text)) {
        delete symbolFile;
        return nullptr;
    }
    char buf[80];
    int nbRead = 0;
    while ((nbRead = symbolFile->readLine(buf, sizeof(buf) - 1)) > 0) {
        buf[nbRead] = 0;
        char *name = buf;
        while ((*name == ' ') || (*name == '\t')) {
            name++;
        }
        if ((*name != ';') && (*name != '\n') && (*name != 0)) {
            char *equals = name;
            while ((*equals != '=') && (*equals != '\n') && (*equals != 0)) {
                equals++;
            }
            if (*equals == '=') {
                char *space = equals - 1;
                while ((*space == ' ') || (*space == '\t')) {
                    *space-- = 0;
                }
                *equals++ = 0;
                while ((*equals == ' ') || (*equals == '\t')) {
                    equals++;
                }
                if (*equals == '$') {
                    char *addr = ++equals;
                    int address = 0;
					int address2 = 0;
                    for (int i = 0; (i < 4) && (*addr != 0); i++) {
                        if ((*addr >= '0') && (*addr <= '9')) {
                            address <<= 4;
                            address |= *addr - '0';
                        }
                        else if ((*addr >= 'A') && (*addr <= 'F')) {
                            address <<= 4;
                            address |= *addr - 'A' + 10;
                        }
                        else if ((*addr >= 'a') && (*addr <= 'f')) {
                            address <<= 4;
                            address |= *addr - 'a' + 10;
                        }
                        else {
                            break;
                        }
                        addr++;
                    }
					if ((qstricmp(name, "#skip") == 0) || (qstricmp(name, "#command") == 0) || (qstricmp(name, "#frame") == 0)) {
						if (qstricmp(name, "#command") != 0) {
							char *minus = addr;
							while ((*minus == ' ') || (*minus == '\t')) {
								minus++;
							}
                            if (*minus++ == '-') {
								while ((*minus == ' ') || (*minus == '\t')) {
									minus++;
								}
								if (*minus == '$') {
									addr = ++minus;
									for (int i = 0; (i < 4) && (*addr != 0); i++) {
										if ((*addr >= '0') && (*addr <= '9')) {
											address2 <<= 4;
											address2 |= *addr - '0';
										}
										else if ((*addr >= 'A') && (*addr <= 'F')) {
											address2 <<= 4;
											address2 |= *addr - 'A' + 10;
										}
										else if ((*addr >= 'a') && (*addr <= 'f')) {
											address2 <<= 4;
											address2 |= *addr - 'a' + 10;
										}
										else {
											break;
										}
										addr++;
									}
								}
							}
                            minus++;
						}
					}
					else if (*name == '#') {
						// unknown directive
						continue;
					}
					struct ROM_SYMBOLS *sym = (struct ROM_SYMBOLS *)malloc(sizeof(struct ROM_SYMBOLS));
					strncpy(sym->name, name, sizeof(sym->name));
					sym->address = (unsigned short)address;
                    sym->address2 = (unsigned short)address2;
                    if ((qstricmp(name, "#skip") == 0) && (address2 != 0)) {
                        int size = address2 - address + 1;
                        sym->displayed = (unsigned char *)malloc(size);
                        memset(sym->displayed, 0, size);
                    }
                    else {
                        sym->displayed = nullptr;
                    }
                    sym->next = head;
					head = sym;
                }
            }
        }
    }
    symbolFile->close();
    delete symbolFile;
    return head;
}

bool Rom::LoadRomFile(QString filename)
{
    m_romFilename = filename;
    m_firmware = LoadBinary(m_romFilename, &m_length);
	if (m_firmware) {
        m_symFilename = filename;
        int dot = m_symFilename.lastIndexOf('.');
        int slash = m_symFilename.lastIndexOf('/');
        int backslash = m_symFilename.lastIndexOf('\\');
        int sep = (slash > backslash) ? slash : backslash;
        if (dot > sep) {
            m_symFilename.replace(dot, m_symFilename.size() - dot, ".sym");
		}
		else {
            m_symFilename += ".sym";
		}
		m_symbols = LoadSymbolFile(m_symFilename);
		return true;
	}
	return false;
}

unsigned char Rom::ReadRegister(unsigned short addr)
{
	return m_firmware[addr & ((unsigned short) (m_length - 1))];
}

void Rom::WriteRegister(unsigned short, unsigned char)
{
}

char *Rom::GetAddressLabel(unsigned short addr)
{
	struct ROM_SYMBOLS *head = m_symbols;
    while (head != nullptr) {
		if ((head->name[0] != '#') && (head->address == addr)) {
			return head->name;
		}
		head = head->next;
	}
    return nullptr;
}

bool Rom::IsAddressSkipped(unsigned short addr)
{
	struct ROM_SYMBOLS *head = m_symbols;
    while (head != nullptr) {
		if ((head->name[0] == '#') && (head->name[1] == 's') && (addr >= head->address) && (addr <= head->address2)) {
            if (head->displayed) {
                if (m_lastSkip != head) {
                    m_lastSkip = head;
                    for (int i = 0; i < (head->address2 - head->address + 1); i++) {
                        head->displayed[i] = 0x00;
                    }
                }
                if (head->displayed[addr - head->address]) {
                    return true;
                }
                head->displayed[addr - head->address] = 0xFF;
                return false;
            }
            m_lastSkip = nullptr;
            return true;
		}
		head = head->next;
	}
    m_lastSkip = nullptr;
    return false;
}

bool Rom::IsCommandActive(unsigned char command, unsigned short aux)
{
	struct ROM_SYMBOLS *head = m_symbols;
    while (head != nullptr) {
		if (head->name[0] == '#') {
			if (head->name[1] == 'c') {
				if (head->address == (unsigned short) command) {
					return true;
				}
			}
			else if (head->name[1] == 'f') {
				if ((head->address == (unsigned short) command) && (head->address2 == aux)) {
					return true;
				}
			}
		}
		head = head->next;
	}
	return false;
}
