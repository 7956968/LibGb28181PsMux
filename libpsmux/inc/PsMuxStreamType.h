#pragma once

enum PsMuxStreamType { /* Table 2-29 in spec */
    PSMUX_ST_RESERVED                   = 0x00,
    PSMUX_ST_VIDEO_MPEG1                = 0x01,
    PSMUX_ST_VIDEO_MPEG2                = 0x02,
    PSMUX_ST_AUDIO_MPEG1                = 0x03,
    PSMUX_ST_AUDIO_MPEG2                = 0x04,
    PSMUX_ST_PRIVATE_SECTIONS           = 0x05,
    PSMUX_ST_PRIVATE_DATA               = 0x06,
    PSMUX_ST_MHEG                       = 0x07,
    PSMUX_ST_DSMCC                      = 0x08,
    PSMUX_ST_H222_1                     = 0x09,

    /* later extensions */
    PSMUX_ST_AUDIO_AAC                  = 0x0f,
    PSMUX_ST_VIDEO_MPEG4                = 0x10,
    PSMUX_ST_VIDEO_H264                 = 0x1b,

    /* private stream types */
    PSMUX_ST_PS_AUDIO_AC3               = 0x81,
    PSMUX_ST_PS_AUDIO_DTS               = 0x8a,
    PSMUX_ST_PS_AUDIO_LPCM              = 0x8b,
    PSMUX_ST_PS_AUDIO_G711A             = 0x90,
    PSMUX_ST_PS_AUDIO_G711U             = 0x91,
    PSMUX_ST_PS_AUDIO_G722_1            = 0x92,
    PSMUX_ST_PS_AUDIO_G723_1            = 0x93,
    PSMUX_ST_PS_AUDIO_G729              = 0x99,
    PSMUX_ST_PS_AUDIO_SVAC              = 0x9b,
    PSMUX_ST_PS_DVD_SUBPICTURE          = 0xff,

    /* Non-standard definitions */
    PSMUX_ST_VIDEO_DIRAC                = 0xD1
};
