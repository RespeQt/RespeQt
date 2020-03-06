/*
 * diskimage.cpp
 *
 * Copyright 2015 Joseph Zatarski
 * Copyright 2016 TheMontezuma
 * Copyright 2018 Eric BACHER
 *
 * This file is copyrighted by either Fatih Aygun, Ray Ataergin, or both.
 * However, the years for these copyrights are unfortunately unknown. If you
 * know the specific year(s) please let the current maintainer know.
 */

#include "diskimage.h"
#include "zlib.h"

#include <QFileInfo>
#include "respeqtsettings.h"
#include <QDir>
#include "atarifilesystem.h"
#include "diskeditdialog.h"

#include <QtDebug>

// pattern to write when Super Archiver formats a track with $08 as a fill byte in a sector ($08 is interpreted as the CRC byte by FDC)
quint8 FDC_CRC_PATTERN[] = { 0x08, 0x60, 0x07 };

static quint8 ARCHIVER_SPEED_CHECK[] = {
    0x31, 0x74, 0x02, 0x00, 0x68, 0xF8, 0xA9, 0x00, 0x85, 0x8E, 0xA9, 0x01, 0x85, 0x8F, 0xAD, 0x00,
    0x04, 0x30, 0x16, 0x20, 0xF2, 0xF9, 0x20, 0x2B, 0x10, 0x20, 0x2E, 0x10, 0x20, 0x2E, 0x10, 0xAD,
    0x96, 0x02, 0x49, 0xFF, 0x8D, 0xF8, 0x10, 0x18, 0x60, 0x38, 0x60, 0x6C, 0x96, 0xF2, 0x20, 0x52,
    0xF9, 0xA9, 0xFF, 0x8D, 0x9F, 0x02, 0xA9, 0x88, 0x8D, 0x00, 0x04, 0xAD, 0x9C, 0x02, 0x2C, 0x80,
    0x02, 0x50, 0x06, 0x10, 0xF6, 0x20, 0x52, 0xF9, 0x60, 0xAD, 0x00, 0x04, 0x29, 0x01, 0xD0, 0xEB,
    0x68, 0x68, 0x20, 0x52, 0xF9, 0x20, 0x1E, 0xF7, 0x20, 0x3D, 0xF7, 0x38, 0x60, 0xA9, 0x5E, 0xA2,
    0x64, 0x20, 0x37, 0x63, 0x20, 0x49, 0x65, 0xA9, 0x85, 0xA2, 0x64, 0x20, 0x37, 0x63, 0xA5, 0x54,
    0x8D, 0x79, 0x68, 0xA9, 0x00, 0x8D, 0x59, 0x69, 0xA9, 0x31, 0x8D, 0x00, 0x03, 0xAD, 0x59, 0x64,
    0x8D, 0x01, 0x03, 0xA9, 0x75, 0x8D, 0x02, 0x03, 0xA9, 0x80, 0x8D, 0x03, 0x03, 0xA9, 0x84, 0x8D,
    0x04, 0x03, 0xA9, 0x66, 0x8D, 0x05, 0x03, 0xA9, 0x0A, 0x8D, 0x06, 0x03, 0xA9, 0x00, 0x8D, 0x08,
    0x03, 0xA9, 0x01, 0x8D, 0x09, 0x03, 0x20, 0x59, 0xE4, 0x10, 0x0A, 0xC0, 0x90, 0xD0, 0x03, 0x4C,
    0x8D, 0x69, 0x4C, 0x5A, 0x69, 0xA9, 0x74, 0x8D, 0x02, 0x03, 0xA9, 0x40, 0x8D, 0x03, 0x03, 0xA9,
    0x00, 0x8D, 0x04, 0x03, 0xA9, 0x5F, 0x8D, 0x05, 0x03, 0x20, 0x59, 0xE4, 0x30, 0xE4, 0xAD, 0xF8,
    0x5F, 0x85, 0xD4, 0xA9, 0x00, 0x85, 0xD5, 0x20, 0xAA, 0xD9, 0x20, 0xB6, 0xDD, 0xA0, 0x05, 0xA9,
    0x00, 0x99, 0x71, 0x68, 0x88, 0x10, 0xFA, 0xA2, 0x5E, 0xA0, 0x68, 0x20, 0x89, 0xDD, 0x20, 0x28,
    0xDB, 0x20, 0xD2, 0xD9, 0xA5, 0xD4, 0x8D, 0x77, 0xCC, 0xA5, 0xD5, 0x8D, 0x78, 0x68, 0x20, 0xAA
};

static quint8 ARCHIVER_DIAGNOSTIC[] = {
    0x31, 0x74, 0x02, 0x00, 0x30, 0x8D, 0xFF, 0x10, 0x49, 0xFF, 0xAA, 0x20, 0x37, 0x10, 0xD0, 0x25,
    0xAE, 0xFF, 0x10, 0x20, 0x37, 0x10, 0xD0, 0x1D, 0xEE, 0x38, 0x10, 0xEE, 0x3B, 0x10, 0xEE, 0x3E,
    0x10, 0xD0, 0xDD, 0xEE, 0x39, 0x10, 0xEE, 0x3C, 0x10, 0xEE, 0x3F, 0x10, 0xAD, 0x3F, 0x10, 0xC9,
    0x18, 0x90, 0xCD, 0xB0, 0x0C, 0x38, 0x60, 0xAD, 0x00, 0x18, 0x8E, 0x00, 0x18, 0xEC, 0x00, 0x18,
    0x60, 0xA9, 0x00, 0x8D, 0xF8, 0x10, 0x8D, 0xF9, 0x10, 0xA8, 0xB9, 0x00, 0x20, 0x18, 0x6D, 0xF8,
    0x10, 0x8D, 0xF8, 0x10, 0xAD, 0xF9, 0x10, 0x69, 0x00, 0x8D, 0xF9, 0x10, 0xC8, 0xD0, 0xEB, 0xEE,
    0x4C, 0x10, 0xAD, 0x4C, 0x10, 0xC9, 0x20, 0x90, 0xE1, 0xAD, 0x00, 0xF1, 0x8D, 0xFA, 0x10, 0xAD,
    0x01, 0xF1, 0x8D, 0xFB, 0x10, 0x18, 0x60, 0x45, 0x3A, 0x9B, 0x20, 0x52, 0x10, 0xD0, 0x1F, 0x8D,
    0xFF, 0x10, 0x49, 0xFF, 0xAA, 0x20, 0x52, 0x10, 0xD0, 0x14, 0xAE, 0xFF, 0x10, 0x20, 0x52, 0x10,
    0xD0, 0x0C, 0xEE, 0x53, 0x10, 0xEE, 0x55, 0x10, 0xEE, 0x57, 0x10, 0x10, 0xDD, 0x2C, 0x38, 0x60,
    0x20, 0x52, 0x10, 0xD0, 0x25, 0x8D, 0xFF, 0x10, 0x49, 0xFF, 0xAA, 0x20, 0x52, 0x10, 0xD0, 0x1A,
    0xAE, 0xFF, 0x10, 0x20, 0x52, 0x10, 0xD0, 0x12, 0xEE, 0x53, 0x10, 0xEE, 0x55, 0x10, 0xEE, 0x57,
    0x10, 0xAD, 0x57, 0x10, 0xC9, 0xF0, 0xD0, 0xD8, 0x18, 0x60, 0x38, 0x60, 0xA5, 0x00, 0x86, 0x00,
    0xE4, 0x00, 0x60, 0x85, 0xCB, 0x86, 0xCC, 0xAD, 0x20, 0x01, 0xC9, 0x50, 0xD0, 0xEE, 0xA9, 0x0B,
    0xA2, 0x00, 0x9D, 0x42, 0x03, 0x8E, 0x48, 0x03, 0x8E, 0x49, 0x03, 0xA0, 0x00, 0xB1, 0xCB, 0xF0,
    0x0C, 0x20, 0x56, 0xE4, 0xE6, 0xCB, 0xD0, 0xF3, 0xB9, 0xC7, 0x01, 0x06, 0x63, 0x60, 0x7D, 0xFF
};

// Signature sent by Happy at $0800 to execute code in drive
static quint8 HAPPY_SIGNATURE[] = { 0x26, 0x11, 0x34, 0x14, 0x15, 0x57, 0x37, 0x85, 0x86 };

static quint8 HAPPY_RAM_CHECK[] = {
    0x00, 0x00, 0x01, 0x60, 0xE6, 0x80, 0xD0, 0x02, 0xE6, 0x81, 0x60, 0xA9, 0xA9, 0x85, 0x80, 0xA9,
    0x08, 0x85, 0x81, 0x60, 0x49, 0x01, 0xA2, 0x08, 0x91, 0x80, 0xD1, 0x80, 0xD0, 0xD1, 0x18, 0xC9,
    0x00, 0x10, 0x01, 0x38, 0x2A, 0xCA, 0xD0, 0xF0, 0x60, 0x26, 0x11, 0x34, 0x14, 0x15, 0x57, 0x37,
    0x85, 0x86, 0x4C, 0x37, 0x1F, 0x4C, 0x21, 0x08, 0x4C, 0x80, 0x14, 0x4C, 0x36, 0x1D, 0x4C, 0xC3,
    0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0x2D, 0x14, 0x20, 0x77, 0x1C, 0xA9, 0x00, 0x85,
    0x84, 0xA9, 0x14, 0x85, 0x85, 0xA9, 0xFD, 0x85, 0x86, 0xA9, 0x17, 0x85, 0x87, 0xAD, 0xDE, 0x1D,
    0xC9, 0x07, 0xD0, 0x08, 0xA9, 0x01, 0x85, 0x82, 0xA9, 0x9D, 0xD0, 0x06, 0xA9, 0x0C, 0x85, 0x82,
    0xA9, 0xD5, 0x85, 0x83, 0x20, 0x81, 0x08, 0xA9, 0x00, 0x85, 0x84, 0xA9, 0x18, 0x85, 0x85, 0xA9
};

