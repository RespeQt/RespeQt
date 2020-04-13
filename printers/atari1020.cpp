#include "atari1020.h"
#include "logdisplaydialog.h"
#include "respeqtsettings.h"

#include <cmath>
#include <QFontDatabase>
#include <QPoint>
#include <utility> 

static struct GRAPHICS_COMMAND ALLOWED_GRAPHICS_COMMANDS[] = {
    { 'A', 0, false, AUTOMATA_END },
    { 'C', 1, false, AUTOMATA_FIRST_NUM },
    { 'D', 2, true,  AUTOMATA_FIRST_NUM },
    { 'H', 0, false, AUTOMATA_END },
    { 'I', 0, false, AUTOMATA_END },
    { 'J', 2, true,  AUTOMATA_FIRST_NUM },
    { 'L', 1, false, AUTOMATA_FIRST_NUM },
    { 'M', 2, true,  AUTOMATA_FIRST_NUM },
    { 'P', 0, false, AUTOMATA_TEXT },
    { 'Q', 1, false, AUTOMATA_FIRST_NUM },
    { 'R', 2, true,  AUTOMATA_FIRST_NUM },
    { 'S', 1, false, AUTOMATA_FIRST_NUM },
    { 'X', 3, false, AUTOMATA_FIRST_NUM }
};

namespace Printers
{
    Atari1020::Atari1020(SioWorkerPtr sio)
        : AtariPrinter(std::move(sio)),
          mEsc(false),
          mStartOfLogicalLine(true),
          mGraphicsMode(false),
          mTextOrientation(0),
          mFontSize(10),
          mColor(0),
          mLineType(0),
          mScale(0),
          mPrintText("")
    {
    }

    void Atari1020::setupOutput()
    {
        AtariPrinter::setupOutput();
        if (mOutput && mOutput->painter()) {
            mOutput->painter()->setWindow(QRect(0, -999, 720, 1000));
        }
    }

    void Atari1020::setupFont()
    {
        if (mOutput) {
            QFontDatabase fonts;
            if (!fonts.hasFamily("Atari  Console")) {
                QFontDatabase::addApplicationFont(":/fonts/1020");
            }
            // TODO calculate the correct font size.
            mFontSize = 10;
            QFontPtr font = QFontPtr::create("Atari  Console", mFontSize);
            font->setUnderline(false);
            mOutput->setFont(font);
            mOutput->calculateFixedFontSize(80);
            if (mOutput->painter()) {
                mOutput->painter()->setPen(QColor("black"));
            }
        }
    }

