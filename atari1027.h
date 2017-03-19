#ifndef ATARI1027_H
#define ATARI1027_H

#include "baseprinter.h"

class Atari1027 : public BasePrinter
{
    Q_OBJECT
public:
    Atari1027(SioWorker *worker);

    virtual void handleCommand(quint8 command, quint16 aux);
private:
    int m_lastOperation;
    static bool conversionMsgdisplayedOnce;
};

#endif // ATARI1027_H