static quint8 HAPPY_ROM810_CHECK[] = {
    0x38, 0xFA, 0x38, 0xFA, 0xCA, 0xF0, 0xED, 0xE6, 0x93, 0xA5, 0x93, 0xC9, 0x20, 0xD0, 0xC0, 0x60,
    0x38, 0xFA, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static quint8 HAPPY_ROM1050_CHECK[] = {
    0x2D, 0x80, 0x43, 0x16, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xC8, 0xD8, 0xBA, 0x24, 0xB7, 0x7C, 0x8F, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBD, 0x80, 0xFF,
    0x97, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x09, 0x00, 0x40, 0xCD
};

static quint8 HAPPY_RAM_EXTENDED_CHECK[] = {
    0x00, 0x00, 0xA9, 0x02, 0xFF, 0x1F, 0xFF, 0x1F, 0x84, 0x18, 0x65, 0x80, 0x85, 0x80, 0xA5, 0x81,
    0x69, 0x00, 0x18, 0x10, 0x01, 0x38, 0x26, 0x80, 0x2A, 0x50, 0x07, 0x38, 0x30, 0x01, 0x18, 0x26,
    0x80, 0x2A, 0x49, 0x12, 0x85, 0x81, 0xA5, 0x80, 0x49, 0x5A, 0x85, 0x80, 0xE6, 0x84, 0xD0, 0x02,
    0xE6, 0x85, 0xA5, 0x84, 0xC5, 0x86, 0xD0, 0xCF, 0xA5, 0x85, 0xC5, 0x87, 0xD0, 0xC9, 0xA5, 0x80,
    0xC5, 0x82, 0xD0, 0xBA, 0xA5, 0x81, 0xC5, 0x83, 0xD0, 0xB4, 0x60, 0x26, 0x11, 0x34, 0x14, 0x15,
    0x57, 0x37, 0x85, 0x86, 0x4C, 0x37, 0x1F, 0x4C, 0x21, 0x08, 0x4C, 0x80, 0x14, 0x4C, 0x36, 0x1D,
    0x4C, 0xC3, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0x2D, 0x14, 0x20, 0x77, 0x1C, 0xA9,
    0x01, 0x85, 0x83, 0xA9, 0x00, 0x85, 0x86, 0xA9, 0x56, 0x85, 0x87, 0xA9, 0x5D, 0x85, 0x88, 0xA2
};

static quint8 HAPPY_DIAGNOSTIC[] = {
    0x00, 0x00, 0x00, 0x02, 0x03, 0xD0, 0x01, 0x01, 0x05, 0xF8, 0xF0, 0x1F, 0xCC, 0x9C, 0x03, 0xD0,
    0x20, 0xE4, 0x86, 0x90, 0x22, 0xE4, 0x88, 0xB0, 0x24, 0x98, 0xF0, 0x08, 0x88, 0xA6, 0x87, 0x86,
    0x86, 0x4C, 0x00, 0x09, 0x60, 0x85, 0x80, 0xA9, 0x00, 0xF0, 0x16, 0x85, 0x80, 0xA9, 0x01, 0xD0,
    0x10, 0x85, 0x80, 0xA9, 0x02, 0xD0, 0x0A, 0x85, 0x80, 0xA9, 0x03, 0xD0, 0x04, 0x85, 0x80, 0xA9,
    0x04, 0x85, 0x84, 0xAD, 0x9C, 0x03, 0x85, 0x85, 0x86, 0x81, 0x84, 0x82, 0x4C, 0x51, 0x19, 0x26,
    0x11, 0x34, 0x14, 0x15, 0x57, 0x37, 0x85, 0x86, 0x4C, 0x37, 0x1F, 0x4C, 0x21, 0x08, 0x4C, 0x80,
    0x14, 0x4C, 0x36, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0x2D, 0x14,
    0x20, 0x77, 0x1C, 0xAD, 0xF9, 0x1F, 0x20, 0x43, 0x08, 0xA5, 0x90, 0x85, 0x82, 0xA5, 0x91, 0x85
};

static quint8 HAPPY_SPEED_CHECK[] = {
    0xF7, 0x16, 0xD0, 0xED, 0x4C, 0x67, 0x19, 0x98, 0xC5, 0x03, 0xA0, 0x01, 0xB8, 0x70, 0xF1, 0xCD,
    0x9C, 0x03, 0x2C, 0x80, 0x03, 0x10, 0xF6, 0xC5, 0x03, 0xC8, 0x10, 0xF6, 0x85, 0x81, 0x86, 0x80,
    0x4C, 0x51, 0x19, 0x26, 0x11, 0x34, 0x14, 0x15, 0x57, 0x37, 0x85, 0x86, 0x4C, 0x37, 0x1F, 0x4C,
    0x21, 0x08, 0x4C, 0x80, 0x14, 0x4C, 0x36, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0xC3,
    0x1D, 0x4C, 0x2D, 0x14, 0x20, 0x77, 0x1C, 0x20, 0x8B, 0x08, 0xA0, 0x00, 0xA9, 0x00, 0x91, 0x80,
    0x20, 0x75, 0x08, 0x90, 0xF7, 0x20, 0x8B, 0x08, 0xB1, 0x80, 0xCD, 0x74, 0x08, 0xD0, 0x33, 0x20,
    0x94, 0x08, 0xAD, 0x74, 0x08, 0x49, 0xFF, 0x20, 0x94, 0x08, 0xAD, 0x74, 0x08, 0x49, 0xFF, 0x91,
    0x80, 0x20, 0x75, 0x08, 0x90, 0xE2, 0xAD, 0x74, 0x08, 0x49, 0xFF, 0x8D, 0x74, 0x08, 0xD0, 0xD5
};

static quint8 HAPPY_RW_CHECK[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static quint8 HAPPY_BACKUP_BUF1[] = {
    0x99, 0x90, 0x44, 0x77, 0x85, 0x00, 0xAD, 0x9C, 0x03, 0xC9, 0x8E, 0x90, 0x13, 0x2C, 0x80, 0x03,
    0x10, 0xF4, 0xA5, 0x03, 0xCA, 0xD0, 0xF6, 0x24, 0x00, 0xF0, 0xFC, 0xA5, 0x00, 0x85, 0x81, 0x60,
    0xA9, 0x45, 0x4C, 0x53, 0x19, 0xAD, 0x2D, 0x06, 0x10, 0x38, 0x20, 0xF5, 0x26, 0x9B, 0x9B, 0x43,
    0x41, 0x4E, 0x4E, 0x4F, 0x54, 0x20, 0x44, 0x4F, 0x20, 0x54, 0x48, 0x49, 0x53, 0x20, 0x57, 0x49,
    0x54, 0x48, 0x20, 0x53, 0x54, 0x41, 0x4E, 0x44, 0x41, 0x52, 0x44, 0x9B, 0x52, 0x41, 0x4D, 0x20,
    0x44, 0x49, 0x53, 0x4B, 0x20, 0x45, 0x4E, 0x41, 0x42, 0x4C, 0x45, 0x44, 0x9B, 0x9B, 0x00, 0x4C,
    0xC6, 0x26, 0x20, 0x17, 0x29, 0xA2, 0x01, 0xBD, 0x28, 0x06, 0x10, 0x03, 0xE8, 0xD0, 0xF8, 0x8A,
    0x8E, 0x01, 0x03, 0x8E, 0x8D, 0x29, 0x18, 0x69, 0x30, 0x8D, 0x78, 0x2C, 0x8D, 0xA4, 0x2C, 0x20
};

static quint8 HAPPY_BACKUP_BUF2[] = {
    0x90, 0x44, 0x34, 0x14, 0x15, 0x57, 0x37, 0x85, 0x86, 0x4C, 0x37, 0x1F, 0x4C, 0x21, 0x08, 0x4C,
    0x80, 0x14, 0x4C, 0x36, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0x2D,
    0x14, 0x20, 0x77, 0x1C, 0xAD, 0xFE, 0x17, 0x85, 0x80, 0xAD, 0xFF, 0x17, 0x85, 0x81, 0xA2, 0x00,
    0xCA, 0xD0, 0xFD, 0x4C, 0x51, 0x19, 0x26, 0x11, 0x34, 0x14, 0x15, 0x57, 0x37, 0x85, 0x86, 0x4C,
    0x37, 0x1F, 0x4C, 0x21, 0x08, 0x4C, 0x80, 0x14, 0x4C, 0x36, 0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0xC3,
    0x1D, 0x4C, 0xC3, 0x1D, 0x4C, 0x2D, 0x14, 0x20, 0x77, 0x1C, 0x20, 0xAF, 0x17, 0x20, 0xCD, 0x1F,
    0xAD, 0xDD, 0x01, 0x4A, 0x4A, 0x4A, 0x90, 0x05, 0xA0, 0x2B, 0x20, 0x8B, 0x1C, 0xA9, 0xFF, 0x20,
    0x1B, 0x1C, 0x20, 0x18, 0x1F, 0x20, 0x63, 0x1F, 0xA9, 0x6C, 0x20, 0xE1, 0x18, 0x20, 0x64, 0x08
};

static quint8 HAPPY_INITIAL_SECTOR_LIST[] = {
    0x00, 0x18, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80, 0x43, 0xFF, 0x12,
    0x00, 0x01, 0x23, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF9, 0xFB, 0xFD, 0xEE, 0xF0, 0xF2,
    0xF4, 0xF6, 0xF8, 0xFA, 0xFC, 0xFE, 0xED, 0xEF, 0xF1, 0xF3, 0xF5, 0xF7, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#if HAPPY_PATCH_V53
static quint8 HAPPY_DOUBLESPEEDREV5_PATCH1[] = {
    0x54, 0x8D, 0x02, 0x03, 0xA9, 0x80, 0x8D, 0x03, 0x03, 0x85, 0xEE, 0x20, 0x48, 0x01, 0xAD, 0x0B,
    0x03, 0xCE, 0x0B, 0x03, 0xC9, 0xED, 0xD0, 0x0B, 0x20, 0x92, 0x06, 0xCE, 0x0A, 0x03, 0xA9, 0xFE,
    0x8D, 0x0B, 0x03, 0x20, 0xD0, 0x06, 0x90, 0xD2, 0x08, 0x20, 0x92, 0x06, 0x28, 0xF0, 0x12, 0xA5,
    0xEC, 0x85, 0xE5, 0xA5, 0xED, 0x85, 0xE6, 0xA5, 0xE0, 0xD0, 0x03, 0x4C, 0xDE, 0x04, 0x4C, 0xFE,
    0x04, 0x20, 0x80, 0x00, 0x44, 0x4F, 0x4E, 0x45, 0x2C, 0x20, 0x00, 0x20, 0x64, 0x06, 0x4C, 0x03,
    0x04, 0x20, 0x80, 0x00, 0x50, 0x55, 0x54, 0x20, 0x44, 0x45, 0x53, 0x20, 0x44, 0x53, 0x4B, 0x20,
    0x44, 0x00, 0x9B, 0x00, 0x20, 0x80, 0x00, 0x48, 0x49, 0x54, 0x20, 0x52, 0x45, 0x54, 0x4E, 0x9B,
    0x9B, 0x00, 0x4C, 0x73, 0x07, 0x18, 0xA5, 0xF0, 0xF0, 0x17, 0xAD, 0x04, 0x03, 0x8D, 0x89, 0x06
};

static quint8 HAPPY_DOUBLESPEEDREV5_PATCH2[] = {
    0x9B, 0x9B, 0x43, 0x41, 0x4E, 0x4E, 0x4F, 0x54, 0x20, 0x43, 0x4F, 0x4E, 0x54, 0x49, 0x4E, 0x55,
    0x45, 0x00, 0x4C, 0x12, 0x0C, 0xA9, 0x10, 0x8D, 0x04, 0xD2, 0xAD, 0x03, 0x03, 0x10, 0x0F, 0x20,
    0x00, 0x00, 0x20, 0x00, 0x00, 0xA5, 0x30, 0xC9, 0x01, 0xD0, 0xE7, 0x4C, 0x00, 0x00, 0x4C, 0x00,
    0x00, 0xA5, 0xE1, 0x85, 0xEE, 0xA5, 0xE2, 0x85, 0xEF, 0x4C, 0x4F, 0x05, 0xA0, 0x54, 0x20, 0x00,
    0x13, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xEA, 0xA9, 0x00, 0x85, 0xF3, 0x85, 0xF1, 0xAD, 0xE5, 0x02,
    0x38, 0xE9, 0x15, 0x85, 0xE7, 0xAD, 0xE6, 0x02, 0xE9, 0x08, 0x85, 0xE8, 0xA5, 0xE7, 0x0A, 0xA5,
    0xE8, 0x2A, 0x85, 0xE7, 0xA9, 0x00, 0x2A, 0x85, 0xE8, 0xA9, 0x00, 0x85, 0xE2, 0xA9, 0x01, 0x85,
    0xE1, 0xA9, 0x02, 0x85, 0xE4, 0xA9, 0xD0, 0x85, 0xE3, 0xA9, 0x00, 0x85, 0xF0, 0xA0, 0x00, 0xA2
};

static quint8 HAPPY_DOUBLESPEEDREV5_PATCH3[] = {
    0xB9, 0x33, 0x12, 0x99, 0x7F, 0x00, 0x88, 0xD0, 0xF7, 0xA2, 0x0B, 0xBD, 0x39, 0x13, 0x9D, 0x00,
    0x03, 0xCA, 0x10, 0xF7, 0xA9, 0xE0, 0x8D, 0x7E, 0x13, 0x20, 0x59, 0xE4, 0xAE, 0x3A, 0x13, 0xAD,
    0x7E, 0x13, 0x9D, 0x40, 0x01, 0xCA, 0xF0, 0x05, 0xCE, 0x3A, 0x13, 0xD0, 0xDC, 0xA2, 0x2A, 0xBD,
    0x45, 0x13, 0x9D, 0x48, 0x01, 0xCA, 0x10, 0xF7, 0x60, 0x31, 0x04, 0x53, 0x40, 0x7C, 0x13, 0x0F,
    0x00, 0x04, 0x00, 0xCD, 0xAB, 0xA9, 0x04, 0x85, 0xF6, 0xAD, 0x03, 0x03, 0x85, 0xF7, 0x20, 0x63,
    0x01, 0x30, 0x01, 0x60, 0xA5, 0xF7, 0x8D, 0x03, 0x03, 0xC6, 0xF6, 0xD0, 0xF1, 0x4C, 0x3F, 0x07,
    0xAC, 0x01, 0x03, 0xB9, 0x40, 0x01, 0xC9, 0xEB, 0xF0, 0x03, 0x4C, 0x89, 0x07, 0x4C, 0x59, 0xE4,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
#endif

static quint8 HAPPY_DOUBLESPEEDREV7_PATCH1[] = {
    0x8E, 0x32, 0x41, 0x8C, 0x33, 0x41, 0x8E, 0x8A, 0x41, 0x8C, 0x8B, 0x41, 0x20, 0x00, 0x4C, 0x8E,
    0xE5, 0x40, 0x8C, 0xE6, 0x40, 0x8E, 0x7B, 0x41, 0x8C, 0x7C, 0x41, 0xA8, 0x10, 0x49, 0xA5, 0xF3,
    0x24, 0xF3, 0x50, 0x05, 0x20, 0x81, 0x40, 0x30, 0x49, 0x24, 0xF4, 0x50, 0x11, 0xA9, 0x64, 0x8D,
    0xD9, 0x41, 0xA5, 0xF4, 0x20, 0x81, 0x40, 0x30, 0x39, 0xA9, 0x60, 0x8D, 0xD9, 0x41, 0xA9, 0xFF,
    0x8D, 0x9A, 0x4B, 0xA5, 0xF1, 0xC9, 0x03, 0x90, 0x2E, 0xA5, 0xF5, 0x24, 0xF5, 0x50, 0x05, 0x20,
    0x81, 0x40, 0x30, 0x1E, 0xA5, 0xF1, 0xC9, 0x04, 0x90, 0x1D, 0xA5, 0xF6, 0x24, 0xF6, 0x50, 0x17,
    0x20, 0x81, 0x40, 0x30, 0x0D, 0x10, 0x10, 0xA5, 0xF1, 0x24, 0xF1, 0x50, 0x06, 0x20, 0x81, 0x40,
    0x10, 0x01, 0x60, 0xA5, 0xF3, 0xF0, 0x04, 0xA9, 0x00, 0x10, 0xF7, 0xA5, 0xF5, 0x24, 0xF5, 0x50
};

static quint8 HAPPY_DOUBLESPEEDREV7_PATCH2[] = {
    0x8D, 0x9C, 0x4B, 0x8E, 0x78, 0x4C, 0x8C, 0x79, 0x4C, 0xA2, 0x0B, 0xBD, 0x00, 0x03, 0x9D, 0x50,
    0x01, 0xCA, 0x10, 0xF7, 0xA2, 0x0B, 0xBD, 0x5C, 0x4C, 0x9D, 0x00, 0x03, 0xCA, 0x10, 0xF7, 0xA9,
    0xE0, 0x8D, 0x82, 0x4C, 0x20, 0x59, 0xE4, 0xAE, 0x5D, 0x4C, 0xAD, 0x82, 0x4C, 0x9D, 0x40, 0x01,
    0xCA, 0xF0, 0x05, 0xCE, 0x5D, 0x4C, 0xD0, 0xDC, 0xA2, 0x0B, 0xBD, 0x50, 0x01, 0x9D, 0x00, 0x03,
    0xCA, 0x10, 0xF7, 0xA2, 0x14, 0xBD, 0x68, 0x4C, 0x9D, 0x48, 0x01, 0xCA, 0x10, 0xF7, 0xA2, 0x48,
    0xA0, 0x01, 0x8E, 0x1D, 0x1E, 0x8C, 0x1E, 0x1E, 0xAD, 0x9C, 0x4B, 0x60, 0x31, 0x04, 0x53, 0x40,
    0x80, 0x4C, 0x0F, 0x00, 0x04, 0x00, 0xCD, 0xAB, 0x2C, 0x03, 0x03, 0x10, 0x0A, 0xAC, 0x01, 0x03,
    0xB9, 0x40, 0x01, 0xC9, 0xEB, 0xF0, 0x03, 0x4C, 0xAA, 0xAA, 0x4C, 0x59, 0xE4, 0x00, 0x00, 0x00
};

/* DiskGeometry */

DiskGeometry::DiskGeometry()
    :QObject()
{
    m_isDoubleSided = false;
    m_tracksPerSide = 0;
    m_sectorsPerTrack = 0;
    m_bytesPerSector = 0;
    m_totalSize = 0;
    m_sectorCount = 0;
}

DiskGeometry::DiskGeometry(const DiskGeometry &other)
    :QObject()
{
    initialize(other);
}

void DiskGeometry::initialize(const DiskGeometry &other)
{
    m_isDoubleSided = other.isDoubleSided();
    m_tracksPerSide = other.tracksPerSide();
    m_sectorsPerTrack = other.sectorsPerTrack();
    m_bytesPerSector = other.bytesPerSector();
    m_sectorCount = other.sectorCount();
    m_totalSize = other.totalSize();
}

void DiskGeometry::initialize(bool aIsDoubleSided, quint8 aTracksPerSide, quint16 aSectorsPerTrack, quint16 aBytesPerSector)
{
    m_isDoubleSided = aIsDoubleSided;
    m_tracksPerSide = aTracksPerSide;
    m_sectorsPerTrack = aSectorsPerTrack;
    m_bytesPerSector = aBytesPerSector;
    m_sectorCount = (m_isDoubleSided + 1) * m_tracksPerSide * m_sectorsPerTrack;
    if (m_bytesPerSector == 256) {
        m_totalSize = m_sectorCount * 128;
        if (m_totalSize > 384) {
            m_totalSize += (m_bytesPerSector - 128) * (m_sectorCount - 3);
        }
    } else {
        m_totalSize = m_sectorCount * m_bytesPerSector;
    }
}

void DiskGeometry::initialize(uint aTotalSize, quint16 aBytesPerSector)
{
    bool ds;
    quint8 tps;
    quint16 spt;

    if (aTotalSize == 92160 && aBytesPerSector == 128) {
        ds = false;
        tps = 40;
        spt = 18;
    } else if (aTotalSize == 133120 && aBytesPerSector == 128) {
        ds = false;
        tps = 40;
        spt = 26;
    } else if (((aTotalSize == 183936) || (aTotalSize == 184320)) && aBytesPerSector == 256) { // ATR or XFD
        ds = false;
        tps = 40;
        spt = 18;
    } else if (aTotalSize == 368256 && aBytesPerSector == 256) {
        ds = true;
        tps = 40;
        spt = 18;
    } else if (aTotalSize == 736896 && aBytesPerSector == 256) {
        ds = true;
        tps = 80;
        spt = 18;
    } else {
        if (aBytesPerSector == 256) {
            if (aTotalSize <= 384) {
                spt = (aTotalSize + 127) / 128;
            } else {
                spt = (aTotalSize + 384 + 255) / 256;
            }
        } else {
            spt = (aTotalSize + aBytesPerSector - 1) / aBytesPerSector;
        }
        ds = false;
        tps = 1;
    }

    initialize(ds, tps, spt, aBytesPerSector);
}

void DiskGeometry::initialize(uint aTotalSize)
{
    bool ds;
    quint8 tps;
    quint16 spt;
    quint16 bps;

    if (aTotalSize == 92160) {
        ds = false;
        tps = 40;
        spt = 18;
        bps = 128;
    } else if (aTotalSize == 133120) {
        ds = false;
        tps = 40;
        spt = 26;
        bps = 128;
    } else if ((aTotalSize == 183936) || (aTotalSize == 184320)) { // ATR or XFD
        ds = false;
        tps = 40;
        spt = 18;
        bps = 256;
    } else if (aTotalSize == 368256) {
        ds = true;
        tps = 40;
        spt = 18;
        bps = 256;
    } else if (aTotalSize == 736896) {
        ds = true;
        tps = 80;
        spt = 18;
        bps = 256;
    } else {
        if ((aTotalSize - 384) % 256 == 0) {
            spt = (aTotalSize - 384) / 256;
            bps = 256;
        } else {
            spt = (aTotalSize + 127) / 128;
            bps = 128;
        }
        ds = false;
        tps = 1;
    }

    initialize(ds, tps, spt, bps);
}

void DiskGeometry::initialize(const QByteArray &percom)
{
    quint8 aTracksPerSide = (quint8)percom.at(0);
    quint16 aSectorsPerTrack = (quint8)percom.at(2) * 256 + (quint8)percom.at(3);
    bool aIsDoubleSided = (quint8)percom.at(4);
    quint16 aBytesPerSector = (quint8)percom.at(6) * 256 + (quint8)percom.at(7);
    initialize(aIsDoubleSided, aTracksPerSide, aSectorsPerTrack, aBytesPerSector);
}

bool DiskGeometry::isEqual(const DiskGeometry &other)
{
    return
            m_isDoubleSided == other.isDoubleSided() &&
            m_tracksPerSide == other.tracksPerSide() &&
            m_sectorsPerTrack == other.sectorsPerTrack() &&
            m_bytesPerSector == other.bytesPerSector();
}

bool DiskGeometry::isStandardSD() const
{
    return (!m_isDoubleSided) && m_tracksPerSide == 40 && m_sectorsPerTrack == 18 && m_bytesPerSector == 128;
}

bool DiskGeometry::isStandardED() const
{
    return (!m_isDoubleSided) && m_tracksPerSide == 40 && m_sectorsPerTrack == 26 && m_bytesPerSector == 128;
}

bool DiskGeometry::isStandardDD() const
{
    return (!m_isDoubleSided) && m_tracksPerSide == 40 && m_sectorsPerTrack == 18 && m_bytesPerSector == 256;
}

bool DiskGeometry::isStandardDSDD() const
{
    return m_isDoubleSided && m_tracksPerSide == 40 && m_sectorsPerTrack == 18 && m_bytesPerSector == 256;
}

bool DiskGeometry::isStandardDSQD() const
{
    return m_isDoubleSided && m_tracksPerSide == 80 && m_sectorsPerTrack == 18 && m_bytesPerSector == 256;
}

quint16 DiskGeometry::bytesPerSector(quint16 sector)
{
    quint16 result = m_bytesPerSector;
    if (result == 256 && sector <= 3) {
        result = 128;
    }
    return result;
}

QByteArray DiskGeometry::toPercomBlock()
{
    DiskGeometry temp;
    QByteArray percom(12, 0);
    percom[0] = m_tracksPerSide;
    percom[1] = 1; // Step rate
    percom[2] = m_sectorsPerTrack / 256;
    percom[3] = m_sectorsPerTrack % 256;
    percom[4] = m_isDoubleSided;
    percom[5] = (m_bytesPerSector != 128) * 4 | (m_tracksPerSide == 77) * 2;
    percom[6] = m_bytesPerSector / 256;
    percom[7] = m_bytesPerSector % 256;
    percom[8] = 0xff;
    temp.initialize(percom);
    return percom;
}

QString DiskGeometry::humanReadable() const
{
    QString result, density, sides;

    if (isStandardSD()) {
        result = tr("SD Diskette");
    } else if (isStandardED()) {
        result = tr("ED Diskette");
    } else if (isStandardDD()) {
        result = tr("DD Diskette");
    } else if (isStandardDSDD()) {
        result = tr("DS/DD Diskette");
    } else if (isStandardDSQD()) {
        result = tr("DS/DD Diskette");
    } else if (m_tracksPerSide == 1) {
        if (m_bytesPerSector == 128) {
            result = tr("%1 sector SD hard disk").arg(m_sectorCount);
        } else if (m_bytesPerSector == 256) {
            result = tr("%1 sector DD hard disk").arg(m_sectorCount);
        } else {
            result = tr("%1 sector, %2 bytes/sector hard disk").arg(m_sectorCount).arg(m_bytesPerSector);
        }
    } else {
        result = tr("%1 %2 tracks/side, %3 sectors/track, %4 bytes/sector diskette")
                 .arg(m_isDoubleSided?tr("DS"):tr("SS"))
                 .arg(m_tracksPerSide)
                 .arg(m_sectorsPerTrack)
                 .arg(m_bytesPerSector);
    }

    return tr("%1 (%2k)").arg(result).arg((m_totalSize + 512) / 1024);
}

/* Board (Happy or Super Archiver) */

Board::Board()
    : QObject()
{
    m_chipOpen = false;
    m_happyEnabled = false;
    m_happy1050 = false;
    m_lastArchiverUploadCrc16 = 0;
    m_lastHappyUploadCrc16 = 0;
    m_happyPatchInProgress = false;
    m_translatorActive = false;
    m_translatorState = NOT_BOOTED;
}

Board::~Board()
{
}

Board *Board::getCopy()
{
    Board *copy = new Board();
    copy->m_chipOpen = m_chipOpen;
    memcpy(copy->m_chipRam, m_chipRam, sizeof(m_chipRam));
    copy->m_lastArchiverUploadCrc16 = m_lastArchiverUploadCrc16;
    copy->m_trackData.append(m_trackData);
    copy->m_lastArchiverSpeed = m_lastArchiverSpeed;
    copy->m_happyEnabled = m_happyEnabled;
    copy->m_happy1050 = m_happy1050;
    copy->m_happyRam.append(m_happyRam);
    copy->m_lastHappyUploadCrc16 = m_lastHappyUploadCrc16;
    copy->m_happyPatchInProgress = m_happyPatchInProgress;
    copy->m_translatorActive = false;
    copy->m_translatorState = m_translatorState;
    return copy;
}

void Board::setFromCopy(Board *copy)
{
    m_chipOpen = copy->m_chipOpen;
    memcpy(m_chipRam, copy->m_chipRam, sizeof(m_chipRam));
    m_lastArchiverUploadCrc16 = copy->m_lastArchiverUploadCrc16;
    m_trackData.clear();
    m_trackData.append(copy->m_trackData);
    m_lastArchiverSpeed = copy->m_lastArchiverSpeed;
    m_happyEnabled = copy->m_happyEnabled;
    m_happy1050 = copy->m_happy1050;
    m_happyRam.clear();
    m_happyRam.append(copy->m_happyRam);
    m_lastHappyUploadCrc16 = copy->m_lastHappyUploadCrc16;
    m_happyPatchInProgress = copy->m_happyPatchInProgress;
    m_translatorActive = copy->m_translatorActive;
    m_translatorState = copy->m_translatorState;
}

bool Board::hasHappySignature()
{
    unsigned char *ram = (unsigned char *)m_happyRam.data();
    for (unsigned int i = 0; i < sizeof(HAPPY_SIGNATURE); i++) {
        if (ram[i] != HAPPY_SIGNATURE[i]) {
            return false;
        }
    }
    return true;
}

/* SimpleDiskImage */

SimpleDiskImage::SimpleDiskImage(SioWorker *worker)
    : SioDevice(worker)
{
    m_editDialog = 0;
    m_displayTransmission = false;
    m_dumpDataFrame = false;
    m_displayTrackLayout = false;
    m_wd1771Status = 0xFF;
    m_isLeverOpen = false;
    m_isReady = false;
    m_timer.start();
    m_conversionInProgress = false;
    m_translatorAutomaticDetection = false;
    m_OSBMode = false;
    m_translatorDisk = NULL;
}

SimpleDiskImage::~SimpleDiskImage()
{
    closeTranslator();
    if (isOpen()) {
        close();
    }
}

void SimpleDiskImage::closeTranslator()
{
    if (m_translatorDisk != NULL) {
        m_translatorDisk->close();
        m_translatorDisk = NULL;
    }
}

QString SimpleDiskImage::description() const
{
    QString sides;
    if (m_numberOfSides > 1) {
        sides.append(" - ").append(tr("Image %1/%2").arg(m_currentSide).arg(m_numberOfSides));
    }
    if (m_board.isChipOpen()) {
        return QString(m_geometry.humanReadable()).append(" - ").append(tr("CHIP mode")).append(sides);
    }
    else if (m_board.isHappyEnabled()) {
        return QString(m_geometry.humanReadable()).append(" - ").append(tr("HAPPY mode")).append(sides);
    }
    else {
        return m_geometry.humanReadable().append(sides);
    }
}

QString SimpleDiskImage::getNextSideLabel()
{
    int nextSide = m_currentSide + 1;
    if (nextSide > m_numberOfSides) {
        nextSide = 1;
    }
    return QString(tr("Load image %1 of %2:\n%3").arg(nextSide).arg(m_numberOfSides).arg(m_nextSideFilename));
}

void SimpleDiskImage::refreshNewGeometry()
{
    m_newGeometry.initialize(m_geometry);
}

void SimpleDiskImage::setChipMode(bool enable)
{
    m_board.setChipOpen(enable);
    m_board.setLastArchiverUploadCrc16(0);
    if (m_board.isChipOpen()) {
        m_board.setHappyEnabled(false);
    }
    emit statusChanged(m_deviceNo);
}

void SimpleDiskImage::setHappyMode(bool enable)
{
    m_board.setHappyEnabled(enable);
    m_board.setLastHappyUploadCrc16(0);
    if (m_board.isHappyEnabled()) {
        m_board.setChipOpen(false);
        if (m_board.m_happyRam.size() == 0) {
            m_board.m_happyRam.reserve(6144);
            for (unsigned int i = 0; i < 6144; i++) {
                m_board.m_happyRam[i] = 0;
            }
            m_board.m_happyRam[0] = 0x80;
            for (unsigned int i = 0; i < sizeof(HAPPY_INITIAL_SECTOR_LIST); i++) {
                m_board.m_happyRam[0x280 + i] = HAPPY_INITIAL_SECTOR_LIST[i];
            }
        }
    }
    else {
        m_board.m_happyRam[0] = 0x80;
        m_board.setHappy1050(false);
    }
    emit statusChanged(m_deviceNo);
}

void SimpleDiskImage::setOSBMode(bool enable)
{
    m_OSBMode = enable;
    setTranslatorActive(true);
}

void SimpleDiskImage::setTranslatorActive(bool resetTranslatorState)
{
    bool enable = m_OSBMode && (m_deviceNo == 0x31) && (m_translatorDiskImagePath.size() > 5);
    if (enable) {
        enable = translatorAvailableAvailable();
    }
    if (enable && resetTranslatorState) {
        m_board.setTranslatorState(NOT_BOOTED);
    }
    if (m_board.isTranslatorActive() != enable) {
        m_board.setTranslatorActive(enable);
        if (m_board.isTranslatorActive() != enable) { // needed - don't remove!!!
            emit statusChanged(m_deviceNo);
        }
    }
    if ((!enable) || (!m_board.isTranslatorActive())){
        closeTranslator();
    }
}

bool SimpleDiskImage::translatorAvailableAvailable()
{
    if (m_translatorDiskImagePath.isEmpty()) {
        qWarning() << "!w" << tr("[%1] No Translator disk image defined. Please, check settings in menu Disk images>OS-B emulation.")
                      .arg(deviceName());
        return false;
    }
    QFile *translatorFile = new QFile(m_translatorDiskImagePath);
    if (!translatorFile->open(QFile::ReadOnly)) {
        delete translatorFile;
        qWarning() << "!w" << tr("[%1] Translator '%2' not found. Please, check settings in menu Disk images>OS-B emulation.")
                      .arg(deviceName())
                      .arg(m_translatorDiskImagePath);
        return false;
    }
    translatorFile->close();
    delete translatorFile;
    return true;
}

void SimpleDiskImage::setDisplayTransmission(bool active)
{
    m_displayTransmission = active;
}

void SimpleDiskImage::setSpyMode(bool enable)
{
    m_dumpDataFrame = enable;
}

void SimpleDiskImage::setTrackLayout(bool enable)
{
    m_displayTrackLayout = enable;
}

void SimpleDiskImage::setDisassembleUploadedCode(bool enable)
{
    m_disassembleUploadedCode = enable;
}

void SimpleDiskImage::setTranslatorAutomaticDetection(bool enable)
{
    m_translatorAutomaticDetection = enable;
}

void SimpleDiskImage::setTranslatorDiskImagePath(const QString &filename)
{
    m_translatorDiskImagePath = filename;
    setTranslatorActive(false);
}

void SimpleDiskImage::setLeverOpen(bool open)
{
    if (m_isLeverOpen != open) {
        m_isLeverOpen = open;
        if (open) {
            qDebug() << "!n" << tr("[%1] Drive door lever open. Drive is no longer ready").arg(deviceName());
        }
        else {
            qDebug() << "!n" << tr("[%1] Drive door lever closed. Drive is now ready").arg(deviceName());
        }
    }
}

bool SimpleDiskImage::openDcm(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("DCM images are not supported yet."));
    return false;
}

bool SimpleDiskImage::openDi(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("DI images are not supported yet."));
    return false;
}

bool SimpleDiskImage::open(const QString &fileName, FileTypes::FileType type)
{
    m_originalImageType = type;
    m_currentSide = 1;
    m_numberOfSides = 1;
    m_nextSideFilename.clear();
    bool bResult = false;
    switch (type) {
        case FileTypes::Atr:
        case FileTypes::AtrGz:
            bResult = openAtr(fileName);
        break;
        case FileTypes::Xfd:
        case FileTypes::XfdGz:
            bResult = openXfd(fileName);
        break;
        case FileTypes::Dcm:
        case FileTypes::DcmGz:
            bResult = openDcm(fileName);
        break;
        case FileTypes::Di:
        case FileTypes::DiGz:
            bResult = openDi(fileName);
        break;
        case FileTypes::Pro:
        case FileTypes::ProGz:
            bResult = openPro(fileName);
        break;
        case FileTypes::Atx:
        case FileTypes::AtxGz:
            bResult = openAtx(fileName);
        break;
        case FileTypes::Scp:
        case FileTypes::ScpGz:
            bResult = openScp(fileName);
        break;
        default:
            qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Unknown file type."));
        break;
    }
    if (bResult) {
        QFileInfo fileInfo(fileName);
        QString baseName = fileInfo.completeBaseName();
        if ((m_translatorAutomaticDetection) && (baseName.contains("OS-B", Qt::CaseInsensitive))) {
            qDebug() << "!u" << tr("Translator '%1' activated")
                        .arg(m_translatorDiskImagePath);
            setOSBMode(true);
        }
        if (baseName.contains("Side", Qt::CaseInsensitive) || baseName.contains("Disk", Qt::CaseInsensitive)) {
            QStringList imageList;
            imageList << fileInfo.absoluteDir().absoluteFilePath(fileInfo.fileName());
            QStringList images = fileInfo.absoluteDir().entryList(QStringList() << "*.xfd" << "*.XFD" << "*.atr" << "*.ATR" << "*.pro" << "*.PRO" << "*.atx" << "*.ATX", QDir::Files);
            foreach(QString otherFileName, images) {
                QFileInfo otherFileInfo(otherFileName);
                QString otherBaseName = otherFileInfo.completeBaseName();
                if (otherBaseName.contains("Side", Qt::CaseInsensitive) || otherBaseName.contains("Disk", Qt::CaseInsensitive)) {
                    if (sameSoftware(baseName, otherBaseName)) {
                        imageList << fileInfo.absoluteDir().absoluteFilePath(otherFileInfo.fileName());
                    }
                }
            }
            if (imageList.size() > 1) {
                m_numberOfSides = imageList.size();
                qSort(imageList.begin(), imageList.end(), qLess<QString>());
                int currentIndex = 0;
                foreach(QString otherFileName, imageList) {
                    if (otherFileName == fileName) {
                        break;
                    }
                    else {
                        currentIndex++;
                    }
                }
                m_currentSide = currentIndex + 1;
                int nextIndex = m_currentSide >= m_numberOfSides ? 0 : m_currentSide;
                m_nextSideFilename.append(imageList.at(nextIndex));
                QFileInfo nextFileInfo(m_nextSideFilename);
                qDebug() << "!u" << tr("Image is %1 of %2. Next image will be %3")
                              .arg(currentIndex + 1)
                              .arg(m_numberOfSides)
                              .arg(nextFileInfo.fileName());
            }
        }
    }
    return bResult;
}

bool SimpleDiskImage::sameSoftware(const QString &fileName, const QString &otherFileName)
{
    int same = 0;
    int minLength = fileName.length() < otherFileName.length() ? fileName.length() : otherFileName.length();
    QChar c1, c2;
    for (int i = 0; i < minLength; i++) {
        if (fileName.data()[i] == otherFileName.data()[i]) {
            same++;
        }
        else {
            c1 = fileName.data()[i];
            c2 = otherFileName.data()[i];
            break;
        }
    }
    if (same == minLength) {
        return false;
    }
    if (same < 7) {
        return false;
    }
    if (c1 >= 'A' && c1 <= 'H') {
        if (c2 < 'A' || c2 > 'H') {
            return false;
        }
    }
    if (c1 >= 'a' && c1 <= 'h') {
        if (c2 < 'a' || c2 > 'h') {
            return false;
        }
    }
    if (c1 >= '1' && c1 <= '8') {
        if (c2 < '1' || c2 > '8') {
            return false;
        }
    }
    QString samePart = fileName.left(same);
    if ((!samePart.contains("Side", Qt::CaseInsensitive)) && (!samePart.contains("Disk", Qt::CaseInsensitive))) {
        return false;
    }
    return true;
}

bool SimpleDiskImage::saveDcm(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Saving DCM images is not supported yet."));
    return false;
}

bool SimpleDiskImage::saveDi(const QString &fileName)
{
    qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Saving DI images is not supported yet."));
    return false;
}

bool SimpleDiskImage::save()
{
    switch (m_originalImageType) {
        case FileTypes::Atr:
        case FileTypes::AtrGz:
            return saveAtr(m_originalFileName);
            break;
        case FileTypes::Xfd:
        case FileTypes::XfdGz:
            return saveXfd(m_originalFileName);
            break;
        case FileTypes::Dcm:
        case FileTypes::DcmGz:
            return saveDcm(m_originalFileName);
            break;
        case FileTypes::Scp:
        case FileTypes::ScpGz:
            return saveScp(m_originalFileName);
            break;
        case FileTypes::Di:
        case FileTypes::DiGz:
            return saveDi(m_originalFileName);
            break;
        case FileTypes::Pro:
        case FileTypes::ProGz:
            return savePro(m_originalFileName);
            break;
        case FileTypes::Atx:
        case FileTypes::AtxGz:
            return saveAtx(m_originalFileName);
            break;
        default:
            qCritical() << "!e" << tr("Cannot save '%1': %2").arg(m_originalFileName).arg(tr("Unknown file type."));
            return false;
    }
}

bool SimpleDiskImage::saveAs(const QString &fileName)
{
    m_currentSide = 1;
    m_numberOfSides = 1;
    m_nextSideFilename.clear();
    if (fileName.endsWith(".ATR", Qt::CaseInsensitive)) {
        FileTypes::FileType destinationImageType = FileTypes::Atr;
        if ((m_originalImageType == FileTypes::Atr) || (m_originalImageType == FileTypes::AtrGz) || (m_originalImageType == FileTypes::Xfd) || (m_originalImageType == FileTypes::XfdGz)) {
            // almost same format but different name
            m_originalImageType = destinationImageType;
            return saveAtr(fileName);
        }
        return saveAsAtr(fileName, destinationImageType);
    } else if (fileName.endsWith(".ATZ", Qt::CaseInsensitive) || fileName.endsWith(".ATR.GZ", Qt::CaseInsensitive)) {
        FileTypes::FileType destinationImageType = FileTypes::AtrGz;
        if ((m_originalImageType == FileTypes::Atr) || (m_originalImageType == FileTypes::AtrGz) || (m_originalImageType == FileTypes::Xfd) || (m_originalImageType == FileTypes::XfdGz)) {
            // almost same format but different name
            m_originalImageType = destinationImageType;
            return saveAtr(fileName);
        }
        return saveAsAtr(fileName, destinationImageType);
    } else if (fileName.endsWith(".XFD", Qt::CaseInsensitive)) {
        FileTypes::FileType destinationImageType = FileTypes::Xfd;
        if ((m_originalImageType == FileTypes::Atr) || (m_originalImageType == FileTypes::AtrGz) || (m_originalImageType == FileTypes::Xfd) || (m_originalImageType == FileTypes::XfdGz)) {
            // almost same format but different name
            m_originalImageType = destinationImageType;
            return saveXfd(fileName);
        }
        return saveAsAtr(fileName, destinationImageType);
    } else if (fileName.endsWith(".XFZ", Qt::CaseInsensitive) || fileName.endsWith(".XFD.GZ", Qt::CaseInsensitive)) {
        FileTypes::FileType destinationImageType = FileTypes::XfdGz;
        if ((m_originalImageType == FileTypes::Atr) || (m_originalImageType == FileTypes::AtrGz) || (m_originalImageType == FileTypes::Xfd) || (m_originalImageType == FileTypes::XfdGz)) {
            // almost same format but different name
            m_originalImageType = destinationImageType;
            return saveXfd(fileName);
        }
        return saveAsAtr(fileName, destinationImageType);
    } else if (fileName.endsWith(".DCM", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::Dcm;
        return saveDcm(fileName);
    } else if (fileName.endsWith(".DI", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::Di;
        return saveDi(fileName);
    } else if (fileName.endsWith(".PRO", Qt::CaseInsensitive)) {
        FileTypes::FileType destinationImageType = FileTypes::Pro;
        if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
            // same format but different name
            m_originalImageType = destinationImageType;
            return savePro(fileName);
        }
        return saveAsPro(fileName, destinationImageType);
    } else if (fileName.endsWith(".PRO.GZ", Qt::CaseInsensitive)) {
        FileTypes::FileType destinationImageType = FileTypes::ProGz;
        if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
            // same format but different name
            m_originalImageType = destinationImageType;
            return savePro(fileName);
        }
        return saveAsPro(fileName, destinationImageType);
    } else if (fileName.endsWith(".ATX", Qt::CaseInsensitive)) {
        FileTypes::FileType destinationImageType = FileTypes::Atx;
        if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
            // same format but different name
            m_originalImageType = destinationImageType;
            return saveAtx(fileName);
        }
        return saveAsAtx(fileName, destinationImageType);
    } else if (fileName.endsWith(".ATX.GZ", Qt::CaseInsensitive)) {
        FileTypes::FileType destinationImageType = FileTypes::AtxGz;
        if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
            // same format but different name
            m_originalImageType = destinationImageType;
            return saveAtx(fileName);
        }
        return saveAsAtx(fileName, destinationImageType);
    } else if (fileName.endsWith(".SCP", Qt::CaseInsensitive)) {
        m_originalImageType = FileTypes::Scp;
        return saveScp(fileName);
    } else {
        qCritical() << "!e" << tr("Cannot save '%1': %2").arg(fileName).arg(tr("Unknown file extension."));
        return false;
    }
}

