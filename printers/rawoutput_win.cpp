#include "rawoutput.h"
#include "windows.h"

namespace Printers {
    RawOutput::RawOutput()
        :NativeOutput()
    {}

    RawOutput::~RawOutput()
    {}

    void RawOutput::updateBoundingBox()
    {}

    void RawOutput::newPage(bool /*linefeed*/)
    {}

    bool RawOutput::setupOutput()
    {
        return true;
    }

    bool RawOutput::beginOutput()
    {
        return false;
    }

    bool RawOutput::endOutput()
    {
        return true;
    }

    bool RawOutput::sendBuffer(const QByteArray &b, unsigned int len)
    {
        return false;
    }
}
