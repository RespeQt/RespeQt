//
// Crc16 class
// (c) 2016 Eric BACHER
//

#ifndef CRC16_HPP
#define CRC16_HPP 1

// This class computes CRC16.
class Crc16 {

private:

	unsigned short			m_crc;
	int						m_bytesInCrc; // number of bytes added in CRC

public:

	// constructor and destructor
							Crc16();
	virtual					~Crc16() { }

	virtual unsigned short	GetCrc(void) { return m_crc; }
	virtual int				GetBytesInCrc(void) { return m_bytesInCrc; }

	virtual void			Reset(void);
	virtual unsigned char	Add(unsigned char data);
	virtual void			Abort(void);
};

#endif