bool SimpleDiskImage::create(int untitledName)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return createPro(untitledName);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return createAtx(untitledName);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return createScp(untitledName);
    }
    return createAtr(untitledName);
}

void SimpleDiskImage::reopen()
{
    close();
    open(m_originalFileName, m_originalImageType);
}

void SimpleDiskImage::close()
{
    closeTranslator();
    m_currentSide = 1;
    m_numberOfSides = 1;
    m_nextSideFilename.clear();
    if (m_editDialog) {
        delete m_editDialog;
    }
    if ((m_originalImageType == FileTypes::Atr) || (m_originalImageType == FileTypes::AtrGz) ||
            (m_originalImageType == FileTypes::Xfd) || (m_originalImageType == FileTypes::XfdGz)) {
        file.close();
    }
    setReady(false);
}

void SimpleDiskImage::setReady(bool bReady)
{
    m_isReady = bReady;
}

bool SimpleDiskImage::executeArchiverCode(quint16 aux, QByteArray &data)
{
    bool ok = true;
    if (((quint8)data[0] == 0x18) && ((quint8)data[1] == 0x60)) { // Super Archiver 3.12 command to check if chip is open
        qDebug() << "!u" << tr("[%1] Uploaded code is: Check if drive is a Super Archiver")
                    .arg(deviceName());
    }
    else {
        // compute CRC16 and save it to know what data should be returned by Read Memory command
        Crc16 crc16;
        crc16.Reset();
        unsigned short firstDataPartCrc16 = 0;
        for (int j = 0; j < data.size(); j++) {
            crc16.Add((unsigned char) (data.at(j)));
            if (j == 12) {
                // when SK+ option is on in Super Archiver, an Execute Code command is sent with code and data.
                // The code is in the first 13 bytes and the data is at offset 128 for Super Archiver 3.03
                // and 13 (just after the code) for Super Archiver 3.02 and 3.12.
                // The track data looks like the data sent in the Write Track command (sector list, fill bytes, gap sizes,...).
                // So the crc in this specific case should be computed only on the first 13 bytes.
                firstDataPartCrc16 = crc16.GetCrc();
            }
        }
        m_board.setLastArchiverUploadCrc16(crc16.GetCrc());
        if (m_board.getLastArchiverUploadCrc16() == 0xFD2E) {
            qDebug() << "!u" << tr("[%1] Uploaded code is: Speed check")
                        .arg(deviceName());
        }
        else if (m_board.getLastArchiverUploadCrc16() == 0x61F6) {
            qDebug() << "!u" << tr("[%1] Uploaded code is: Diagnostic")
                        .arg(deviceName());
        }
        else if ((firstDataPartCrc16 == 0x9537) || (firstDataPartCrc16 == 0xBEAF) || (firstDataPartCrc16 == 0xD2A0)) { // Super Archiver 3.02, 3.03, 3.12 respectively
            m_board.setLastArchiverUploadCrc16(firstDataPartCrc16);
            int startOffset = ((int)data[3]) & 0xFF; // this byte contains the offset of the first track data byte
            m_board.m_trackData.clear();
            m_board.m_trackData.append(data.mid(startOffset,128));
            qDebug() << "!u" << tr("[%1] Uploaded code is: Prepare track data at offset $%2")
                        .arg(deviceName())
                        .arg(startOffset, 2, 16, QChar('0'));
        }
        else {
            // when SK+ option is on in Archiver 3.03, an Execute Code command is sent with code and data.
            // The code begins at offset 48. The first part is the sector list of the track given in AUX1.
            // We have to build a sector list with the byte offsets for track AUX2 respecting skew alignment with AUX1.
            crc16.Reset();
            for (int j = 0x30; j < 0xDA; j++) {
                crc16.Add((unsigned char) (data.at(j)));
            }
            if ((crc16.GetCrc() == 0x603D) || (crc16.GetCrc() == 0xBAC2) || (crc16.GetCrc() == 0xDFFF)) { // Super Archiver 3.02, 3.03, 3.12 respectively
                m_board.setLastArchiverUploadCrc16(crc16.GetCrc());
                // 2 implementations: one expects the sector list (Super Archiver 3.02 and 3.03) and the second expects a timing (3.12)
                bool timingOnly = crc16.GetCrc() == 0xDFFF;
                if (timingOnly) {
                    qDebug() << "!u" << tr("[%1] Uploaded code is: Skew alignment of track $%2 sector $%3 with track $%4 sector $%5")
                                .arg(deviceName())
                                .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'))
                                .arg(data[3], 2, 16, QChar('0'))
                                .arg((aux & 0xFF), 2, 16, QChar('0'))
                                .arg(data[4], 2, 16, QChar('0'));
                }
                else {
                    qDebug() << "!u" << tr("[%1] Uploaded code is: Skew alignment of track $%2 (%3 sectors) with track $%4 (%5 sectors)")
                                .arg(deviceName())
                                .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'))
                                .arg(data[3], 2, 16, QChar('0'))
                                .arg((aux & 0xFF), 2, 16, QChar('0'))
                                .arg(data[4], 2, 16, QChar('0'));
                }
                ok = readSkewAlignment(aux, data, timingOnly);
            }
            else if ((crc16.GetCrc() == 0xAD1D) || (crc16.GetCrc() == 0x8A65) || (crc16.GetCrc() == 0x64B1)) { // Super Archiver 3.02, 3.03, 3.12 respectively
                m_board.setLastArchiverUploadCrc16(crc16.GetCrc());
                if (crc16.GetCrc() == 0x64B1) {
                    quint16 timing = (((quint16)data[4]) & 0xFF) + ((((quint16)data[5]) << 8) & 0xFF00);
                    qDebug() << "!u" << tr("[%1] Uploaded code is: Format track $%2 with skew alignment $%3 with track $%4")
                                .arg(deviceName())
                                .arg(aux & 0xFF, 2, 16, QChar('0'))
                                .arg(timing, 4, 16, QChar('0'))
                                .arg(data[3] & 0x3F, 2, 16, QChar('0'));
                }
                else {
                    qDebug() << "!u" << tr("[%1] Uploaded code is: Format track $%2 with skew alignment with track $%3")
                                .arg(deviceName())
                                .arg(aux & 0xFF, 2, 16, QChar('0'))
                                .arg(data[3] & 0x3F, 2, 16, QChar('0'));
                }
                ok = writeTrackWithSkew(aux, data);
            }
            else {
                qWarning() << "!w" << tr("[%1] Data Crc16 is $%2. Command ignored")
                            .arg(deviceName())
                            .arg(crc16.GetCrc(), 4, 16, QChar('0'));
            }
        }
    }
    return ok;
}

