#ifndef EMULATOR_HPP
#define EMULATOR_HPP 1

#define MAXDRIVES 4

enum eMODULES { CPU_MODULE, VAP_MODULE, SIO_MODULE, FDC_MODULE, TRK_MODULE, CFG_MODULE, _LAST_MODULE };

enum eHARDWARE { SIO_EMULATION, HARDWARE_810, HARDWARE_810_WITH_THE_CHIP, HARDWARE_810_WITH_HAPPY, HARDWARE_1050, HARDWARE_1050_WITH_THE_ARCHIVER, HARDWARE_1050_WITH_HAPPY, HARDWARE_1050_WITH_SPEEDY, HARDWARE_1050_WITH_TURBO, HARDWARE_1050_WITH_DUPLICATOR };

#include <QtDebug>

#include "Atari810.hpp"
#include "Atari810Happy.hpp"
#include "Atari1050.hpp"
#include "Atari1050Happy.hpp"
#include "Atari1050Speedy.hpp"
#include "Atari1050Turbo.hpp"
#include "Atari1050Duplicator.hpp"
#include "Fdc.hpp"
#include "RiotDevices.hpp"
#include "Ram.hpp"
#include "Rom.hpp"
#include "RomProvider.hpp"

#endif
