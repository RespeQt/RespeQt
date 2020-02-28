#include "atari1020.h"
#include "logdisplaydialog.h"
#include "respeqtsettings.h"

#include <cmath>
#include <stdexcept>
#include <QFontDatabase>
#include <QPoint>
#include <QException>

namespace Printers
{
    Atari1020::Atari1020(SioWorkerPtr sio)
        : AtariPrinter(sio),
          mGraphicsMode(true),
          mEsc(false),
          mPrintText(false),
          mTextOrientation(0),
          mFontSize(10)
    {}

    void Atari1020::setupOutput()
    {
        AtariPrinter::setupOutput();
        if (mOutput)
        {
            mOutput->painter()->setWindow(QRect(0, -999, 480, 1000));
        }
    }

    void Atari1020::setupFont()
    {
        if (mOutput)
        {
            QFontDatabase fonts;
            if (!fonts.hasFamily("Atari  Console"))
            {
                QFontDatabase::addApplicationFont(":/fonts/1020");
            }
            // TODO calculate the correct font size.
            mFontSize = 10;
            QFontPtr font = QFontPtr::create("Atari  Console", mFontSize);
            font->setUnderline(false);
            mOutput->setFont(font);
            mOutput->calculateFixedFontSize(80);
            mOutput->painter()->setPen(QColor("black"));
        }
    }

    bool Atari1020::handleBuffer(QByteArray &buffer, unsigned int len)
    {
        len = std::min(static_cast<unsigned int>(buffer.count()), len);
        for(unsigned int i = 0; i < len; i++) {
            unsigned char b = static_cast<unsigned char>(buffer.at(static_cast<int>(i)));

            if (b == 155) // EOL
            {
                endCommandLine();
                return true; // Drop the rest of the buffer
            }

            if (mGraphicsMode) {
                if (b == 65) // A; Leave graphics mode
                {
                    mGraphicsMode = false;
                } else if (mPrintText)
                    handlePrintableCodes(b);
                else
                    handleGraphicsMode(buffer, len, i);
            } else {
                // Textmode
                if (b == 27) // ESC
                {
                    mEsc = true;
                    return true;
                }
                if (mEsc)
                {
                    if (b == 27) // Cancel Esc
                    {
                        mEsc = false;
                        handlePrintableCodes(27);
                    } else if (b == 7) // CTRL+G: Enter Graphics Mode
                    {
                        mGraphicsMode = true;
                        mEsc = true;
                    } else if (b == 16) // CTRL+P: 20 characters
                    {
                        mOutput->calculateFixedFontSize(20);
                        mEsc = false;
                    } else if (b == 19) // CTRL+S: 80 characters
                    {
                        mOutput->calculateFixedFontSize(80);
                        mEsc = false;
                    } else if (b == 14) // CTRL+N: 40 characters
                    {
                        mOutput->calculateFixedFontSize(40);
                        mEsc = false;
                    } else if (b == 23) // CTRL+W: Enter international mode
                    {
                        mInternational = true;
                        mEsc = false;
                    } else if (b == 24) // CTRL+X: Exit international mode
                    {
                        mInternational = false;
                        mEsc = false;
                    } else {
                        // Not known control codes are consumed.
                        mEsc = false;
                        return true;
                    }
                } else
                    handlePrintableCodes(b);
            }
        }

        return true;
    }

    void Atari1020::endCommandLine()
    {
        if (mGraphicsMode)
        {
            mPrintText = false;
        } else {
            mOutput->newLine();
        }
    }