void SimpleDiskImage::readHappyTrack(int trackNumber, bool happy1050)
{
    Crc16 crc;
    QByteArray data;
    readTrack(0xEA00 | (quint8)trackNumber, data, 256);
    quint8 nbSectors = data[0];
    int nbSectorsInTrack = sectorsInCurrentTrack();
    if (nbSectors > (nbSectorsInTrack + 1)) {
        nbSectors = nbSectorsInTrack + 1;
    }
    int srcOffset = 1;
    int startOffset;
    int dstOffset;
    if (happy1050) {
        startOffset = 0xD00;
        dstOffset = startOffset + 1;
        m_board.m_happyRam[dstOffset++] = 0x8D;
        m_board.m_happyRam[dstOffset++] = 0x7F;
    }
    else {
        startOffset = 0x300;
        dstOffset = startOffset + 1;
        m_board.m_happyRam[dstOffset++] = 0x0B;
        m_board.m_happyRam[dstOffset++] = 0xFE;
    }
    quint8 totalTiming = 0;
    for (quint8 sector = 0; sector < nbSectors; sector++) {
        // ignore short sectors
        if ((unsigned char)data[srcOffset + 4] >= 2) {
            crc.Reset();
            crc.Add(DISK_ID_ADDR_MARK);
            for (int i = 0; i < 4; i++) {
                m_board.m_happyRam[dstOffset++] = 0xFF - crc.Add((unsigned char)data[srcOffset++]);
            }
            m_board.m_happyRam[dstOffset++] = 0xFF - (unsigned char)((crc.GetCrc() >> 8) & 0xFF);
            m_board.m_happyRam[dstOffset++] = 0xFF - (unsigned char)(crc.GetCrc() & 0xFF);
            m_board.m_happyRam[dstOffset++] = 0xFF;
            totalTiming += (quint8)data[srcOffset++];
            srcOffset++;
            if (happy1050) {
                m_board.m_happyRam[dstOffset++] = 0x7F - totalTiming;
            }
            else {
                m_board.m_happyRam[dstOffset++] = 0xFF - totalTiming;
            }
        }
        else {
            totalTiming += (quint8)data[srcOffset + 4];
            srcOffset += 6;
        }
    }
    m_board.m_happyRam[startOffset] = (quint8)((dstOffset - 1) & 0xFF);
    for (int i = dstOffset; i < (startOffset + 0x100); i++) {
        m_board.m_happyRam[i] = 0;
    }
    if (! happy1050) {
        m_board.m_happyRam[0x28D] = (quint8)0x43;
    }
}

QByteArray SimpleDiskImage::readHappySectors(int trackNumber, int afterSectorNumber, bool happy1050)
{
    int baseOffset = happy1050 ? 0xC80 : 0x280;
    bool rev50 = happy1050 ? false : (quint8)m_board.m_happyRam[0x14D] == 0x18;
    quint8 fdcMask = rev50 ? 0x18 : m_board.m_happyRam[baseOffset + 0x01];
    quint8 nbSectors = m_board.m_happyRam[baseOffset + 0x0F];
    int start = (int)m_board.m_happyRam[baseOffset + 0x12];
    if (start < 0) {
        start = 0;
    }
    if (start > 35) {
        start = 35;
    }
    int nbRetry = 4;
    bool firstSector = true;
    do {

        // read all sectors requested by Happy
        int index = 0;
        for (int i = start; i >= 0; i--) {
            if (((0xFF - m_board.m_happyRam[baseOffset + 0x14 + i]) & fdcMask) != 0) {
                int sector = (int)((0xFF - m_board.m_happyRam[baseOffset + 0x38 + i]) & 0xFF);
                int offset = baseOffset + 0x80 + ((i % 18) * 128);
                QByteArray data;
                int after = firstSector ? afterSectorNumber : 0;
                firstSector = false;
                readHappySectorAtPosition(trackNumber, sector, after, index, data);
                for (int j = 0; j < data.size(); j++) {
                    m_board.m_happyRam[offset + j] = data[j];
                }
                m_board.m_happyRam[baseOffset + 0x14 + i] = m_wd1771Status;
                nbSectors--;
                if (nbSectors <= 0) {
                    break;
                }
            }
        }

        // check that all sectors have been read
        nbSectors = 0;
        int nbSlots = 18;
        for (int i = start; i >= 0; i--) {
            if (((0xFF - m_board.m_happyRam[baseOffset + 0x14 + i]) & fdcMask) != 0) {
                nbSectors++;
                if ((m_board.m_happyRam[baseOffset + 0x5C + i] & 0x80) != 0) {
                    int nbOtherSlots = 18;
                    for (int j = start; j >= 0; j--) {
                        if ((m_board.m_happyRam[baseOffset + 0x38 + i] == m_board.m_happyRam[baseOffset + 0x38 + j]) && (((0xFF - m_board.m_happyRam[baseOffset + 0x14 + j]) & fdcMask) == 0)) {
                            m_board.m_happyRam[baseOffset + 0x14 + j] = 0xEF;
                            /*
                            if (j >= *$0091) {
                                nbSectors++;
                            }
                            */
                        }
                        nbOtherSlots--;
                        if (nbOtherSlots <= 0) {
                            break;
                        }
                    }
                }
            }
            nbSlots--;
            if (nbSlots <= 0) {
                break;
            }
        }
        nbRetry--;
    }
    while ((nbSectors > 0) && (nbRetry > 0));
    if (! happy1050) {
        m_board.m_happyRam[0x293] = 0x03;
        m_board.m_happyRam[0x290] = 0x7F;
    }
    m_board.m_happyRam[baseOffset + 0x0F] = 0x00;

    QByteArray status = m_board.m_happyRam.mid(baseOffset, 128);
    return status;
}

bool SimpleDiskImage::readHappySectorAtPosition(int trackNumber, int sectorNumber, int afterSectorNumber, int &index, QByteArray &data)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return readHappyProSectorAtPosition(trackNumber, sectorNumber, afterSectorNumber, index, data);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return readHappyAtxSectorAtPosition(trackNumber, sectorNumber, afterSectorNumber, index, data);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return readHappyScpSectorAtPosition(trackNumber, sectorNumber, afterSectorNumber, index, data);
    }
    return readHappyAtrSectorAtPosition(trackNumber, sectorNumber, afterSectorNumber, index, data);
}

bool SimpleDiskImage::readHappySkewAlignment(bool happy1050)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return readHappyProSkewAlignment(happy1050);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return readHappyAtxSkewAlignment(happy1050);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return readHappyScpSkewAlignment(happy1050);
    }
    return readHappyAtrSkewAlignment(happy1050);
}

bool SimpleDiskImage::writeHappyTrack(int trackNumber, bool happy1050)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return writeHappyProTrack(trackNumber, happy1050);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return writeHappyAtxTrack(trackNumber, happy1050);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return writeHappyScpTrack(trackNumber, happy1050);
    }
    return writeHappyAtrTrack(trackNumber, happy1050);
}

bool SimpleDiskImage::writeHappySectors(int trackNumber, int afterSectorNumber, bool happy1050)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return writeHappyProSectors(trackNumber, afterSectorNumber, happy1050);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return writeHappyAtxSectors(trackNumber, afterSectorNumber, happy1050);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return writeHappyScpSectors(trackNumber, afterSectorNumber, happy1050);
    }
    return writeHappyAtrSectors(trackNumber, afterSectorNumber, happy1050);
}

bool SimpleDiskImage::format(const DiskGeometry &geo)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return formatPro(geo);
    }
    else if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return formatAtx(geo);
    }
    else if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return formatScp(geo);
    }
    return formatAtr(geo);
}

void SimpleDiskImage::readTrack(quint16 aux, QByteArray &data, int length)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        readProTrack(aux, data, length);
    }
    else if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        readAtxTrack(aux, data, length);
    }
    else if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        readScpTrack(aux, data, length);
    }
    else {
        readAtrTrack(aux, data, length);
    }
}

bool SimpleDiskImage::readSectorStatuses(QByteArray &data)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return readProSectorStatuses(data);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return readAtxSectorStatuses(data);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return readScpSectorStatuses(data);
    }
    return readAtrSectorStatuses(data);
}

bool SimpleDiskImage::readSectorUsingIndex(quint16 aux, QByteArray &data)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return readProSectorUsingIndex(aux, data);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return readAtxSectorUsingIndex(aux, data);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return readScpSectorUsingIndex(aux, data);
    }
    return readAtrSectorUsingIndex(aux, data);
}

bool SimpleDiskImage::readSector(quint16 aux, QByteArray &data)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return readProSector(aux, data);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return readAtxSector(aux, data);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return readScpSector(aux, data);
    }
    return readAtrSector(aux, data);
}

bool SimpleDiskImage::readSkewAlignment(quint16 aux, QByteArray &data, bool timingOnly)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return readProSkewAlignment(aux, data, timingOnly);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return readAtxSkewAlignment(aux, data, timingOnly);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return readScpSkewAlignment(aux, data, timingOnly);
    }
    return readAtrSkewAlignment(aux, data, timingOnly);
}

bool SimpleDiskImage::resetTrack(quint16 aux)
{
    if (!m_isModified) {
        m_isModified = true;
        emit statusChanged(m_deviceNo);
    }
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return resetProTrack(aux);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return resetAtxTrack(aux);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return resetScpTrack(aux);
    }
    return resetAtrTrack(aux);
}

bool SimpleDiskImage::writeTrack(quint16 aux, const QByteArray &data)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return writeProTrack(aux, data);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return writeAtxTrack(aux, data);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return writeScpTrack(aux, data);
    }
    return writeAtrTrack(aux, data);
}

bool SimpleDiskImage::writeTrackWithSkew(quint16 aux, const QByteArray &data) {
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return writeProTrackWithSkew(aux, data);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return writeAtxTrackWithSkew(aux, data);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return writeScpTrackWithSkew(aux, data);
    }
    return writeAtrTrackWithSkew(aux, data);
}

bool SimpleDiskImage::writeSectorUsingIndex(quint16 aux, const QByteArray &data, bool fuzzy)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return writeProSectorUsingIndex(aux, data, fuzzy);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return writeAtxSectorUsingIndex(aux, data, fuzzy);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return writeScpSectorUsingIndex(aux, data, fuzzy);
    }
    return writeAtrSectorUsingIndex(aux, data, fuzzy);
}

bool SimpleDiskImage::writeFuzzySector(quint16 aux, const QByteArray &data)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return writeFuzzyProSector(aux, data);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return writeFuzzyAtxSector(aux, data);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return writeFuzzyScpSector(aux, data);
    }
    return writeFuzzyAtrSector(aux, data);
}

bool SimpleDiskImage::writeSector(quint16 aux, const QByteArray &data)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return writeProSector(aux, data);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return writeAtxSector(aux, data);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return writeScpSector(aux, data);
    }
    return writeAtrSector(aux, data);
}

bool SimpleDiskImage::writeSectorExtended(int bitNumber, quint8 dataType, quint8 trackNumber, quint8 sideNumber, quint8 sectorNumber, quint8 sectorSize, const QByteArray &data, bool crcError, int weakOffset)
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return writeProSectorExtended(bitNumber, dataType, trackNumber, sideNumber, sectorNumber, sectorSize, data, crcError, weakOffset);
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return writeAtxSectorExtended(bitNumber, dataType, trackNumber, sideNumber, sectorNumber, sectorSize, data, crcError, weakOffset);
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return writeScpSectorExtended(bitNumber, dataType, trackNumber, sideNumber, sectorNumber, sectorSize, data, crcError, weakOffset);
    }
    return writeAtrSectorExtended(bitNumber, dataType, trackNumber, sideNumber, sectorNumber, sectorSize, data, crcError, weakOffset);
}

int SimpleDiskImage::sectorsInCurrentTrack()
{
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz)) {
        return sectorsInCurrentProTrack();
    }
    if ((m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        return sectorsInCurrentAtxTrack();
    }
    if ((m_originalImageType == FileTypes::Scp) || (m_originalImageType == FileTypes::ScpGz)) {
        return sectorsInCurrentScpTrack();
    }
    return sectorsInCurrentAtrTrack();
}

int SimpleDiskImage::findNearestSpeed(int speed)
{
    if (speed < 20000) {
        return 19200;
    }
    int lastSpeed = 300000;
    int bestSpeed = 19200;
    for (int divisor = 0; divisor <= 40; divisor++) {
        int newSpeed = sio->port()->divisorToBaud(divisor);
        if (speed == newSpeed) {
            bestSpeed = newSpeed;
            break;
        }
        if ((speed >= newSpeed) && (speed <= lastSpeed)) {
            bestSpeed = ((speed - newSpeed) > (lastSpeed - speed)) ? lastSpeed : newSpeed;
            break;
        }
        lastSpeed = newSpeed;
    }
    return bestSpeed;
}

