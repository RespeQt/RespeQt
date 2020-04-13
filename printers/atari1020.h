#ifndef ATARI1020_H
#define ATARI1020_H

#include "atariprinter.h"
#include "graphicsprimitive.h"

enum AUTOMATA_STATES {
    AUTOMATA_START = 0,
    AUTOMATA_END,
    AUTOMATA_FIRST_NUM,
    AUTOMATA_FIRST_INT,
    AUTOMATA_COMA,
    AUTOMATA_SECOND_NUM,
    AUTOMATA_SECOND_INT,
    AUTOMATA_SECOND_COMA,
    AUTOMATA_THIRD_NUM,
    AUTOMATA_THIRD_INT,
    AUTOMATA_TEXT
};

struct GRAPHICS_COMMAND {
    unsigned char command;          // command character
    int parameters;                 // number of parameters
    bool repeat;                    // allows a list of parameters to repeat the same command using the ';' character
    enum AUTOMATA_STATES automata;  // initial state of the automata for this command
};

namespace Printers
{
    class Atari1020 : public AtariPrinter
    {
    public:
        Atari1020(SioWorkerPtr sio);

        virtual void handleCommand(const quint8 command, const quint16 aux);
        virtual bool handleBuffer(const QByteArray &buffer, const unsigned int len) override;
        virtual void setupFont() override;
        virtual void setupOutput() override;

        static QString typeName() {
            return "Atari 1020";
        }

    protected:
        bool mEsc;
        bool mStartOfLogicalLine;
        bool mGraphicsMode;
        QPointF mPenPoint;
        int mTextOrientation;
        int mFontSize;
        int mColor;
        int mLineType;
        int mScale;
        QByteArray mPrintText;
        enum AUTOMATA_STATES mAutomataState;
        unsigned char mCurrentCommand;
        bool mRepeatAllowed;
        int mParametersExpected;
        bool mFirstNegative;
        bool mSecondNegative;
        bool mThirdNegative;
        QByteArray mFirstNumber;
        QByteArray mSecondNumber;
        QByteArray mThirdNumber;

        void executeGraphicsCommand();
        void resetGraphics();
        void executeAndRepeatCommand();
        void executeGraphicsPrimitive(GraphicsPrimitive *primitive);
        bool checkGraphicsCommand(const unsigned char b);
        void handleGraphicsCodes(const unsigned char b);
        bool handlePrintableCodes(const unsigned char b);
        bool handleGraphicsMode(const QByteArray &buffer, const unsigned int len, unsigned int &i);
        int getFirstNumber(const int defaultValue = 0);
        int getSecondNumber(const int defaultValue = 0);
        int getThirdNumber(const int defaultValue = 0);
        bool drawAxis(bool xAxis, int size, int count);
        QByteArray readDataFrame(uint size);
        bool writeDataFrame(QByteArray data);
        void dumpBuffer(unsigned char *buf, int len);
        void fillBuffer(char *line, unsigned char *buf, int len, int ofs, bool dumpAscii);
    };
}
#endif // ATARI1020_H