    void Atari1020::handleCommand(const quint8 command, const quint16 aux)
    {
        if (respeqtSettings->printerEmulation() && mOutput) {  // Ignore printer commands if Emulation turned OFF)
            switch(command) {
            case 0x53: // Get status
                {
                    if (!sio->port()->writeCommandAck()) {
                        return;
                    }

                    /*
                     * STATUS command always returns $00 $00 $FF $40.
                     * There is no byte in the status used to notify the Atari that the printer is busy.
                     * When the printer is busy, it does not answer SIO command.
                     * Sending a STATUS command to a busy 1020 results in a timeout.
                     */
                    QByteArray status(4, 0);
                    status[0] = 0;
                    status[1] = 0;
                    status[2] = 0xFF;
                    status[3] = 0x40;
                    sio->port()->writeComplete();
                    qDebug() << "!n" << tr("[%1] Get status: $%2")
                                .arg(deviceName())
                                .arg((unsigned char)status[1], 2, 16, QChar('0'));
                    writeDataFrame(status);

                    /*
                     * Important: the STATUS command exits from the Graphics mode.
                     * The next buffer sent will be interpreted as plain text.
                     */
                    mGraphicsMode = false;
                    mEsc = false;
                    mStartOfLogicalLine = true;
                    mInternational = false;
                    break;
                }

            case 0x57: // Write buffer
                {
                    if (!sio->port()->writeCommandAck()) {
                        return;
                    }

                    /*
                     * 1020 printer does not look at AUX value.
                     * The printer always expects 40 bytes whatever value is send in AUX.
                     * There is no possibility to send different buffer size.
                     */
                    unsigned int len = 40; // 1020 only support 40 bytes
                    QByteArray data = readDataFrame(len);
                    if (data.isEmpty()) {
                        qCritical() << "!e" << tr("[%1] Print: data frame failed")
                                      .arg(deviceName());
                        sio->port()->writeDataNak();
                        return;
                    }

                    if (!sio->port()->writeDataAck()) {
                        return;
                    }
                    handleBuffer(data, len);
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

    bool Atari1020::handleBuffer(const QByteArray &buffer, const unsigned int len)
    {
        /*
         * Characters and commands sent to the printer should be handled within logical lines with the following structure
         *   <any sequence of printable character, escape sequence and Graphics instructions> EOL
         * This sequence may be of any size.
         * If the sequence is longer than 40 characters, it is sent by the Atari using several 40-bytes buffers.
         * All characters after an EOL are ignored meaning that a logical line always starts at the begining of a new buffer.
         */
        auto lenmin = std::min(static_cast<unsigned int>(buffer.count()), len);
        for(unsigned int i = 0; i < lenmin; i++) {
            auto b = static_cast<unsigned char>(buffer.at(static_cast<int>(i)));

            /*
             * EOL prevents printer from looking at the rest of the buffer.
             * But if the printer is in Graphics mode, an unfinished command is executed with default parameters.
             * For example, if bytes 'C' and EOL are received (change pen with no parameter), the command C0 is executed.
             * Or if bytes 'M' and EOL are received (move pen), the command M0,<current line> is executed.
             * EOL does not exit from Graphics mode.
             * EOL can not be escaped, meaning that Esc EOL will have the same effect as EOL.
             */
            if (b == 0x9B) { // EOL
                if (mGraphicsMode) {
                    executeGraphicsCommand();
                    resetGraphics();
                } else {
                    mOutput->newLine();
                }
                mEsc = false;
                mStartOfLogicalLine = true;
                return true; // Drop the rest of the buffer
            }

            /*
             * Graphics mode is entered by an Esc sequence and terminated with the A instruction or a STATUS command.
             * Multiple Graphics instructions can be sent in the same logical line using the '*' character.
             * An instruction may start in a physical buffer (in a previous SIO $53 command) and continue in the next one.
             * For example we can have:
             * 1st physical buffer:
             *   $1B $07 M100,-100 ..... *C     for a total of 40 bytes
             * 2nd physical buffer:
             *   2*D300,-100 $9B
             * The intruction to change the pen color (C2) starts in the first buffer and continues on the next one.
             * Escape sequences are consumed but not interpreted.
             * The instruction P is used to print text. When activated, the only way to get out of the text mode is
             * using an EOL.
             * After exiting Graphics mode using the A instruction, the printer is still expecting either an asterisk
             * or an EOL to print plain ASCII text.
             *   $1B $07 M100,-100*C1*ATXT1*TXT2
             * In the above example, once *A has been executed, the following characters (TXT1) are ignored until
             * the asterisk is encoutered and TXT2 is then printed.
             * It means that, in Graphics mode, the printer decodes one instruction with its parameters and then
             * consumes all other characters until the next asterisk or the next EOL.
             */
            if (mGraphicsMode) {
                handleGraphicsCodes(b);
                mStartOfLogicalLine = false;
            }

            /*
             * In plain text mode, Escape sequences are available.
             * Changing the number of characters per line (20, 40 and 80) is only possible at the start of a logical line.
             *
             * NOTE: Switching from standard character set to international one can probably be done at any time but
             *       I did not manage to print any international characters with my 1020.
             *       The following implementation assumes it switching works at any time.
             */
            else {

                /*
                 * If Esc is found at the start of a logical line, then the escape sequence is interpreted.
                 * Any number of consecutive Esc chars at the start of the logical line are interpreted as only one Esc.
                 * If Esc char is found at the start of a buffer which is not the start of the logical line, it is ignored.
                 * By default, the next character after an escape always cancels the ESCAPE state.
                 * If this character is also an Esc, the printer enters again in ESCAPE state.
                 * It means that, in a middle of a logical line, all repeated Esc chars and the next non Esc char are ignored:
                 *   Esc X or Esc Esc X or Esc Esc Esc X: all these sequences are ignored.
                 * The ESCAPE state is local to a logical line. It is reset when EOL is encountered.
                 */
                if (mEsc) {
                    mEsc = false;
                    switch (b) {

                    case 0x1B: // Cancel Esc
                        qDebug() << "!n" << tr("[%1] Escape character repeated")
                                    .arg(deviceName());
                        mEsc = true;
                        break;

                    case 0x17: // CTRL+W: Enter international mode
                        qDebug() << "!n" << tr("[%1] Entering international mode")
                                    .arg(deviceName());
                        mInternational = true;
                        break;

                    case 0x18: // CTRL+X: Exit international mode
                        qDebug() << "!n" << tr("[%1] Exiting international mode")
                                    .arg(deviceName());
                        mInternational = false;
                        break;

                    default: // Other commands are ignored unless at the start of a logical line

                        /*
                         * Esc commands are only interpreted at the start of the buffer except for International character set.
                         * For example, sending AB $1B $07 C1 to the printer while in text mode will display AB but it
                         * won't enter Graphics mode and won't change the pen color to blue.
                         * Again, sending AB $1B $13 CD to the printer while in text mode will display ABCD in 40 column mode
                         * even if an Esc Ctrl-S (80-column mode) is found between AB and CD.
                         */
                        if (! mStartOfLogicalLine) {
                            qDebug() << "!n" << tr("[%1] Escape character not on start of line")
                                        .arg(deviceName());
                        } else {
                            switch (b) {

                            case 0x07: // CTRL+G: Enter Graphics Mode
                                qDebug() << "!n" << tr("[%1] Entering Graphics mode")
                                            .arg(deviceName());
                                mStartOfLogicalLine = false;
                                mGraphicsMode = true;
                                resetGraphics();
                                break;

                            case 0x0E: // CTRL+N: 40 characters
                                qDebug() << "!n" << tr("[%1] Switching to 40 columns")
                                            .arg(deviceName());
                                mOutput->calculateFixedFontSize(40);
                                break;

                            case 0x10: // CTRL+P: 20 characters
                                qDebug() << "!n" << tr("[%1] Switching to 20 columns")
                                            .arg(deviceName());
                                mOutput->calculateFixedFontSize(20);
                                break;

                            case 0x13: // CTRL+S: 80 characters
                                qDebug() << "!n" << tr("[%1] Switching to 80 columns")
                                            .arg(deviceName());
                                mOutput->calculateFixedFontSize(80);
                                break;

                            case 0x17: // CTRL+W: Enter international mode
                                qDebug() << "!n" << tr("[%1] Entering international mode")
                                            .arg(deviceName());
                                mInternational = true;
                                break;

                            case 0x18: // CTRL+X: Exit international mode
                                qDebug() << "!n" << tr("[%1] Exiting international mode")
                                            .arg(deviceName());
                                mInternational = false;
                                break;

                            default: // Unknown control codes are consumed.
                                qDebug() << "!n" << tr("[%1] Unknown control code $%2")
                                            .arg(deviceName())
                                            .arg(b, 2, 16, QChar('0'));
                                return true;
                            }
                        }
                    }
                } else if (b == 0x1B) { // ESC
                    mEsc = true;
                } else {
                    handlePrintableCodes(b);
                    mStartOfLogicalLine = false;
                }
            }
        }

        return true;
    }

    void Atari1020::resetGraphics()
    {
        mCurrentCommand = 0;
        mRepeatAllowed = false;
        mParametersExpected = 0;
        mFirstNegative = false;
        mFirstNumber.clear();
        mSecondNegative = false;
        mSecondNumber.clear();
        mPrintText.clear();
        mAutomataState = AUTOMATA_START;
    }

    void Atari1020::executeAndRepeatCommand()
    {
        /*
         * The ';' character is used to repeat the same command but with another set of parameters
         */
        unsigned char currentCommand = mCurrentCommand;
        executeGraphicsCommand();
        resetGraphics();
        mCurrentCommand = currentCommand;
        mAutomataState = AUTOMATA_FIRST_NUM;
    }

    bool Atari1020::checkGraphicsCommand(const unsigned char b)
    {
        for (int i = 0; i < (int)(sizeof(ALLOWED_GRAPHICS_COMMANDS) / sizeof(struct GRAPHICS_COMMAND)); i++) {
            if (ALLOWED_GRAPHICS_COMMANDS[i].command == b) {
                mCurrentCommand = ALLOWED_GRAPHICS_COMMANDS[i].command;
                mRepeatAllowed = ALLOWED_GRAPHICS_COMMANDS[i].repeat;
                mParametersExpected = ALLOWED_GRAPHICS_COMMANDS[i].parameters;
                mAutomataState = ALLOWED_GRAPHICS_COMMANDS[i].automata;
                break;
            }
        }
        return mCurrentCommand != 0;
    }

    void Atari1020::handleGraphicsCodes(const unsigned char b)
    {

        /*
         * In Graphics mode, the asterisk is used to mark the end of the command.
         * This is the same purpose as the EOL. It executes the current graphical command even if not complete.
         * For example M120* will move the printer head to X=120 and Y=0 because Y has not been specified.
         * If the last command was P (for Print text) then the asterisk is not a marker but a simple char to print.
         */
        if ((mAutomataState != AUTOMATA_TEXT) && (b == '*')) { // end of Graphics instruction
            executeGraphicsCommand();
            resetGraphics();
            return;
        }

        switch (mAutomataState) {
        case AUTOMATA_START:
            if (! checkGraphicsCommand(b)) {
                qDebug() << "!n" << tr("[%1] Unknown Graphics command $%2")
                            .arg(deviceName())
                            .arg(b, 2, 16, QChar('0'));
                mAutomataState = AUTOMATA_END; // wait for the next '*' or EOL
            }
            break;

        case AUTOMATA_FIRST_NUM:
            if (b == '-') {
                mFirstNegative = true;
                mAutomataState = AUTOMATA_FIRST_INT;
                break;
            }
            // FALL THRU if '-' is not found

        case AUTOMATA_FIRST_INT:
            if (b == ',') {
                if (mParametersExpected > 1) {
                    mAutomataState = AUTOMATA_SECOND_NUM;
                } else {
                    mAutomataState = AUTOMATA_END; // wait for the next '*' or EOL
                }
            } else if ((b >= '0') && (b <= '9')) {
                if (mFirstNumber.length() < 3) {
                    mFirstNumber.append(b);
                } else {
                    if (mParametersExpected > 1) {
                        mAutomataState = AUTOMATA_COMA; // wait for ','
                    } else {
                        mAutomataState = AUTOMATA_END; // wait for the next '*' or EOL
                    }
                }
            } else if (b == ';') {
                if (mRepeatAllowed) {
                    executeAndRepeatCommand();
                }
            } else {
                if (mParametersExpected > 1) {
                    mAutomataState = AUTOMATA_COMA; // wait for ','
                } else {
                    mAutomataState = AUTOMATA_END; // wait for the next '*' or EOL
                }
            }
            break;

        case AUTOMATA_COMA:
            if (b == ',') {
                mAutomataState = AUTOMATA_SECOND_NUM;
            } else if (b == ';') {
                if (mRepeatAllowed) {
                    executeAndRepeatCommand();
                }
            }
            break;

        case AUTOMATA_SECOND_NUM:
            if (b == '-') {
                mSecondNegative = true;
                mAutomataState = AUTOMATA_SECOND_INT;
                break;
            }
            // FALL THRU if '-' is not found

        case AUTOMATA_SECOND_INT:
            if (b == ',') {
                if (mParametersExpected > 2) {
                    mAutomataState = AUTOMATA_THIRD_NUM;
                } else {
                    mAutomataState = AUTOMATA_END; // wait for the next '*' or EOL
                }
            } else if ((b >= '0') && (b <= '9')) {
                if (mSecondNumber.length() < 3) {
                    mSecondNumber.append(b);
                } else {
                    if (mParametersExpected > 2) {
                        mAutomataState = AUTOMATA_SECOND_COMA; // wait for ','
                    } else {
                        mAutomataState = AUTOMATA_END; // wait for the next '*' or EOL
                    }
                }
            } else if (b == ';') {
                if (mRepeatAllowed) {
                    executeAndRepeatCommand();
                }
            } else {
                if (mParametersExpected > 2) {
                    mAutomataState = AUTOMATA_COMA; // wait for ','
                } else {
                    mAutomataState = AUTOMATA_END; // wait for the next '*' or EOL
                }
            }
            break;

        case AUTOMATA_SECOND_COMA:
            if (b == ',') {
                mAutomataState = AUTOMATA_THIRD_NUM;
            }
            break;

        case AUTOMATA_THIRD_NUM:
            if (b == '-') {
                mThirdNegative = true;
                mAutomataState = AUTOMATA_THIRD_INT;
                break;
            }
            // FALL THRU if '-' is not found

        case AUTOMATA_THIRD_INT:
            if ((b >= '0') && (b <= '9')) {
                if (mThirdNumber.length() < 3) {
                    mThirdNumber.append(b);
                } else {
                    mAutomataState = AUTOMATA_END; // wait for the next '*' or EOL
                }
            } else {
                mAutomataState = AUTOMATA_END; // wait for the next '*' or EOL
            }
            break;

        case AUTOMATA_TEXT:
            mPrintText.append(b);
            break;

        case AUTOMATA_END:
        default:
            // ignore characters
            break;
        }
    }

    void Atari1020::executeGraphicsPrimitive(GraphicsPrimitive *primitive)
    {
        if (mOutput) {
            mOutput->executeGraphicsPrimitive(primitive);
        }
    }

    void Atari1020::executeGraphicsCommand()
    {
        if (mCurrentCommand) {
            switch (mCurrentCommand) {
            case 'A': // Abandon Graphics mode
                mGraphicsMode = false;
                qDebug() << "!n" << tr("[%1] Leaving Graphics mode").arg(deviceName());
                break;

            case 'H': // set to HOME
                mPenPoint.setX(0);
                mPenPoint.setY(0);
                qDebug() << "!n" << tr("[%1] Move to Home")
                            .arg(deviceName());
                break;

            case 'S': // Scale characters
                {
                    auto scale = getFirstNumber();
                    if (scale >= 0 && scale <= 63) {
                        mScale = scale;
                        qDebug() << "!n" << tr("[%1] Scale characters to %2")
                                    .arg(deviceName())
                                    .arg(scale);
                        QFontPtr font = mOutput->font();
                        font->setPixelSize(font->pixelSize() * scale);
                    } else {
                        qDebug() << "!n" << tr("[%1] Scale command ignored (%2 should be in range 0-63)")
                                    .arg(deviceName())
                                    .arg(QString(mFirstNumber));
                    }
                }
                break;

            case 'C': // Set Color
                {
                    auto next = getFirstNumber();
                    if (next >= 0 && next <= 3) {
                        mColor = next;
                        const char *colorName = "black";
                        QColor temp(colorName);
                        switch(next) {
                            case 1:
                                colorName = "blue";
                                temp = QColor(colorName);
                                break;
                            case 2:
                                colorName = "green";
                                temp = QColor(colorName);
                                break;
                            case 3:
                                colorName = "red";
                                temp = QColor(colorName);
                                break;
                            default:
                            case 0:
                                break;
                        }

                        qDebug() << "!n" << tr("[%1] Set color to %2")
                                    .arg(deviceName())
                                    .arg(colorName);
                        if (mOutput && mOutput->painter()) {
                            mOutput->painter()->setPen(temp);
                        }
                    } else {
                        qDebug() << "!n" << tr("[%1] Set color command ignored (%2 should be in range 0-3)")
                                    .arg(deviceName())
                                    .arg(QString(mFirstNumber));
                    }
                }
                break;

            case 'L': // Line mode 0 = solid, anything else dashed
                {
                    auto line = getFirstNumber();
                    if (line >= 0 && line <= 15) {
                        mLineType = line;
                        qDebug() << "!n" << tr("[%1] Set line mode to %2")
                                    .arg(deviceName())
                                    .arg(line == 0 ? tr("solid") : tr("dashed %1").arg(line));
                        if (mOutput && mOutput->painter()) {
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
                        }
                    } else {
                        qDebug() << "!n" << tr("[%1] Set line mode command ignored (%2 should be in range 0-15)")
                                    .arg(deviceName())
                                    .arg(QString(mFirstNumber));
                    }
                }
                break;

            case 'I': // Init plotter
                qDebug() << "!n" << tr("[%1] Initialize plotter")
                            .arg(deviceName());
                mGraphicsMode = false;
                mEsc = false;
                mStartOfLogicalLine = true;
                mInternational = false;
                if (mOutput && mOutput->painter()) {
                    mOutput->painter()->translate(mPenPoint);
                }
                break;

            case 'D': // draw to point
            case 'M': // move to point
            case 'J': // draw relative
            case 'R': // move relative
                {
                    auto x = getFirstNumber();
                    auto y = getSecondNumber();
                    if ((x >= 0) && (x <= 480)) {
                        QPointF point(x, y);

                        switch (mCurrentCommand) {
                            case 'D':
                                if (mOutput && mOutput->painter()) {
                                    mOutput->painter()->drawLine(mPenPoint, point);
                                }
                                qDebug() << "!n" << tr("[%1] Draw to point (%2,%3)")
                                            .arg(deviceName())
                                            .arg(x)
                                            .arg(y);
                                executeGraphicsPrimitive(new GraphicsDrawLine(mPenPoint.x(), mPenPoint.y(), mColor, point.x(), point.y(), mLineType));
                                mPenPoint = point;
                            break;

                            case 'J':
                                point += mPenPoint;
                                if (mOutput && mOutput->painter()) {
                                    mOutput->painter()->drawLine(mPenPoint, point);
                                }
                                qDebug() << "!n" << tr("[%1] Draw relative to point (%2,%3)")
                                            .arg(deviceName())
                                            .arg(x)
                                            .arg(y);
                                executeGraphicsPrimitive(new GraphicsDrawLine(mPenPoint.x(), mPenPoint.y(), mColor, point.x(), point.y(), mLineType));
                                mPenPoint = point;
                            break;

                            case 'M':
                                qDebug() << "!n" << tr("[%1] Move to point (%2,%3)")
                                            .arg(deviceName())
                                            .arg(x)
                                            .arg(y);
                                mPenPoint = point;
                            break;

                            case 'R':
                                qDebug() << "!n" << tr("[%1] Move relative to point (%2,%3)")
                                            .arg(deviceName())
                                            .arg(x)
                                            .arg(y);
                                mPenPoint += point;
                            break;
                        }
                    }
                }
                break;

            case 'X': // draw axes 0 = Y-axis, otherwise X-axis
                {
                    auto mode = getFirstNumber();
                    auto size = getSecondNumber();
                    auto count = getThirdNumber();
                    auto xAxe = (mode != 0);
                    if (xAxe) {
                        qDebug() << "!n" << tr("[%1] Draw both axes with size %2 and %3 marks")
                                    .arg(deviceName())
                                    .arg(size)
                                    .arg(count);
                    } else {
                        qDebug() << "!n" << tr("[%1] Draw Y axe only with size %2 and %3 marks")
                                    .arg(deviceName())
                                    .arg(size)
                                    .arg(count);
                    }

                    drawAxis(xAxe, size, count);
                }
                break;

            case 'Q': // Set text orientation
                {
                    auto orientation = getFirstNumber();
                    if (orientation >= 0 && orientation <= 3) {
                        mTextOrientation = orientation;
                        qDebug() << "!n" << tr("[%1] Set text orientation to %2")
                                    .arg(deviceName())
                                    .arg(mTextOrientation);
                    } else {
                        qDebug() << "!n" << tr("[%1] Set text orientation command ignored (%2 should be in range 0-3)")
                                    .arg(deviceName())
                                    .arg(QString(mFirstNumber));
                    }
                }
                break;

            case 'P': // print text
                qDebug() << "!n" << tr("[%1] Print \"%2\" in Graphics mode")
                            .arg(deviceName())
                            .arg(QString(mPrintText));
                executeGraphicsPrimitive(new GraphicsDrawText(0, 0, mColor, mTextOrientation, mScale, QString(mPrintText)));
                break;

            }
        }
        mCurrentCommand = 0; // to prevent execution of this command a second time !
    }

    int Atari1020::getFirstNumber(const int defaultValue)
    {
        if (mFirstNumber.length() == 0) {
            return defaultValue;
        }
        bool ok;
        int num = mFirstNumber.toInt(&ok, 10);
        if (!ok) {
            return defaultValue;
        }
        if (mFirstNegative) {
            return -num;
        }
        return num;
    }

    int Atari1020::getSecondNumber(const int defaultValue)
    {
        if (mSecondNumber.length() == 0) {
            return defaultValue;
        }
        bool ok;
        int num = mSecondNumber.toInt(&ok, 10);
        if (!ok) {
            return defaultValue;
        }
        if (mSecondNegative) {
            return -num;
        }
        return num;
    }

    int Atari1020::getThirdNumber(const int defaultValue)
    {
        if (mThirdNumber.length() == 0) {
            return defaultValue;
        }
        bool ok;
        int num = mThirdNumber.toInt(&ok, 10);
        if (!ok) {
            return defaultValue;
        }
        if (mThirdNegative) {
            return -num;
        }
        return num;
    }

    bool Atari1020::drawAxis(bool xAxis, int size, int count)
    {
        QPointF start = QPointF(mPenPoint);
        QPointF end = QPointF(start);

        if (xAxis) {
            end.setX(end.x() + size * count);
        } else {
            end.setY(end.y() + size * count);
        }
        if (mOutput && mOutput->painter()) {
            mOutput->painter()->drawLine(start, end);
        }

        for (int c = 1; c <= count; c++) {
            if (xAxis) {
                qreal xc = mPenPoint.x() + c * size;
                if (mOutput && mOutput->painter()) {
                    mOutput->painter()->drawLine(QPointF(xc, mPenPoint.y() - 2), QPointF(xc, mPenPoint.y() + 2));
                }
            } else {
                qreal yc = mPenPoint.y() + c * size;
                if (mOutput && mOutput->painter()) {
                    mOutput->painter()->drawLine(QPointF(mPenPoint.x() - 2, yc), QPointF(mPenPoint.x() + 2, yc));
                }
            }
        }

        return true;
    }

    bool Atari1020::handlePrintableCodes(const unsigned char b)
    {
        QChar qb = translateAtascii(b & 127); // Masking inverse characters.
        mOutput->printChar(qb);

        return true;
    }

    QByteArray Atari1020::readDataFrame(uint size)
    {
        QByteArray data = sio->port()->readDataFrame(size);
        if (respeqtSettings->isPrinterSpyMode()) {
            qDebug() << "!u" << tr("[%1] Receiving %2 bytes from Atari").arg(deviceName()).arg(data.length());
            dumpBuffer((unsigned char *) data.data(), data.length());
        }
        return data;
    }

    bool Atari1020::writeDataFrame(QByteArray data)
    {
        if (respeqtSettings->isPrinterSpyMode()) {
            qDebug() << "!u" << tr("[%1] Sending %2 bytes to Atari").arg(deviceName()).arg(data.length());
            dumpBuffer((unsigned char *) data.data(), data.length());
        }
        return sio->port()->writeDataFrame(data);
    }

    void Atari1020::dumpBuffer(unsigned char *buf, int len)
    {
        for (int i = 0; i < ((len + 15) >> 4); i++) {
            char line[80];
            int ofs = i << 4;
            fillBuffer(line, buf, len, ofs, true);
            qDebug() << "!u" << tr("[%1] §%2").arg(deviceName()).arg(line);
        }
    }

    void Atari1020::fillBuffer(char *line, unsigned char *buf, int len, int ofs, bool dumpAscii)
    {
        *line = 0;
        if ((len - ofs) >= 16) {
            if (dumpAscii) {
                unsigned char car[16];
                for (int j = 0; j < 16; j++) {
                    if ((buf[ofs + j] > 32) && (buf[ofs + j] < 127)) {
                        car[j] = buf[ofs + j];
                    }
                    else if ((buf[ofs + j] > 160) && (buf[ofs + j] < 255)) {
                        car[j] = buf[ofs + j] & 0x7F;
                    }
                    else {
                        car[j] = ' ';
                    }
                }
                sprintf(line, "$%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X | %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                    ofs & 0xFFFF, ((unsigned int)buf[ofs + 0]) & 0xFF, ((unsigned int)buf[ofs + 1]) & 0xFF,
                    ((unsigned int)buf[ofs + 2]) & 0xFF, ((unsigned int)buf[ofs + 3]) & 0xFF, ((unsigned int)buf[ofs + 4]) & 0xFF,
                    ((unsigned int)buf[ofs + 5]) & 0xFF, ((unsigned int)buf[ofs + 6]) & 0xFF, ((unsigned int)buf[ofs + 7]) & 0xFF,
                    ((unsigned int)buf[ofs + 8]) & 0xFF, ((unsigned int)buf[ofs + 9]) & 0xFF, ((unsigned int)buf[ofs + 10]) & 0xFF,
                    ((unsigned int)buf[ofs + 11]) & 0xFF, ((unsigned int)buf[ofs + 12]) & 0xFF, ((unsigned int)buf[ofs + 13]) & 0xFF,
                    ((unsigned int)buf[ofs + 14]) & 0xFF, ((unsigned int)buf[ofs + 15]) & 0xFF, car[0], car[1], car[2], car[3],
                    car[4], car[5], car[6], car[7], car[8], car[9], car[10], car[11], car[12], car[13], car[14], car[15]);
            }
            else {
                sprintf(line, "$%04X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                    ofs & 0xFFFF, ((unsigned int)buf[ofs + 0]) & 0xFF, ((unsigned int)buf[ofs + 1]) & 0xFF,
                    ((unsigned int)buf[ofs + 2]) & 0xFF, ((unsigned int)buf[ofs + 3]) & 0xFF, ((unsigned int)buf[ofs + 4]) & 0xFF,
                    ((unsigned int)buf[ofs + 5]) & 0xFF, ((unsigned int)buf[ofs + 6]) & 0xFF, ((unsigned int)buf[ofs + 7]) & 0xFF,
                    ((unsigned int)buf[ofs + 8]) & 0xFF, ((unsigned int)buf[ofs + 9]) & 0xFF, ((unsigned int)buf[ofs + 10]) & 0xFF,
                    ((unsigned int)buf[ofs + 11]) & 0xFF, ((unsigned int)buf[ofs + 12]) & 0xFF, ((unsigned int)buf[ofs + 13]) & 0xFF,
                    ((unsigned int)buf[ofs + 14]) & 0xFF, ((unsigned int)buf[ofs + 15]) & 0xFF);
            }
        }
        else if (ofs < len) {
            int nbRemaining = len - ofs;
            memset(line, ' ', 73);
            line[73] = 0;
            sprintf(line, "$%04X:", ofs);
            for (int i = 0; i < nbRemaining; i++) {
                sprintf(&line[strlen(line)], " %02X", ((unsigned int)buf[ofs + i]) & 0xFF);
            }
            if (dumpAscii) {
                for (int i = strlen(line); i < 54; i++) {
                    line[i] = ' ';
                }
                strcpy(&line[54], " | ");
                for (int i = 0; i < nbRemaining; i++) {
                    if ((buf[ofs + i] > 32) && (buf[ofs + i] < 127)) {
                        line[57 + i] = buf[ofs + i];
                    }
                    else if ((buf[ofs + i] > 160) && (buf[ofs + i] < 255)) {
                        line[57 + i] = buf[ofs + i] & 0x7F;
                    }
                }
                line[57 + nbRemaining] = 0;
            }
        }
    }
}