void SimpleDiskImage::handleCommand(quint8 command, quint16 aux)
{
    if ((! isReady()) || isLeverOpen()) {
        if ((command != 0x4E) && (command != 0x4F) && (command != 0x53)) {
            qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 ignored because the drive is not ready.")
                           .arg(deviceName())
                           .arg(command, 2, 16, QChar('0'))
                           .arg(aux, 4, 16, QChar('0'));
            writeCommandNak();
            return;
        }
    }
    if ((m_deviceNo == 0x31) && (command == 0x52) && (m_board.isTranslatorActive())) {
        quint16 sector = aux;
        if (m_board.isChipOpen()) {
            sector = aux & 0x3FF;
        }
        if (sector == 1) {
            if (m_board.getTranslatorState() == NOT_BOOTED) {
                m_translatorDisk = new SimpleDiskImage(sio);
                m_translatorDisk->setReadOnly(true);
                m_translatorDisk->setDeviceNo(0x31);
                m_translatorDisk->setDisplayTransmission(false);
                m_translatorDisk->setSpyMode(false);
                m_translatorDisk->setTrackLayout(false);
                m_translatorDisk->setDisassembleUploadedCode(false);
                m_translatorDisk->setTranslatorAutomaticDetection(false);
                m_translatorDisk->setReady(true);
                FileTypes::FileType type = FileTypes::getFileType(m_translatorDiskImagePath);
                if (!m_translatorDisk->open(m_translatorDiskImagePath, type)) {
                    m_board.setTranslatorActive(false);
                }
                else {
                    m_board.setTranslatorState(FIRST_SECTOR_1);
                    qWarning() << "!i" << tr("[%1] Booting Translator '%2' first")
                                .arg(deviceName())
                                .arg(m_translatorDiskImagePath);
                }
            }
            else if (m_board.getTranslatorState() == READ_OTHER_SECTOR) {
                m_board.setTranslatorState(SECOND_SECTOR_1);
                m_board.setTranslatorActive(false);
                setTranslatorActive(false);
                qWarning() << "!i" << tr("[%1] Removing Translator to boot on '%2'")
                            .arg(deviceName())
                            .arg(m_originalFileName);
            }
        }
        else if ((sector != 1) && (m_board.getTranslatorState() == FIRST_SECTOR_1)) {
            m_board.setTranslatorState(READ_OTHER_SECTOR);
        }
        if (m_board.isTranslatorActive() && (m_translatorDisk != NULL)) {
            m_translatorDisk->handleCommand(command, aux);
            return;
        }
    }
    switch (command) {
    case 0x21:  // Format
        {
            if (!writeCommandAck()) {
                break;
            }
            QByteArray data(m_newGeometry.bytesPerSector(), 0);
            data[0] = 0xff;
            data[1] = 0xff;
            if (!m_isReadOnly) {
                if (format(m_newGeometry)) {
                    qWarning() << "!w" << tr("[%1] Format.")
                                  .arg(deviceName());
                    writeComplete();
                }
                else {
                    writeError();
                    qCritical() << "!e" << tr("[%1] Format failed.")
                                   .arg(deviceName());
                }
            }
            else {
                qWarning() << "!w" << tr("[%1] Format denied.")
                              .arg(deviceName());
                writeError();
            }
            writeDataFrame(data);
            break;
        }
    case 0x22:  // Format ED
        {
            if (!writeCommandAck()) {
                break;
            }
            QByteArray data(128, 0);
            data[0] = data[1] = 0xFF;
            if (m_isReadOnly) {
                writeError();
                qWarning() << "!w" << tr("[%1] Format ED denied.")
                              .arg(deviceName());
                break;
            }
            else {
                m_newGeometry.initialize(false, 40, 26, 128);
                if (format(m_newGeometry)) {
                    qDebug() << "!n" << tr("[%1] Format ED.")
                                .arg(deviceName());
                    writeComplete();
                }
                else {
                    writeError();
                    qCritical() << "!e" << tr("[%1] Format ED failed.")
                                   .arg(deviceName());
                }
            }
            writeDataFrame(data);
            break;
        }
    case 0x23:  // Run Speed Diagnostic (1050)
        {
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Run Speed Diagnostic with AUX1=$%2 and AUX2=$%3")
                        .arg(deviceName())
                        .arg((aux & 0xFF), 2, 16, QChar('0'))
                        .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
            QByteArray data = readDataFrame(m_geometry.bytesPerSector());
            if (!writeDataAck()) {
                break;
            }
            m_diagData.clear();
            m_diagData.append(data);
            writeComplete();
            break;
        }
    case 0x24:  // Run Diagnostic (1050)
        {
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Run Diagnostic with AUX1=$%2 and AUX2=$%3")
                        .arg(deviceName())
                        .arg((aux & 0xFF), 2, 16, QChar('0'))
                        .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
            if (m_diagData.size() < 128) {
                m_diagData.resize(128);
                m_diagData[0] = m_diagData[1] = 0x00;
            }
            if (((quint8)m_diagData[0] == (quint8)0x01) && ((quint8)m_diagData[1] == (quint8)0x9B)) {
                m_diagData.resize(122);
                m_diagData[0] = 0x02;
                m_diagData[1] = 0x00;
                m_diagData[122] = 0x14;
                m_diagData[123] = 0x00;
                m_diagData[124] = 0x02;
                m_diagData[125] = 0x00;
                m_diagData[126] = 0x56;
                m_diagData[127] = 0xC6;
            }
            else if (((quint8)m_diagData[0] == (quint8)0x00) && ((quint8)m_diagData[1] == (quint8)0x00)) {
                m_diagData.resize(122);
                m_diagData[0] = 0x22;
                m_diagData[1] = 0x08;
                m_diagData[122] = 0x14;
                m_diagData[123] = 0x00;
                m_diagData[124] = 0x02;
                m_diagData[125] = 0x4C;
                m_diagData[126] = 0x04;
                m_diagData[127] = 0x1A;
            }
            writeComplete();
            writeDataFrame(m_diagData);
            break;
        }
    case 0x3F:  // Speed poll
        {
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Speed poll")
                        .arg(deviceName());
            writeComplete();
            QByteArray speed(1, 0);
            speed[0] = sio->port()->speedByte();
            writeDataFrame(speed);
            break;
        }
    case 0x41:  // Read track (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled() && m_board.isHappy1050()) {
                if (m_board.getLastHappyUploadCrc16() == 0x4312) {
                    if (!writeCommandAck()) {
                        break;
                    }
                    int trackNumber = (int)(0xFF - (aux & 0xFF));
                    qDebug() << "!n" << tr("[%1] Happy Read Track %2 ($%3)")
                                .arg(deviceName())
                                .arg(trackNumber)
                                .arg(trackNumber, 2, 16, QChar('0'));
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    writeComplete();
                    readHappyTrack(trackNumber, true);
                    writeDataFrame(m_board.m_happyRam.mid(0xD00, 128));
                }
                else {
                    qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                .arg(deviceName())
                                .arg(command, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'))
                                .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    writeCommandNak();
                }
            }
            else {
                qWarning() << "!w" << tr("[%1] Command $41 NAKed (HAPPY is not enabled).")
                               .arg(deviceName());
                writeCommandNak();
            }
            break;
        }
    case 0x42: // Write Sector using Index (CHIP/ARCHIVER) or Read All Sectors (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled() && m_board.isHappy1050()) {
                if (m_board.getLastHappyUploadCrc16() == 0x4312) {
                    if (!writeCommandAck()) {
                        break;
                    }
                    int trackNumber = (int)(0xFF - (aux & 0xFF));
                    int aux2 = (aux >> 8) & 0xFF;
                    int afterSectorNumber = (aux2 == 0) ? 0 : 0xFF - aux2;
                    if (afterSectorNumber == 0) {
                        qDebug() << "!n" << tr("[%1] Happy Read Sectors of track %2 ($%3)")
                                    .arg(deviceName())
                                    .arg(trackNumber)
                                    .arg(trackNumber, 2, 16, QChar('0'));
                    }
                    else {
                        qDebug() << "!n" << tr("[%1] Happy Read Sectors of track %2 ($%3) starting after sector %4 ($%5)")
                                    .arg(deviceName())
                                    .arg(trackNumber)
                                    .arg(trackNumber, 2, 16, QChar('0'))
                                    .arg(afterSectorNumber)
                                    .arg(afterSectorNumber, 2, 16, QChar('0'));
                    }
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    QByteArray data = readHappySectors(trackNumber, afterSectorNumber, true);
                    if (!writeComplete()) {
                        break;
                    }
                    writeDataFrame(data);
                }
                else {
                    qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                .arg(deviceName())
                                .arg(command, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'))
                                .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    writeCommandNak();
                }
                break;
            }
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Write Sector using Index denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Write Sector using Index with AUX1=$%2 and AUX2=$%3")
                        .arg(deviceName())
                        .arg((aux & 0xFF), 2, 16, QChar('0'))
                        .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
            QByteArray data = readDataFrame(m_geometry.bytesPerSector());
            if (!data.isEmpty()) {
                if (!writeDataAck()) {
                    break;
                }
                if (m_isReadOnly) {
                    writeError();
                    qWarning() << "!w" << tr("[%1] Super Archiver Write Sector using Index denied.")
                                  .arg(deviceName());
                    break;
                }
                if (writeSectorUsingIndex(aux, data, false)) {
                    writeComplete();
                } else {
                    writeError();
                    qCritical() << "!e" << tr("[%1] Super Archiver Write Sector using Index failed.")
                                   .arg(deviceName());
                }
            } else {
                qCritical() << "!e" << tr("[%1] Super Archiver Write Sector using Index data frame failed.")
                               .arg(deviceName());
                writeDataNak();
            }
            break;
        }
    case 0x43:  // Read all Sector Statuses (CHIP/ARCHIVER) or Set Skew Alignment (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled() && m_board.isHappy1050()) {
                if (m_board.getLastHappyUploadCrc16() == 0x4312) {
                    if (!writeCommandAck()) {
                        break;
                    }
                    for (int i = 0; i < 128; i++) {
                        m_board.m_happyRam[0x380 + i] = 0;
                    }
                    m_board.m_happyRam[0x3CB] = aux & 0xFF;
                    m_board.m_happyRam[0x3CC] = (aux >> 8) & 0xFF;
                    qDebug() << "!n" << tr("[%1] Happy Set Skew Alignment track %2 ($%3) and sector %4 ($%5)")
                                .arg(deviceName())
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CB])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CB], 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CC])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CC], 2, 16, QChar('0'));
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    writeComplete();
                }
                else {
                    qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                .arg(deviceName())
                                .arg(command, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'))
                                .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    writeCommandNak();
                }
                break;
            }
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Read all Sector Statuses denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Read All Sector Statuses").arg(deviceName());
            QByteArray data;
            if (readSectorStatuses(data)) {
                if (!writeComplete()) {
                    break;
                }
                writeDataFrame(data);
            } else {
                qCritical() << "!e" << tr("[%1] Super Archiver Read All Sector Statuses failed.")
                               .arg(deviceName());
                QByteArray result(128, 0);
                if (!writeError()) {
                    break;
                }
                writeDataFrame(result);
            }
            break;
        }
    case 0x44:  // Read Sector using Index (CHIP/ARCHIVER) or Read Skew Alignment (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled() && m_board.isHappy1050()) {
                if (m_board.getLastHappyUploadCrc16() == 0x4312) {
                    if (!writeCommandAck()) {
                        break;
                    }
                    m_board.m_happyRam[0x3C9] = aux & 0xFF;
                    m_board.m_happyRam[0x3CA] = (aux >> 8) & 0xFF;
                    qDebug() << "!n" << tr("[%1] Happy Read Skew alignment of track %2 ($%3) sector %4 ($%5) with track %6 ($%7) sector %8 ($%9)")
                                .arg(deviceName())
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CB])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CB], 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CC])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CC], 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3C9])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3C9], 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CA])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CA], 2, 16, QChar('0'));
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    if (readHappySkewAlignment(true)) {
                        if (!writeComplete()) {
                            break;
                        }
                    } else {
                        qCritical() << "!e" << tr("[%1] Happy Read Skew alignment failed.")
                                       .arg(deviceName())
                                       .arg(aux, 3, 16, QChar('0'));
                        if (!writeError()) {
                            break;
                        }
                    }
                    writeDataFrame(m_board.m_happyRam.mid(0x380, 128));
                }
                else {
                    qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                .arg(deviceName())
                                .arg(command, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'))
                                .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    writeCommandNak();
                }
                break;
            }
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Read Sector using Index denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Read Sector using Index with AUX1=$%2 and AUX2=$%3")
                        .arg(deviceName())
                        .arg((aux & 0xFF), 2, 16, QChar('0'))
                        .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
            QByteArray data;
            if (readSectorUsingIndex(aux, data)) {
                if (!writeComplete()) {
                    break;
                }
                writeDataFrame(data);
            } else {
                qCritical() << "!e" << tr("[%1] Super Archiver Read sector using Index with AUX1=$%2 and AUX2=$%3 failed.")
                               .arg(deviceName())
                               .arg((aux & 0xFF), 2, 16, QChar('0'))
                               .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
                QByteArray result(128, 0);
                if (!writeError()) {
                    break;
                }
                writeDataFrame(result);
            }
            break;
        }
    case 0x46:  // Write Track (CHIP/ARCHIVER) or Write Track (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled() && m_board.isHappy1050()) {
                if (m_board.getLastHappyUploadCrc16() == 0x4312) {
                    if (!writeCommandAck()) {
                        break;
                    }
                    int trackNumber = (int)(0xFF - (aux & 0xFF));
                    qDebug() << "!n" << tr("[%1] Happy Write track %2 ($%3)")
                                .arg(deviceName())
                                .arg(trackNumber)
                                .arg(trackNumber, 2, 16, QChar('0'));
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    if (writeHappyTrack(trackNumber, true)) {
                        writeComplete();
                    } else {
                        writeError();
                        qCritical() << "!e" << tr("[%1] Happy Write track failed.")
                                       .arg(deviceName());
                    }
                }
                else {
                    qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                .arg(deviceName())
                                .arg(command, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'))
                                .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    writeCommandNak();
                }
                break;
            }
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Write Track denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Write Track with AUX1=$%2 and AUX2=$%3")
                          .arg(deviceName())
                          .arg((aux & 0xFF), 2, 16, QChar('0'))
                          .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
            QByteArray data = readDataFrame(m_geometry.bytesPerSector(aux));
            if (!data.isEmpty()) {
                if (!writeDataAck()) {
                    break;
                }
                if (m_isReadOnly) {
                    writeError();
                    qWarning() << "!w" << tr("[%1] Super Archiver Write Track denied.")
                                  .arg(deviceName());
                    break;
                }
                if (writeTrack(aux, data)) {
                    writeComplete();
                } else {
                    writeError();
                    qCritical() << "!e" << tr("[%1] Super Archiver Write track failed.")
                                   .arg(deviceName());
                }
            } else {
                qCritical() << "!e" << tr("[%1] Super Archiver Write track data frame failed.")
                               .arg(deviceName());
                writeDataNak();
            }
            break;
        }
    case 0x47:  // Read Track (128 bytes) (CHIP/ARCHIVER) or Write all Sectors (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled() && m_board.isHappy1050()) {
                if (m_board.getLastHappyUploadCrc16() == 0x4312) {
                    if (!writeCommandAck()) {
                        break;
                    }
                    int trackNumber = (int)(0xFF - (aux & 0xFF));
                    int aux2 = (aux >> 8) & 0xFF;
                    int afterSectorNumber = (aux2 == 0) ? 0 : 0xFF - aux2;
                    if (afterSectorNumber == 0) {
                        qDebug() << "!n" << tr("[%1] Happy Write Sectors of track %2 ($%3)")
                                    .arg(deviceName())
                                    .arg(trackNumber)
                                    .arg(trackNumber, 2, 16, QChar('0'));
                    }
                    else {
                        qDebug() << "!n" << tr("[%1] Happy Write Sectors of track %2 ($%3) starting after sector %4 ($%5)")
                                    .arg(deviceName())
                                    .arg(trackNumber)
                                    .arg(trackNumber, 2, 16, QChar('0'))
                                    .arg(afterSectorNumber)
                                    .arg(afterSectorNumber, 2, 16, QChar('0'));
                    }
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    if (writeHappySectors(trackNumber, afterSectorNumber, true)) {
                        if (!writeComplete()) {
                            break;
                        }
                    } else {
                        qCritical() << "!e" << tr("[%1] Happy Write Sectors failed.")
                                       .arg(deviceName())
                                       .arg(aux, 3, 16, QChar('0'));
                        if (!writeError()) {
                            break;
                        }
                    }
                    writeDataFrame(m_board.m_happyRam.mid(m_board.isHappy1050() ? 0xC80 : 0x280, 128));
                }
                else {
                    qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                .arg(deviceName())
                                .arg(command, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'))
                                .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    writeCommandNak();
                }
                break;
            }
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Read Track (128 bytes) denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Read Track (128 bytes) with AUX1=$%2 and AUX2=$%3")
                        .arg(deviceName())
                        .arg((aux & 0xFF), 2, 16, QChar('0'))
                        .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
            QByteArray data;
            readTrack(aux, data, 128);
            if (!writeComplete()) {
                break;
            }
            writeDataFrame(data);
            break;
        }
    case 0x48:  // Configure drive (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled()) {
                if (!writeCommandAck()) {
                    break;
                }
                qDebug() << "!n" << tr("[%1] Happy Configure drive with AUX $%2")
                               .arg(deviceName())
                               .arg(aux, 4, 16, QChar('0'));
                if (aux == 0x0003) {
                    for (int i = 0; i < m_board.m_happyRam.length(); i++) {
                        m_board.m_happyRam[i] = (unsigned char) 0;
                    }
                }
                writeComplete();
                m_board.setHappy1050(true);
            }
            else {
                writeCommandNak();
                qWarning() << "!w" << tr("[%1] Happy Configure drive NAKed (HAPPY is not enabled).")
                               .arg(deviceName());
            }
            break;
        }
    case 0x49:  // Write track with skew alignment (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled() && m_board.isHappy1050()) {
                if (m_board.getLastHappyUploadCrc16() == 0x4312) {
                    if (!writeCommandAck()) {
                        break;
                    }
                    int trackNumber = (int)(0xFF - (aux & 0xFF));
                    qDebug() << "!n" << tr("[%1] Happy Write track using skew alignment of track %2 ($%3) with track %4 ($%5) sector %6 ($%7)")
                                .arg(deviceName())
                                .arg(trackNumber)
                                .arg(trackNumber, 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CB])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CB], 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CC])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CC], 2, 16, QChar('0'));
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    if (writeHappyTrack(trackNumber, true)) {
                        writeComplete();
                    }
                    else {
                        writeError();
                        qCritical() << "!e" << tr("[%1] Happy Write track failed.")
                                       .arg(deviceName());
                    }
                    writeComplete();
                }
                else {
                    qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                .arg(deviceName())
                                .arg(command, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'))
                                .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    writeCommandNak();
                }
            }
            else {
                qWarning() << "!w" << tr("[%1] Command $49 NAKed (HAPPY is not enabled).")
                               .arg(deviceName());
                writeCommandNak();
            }
            break;
        }
    case 0x4A:  // Init Skew Alignment (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled() && m_board.isHappy1050()) {
                if (m_board.getLastHappyUploadCrc16() == 0x4312) {
                    if (!writeCommandAck()) {
                        break;
                    }
                    qDebug() << "!n" << tr("[%1] Happy Init Skew alignment")
                                .arg(deviceName());
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    writeComplete();
                }
                else {
                    qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                .arg(deviceName())
                                .arg(command, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'))
                                .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    writeCommandNak();
                }
            }
            else {
                qWarning() << "!w" << tr("[%1] Command $4A NAKed (HAPPY is not enabled).")
                               .arg(deviceName());
                writeCommandNak();
            }
            break;
        }
    case 0x4B:  // Prepare backup (HAPPY Rev.7)
        {
            if (m_board.isHappyEnabled()) {
                if (!writeCommandAck()) {
                    break;
                }
                qDebug() << "!n" << tr("[%1] Happy Prepare backup with AUX $%2")
                               .arg(deviceName())
                               .arg(aux, 4, 16, QChar('0'));
                QThread::usleep(150);
                sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                writeComplete();
                m_board.setHappy1050(true);
            }
            else {
                writeCommandNak();
                qWarning() << "!w" << tr("[%1] Happy Prepare backup NAKed (HAPPY is not enabled).")
                               .arg(deviceName());
            }
            break;
        }
    case 0x4C:  // Set RAM Buffer (CHIP/ARCHIVER)
        {
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Set RAM Buffer denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Set RAM Buffer")
                        .arg(deviceName());
            QByteArray ram = readDataFrame(128);
            if (ram.size() == 128) {
                if (! writeDataAck()) {
                    break;
                }
                writeComplete();
                for (int i = 0; i < 32; i++) {
                    m_board.m_chipRam[i] = (unsigned char) ram[i];
                }
            } else {
                qCritical() << "!e" << tr("[%1] Super Archiver Set RAM Buffer data frame failed.")
                               .arg(deviceName());
                m_board.m_chipRam[0] = 0;
                writeDataNak();
            }
            break;
        }
    case 0x4D:  // Execute code (CHIP)
        {
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Chip Execute code denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Chip Execute code (ignored)")
                        .arg(deviceName());
            QByteArray ram = readDataFrame(128);
            if (ram.size() == 128) {
                if (! writeDataAck()) {
                    break;
                }
                if (m_disassembleUploadedCode) {
                    int address = getUploadCodeStartAddress(command, aux, ram);
                    if (address != -1) {
                        m_remainingBytes.clear();
                        disassembleCode(ram, (unsigned short) address, false, false);
                    }
                }
                writeComplete();
            } else {
                qCritical() << "!e" << tr("[%1] Chip Execute code data frame failed.")
                               .arg(deviceName());
                writeDataNak();
            }
            break;
        }
    case 0x4E:  // Get PERCOM block
        {
            if (!writeCommandAck()) {
                break;
            }
            QByteArray percom = m_newGeometry.toPercomBlock();
            qDebug() << "!n" << tr("[%1] Get PERCOM block (%2).")
                           .arg(deviceName())
                           .arg(m_newGeometry.humanReadable());
            writeComplete();
            writeDataFrame(percom);
            break;
        }
    case 0x4F:  // Set PERCOM block or Open CHIP if code is 2267 or ABCD
        {
            if (!writeCommandAck()) {
                break;
            }
            if ((aux == 0x2267) || (aux == 0xABCD)) {
                if (m_geometry.sectorCount() <= 1040 && m_geometry.bytesPerSector() == 128) {
                    m_board.setChipOpen(true);
                    m_board.setLastArchiverUploadCrc16(0);
                    emit statusChanged(m_deviceNo);
                    qDebug() << "!n" << tr("[%1] Open CHIP with code %2")
                                .arg(deviceName())
                                .arg(aux, 4, 16, QChar('0'));
                }
                else {
                    qWarning() << "!w" << tr("[%1] Open CHIP denied on disk %2")
                                  .arg(deviceName())
                                  .arg(m_newGeometry.humanReadable());
                }
                writeComplete();
                break;
            }
            qDebug() << "!n" << tr("[%1] Set PERCOM block (%2).")
                          .arg(deviceName())
                          .arg(m_newGeometry.humanReadable());
            QByteArray percom = readDataFrame(12);
            if (percom.isEmpty()) {
                writeDataNak();
                break;
            }
            if (!writeDataAck()) {
                break;
            }
            m_newGeometry.initialize(percom);

            writeComplete();
            break;
        }
    case 0x50:  // Write sector with verify (ALL) or Write memory (HAPPY 810)
    case 0x57:  // Write sector without verify (ALL) or Write memory (HAPPY 810)
    case 0x70:  // High Speed Write sector with verify or Write memory (HAPPY 1050)
    case 0x77:  // High Speed Write sector without verify or Write memory (HAPPY 1050)
        {
            quint16 sector = aux;
            if (m_board.isChipOpen()) {
                sector = aux & 0x3FF;
            }
            else if (m_board.isHappyEnabled()) {
                if ((! m_board.isHappy1050()) && (aux >= 0x0800) && (aux <= 0x1380)) { // Write memory (HAPPY 810)
                    if (!writeCommandAck()) {
                        break;
                    }
                    qDebug() << "!n" << tr("[%1] Happy Write memory at $%2")
                                .arg(deviceName())
                                .arg(aux, 4, 16, QChar('0'));
                    QByteArray ram = readDataFrame(m_geometry.bytesPerSector(sector));
                    if (!ram.isEmpty()) {
                        if (!writeDataAck()) {
                            break;
                        }
                        for (int i = 0; i < ram.size(); i++) {
                            m_board.m_happyRam[aux - 0x0800 + i] = ram[i];
                        }
                        if (aux == 0x0800) {
                            // compute CRC16 and save it to know what data should be returned by other commands
                            Crc16 crc16;
                            crc16.Reset();
                            for (int j = 0; j < ram.size(); j++) {
                                crc16.Add((unsigned char) (ram.at(j)));
                            }
                            m_board.setLastHappyUploadCrc16(crc16.GetCrc());
                            m_remainingBytes.clear();
                        }
                        if (m_disassembleUploadedCode) {
                            int address = getUploadCodeStartAddress(command, aux, ram);
                            if (address != -1) {
                                disassembleCode(ram, (unsigned short) address, false, m_board.hasHappySignature());
                            }
                        }
                        writeComplete();
                    }
                    else {
                        writeError();
                    }
                    break;
                }
                else if ((aux & 0x8000) && (aux < 0x9800)) { // Write memory (HAPPY Rev.7)
                    if (!writeCommandAck()) {
                        break;
                    }
                    m_board.setHappy1050(true);
                    qDebug() << "!n" << tr("[%1] Happy %2Write memory at $%3")
                                .arg(deviceName())
                                .arg((command == 0x70) || (command == 0x77) ? tr("High Speed ") : "")
                                .arg(aux, 4, 16, QChar('0'));
                    // We should set high speed for data with Happy 1050 Rev.7 but it does not work.
                    // A patched version of Happy Warp Speed Software V7.1.atr has been prepared to send data at normal speed to RespeQt
                    // The original version is not compatible with RespeQt.
                    QByteArray ram = readDataFrame(m_geometry.bytesPerSector(sector));
                    if (!ram.isEmpty()) {
                        if (!writeDataAck()) {
                            break;
                        }
                        for (int i = 0; i < ram.size(); i++) {
                            m_board.m_happyRam[aux - 0x8000 + i] = ram[i];
                        }
                        if (aux == 0x8000) {
                            // compute CRC16 and save it to know what data should be returned by other commands
                            Crc16 crc16;
                            crc16.Reset();
                            for (int j = 0; j < ram.size(); j++) {
                                crc16.Add((unsigned char) (ram.at(j)));
                            }
                            m_board.setLastHappyUploadCrc16(crc16.GetCrc());
                            m_remainingBytes.clear();
                        }
                        if (m_disassembleUploadedCode) {
                            int address = getUploadCodeStartAddress(command, aux, ram);
                            if (address != -1) {
                                disassembleCode(ram, (unsigned short) address, true, true);
                            }
                        }
                        writeComplete();
                    }
                    else {
                        writeError();
                    }
                    break;
                }
                else {
                    writeCommandNak();
                    qWarning() << "!w" << tr("[%1] Write memory at $%2 NAKed (Address out of range).")
                                   .arg(deviceName())
                                   .arg(aux, 4, 16, QChar('0'));
                }
            }
            if ((sector >= 1) && (sector <= m_geometry.sectorCount())) {
                if (!writeCommandAck()) {
                    break;
                }
                int track = (sector - 1) / m_geometry.sectorsPerTrack();
                int relativeSector = ((sector - 1) % m_geometry.sectorsPerTrack()) + 1;
                if (aux != sector) {
                    qDebug() << "!n" << tr("[%1] %2Write Sector %3 ($%4) #%5 in track %6 ($%7) with AUX=$%8")
                                .arg(deviceName())
                                .arg((command == 0x70) || (command == 0x77) ? tr("Happy High Speed ") : "")
                                .arg(sector)
                                .arg(sector, 3, 16, QChar('0'))
                                .arg(relativeSector)
                                .arg(track)
                                .arg(track, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'));
                }
                else {
                    qDebug() << "!n" << tr("[%1] %2Write Sector %3 ($%4) #%5 in track %6 ($%7)")
                                .arg(deviceName())
                                .arg((command == 0x70) || (command == 0x77) ? tr("Happy High Speed ") : "")
                                .arg(sector)
                                .arg(sector, 3, 16, QChar('0'))
                                .arg(relativeSector)
                                .arg(track)
                                .arg(track, 2, 16, QChar('0'));
                }
                // We should set high speed for data with Happy 1050 Rev.7 but it does not work.
                // A patched version of Happy Warp Speed Software V7.1.atr has been prepared to send data at normal speed to RespeQt
                // The original version is not compatible with RespeQt.
                QByteArray data = readDataFrame(m_geometry.bytesPerSector(sector));
                if (!data.isEmpty()) {
                    if (!writeDataAck()) {
                        break;
                    }
                    if (m_isReadOnly) {
                        writeError();
                        qWarning() << "!w" << tr("[%1] Write sector denied.")
                                      .arg(deviceName());
                        break;
                    }
                    if (writeSector(aux, data)) {
                        writeComplete();
                    }
                    else {
                        writeError();
                        qCritical() << "!e" << tr("[%1] Write sector failed.")
                                       .arg(deviceName());
                    }
                } else {
                    qCritical() << "!e" << tr("[%1] Write sector data frame failed.")
                                   .arg(deviceName());
                    writeDataNak();
                }
            }
            else {
                writeCommandNak();
                qWarning() << "!w" << tr("[%1] Write sector %2 ($%3) NAKed.")
                               .arg(deviceName())
                               .arg(aux)
                               .arg(aux, 4, 16, QChar('0'));
            }
            break;
        }
    case 0x51:  // Execute custom code (HAPPY)
        {
            if (m_board.isHappyEnabled() && m_board.hasHappySignature()) {
                if (!writeCommandAck()) {
                    break;
                }
                writeComplete();
                if ((m_board.getLastHappyUploadCrc16() == 0xEF1A) || (m_board.getLastHappyUploadCrc16() == 0xC8C8)) { // Happy Rev.5 and Rev.7
                    qDebug() << "!n" << tr("[%1] Happy Ram check")
                                .arg(deviceName());
                    writeDataFrame(QByteArray((const char *)HAPPY_RAM_CHECK, sizeof(HAPPY_RAM_CHECK)));
                }
                else if (m_board.getLastHappyUploadCrc16() == 0xC545) {
                    qDebug() << "!n" << tr("[%1] Happy Rom 810 test")
                                .arg(deviceName());
                    writeDataFrame(QByteArray((const char *)HAPPY_ROM810_CHECK, sizeof(HAPPY_ROM810_CHECK)));
                }
                else if (m_board.getLastHappyUploadCrc16() == 0x72CD) {
                    qDebug() << "!n" << tr("[%1] Happy Extended ram check")
                                .arg(deviceName());
                    writeDataFrame(QByteArray((const char *)HAPPY_RAM_EXTENDED_CHECK, sizeof(HAPPY_RAM_EXTENDED_CHECK)));
                }
                else if (m_board.getLastHappyUploadCrc16() == 0xEABE) {
                    qDebug() << "!n" << tr("[%1] Happy Run diagnostic")
                                .arg(deviceName());
                    writeDataFrame(QByteArray((const char *)HAPPY_DIAGNOSTIC, sizeof(HAPPY_DIAGNOSTIC)));
                }
                else if (m_board.getLastHappyUploadCrc16() == 0x8735) {
                    qDebug() << "!n" << tr("[%1] Happy Speed check")
                                .arg(deviceName());
                    writeDataFrame(QByteArray((const char *)HAPPY_SPEED_CHECK, sizeof(HAPPY_SPEED_CHECK)));
                }
                else if (m_board.getLastHappyUploadCrc16() == 0x8ABA) {
                    // Step-in, Step-out commands. No data to return
                    qDebug() << "!n" << tr("[%1] Happy Step-in / Step-out")
                                .arg(deviceName());
                }
                else if (m_board.getLastHappyUploadCrc16() == 0xE610) {
                    qDebug() << "!n" << tr("[%1] Happy Read/Write check")
                                .arg(deviceName());
                    writeDataFrame(QByteArray((const char *)HAPPY_RW_CHECK, sizeof(HAPPY_RW_CHECK)));
                }
                else if (m_board.getLastHappyUploadCrc16() == 0x9831) {
                    qDebug() << "!n" << tr("[%1] Happy Set unhappy mode")
                                .arg(deviceName());
                    m_board.setHappyEnabled(false);
                    emit statusChanged(m_deviceNo);
                    writeDataFrame(m_board.m_happyRam.left(128));
                }
                else if (m_board.getLastHappyUploadCrc16() == 0x5D3D) {
                    int trackNumber = (int)(0xFF - (aux & 0xFF));
                    qDebug() << "!n" << tr("[%1] Happy Read Track %2 ($%3)")
                                .arg(deviceName())
                                .arg(trackNumber)
                                .arg(trackNumber, 2, 16, QChar('0'));
                    readHappyTrack(trackNumber, false);
                    writeDataFrame(m_board.m_happyRam.mid(0x300, 128));
                }
                else if (m_board.getLastHappyUploadCrc16() == 0xEF08) {
                    qDebug() << "!n" << tr("[%1] Happy init backup 1")
                                .arg(deviceName());
                    writeDataFrame(QByteArray((const char *)HAPPY_BACKUP_BUF1, sizeof(HAPPY_BACKUP_BUF1)));
                }
                else if (m_board.getLastHappyUploadCrc16() == 0xCC68) {
                    qDebug() << "!n" << tr("[%1] Happy init backup 2")
                                .arg(deviceName());
                    writeDataFrame(QByteArray((const char *)HAPPY_BACKUP_BUF2, sizeof(HAPPY_BACKUP_BUF2)));
                }
                else {
                    // Clear buffer command contains 64 bytes of code but the remaining bytes are garbage.
                    // Just check the last instruction ($083D: JMP $1EFC)
                    if (((quint8)m_board.m_happyRam[0x3D] == (quint8)0x4C) && ((quint8)m_board.m_happyRam[0x3E] == (quint8)0xFC) && ((quint8)m_board.m_happyRam[0x3F] == (quint8)0x1E)) {
                        qDebug() << "!n" << tr("[%1] Happy Clear buffer")
                                    .arg(deviceName());
                        m_board.m_happyRam[0] = 0x80;
                    }
                    else {
                        qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                    .arg(deviceName())
                                    .arg(command, 2, 16, QChar('0'))
                                    .arg(aux, 4, 16, QChar('0'))
                                    .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    }
                }
            }
            else if (m_board.isHappyEnabled() && m_board.isHappy1050()) {
                if (m_board.getLastHappyUploadCrc16() == 0x4312) {
                    if (!writeCommandAck()) {
                        break;
                    }
                    qDebug() << "!n" << tr("[%1] Happy init backup")
                                .arg(deviceName());
                    writeComplete();
                }
                else {
                    qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                .arg(deviceName())
                                .arg(command, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'))
                                .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                    writeCommandNak();
                }
            }
            else {
                qWarning() << "!w" << tr("[%1] Command $51 NAKed (HAPPY is not enabled).")
                               .arg(deviceName());
                writeCommandNak();
            }
            break;
        }
    case 0x52:  // Read sector (ALL) or Read memory (HAPPY 810)
    case 0x72:  // High Speed Read sector or Read memory (HAPPY 1050)
        {
            quint16 sector = aux;
            if (m_board.isChipOpen()) {
                sector = aux & 0x3FF;
            }
            else if (m_board.isHappyEnabled()) {
                if ((! m_board.isHappy1050()) && (aux >= 0x800) && (aux <= 0x1380)) { // Read memory (HAPPY 810 Rev.5)
                    if (!writeCommandAck()) {
                        break;
                    }
                    qDebug() << "!n" << tr("[%1] Happy Read memory at $%2")
                                .arg(deviceName())
                                .arg(aux, 4, 16, QChar('0'));
                    if (!writeComplete()) {
                        break;
                    }
                    writeDataFrame(m_board.m_happyRam.mid(aux - 0x800, 128));
                    break;
                }
                else if ((aux & 0x8000) && (aux < 0x9800)) { // Read memory (HAPPY Rev.7)
                    if (!writeCommandAck()) {
                        break;
                    }
                    qDebug() << "!n" << tr("[%1] Happy %2Read memory at $%3")
                                .arg(deviceName())
                                .arg((command == 0x72) ? tr("High Speed ") : "")
                                .arg(aux, 4, 16, QChar('0'));
                    if (command == 0x72) {
                        QThread::usleep(150);
                        sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    }
                    if (!writeComplete()) {
                        break;
                    }
                    writeDataFrame(m_board.m_happyRam.mid(aux & 0x7FFF, 128));
                    break;
                }
            }
            if ((sector >= 1) && (sector <= m_geometry.sectorCount())) {
                if (!writeCommandAck()) {
                    break;
                }
                int track = (sector - 1) / m_geometry.sectorsPerTrack();
                int relativeSector = ((sector - 1) % m_geometry.sectorsPerTrack()) + 1;
                if (aux != sector) {
                    qDebug() << "!n" << tr("[%1]%2 Read Sector %3 ($%4) #%5 in track %6 ($%7) with AUX=$%8")
                                .arg(deviceName())
                                .arg((command == 0x72) ? tr("Happy High Speed ") : "")
                                .arg(sector)
                                .arg(sector, 3, 16, QChar('0'))
                                .arg(relativeSector)
                                .arg(track)
                                .arg(track, 2, 16, QChar('0'))
                                .arg(aux, 4, 16, QChar('0'));
                }
                else {
                    qDebug() << "!n" << tr("[%1] %2Read Sector %3 ($%4) #%5 in track %6 ($%7)")
                                .arg(deviceName())
                                .arg((command == 0x72) ? tr("Happy High Speed ") : "")
                                .arg(sector)
                                .arg(sector, 3, 16, QChar('0'))
                                .arg(relativeSector)
                                .arg(track)
                                .arg(track, 2, 16, QChar('0'));
                }
                if (command == 0x72) {
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                }
                QByteArray data;
                if (readSector(aux, data)) {
                    quint16 chipFlags = m_board.isChipOpen() ? aux & 0xFC00 : 0;
                    if ((m_wd1771Status != 0xFF) && (chipFlags == 0)) {
                        if (!writeError()) {
                            break;
                        }
                    }
                    else {
                        if (!writeComplete()) {
                            break;
                        }
                        // Are we booting Happy software in Atari memory ?
                        // Then patch Happy software on the fly to remove Write commands at double speed
                        if (m_board.isHappyEnabled() && (m_deviceNo == 0x31)) {
                            if ((command == 0x72) && ((sector == 397) || (sector == 421))) {
                                // if we are loading Happy Warp Speed Software V7.1 disk A.atr from RespeQt, patch it to remove Write commands at double speed !
                                Crc16 crc16;
                                crc16.Reset();
                                for (int j = 0; j < data.size(); j++) {
                                    crc16.Add((unsigned char) (data.at(j)));
                                }
                                if ((sector == 397) && (crc16.GetCrc() == 0x7102)) {
                                    writeDataFrame(QByteArray((const char *)HAPPY_DOUBLESPEEDREV7_PATCH1, sizeof(HAPPY_DOUBLESPEEDREV7_PATCH1)));
                                    m_board.setHappyPatchInProgress(true);
                                    qDebug() << "!u" << tr("[%1] Happy Warp Speed Software V7.1 patched on the fly to be compatible with RespeQt")
                                                .arg(deviceName());
                                    break;
                                }
                                else if ((sector == 421) && (crc16.GetCrc() == 0xF00A) && (m_board.isHappyPatchInProgress())) {
                                    writeDataFrame(QByteArray((const char *)HAPPY_DOUBLESPEEDREV7_PATCH2, sizeof(HAPPY_DOUBLESPEEDREV7_PATCH2)));
                                    m_board.setHappyPatchInProgress(false);
                                    break;
                                }
                            }
#if HAPPY_PATCH_V53
                            // not mandatory as the main feature (Backup protected disk) does not use double speed (only Sector copy uses double speed)
                            else if ((command == 0x52) && ((sector == 59) || (sector == 71) || (sector == 85))) {
                                // if we are loading Happy Warp Speed Software V5.3.atr from RespeQt, patch it to remove Write commands at double speed !
                                Crc16 crc16;
                                crc16.Reset();
                                for (int j = 0; j < data.size(); j++) {
                                    crc16.Add((unsigned char) (data.at(j)));
                                }
                                if ((sector == 59) && (crc16.GetCrc() == 0x8CB3)) {
                                    writeDataFrame(QByteArray((const char *)HAPPY_DOUBLESPEEDREV5_PATCH1, sizeof(HAPPY_DOUBLESPEEDREV5_PATCH1)));
                                    m_board.setHappyPatchInProgress(true);
                                    qDebug() << "!u" << tr("[%1] Happy Warp Speed Software V5.3 patched on the fly to be compatible with RespeQt")
                                                .arg(deviceName());
                                    break;
                                }
                                else if ((sector == 71) && (crc16.GetCrc() == 0x4552) && (m_board.isHappyPatchInProgress())) {
                                    writeDataFrame(QByteArray((const char *)HAPPY_DOUBLESPEEDREV5_PATCH2, sizeof(HAPPY_DOUBLESPEEDREV5_PATCH2)));
                                    break;
                                }
                                else if ((sector == 85) && (crc16.GetCrc() == 0xF00A) && (m_board.isHappyPatchInProgress())) {
                                    writeDataFrame(QByteArray((const char *)HAPPY_DOUBLESPEEDREV5_PATCH3, sizeof(HAPPY_DOUBLESPEEDREV5_PATCH3)));
                                    m_board.setHappyPatchInProgress(false);
                                    break;
                                }
                            }
#endif
                        }
                    }
                    writeDataFrame(data);
                } else {
                    qCritical() << "!e" << tr("[%1] Read sector failed.")
                                   .arg(deviceName());
                    if (!writeError()) {
                        break;
                    }
                    writeDataFrame(data);
                }
            }
            else {
                writeCommandNak();
                qWarning() << "!w" << tr("[%1] Read sector $%2 NAKed.")
                               .arg(deviceName())
                               .arg(aux, 4, 16, QChar('0'));
            }
            break;
        }
    case 0x53:  // Get status
        {
            if (!writeCommandAck()) {
                break;
            }
            QByteArray status(4, 0);
            getStatus(status);
            if (aux == 0xABCD) {
                status[2] = 0xEB; // This is a RespeQt drive
                status[3] = computeVersionByte();
                qDebug() << "!n" << tr("[%1] RespeQt version inquiry: $%2")
                            .arg(deviceName())
                            .arg((unsigned char)status[3], 2, 16, QChar('0'));
            }
            else {
                qDebug() << "!n" << tr("[%1] Get status: $%2")
                            .arg(deviceName())
                            .arg((unsigned char)status[1], 2, 16, QChar('0'));
            }
            writeComplete();
            writeDataFrame(status);
            break;
        }
    case 0x54:  // Get RAM Buffer (CHIP/ARCHIVER) or Write sector and Write track (Happy 810 Rev.5)
        {
            if (m_board.isHappyEnabled()) {
                if (m_board.hasHappySignature()) { // Happy 810 Rev. 5
                    if (m_board.getLastHappyUploadCrc16() == 0x4416) {
                        if (!writeCommandAck()) {
                            break;
                        }
                        quint16 track = 0xFF - (quint8)(aux & 0xFF);
                        quint16 relativeSector = 0xFF - (quint8)((aux >> 8) & 0xFF);
                        quint16 sector = (track * m_geometry.sectorsPerTrack()) + relativeSector;
                        qDebug() << "!n" << tr("[%1] Happy High Speed Write Sector %2 ($%3) #%4 in track %5 ($%6)")
                                    .arg(deviceName())
                                    .arg(sector)
                                    .arg(sector, 3, 16, QChar('0'))
                                    .arg(relativeSector)
                                    .arg(track)
                                    .arg(track, 2, 16, QChar('0'));
                        // We should set high speed for data with Happy 810 Rev.5 but it does not work.
                        // A patched version of Happy Warp Speed Software V5.3.atr has been prepared to send data at normal speed to RespeQt
                        // The original version is not compatible with RespeQt.
                        QByteArray data = readDataFrame(m_geometry.bytesPerSector(sector));
                        if (!data.isEmpty()) {
                            if (!writeDataAck()) {
                                break;
                            }
                            if (m_isReadOnly) {
                                writeError();
                                qWarning() << "!w" << tr("[%1] Happy Write sector denied.")
                                              .arg(deviceName());
                                break;
                            }
                            if (writeSector(sector, data)) {
                                writeComplete();
                            } else {
                                writeError();
                                qCritical() << "!e" << tr("[%1] Happy Write sector failed.")
                                               .arg(deviceName());
                            }
                        } else {
                            qCritical() << "!e" << tr("[%1] Happy Write sector data frame failed.")
                                           .arg(deviceName());
                            writeDataNak();
                        }
                        break;
                    }
                    else if (m_board.getLastHappyUploadCrc16() == 0xC96C) {
                        if (!writeCommandAck()) {
                            break;
                        }
                        int trackNumber = (int)(0xFF - (aux & 0xFF));
                        qDebug() << "!n" << tr("[%1] Happy Write track %2 ($%3)")
                                    .arg(deviceName())
                                    .arg(trackNumber)
                                    .arg(trackNumber, 2, 16, QChar('0'));
                        if (writeHappyTrack(trackNumber, false)) {
                            writeComplete();
                        } else {
                            writeError();
                            qCritical() << "!e" << tr("[%1] Happy Write track failed.")
                                           .arg(deviceName());
                        }
                        break;
                    }
                }
                else if (m_board.isHappy1050()) {
                    // try computing CRC16 for 0x9500 which is where diagnostic code is uploaded by Rev.7
                    Crc16 crc16;
                    crc16.Reset();
                    QByteArray ram = m_board.m_happyRam.mid(0x1500, 128);
                    for (int j = 0; j < ram.size(); j++) {
                        crc16.Add((unsigned char) (ram.at(j)));
                    }
                    if (crc16.GetCrc() == 0x544B) {
                        if (!writeCommandAck()) {
                            break;
                        }
                        if (!writeComplete()) {
                            break;
                        }
                        qDebug() << "!n" << tr("[%1] Happy Rom 1050 test")
                                    .arg(deviceName());
                        writeDataFrame(QByteArray((const char *)HAPPY_ROM1050_CHECK, sizeof(HAPPY_ROM1050_CHECK)));
                        break;
                    }
                }
            }
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Get RAM Buffer denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Get RAM Buffer")
                        .arg(deviceName());
            if (!writeComplete()) {
                break;
            }
            QByteArray result(128, 0);
            for (int i = 0; i < 32; i++) {
                result[i] = m_board.m_chipRam[i];
            }
            writeDataFrame(result);
            break;
        }
    case 0x55:  // Execute custom code (HAPPY)
    case 0x56:  // Execute custom code (HAPPY)
        {
            if (m_board.isHappyEnabled() && m_board.hasHappySignature()) {
                if (!writeCommandAck()) {
                    break;
                }
                if ((command == 0x55) && (m_board.getLastHappyUploadCrc16() == 0x4416)) {
                    // Read sector at high speed
                    int track = (aux - 1) / m_geometry.sectorsPerTrack();
                    qDebug() << "!n" << tr("[%1] Happy High Speed Read Sector %2 ($%3) in track %4 ($%5)")
                                .arg(deviceName())
                                .arg(aux)
                                .arg(aux, 3, 16, QChar('0'))
                                .arg(track)
                                .arg(track, 2, 16, QChar('0'));
                    QThread::usleep(150);
                    sio->port()->setSpeed(findNearestSpeed(38400) + 1); // +1 (odd number) is a trick to set 2 stop bits (needed by Happy 810)
                    QByteArray data;
                    if (readSector(aux, data)) {
                        if (m_wd1771Status != 0xFF) {
                            if (!writeError()) {
                                break;
                            }
                        }
                        else if (!writeComplete()) {
                            break;
                        }
                        writeDataFrame(data);
                    }
                    else {
                        qCritical() << "!e" << tr("[%1] Happy Read Sector failed.")
                                       .arg(deviceName())
                                       .arg(aux, 3, 16, QChar('0'));
                        writeError();
                        writeDataFrame(data);
                    }
                }
                else if ((command == 0x55) && (m_board.getLastHappyUploadCrc16() == 0x5D3D)) {
                    qDebug() << "!n" << tr("[%1] Happy Read Skew alignment of track %2 ($%3) sector %4 ($%5) with track %6 ($%7) sector %8 ($%9)")
                                .arg(deviceName())
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CB])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CB], 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CC])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CC], 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3C9])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3C9], 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CA])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x3CA], 2, 16, QChar('0'));
                    if (readHappySkewAlignment(false)) {
                        if (!writeComplete()) {
                            break;
                        }
                    }
                    else {
                        qCritical() << "!e" << tr("[%1] Happy Read Skew alignment failed.")
                                       .arg(deviceName())
                                       .arg(aux, 3, 16, QChar('0'));
                        if (!writeError()) {
                            break;
                        }
                    }
                    writeDataFrame(m_board.m_happyRam.mid(0x380, 128));
                }
                else if ((command == 0x56) && (m_board.getLastHappyUploadCrc16() == 0x5D3D)) {
                    int trackNumber = (int)(0xFF - (aux & 0xFF));
                    int aux2 = (aux >> 8) & 0xFF;
                    int afterSectorNumber = (aux2 == 0) ? 0 : 0xFF - aux2;
                    if (afterSectorNumber == 0) {
                        qDebug() << "!n" << tr("[%1] Happy Read Sectors of track %2 ($%3)")
                                    .arg(deviceName())
                                    .arg(trackNumber)
                                    .arg(trackNumber, 2, 16, QChar('0'));
                    }
                    else {
                        qDebug() << "!n" << tr("[%1] Happy Read Sectors of track %2 ($%3) starting after sector %4 ($%5)")
                                    .arg(deviceName())
                                    .arg(trackNumber)
                                    .arg(trackNumber, 2, 16, QChar('0'))
                                    .arg(afterSectorNumber)
                                    .arg(afterSectorNumber, 2, 16, QChar('0'));
                    }
                    QByteArray data = readHappySectors(trackNumber, afterSectorNumber, false);
                    if (!writeComplete()) {
                        break;
                    }
                    writeDataFrame(data);
                }
                else if ((command == 0x55) && (m_board.getLastHappyUploadCrc16() == 0xC96C)) {
                    int trackNumber = (int)(0xFF - (aux & 0xFF));
                    int aux2 = (aux >> 8) & 0xFF;
                    int afterSectorNumber = (aux2 == 0) ? 0 : 0xFF - aux2;
                    if (afterSectorNumber == 0) {
                        qDebug() << "!n" << tr("[%1] Happy Write Sectors of track %2 ($%3)")
                                    .arg(deviceName())
                                    .arg(trackNumber)
                                    .arg(trackNumber, 2, 16, QChar('0'));
                    }
                    else {
                        qDebug() << "!n" << tr("[%1] Happy Write Sectors of track %2 ($%3) starting after sector %4 ($%5)")
                                    .arg(deviceName())
                                    .arg(trackNumber)
                                    .arg(trackNumber, 2, 16, QChar('0'))
                                    .arg(afterSectorNumber)
                                    .arg(afterSectorNumber, 2, 16, QChar('0'));
                    }
                    if (writeHappySectors(trackNumber, afterSectorNumber, false)) {
                        if (!writeComplete()) {
                            break;
                        }
                    }
                    else {
                        qCritical() << "!e" << tr("[%1] Happy Write Sectors failed.")
                                       .arg(deviceName())
                                       .arg(aux, 3, 16, QChar('0'));
                        if (!writeError()) {
                            break;
                        }
                    }
                    writeDataFrame(m_board.m_happyRam.mid(0x280, 128));
                }
                else if ((command == 0x56) && (m_board.getLastHappyUploadCrc16() == 0xC96C)) {
                    int trackNumber = (int)((quint8)0xFF - (quint8)m_board.m_happyRam[0x84F]);
                    int sectorNumber = (int)((quint8)0xFF - (quint8)m_board.m_happyRam[0x81A]);
                    qDebug() << "!n" << tr("[%1] Happy Write track %2 ($%3) skew aligned with track %4 ($%5) sector %6 ($%7)")
                                .arg(deviceName())
                                .arg(trackNumber)
                                .arg(trackNumber, 2, 16, QChar('0'))
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x816])
                                .arg((quint8)0xFF - (quint8)m_board.m_happyRam[0x816], 2, 16, QChar('0'))
                                .arg(sectorNumber)
                                .arg(sectorNumber, 2, 16, QChar('0'));
                    if (writeHappyTrack(trackNumber, false)) {
                        writeComplete();
                    }
                    else {
                        writeError();
                        qCritical() << "!e" << tr("[%1] Happy Write track failed.")
                                       .arg(deviceName());
                    }
                }
                else if ((command == 0x56) && (m_board.getLastHappyUploadCrc16() == 0x4416)) {
                    writeComplete();
                    qDebug() << "!n" << tr("[%1] Happy init sector copy")
                                .arg(deviceName());
                }
                else {
                    if (m_board.getLastHappyUploadCrc16() == 0x8ABA) {
                        // Step-in, Step-out commands. No data to return
                        qDebug() << "!n" << tr("[%1] Happy Step-in / Step-out")
                                    .arg(deviceName());
                        writeComplete();
                    }
                    else {
                        qWarning() << "!w" << tr("[%1] Happy Execute custom code $%2 with AUX $%3 and CRC16 $%4. Ignored")
                                    .arg(deviceName())
                                    .arg(command, 2, 16, QChar('0'))
                                    .arg(aux, 4, 16, QChar('0'))
                                    .arg(m_board.getLastHappyUploadCrc16(), 4, 16, QChar('0'));
                        writeError();
                    }
                }
            }
            else {
                writeCommandNak();
                qWarning() << "!w" << tr("[%1] Command $%2 NAKed (HAPPY is not enabled).")
                               .arg(deviceName())
                               .arg(command, 2, 16, QChar('0'));
            }
            break;
        }
    case 0x62: // Write Fuzzy Sector using Index (ARCHIVER)
        {
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Write Fuzzy Sector using Index denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Write Fuzzy Sector using Index with AUX1=$%2 and AUX2=$%3")
                        .arg(deviceName())
                        .arg((aux & 0xFF), 2, 16, QChar('0'))
                        .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
            QByteArray data = readDataFrame(m_geometry.bytesPerSector());
            if (!data.isEmpty()) {
                if (!writeDataAck()) {
                    break;
                }
                if (m_isReadOnly) {
                    writeError();
                    qWarning() << "!w" << tr("[%1] Super Archiver Write Fuzzy Sector using Index denied.")
                                  .arg(deviceName());
                    break;
                }
                if (writeSectorUsingIndex(aux, data, true)) {
                    writeComplete();
                } else {
                    writeError();
                    qCritical() << "!e" << tr("[%1] Super Archiver Write Fuzzy Sector using Index failed.")
                                   .arg(deviceName());
                }
            } else {
                qCritical() << "!e" << tr("[%1] Super Archiver Write Fuzzy Sector using Index data frame failed.")
                               .arg(deviceName());
                writeDataNak();
            }
            break;
        }
    case 0x66:  // Format with custom sector skewing
        {
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!u" << tr("[%1] Format with custom sector skewing.")
                        .arg(deviceName());
            QByteArray percom = readDataFrame(128);
            if (percom.isEmpty()) {
                writeDataNak();
                break;
            }
            writeDataAck();
            m_newGeometry.initialize(percom);
            if (!m_isReadOnly) {
                if (!format(m_newGeometry)) {
                    qCritical() << "!e" << tr("[%1] Format with custom sector skewing failed.")
                                   .arg(deviceName());
                    break;
                }
                writeComplete();
            } else {
                writeError();
                qWarning() << "!w" << tr("[%1] Format with custom sector skewing denied.")
                               .arg(deviceName());
            }
            break;
        }
    case 0x67:  // Read Track (256 bytes) (ARCHIVER)
        {
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Read Track (256 bytes) denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Read Track (256 bytes) with AUX1=$%2 and AUX2=$%3")
                        .arg(deviceName())
                        .arg((aux & 0xFF), 2, 16, QChar('0'))
                        .arg(((aux >> 8) & 0xFF), 2, 16, QChar('0'));
            QByteArray data;
            readTrack(aux, data, 256);
            if (!writeComplete()) {
                break;
            }
            writeDataFrame(data);
            break;
        }
    case 0x71:  // Write fuzzy sector (ARCHIVER)
        {
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Write Fuzzy sector denied (CHIP is not open)").arg(deviceName());
                writeCommandNak();
                break;
            }
            quint16 sector = aux & 0x3FF;
            if (sector >= 1 && sector <= m_geometry.sectorCount()) {
                if (!writeCommandAck()) {
                    break;
                }
                int track = (aux - 1) / m_geometry.sectorsPerTrack();
                qDebug() << "!n" << tr("[%1] Super Archiver Write Fuzzy Sector %2 ($%3) in track %4 ($%5) with AUX=$%6")
                            .arg(deviceName())
                            .arg(sector)
                            .arg(sector, 3, 16, QChar('0'))
                            .arg(track)
                            .arg(track, 2, 16, QChar('0'))
                            .arg(aux, 4, 16, QChar('0'));
                QByteArray data = readDataFrame(m_geometry.bytesPerSector(sector));
                if (!data.isEmpty()) {
                    if (!writeDataAck()) {
                        break;
                    }
                    if (m_isReadOnly) {
                        writeError();
                        qWarning() << "!w" << tr("[%1] Super Archiver Write Fuzzy sector denied.")
                                      .arg(deviceName());
                        break;
                    }
                    if (writeFuzzySector(aux, data)) {
                        writeComplete();
                    } else {
                        writeError();
                        qCritical() << "!e" << tr("[%1] Super Archiver Write Fuzzy sector failed.")
                                       .arg(deviceName());
                    }
                } else {
                    qCritical() << "!e" << tr("[%1] Super Archiver Write Fuzzy sector data frame failed.")
                                   .arg(deviceName());
                    writeDataNak();
                }
            } else {
                writeCommandNak();
                qWarning() << "!w" << tr("[%1] Super Archiver Write Fuzzy sector $%2 NAKed.")
                               .arg(deviceName())
                               .arg(aux, 4, 16, QChar('0'));
            }
            break;
        }
    case 0x73:  // Set Speed (ARCHIVER)
        {
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Set Speed denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Set Speed %2")
                        .arg(deviceName())
                        .arg(aux);
            m_board.setLastArchiverSpeed(aux & 0xFF);
            if (!writeComplete()) {
                break;
            }
            break;
        }
    case 0x74:  // Read Memory (ARCHIVER)
        {
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Read Memory denied (CHIP is not open)").arg(deviceName());
                writeCommandNak();
                break;
            }
            if (!writeCommandAck()) {
                break;
            }
            if (!writeComplete()) {
                break;
            }
            // this is the code for the speed check (item 5 of Super Archiver menu then Check drive RPM)
            if (m_board.getLastArchiverUploadCrc16() == 0xFD2E) {
                qDebug() << "!n" << tr("[%1] Super Archiver Read memory (Speed check)")
                            .arg(deviceName());
                ARCHIVER_SPEED_CHECK[2] = m_board.getLastArchiverSpeed();
                ARCHIVER_SPEED_CHECK[248] = m_board.getLastArchiverSpeed() == 2 ? 0xCC : 0xD8;
                writeDataFrame(QByteArray((const char *)ARCHIVER_SPEED_CHECK, sizeof(ARCHIVER_SPEED_CHECK)));
            }
            // this is the code for the diagnostic test (Archiver memory, 6532, 6810, Archiver version and rom checksum)
            else if (m_board.getLastArchiverUploadCrc16() == 0x61F6) {
                qDebug() << "!n" << tr("[%1] Super Archiver Read memory (Diagnostic)")
                            .arg(deviceName());
                writeDataFrame(QByteArray((const char *)ARCHIVER_DIAGNOSTIC, sizeof(ARCHIVER_DIAGNOSTIC)));
            }
			// this is the code to get the sector timing for skew alignment
            else if ((m_board.getLastArchiverUploadCrc16() == 0x603D) || (m_board.getLastArchiverUploadCrc16() == 0xBAC2) || (m_board.getLastArchiverUploadCrc16() == 0xDFFF)) { // Super Archiver 3.02, 3.03, 3.12 respectively
                bool timingOnly = m_board.getLastArchiverUploadCrc16() == 0xDFFF;
                if (timingOnly) {
                    quint16 timing = (((quint16)m_board.m_trackData[0xF4]) & 0xFF) + ((((quint16)m_board.m_trackData[0xF5]) << 8) & 0xFF00);
                    qDebug() << "!n" << tr("[%1] Super Archiver Read Memory (Skew alignment of $%2)")
                                .arg(deviceName())
                                .arg(timing, 4, 16, QChar('0'));
                }
                else {
                    qDebug() << "!n" << tr("[%1] Super Archiver Read Memory (Skew alignment)")
                                .arg(deviceName());
                }
                writeDataFrame(m_board.m_trackData);
			}
            else {
                qDebug() << "!n" << tr("[%1] Super Archiver Read Memory")
                            .arg(deviceName());
                writeDataFrame(QByteArray(256, 0));
            }
            break;
        }
    case 0x75:  // Upload and execute code (ARCHIVER)
        {
            m_board.setLastArchiverUploadCrc16(0);
            if (! m_board.isChipOpen()) {
                qWarning() << "!w" << tr("[%1] Super Archiver Upload and execute code denied (CHIP is not open)")
                              .arg(deviceName());
                writeCommandNak();
                break;
            }
            // accept command
            if (!writeCommandAck()) {
                break;
            }
            qDebug() << "!n" << tr("[%1] Super Archiver Upload and Execute Code")
                        .arg(deviceName());
            QByteArray data = readDataFrame(256);
            if (!data.isEmpty()) {
                if (!writeDataAck()) {
                    break;
                }
                if (m_disassembleUploadedCode) {
                    int address = getUploadCodeStartAddress(command, aux, data);
                    if (address != -1) {
                        disassembleCode(data, (unsigned short) address, true, false);
                    }
                }
                bool ok = executeArchiverCode(aux, data);
                if (ok) {
                    writeComplete();
                }
                else {
                    writeError();
                }
            } else {
                qCritical() << "!e" << tr("[%1] Super Archiver Upload and Execute Code data frame failed.")
                               .arg(deviceName());
                writeDataNak();
            }
            break;
        }
    default:    // Unknown command
        {
            writeCommandNak();
            qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                           .arg(deviceName())
                           .arg(command, 2, 16, QChar('0'))
                           .arg(aux, 4, 16, QChar('0'));
            break;
        }
    }
}