    bool Atari1020::handleGraphicsMode(QByteArray &buffer, unsigned int len, unsigned int &i)
    {
        unsigned char b = static_cast<unsigned char>(buffer.at(static_cast<int>(i)));

        switch (b)
        {
            case 'H': // set to HOME
                mPenPoint.setX(0);
                mPenPoint.setY(0);
            break;

            case 'S': // Scale characters
                try {
                    unsigned int end;
                    int scale = fetchIntFromBuffer(buffer, len, i, end);
                    if (scale >= 0 && scale <= 63)
                    {
                        QFontPtr font = mOutput->font();
                        font->setPixelSize(font->pixelSize() * scale);
                        i = end;
                    } else
                        handlePrintableCodes(b);
                } catch(...) {
                    qDebug() << "!n" << tr("[%1] parsing error for scale command").arg(deviceName());
                    handlePrintableCodes(b);
                }
            break;

            case 'C': // Set Color
            {
                unsigned char next = static_cast<unsigned char>(buffer.at(static_cast<int>(i + 1)));
                if (next >= '0' && next <= '3')
                {
                    QColor temp("black");
                    switch(next)
                    {
                        default:
                        case '0':
                            temp = QColor("black");
                            break;
                        case '1':
                            temp = QColor("blue");
                            break;
                        case '2':
                            temp = QColor("green");
                            break;
                        case '3':
                            temp = QColor("red");
                            break;
                    }

                    mOutput->painter()->setPen(temp);
                    i++;
                } else
                    handlePrintableCodes(b);
            }
            break;

            case 'L': // Line mode 0 = solid, anything else dashed
                try {
                    unsigned int end;
                    int line = fetchIntFromBuffer(buffer, len, i, end);
                    if (line >= 0 && line <= 15)
                    {
                        auto pen = mOutput->painter()->pen();
                        if (line == 0) {
                            pen.setStyle(Qt::SolidLine);
                        } else {
                            pen.setStyle(Qt::CustomDashLine);
                            QVector<qreal> pattern;
                            pattern << 1 << line;
                            pen.setDashPattern(pattern);
                        }
                        mOutput->painter()->setPen(pen);
                        i = end;
                    } else {
                        handlePrintableCodes(b);
                    }
                } catch(...) {
                    qDebug() << "!n" << tr("[%1] parsing error for line command").arg(deviceName());
                    handlePrintableCodes(b);
                }
            break;

            case 'I': // Init plotter
                mOutput->painter()->translate(mPenPoint);
            break;

            case 'D': // draw to point
            case 'M': // move to point
            case 'J': // draw relative
            case 'R': // move relative
            {
                unsigned char command = b;
                do {
                    try {

                        unsigned int endx, endy;
                        int x = fetchIntFromBuffer(buffer, len, i, endx);
                        if (buffer.at(static_cast<int>(endx)) != ',')
                        {
                            throw new std::invalid_argument("expected ,");
                        }
                        if (x < 0 || x > 480)
                        {
                            throw new std::invalid_argument("x coordinate out of range");
                        }
                        int y = fetchIntFromBuffer(buffer, len, endx + 1, endy);
                        if (y < -999 && y > 999)
                        {
                            throw new std::invalid_argument("y coordinate out of range");
                        }

                        i = endy;

                        QPointF point(x, y);

                        switch (command) {
                            case 'D':
                                mOutput->painter()->drawLine(mPenPoint, point);
                                mPenPoint = point;
                            break;

                            case 'J':
                                point += mPenPoint;
                                mOutput->painter()->drawLine(mPenPoint, point);
                                mPenPoint = point;
                            break;

                            case 'M':
                                mPenPoint = point;
                            break;

                            case 'R':
                                mPenPoint += point;
                            break;
                        }

                        b = static_cast<unsigned char>(buffer.at(static_cast<int>(i)));
                    } catch(std::exception& e)
                    {
                        qDebug() << "!n" << "std::exception: " << QString(e.what());
                        handlePrintableCodes(b);
                        break;
                    } catch(...)
                    {
                        qDebug() << "!n" << tr("[%1] parsing error for draw command").arg(deviceName());
                        handlePrintableCodes(b);
                        break;
                    }
                } while(b == ';');
                if (b == '*')
                {
                    i++;
                }
            }
            break;

            case 'X': // draw axes 0 = Y-axis, otherwise X-axis
                try {
                    unsigned int i_ = i, end;
                    int mode, size, count;

                    mode = fetchIntFromBuffer(buffer, len, i_, end);
                    if (len < end)
                    {
                        throw new std::invalid_argument("expected comma, buffer too short");
                    }
                    i_ = end; // separator check
                    if (buffer.at(static_cast<int>(i_)) != ',')
                    {
                        throw new std::invalid_argument(QString("expected comma, got %1").arg(b).toStdString());
                    }
                    i_++;

                    size = fetchIntFromBuffer(buffer, len, i_, end);
                    i_ = end; // separator check
                    if (len < end)
                    {
                        throw new std::invalid_argument("expected comma, buffer too short");
                    }
                    if (buffer.at(static_cast<int>(i_)) != ',')
                    {
                        throw new std::invalid_argument(QString("expected comma, got %1").arg(b).toStdString());
                    }
                    i_++;

                    count = fetchIntFromBuffer(buffer, len, i_, end);
                    i = end;

                    drawAxis(mode != 0, size, count);
                } catch(...)
                {
                    qDebug() << "!n" << tr("[%1] parsing error for axis draw").arg(deviceName());
                }
            break;

            case 'Q': // select text orientation
                try {
                    unsigned int endx;
                    mTextOrientation = fetchIntFromBuffer(buffer, len, i, endx);
                    if (mTextOrientation < 0 || mTextOrientation > 3) throw new std::invalid_argument("orientation number out of range");
                    i = endx;
                } catch(...) {
                    qDebug() << "!n" << tr("[%1] parsing error for text orientation").arg(deviceName());
                    handlePrintableCodes(b);
                    break;
                }
            break;

            case 'P': // print text
                mPrintText = true;
            break;
        }

        return true;
    }

    bool Atari1020::drawAxis(bool xAxis, int size, int count)
    {
        QPointF start = QPointF(mPenPoint);
        QPointF end = QPointF(start);

        if (xAxis)
        {
            end.setX(end.x() + size * count);
        } else {
            end.setY(end.y() + size * count);
        }
        mOutput->painter()->drawLine(start, end);

        for (int c = 1; c <= count; c++)
        {
            if (xAxis)
            {
                qreal xc = mPenPoint.x() + c * size;
                mOutput->painter()->drawLine(QPointF(xc, mPenPoint.y() - 2), QPointF(xc, mPenPoint.y() + 2));
            } else {
                qreal yc = mPenPoint.y() + c * size;
                mOutput->painter()->drawLine(QPointF(mPenPoint.x() - 2, yc), QPointF(mPenPoint.x() + 2, yc));
            }
        }

        return true;
    }

    int Atari1020::fetchIntFromBuffer(QByteArray &buffer, unsigned int /*len*/, unsigned int i, unsigned int &end)
    {
        int result = 0;
        QString number("");

        end = i + 1;
        char next = buffer.at(static_cast<int>(end));

        // Is there a sign?
        if (next == '-' || next == '+') {
            number.append(next);
            end++;
            next = buffer.at(static_cast<int>(end));
        }

        // Search for next digit
        while (next >= '0' && next <= '9')
        {
            number.append(next);
            end++;
            next = buffer.at(static_cast<int>(end));
        }
        bool ok;
        result = number.toInt(&ok);
        if (ok)
            return result;
        else
            throw new std::invalid_argument("couldn't convert to int");
    }

    bool Atari1020::handlePrintableCodes(const unsigned char b)
    {
        QChar qb = translateAtascii(b & 127); // Masking inverse characters.
        mOutput->printChar(qb);

        return true;
    }
}
