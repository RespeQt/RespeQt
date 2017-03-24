#include "atari1027.h"
#include "respeqtsettings.h"

#include <QtDebug>

Atari1027::Atari1027(SioWorker *worker)
    : BasePrinter(worker),
      mInternational(false),
      mFirstESC(false), mSecondESC(false)
{
    mTypeId = 2;
    mTypeName = new QString("Atari 2017");
    mRequiresNativePrinter = true;
    mFont.setStyleHint(QFont::TypeWriter);
    mFont.setPointSize(12);
    mFont.setUnderline(false);

    mFontMetrics = new QFontMetrics(mFont);

    mNativePrinter = new QPrinter();
    mPainter = new QPainter();
    mPainter -> setFont(mFont);
    mBoundingBox = mNativePrinter -> pageRect();
    x = mBoundingBox.left();
    y = mBoundingBox.top();
    mPainter->begin(mNativePrinter);
}

void Atari1027::handleCommand(quint8 command, quint16 aux)
{
    if(respeqtSettings->printerEmulation()) {  // Ignore printer commands  if Emulation turned OFF)    //
        switch(command) {
        case 0x53:
            {
                // Get status
                if (!sio->port()->writeCommandAck()) {
                    return;
                }

                QByteArray status(4, 0);
                status[0] = 0;
                status[1] = m_lastOperation;
                status[2] = 3;
                status[3] = 0;
                sio->port()->writeComplete();
                sio->port()->writeDataFrame(status);
                qDebug() << "!n" << tr("[%1] Get status.").arg(deviceName());
                break;
            }
        case 0x57:
            {
                // Write
                int aux2 = aux % 256;
                int len;
                switch (aux2) {
                case 0x4e:
                    // Normal
                    len = 40;
                    break;
                case 0x53:
                    // Sideways
                    len = 29;
                    break;
                case 0x44:
                    // Double width
                    len = 21;
                    break;
                default:
                    sio->port()->writeCommandNak();
                    qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                                   .arg(deviceName())
                                   .arg(command, 2, 16, QChar('0'))
                                   .arg(aux, 4, 16, QChar('0'));
                    return;
                }
                sio->port()->writeCommandAck();

                QByteArray data = sio->port()->readDataFrame(len);
                if (data.isEmpty()) {
                    qCritical() << "!e" << tr("[%1] Print: data frame failed")
                                  .arg(deviceName());
                    sio->port()->writeDataNak();
                    return;
                }
                handleBuffer(data);
                sio->port()->writeDataAck();
                qDebug() << "!n" << tr("[%1] Print (%2 chars)").arg(deviceName()).arg(len);
                sio->port()->writeComplete();
                break;
            }
        default:
            sio->port()->writeCommandNak();
            qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                           .arg(deviceName())
                           .arg(command, 2, 16, QChar('0'))
                           .arg(aux, 4, 16, QChar('0'));
        }
    } else {
        qDebug() << "!u" << tr("[%1] ignored").arg(deviceName());
    }
}

bool Atari1027::handleBuffer(const QByteArray &buffer)
{
    int len = buffer.size();
    for(int i = 0; i < len; i++)
    {
        unsigned char b = buffer.at(i);
        switch(b) {
            case 15: // CTRL+O starts underline mode
                mFont.setUnderline(true);
            break;

            case 14: // CTRL+N ends underline mode
                mFont.setUnderline(false);
            break;

            case 23: // CTRL+W could be ESC code
            case 24: // CTRL+X could be ESC code
            case 25: // CTRL+Y could be ESC code
            case 26: // CTRL+Z could be ESC code
                if (mFirstESC && mSecondESC)
                {
                    if (!handleEscapedCodes(b))
                    {
                        handlePrintableCodes(b);
                    }
                } else {
                    handlePrintableCodes(b);
                }
            break;

            case 155: // EOL
                x = mBoundingBox.left();
                if (y + mFontMetrics->height() > mBoundingBox.bottom())
                {
                    mNativePrinter -> newPage();
                    y = mBoundingBox.top();
                } else {
                    y += mFontMetrics->height();
                }
            break;

            case 27: // ESC could be starting something
                if (mFirstESC && !mSecondESC) { // Only first ESC from last buffer
                    if (i + 1 < len)
                    {
                        mSecondESC = true;
                        i++;
                        b = buffer.at(i);
                        if (i + 1 < len)
                        {
                            i ++;
                            b = buffer.at(i);
                            if (!handleEscapedCodes(b))
                            {
                                handlePrintableCodes(b);
                            }
                        }
                    }
                } else if (mFirstESC && mSecondESC) // We have both ESCs from last buffer
                {
                    if (i + 1 < len)
                    {
                        i++;
                        b = buffer.at(i);
                        if (!handleEscapedCodes(b))
                        {
                            handlePrintableCodes(b);
                        }
                    }
                } else { // No ESC codes from last buffer
                    mFirstESC = true;
                    if (i + 1 < len)
                    {
                        i++;
                        b = buffer.at(i);
                        if (b == 27) { // control needs second ESC
                            mSecondESC = true;
                            if (i + 1 < len) {
                                i ++;
                                b = buffer.at(i);
                                if (!handleEscapedCodes(b))
                                {
                                    handlePrintableCodes(b);
                                }
                            }
                        }
                    }
                }
            default: // Everythings else
                handlePrintableCodes(b);
            break;
        }
    }
    return true;
}

bool Atari1027::handleEscapedCodes(const char b)
{
    // At this time we have seen two ESC.
    switch(b) {
        case 25: // CTRL+Y starts underline mode
            mFont.setUnderline(true);
            mFirstESC = mSecondESC = false;
            return true;

        case 26: // CTRL+Z ends underline mode
            mFont.setUnderline(false);
            mFirstESC = mSecondESC = false;
            return true;

        case 23: // CTRL+W starts international mode
            mInternational = true;
            mFirstESC = mSecondESC = false;
            return true;

        case 24: // CTRL+X ends international mode
            mInternational = false;
            mFirstESC = mSecondESC = false;
            return true;
    }
    return false;
}

bool Atari1027::handlePrintableCodes(const char b)
{
    QChar qb = translateAtascii(b);
    QRect bound = mFontMetrics->boundingRect(qb);
    if (bound.width() + x > mBoundingBox.right()) { // Char has to go on next line
        x = mBoundingBox.left();
        if (y + mFontMetrics->height() > mBoundingBox.bottom())
        {
            mNativePrinter -> newPage();
            y = mBoundingBox.top();
        } else {
            y += mFontMetrics->height();
        }
    }

    mPainter -> drawText(x, y, qb);
    x += bound.width();

    return true;
}