void SimpleDiskImage::getStatus(QByteArray &status)
{
    quint8 motorSpinning = 0x10;
    quint8 readOnlyStatus = isReadOnly() || isLeverOpen() ? 0x08 : 0; // if lever not open, the disk is reported as read-only in the status byte
    if ((m_originalImageType == FileTypes::Pro) || (m_originalImageType == FileTypes::ProGz) ||
            (m_originalImageType == FileTypes::Atx) || (m_originalImageType == FileTypes::AtxGz)) {
        status[0] = motorSpinning | readOnlyStatus | m_driveStatus;
    }
    else {
        status[0] = motorSpinning | readOnlyStatus |
                    ((m_newGeometry.bytesPerSector() == 256) * 32) |
                    ((m_newGeometry.bytesPerSector() == 128 && m_newGeometry.sectorsPerTrack() == 26) * 128);
    }
    status[1] = m_wd1771Status;
    if ((! isReady()) || (isLeverOpen())) {
        status[1] = status[1] & (quint8)~0x80;
    }
    if (isReadOnly()) {
        status[1] = status[1] & (quint8)~0x40;
    }
    status[2] = 0xE0; // Timeout for format ($E0) - Time the drive will need to format a disk (1050 returns 0xe0, XF551 0xfe)
    status[3] = 0;
}

