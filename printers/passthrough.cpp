#include "passthrough.h"
#include "logdisplaydialog.h"
#include "math.h"
#include "respeqtsettings.h"
#include "rawoutput.h"

#include <stdexcept>
#include <QFontDatabase>
#include <QPoint>

namespace Printers
{

    Passthrough::Passthrough(SioWorkerPtr sio)
        : BasePrinter(sio)
    {}

    Passthrough::~Passthrough()
    {}

    void Passthrough::setupOutput()
    {
        BasePrinter::setupOutput();
    }

    void Passthrough::setupFont()
    {}

    bool Passthrough::handleBuffer(QByteArray &buffer, unsigned int len)
    {
        RawOutput *output;
        try {
            output = dynamic_cast<RawOutput*>(mOutput.get());
            if (output == Q_NULLPTR)
                return false;
        } catch(...)
        {
            return false;
        }

        len = std::min(static_cast<unsigned int>(buffer.count()), len);
        for(unsigned int i = 0; i < len; i++) {
            unsigned char b = static_cast<unsigned char>(buffer.at(static_cast<int>(i)));

            if (b == 155) // EOL
            {
                const char lf = 13;
                buffer.replace(static_cast<int>(i), 1, &lf);
                buffer.resize(static_cast<int>(i+1));
                break; // Drop the rest of the buffer
            }
        }

        return output->sendBuffer(buffer, static_cast<unsigned int>(buffer.size()));
    }
}
