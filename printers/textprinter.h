#ifndef TEXTPRINTER_H
#define TEXTPRINTER_H

#include "baseprinter.h"

class TextPrinter : public BasePrinter
{
    Q_OBJECT
public:
    TextPrinter(SioWorker *worker);
    ~TextPrinter();

    virtual bool handleBuffer(QByteArray &buffer, int len);
private:
    int m_lastOperation;
    static bool conversionMsgdisplayedOnce;
signals:
    void print(const QString &text);
};

#endif // TEXTPRINTER_H