quint8 SimpleDiskImage::computeVersionByte()
{
    const char *str = VERSION;
    int index = 0;
    if (str[index] == 'r') {
        index++;
    }
    int major = str[index++] - '0';
    int minor = 0;
    if (str[index] == '.') {
        index++;
        minor = str[index] - '0';
    }
    return (quint8)((major << 4) + minor);
}

int SimpleDiskImage::defaultFileSystem()
{
    if (m_geometry.sectorCount() < 1) {
        return 0;
    }

    QByteArray data;
    if (!readSector(1, data)) {
        return 0;
    }

    if (m_geometry.bytesPerSector() == 512) {
        if ((quint8)data.at(32) == 0x21) {
            return 5;
        } else {
            return 0;
        }
    } else {
        if ((quint8)data.at(7) == 0x80 /*&& ((quint8)data.at(32) == 0x21 || (quint8)data.at(32) == 0x20)*/) {
            return 5;
        } else {
            if (m_geometry.sectorCount() < 368) {
                return 0;
            }
            if (!readSector(360, data)) {
                return 0;
            }
            uint v = (quint8)data.at(0);
            uint s = (quint8)data.at(1) + (quint8)data.at(2) * 256;
            uint f = (quint8)data.at(3) + (quint8)data.at(4) * 256;
            if (m_geometry.isStandardSD() && v == 1 && s == 709 && f <= s) {
                return 1;
            } else if ((m_geometry.isStandardSD() || m_geometry.isStandardDD()) && v == 2 && s == 707 && f <= s) {
                return 2;
            } else if (m_geometry.isStandardED() && v == 2 && s == 1010 && f <= s) {
                return 3;
            } else if (v == 2 && s + 12 == m_geometry.sectorCount() && f <= s) {
                return 4;
            } else if (m_geometry.bytesPerSector() == 128 && s + (v * 2 - 3) + 10 == m_geometry.sectorCount() && f <= s) {
                return 4;
            } else if (m_geometry.bytesPerSector() == 256 && s + v + 9 == m_geometry.sectorCount() && f <= s) {
                return 4;
            } else {
                return 0;
            }
        }
    }
}

bool SimpleDiskImage::writeCommandAck()
{
    if (m_displayTransmission) {
        qDebug() << "!u" << tr("[%1] Sending [COMMAND ACK] to Atari").arg(deviceName());
    }
    return sio->port()->writeCommandAck();
}

bool SimpleDiskImage::writeDataAck()
{
    if (m_displayTransmission) {
        qDebug() << "!u" << tr("[%1] Sending [DATA ACK] to Atari").arg(deviceName());
    }
    return sio->port()->writeDataAck();
}

bool SimpleDiskImage::writeCommandNak()
{
    qWarning() << "!w" << tr("[%1] Sending [COMMAND NAK] to Atari").arg(deviceName());
    return sio->port()->writeCommandNak();
}

bool SimpleDiskImage::writeDataNak()
{
    qWarning() << "!w" << tr("[%1] Sending [DATA NAK] to Atari").arg(deviceName());
    return sio->port()->writeDataNak();
}

bool SimpleDiskImage::writeComplete()
{
    if (m_displayTransmission) {
        qDebug() << "!u" << tr("[%1] Sending [COMPLETE] to Atari").arg(deviceName());
    }
    return sio->port()->writeComplete();
}

bool SimpleDiskImage::writeError()
{
    qWarning() << "!w" << tr("[%1] Sending [ERROR] to Atari").arg(deviceName());
    return sio->port()->writeError();
}

QByteArray SimpleDiskImage::readDataFrame(uint size)
{
    QByteArray data = sio->port()->readDataFrame(size);
    if (m_dumpDataFrame) {
        qDebug() << "!u" << tr("[%1] Receiving %2 bytes from Atari").arg(deviceName()).arg(data.length());
        dumpBuffer((unsigned char *) data.data(), data.length());
    }
    return data;
}

bool SimpleDiskImage::writeDataFrame(QByteArray data)
{
    if (m_dumpDataFrame) {
        qDebug() << "!u" << tr("[%1] Sending %2 bytes to Atari").arg(deviceName()).arg(data.length());
        dumpBuffer((unsigned char *) data.data(), data.length());
    }
    return sio->port()->writeDataFrame(data);
}

quint16 SimpleDiskImage::getBigIndianWord(QByteArray &array, int offset)
{
    return (((quint16) array[offset + 1]) & 0xFF) + ((((quint16) array[offset]) << 8) & 0xFF00);
}

quint16 SimpleDiskImage::getLittleIndianWord(QByteArray &array, int offset)
{
    return (((quint16) array[offset]) & 0xFF) + ((((quint16) array[offset + 1]) << 8) & 0xFF00);
}

quint32 SimpleDiskImage::getLittleIndianLong(QByteArray &array, int offset)
{
    return (((quint32) array[offset]) & 0xFF) + ((((quint32) array[offset + 1]) << 8) & 0xFF00) + ((((quint32) array[offset + 2]) << 16) & 0xFF0000) + ((((quint32) array[offset + 3]) << 24) & 0xFF000000);
}

void SimpleDiskImage::setLittleIndianWord(QByteArray &array, int offset, quint16 value)
{
    array[offset] = (quint8)(value & 0xFF);
    array[offset + 1] = (quint8)((value >> 8) & 0xFF);
}

void SimpleDiskImage::setLittleIndianLong(QByteArray &array, int offset, quint32 value)
{
    array[offset] = (quint8)(value & 0xFF);
    array[offset + 1] = (quint8)((value >> 8) & 0xFF);
    array[offset + 2] = (quint8)((value >> 16) & 0xFF);
    array[offset + 3] = (quint8)((value >> 24) & 0xFF);
}

void SimpleDiskImage::fillBuffer(char *line, unsigned char *buf, int len, int ofs, bool dumpAscii)
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

void SimpleDiskImage::dumpBuffer(unsigned char *buf, int len)
{
    for (int i = 0; i < ((len + 15) >> 4); i++) {
        char line[80];
        int ofs = i << 4;
		fillBuffer(line, buf, len, ofs, false);
        qDebug() << "!u" << tr("[%1] §%2").arg(deviceName()).arg(line);
    }
}

int SimpleDiskImage::getUploadCodeStartAddress(quint8 command, quint16 aux, QByteArray &)
{
    if (m_board.isChipOpen()) {
        if (command == 0x4D) {
            return 0x0080;
        }
        else if (command == 0x75) {
            return 0x1000;
        }
    }
    if (m_board.isHappyEnabled()) {
        if (((command == 0x57) && (aux >= 0x800) && (aux < 0x1380)) || ((command == 0x77) && (aux >= 0x800) && (aux < 0x1380))) {
            if (m_board.hasHappySignature()) {
                return (int) aux;
            }
        }
        if (((command == 0x50) || (command == 0x57) || (command == 0x70) || (command == 0x77)) && (aux >= 0x8000) && (aux <= 0x97FF)) {
            return (int) aux;
        }
    }
    return -1;
}

void SimpleDiskImage::disassembleCode(QByteArray &data, unsigned short address, bool drive1050, bool happy)
{
    // CRC16 is not a good hash but this is only for very few "upload and execute code" commands so this is OK
    // This CRC is used to implement these commands at SIO level in diskimage.cpp.
    // This is useful to make Fuzzy sectors with Super Archiver when hardware emulation level.
    Crc16 crc;
    crc.Reset();
    int len = data.size();
    for (int i = 0; i < len; i++) {
        crc.Add(data[i]);
    }
    if (crc.GetCrc() == 0xF00A) {
        // empty data
        return;
    }
    qDebug() << "!n" << tr("[%1] Disassembly of %2 bytes at $%3 with CRC $%4")
                .arg(deviceName())
                .arg(len)
                .arg(address, 4, 16, QChar('0'))
                .arg(crc.GetCrc(), 4, 16, QChar('0'));
    char buf[256];
    int offset = 0;
    QByteArray code;
    if (happy) {
        if (address == 0x0800) {
            while (offset < 9) {
                qDebug() << "!u" << tr("[%1] §$%2: %3 %4 %5 ; Happy signature")
                            .arg(deviceName())
                            .arg((address + (unsigned short)offset), 4, 16, QChar('0'))
                            .arg((unsigned char)data[offset], 2, 16, QChar('0'))
                            .arg((unsigned char)data[offset + 1], 2, 16, QChar('0'))
                            .arg((unsigned char)data[offset + 2], 2, 16, QChar('0'));
                offset += 3;
            }
            int command = 0x50;
            while (offset < 32) {
                qDebug() << "!u" << tr("[%1] §$%2: %3 %4 %5 JMP $%6 ; Command $%7")
                            .arg(deviceName())
                            .arg((address + (unsigned short)offset), 4, 16, QChar('0'))
                            .arg((unsigned char)data[offset], 2, 16, QChar('0'))
                            .arg((unsigned char)data[offset + 1], 2, 16, QChar('0'))
                            .arg((unsigned char)data[offset + 2], 2, 16, QChar('0'))
                            .arg(((unsigned char)data[offset + 1] + ((unsigned char)data[offset + 2] << 8)), 4, 16, QChar('0'))
                            .arg(command, 2, 16, QChar('0'));
                offset += 3;
                command++;
            }
        }
        else if ((m_remainingAddress == address) && (m_remainingBytes.size() > 0)) {
            address -= m_remainingBytes.size();
            code.append(m_remainingBytes);
        }
    }
    m_remainingAddress = 0;
    m_remainingBytes.clear();
    code.append(data);
    len = code.size();
    unsigned char *codePtr = (unsigned char *) code.data();
    while (offset < len) {
        int lenOpCode = -1;
        if (drive1050) {
            lenOpCode = m_disassembly1050.BuildInstruction(buf, &codePtr[offset], len - offset, address + (unsigned short)offset);
        }
        else {
            lenOpCode = m_disassembly810.BuildInstruction(buf, &codePtr[offset], len - offset, address + (unsigned short)offset);
        }
        if (lenOpCode == -1) {
            break;
        }
        else {
            qDebug() << "!u" << tr("[%1] §%2")
                        .arg(deviceName())
                        .arg(buf);
        }
        offset += lenOpCode;
    }
    if (offset < len) {
        m_remainingAddress = address + len;
        m_remainingBytes.append(code.right(len - offset));
    }
}
