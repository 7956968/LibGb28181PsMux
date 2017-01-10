/* MPEG-PS muxer plugin for GStreamer
 * Copyright 2008 Lin YANG <oxcsnicho@gmail.com>
 */




#ifndef __PSMUXSTREAM_H__
#define __PSMUXSTREAM_H__

#include "psmuxcommon.h"

typedef void (*PsMuxStreamBufferReleaseFunc) (guint8 *data, void *user_data);

struct PsMuxStreamBuffer
{
  guint8 *data;
  guint32 size;

  /* PTS & DTS associated with the contents of this buffer */
  gint64 pts;
  gint64 dts;

  void *user_data;
};

/* PsMuxStream receives elementary streams for parsing.
 * Via the write_bytes() method, it can output a PES stream piecemeal */
struct PsMuxStream{
  PsMuxPacketInfo pi;

  PsMuxStreamType stream_type;
  guint8 stream_id;
  guint8 stream_id_ext; /* extended stream id (13818-1 Amdt 2) */

  /* Current data buffer being consumed */
  PsMuxStreamBuffer *cur_buffer;
  guint32 cur_buffer_consumed;

  /* PES payload */
  guint16 cur_pes_payload_size;
  guint16 pes_bytes_written; /* delete*/

  /* Release function */
  PsMuxStreamBufferReleaseFunc buffer_release;

  /* PTS/DTS to write if the flags in the packet info are set */
  gint64 pts; /* TODO: cur_buffer->pts?*/
  gint64 dts; /* TODO: cur_buffer->dts?*/
  gint64 last_pts;

  /* stream type */
  gboolean is_video_stream;
  gboolean is_audio_stream;

  /* for writing descriptors */
  gint audio_sampling;
  gint audio_channels;
  gint audio_bitrate;

  /* for writing buffer size in system header */
  guint max_buffer_size;
};

/* stream management */
PsMuxStream*    psmux_stream_new                (PsMux * mux, PsMuxStreamType stream_type);
void 		psmux_stream_free 		(PsMuxStream *stream);

int
psmux_stream_mux_frame (PsMuxStream * stream, guint8 * rawBuf, guint rawBuflen, gint64 pts, gint64 dts,
                        guint8 * outBuf, int* pOutSize, int maxOutSize);

/* write corresponding descriptors of the stream */
void 		psmux_stream_get_es_descrs 	(PsMuxStream *stream, guint8 *buf, guint16 *len);

/* get the pts of stream */
guint64 	psmux_stream_get_pts 		(PsMuxStream *stream);

/* stream_id assignemnt */
#define PSMUX_STREAM_ID_MPGA_INIT       0xc0
#define PSMUX_STREAM_ID_MPGA_MAX        0xcf

#define PSMUX_STREAM_ID_MPGV_INIT       0xe0
#define PSMUX_STREAM_ID_MPGV_MAX        0xef

#define PSMUX_STREAM_ID_AC3_INIT        0x80
#define PSMUX_STREAM_ID_AC3_MAX         0x87

#define PSMUX_STREAM_ID_SPU_INIT        0x20
#define PSMUX_STREAM_ID_SPU_MAX        	0x3f

#define PSMUX_STREAM_ID_DTS_INIT        0x88
#define PSMUX_STREAM_ID_DTS_MAX         0x8f

#define PSMUX_STREAM_ID_LPCM_INIT       0xa0
#define PSMUX_STREAM_ID_LPCM_MAX        0xaf

#define PSMUX_STREAM_ID_DIRAC_INIT      0x60
#define PSMUX_STREAM_ID_DIRAC_MAX       0x6f

struct PsMuxStreamIdInfo {
    guint8 id_mpga;
    guint8 id_mpgv;
    guint8 id_ac3;
    guint8 id_spu;
    guint8 id_dts;
    guint8 id_lpcm;
    guint8 id_dirac;
};

static inline void
psmux_stream_id_info_init (PsMuxStreamIdInfo * info)
{
    if(info == NULL) return ;
    info->id_mpga = PSMUX_STREAM_ID_MPGA_INIT;
    info->id_mpgv = PSMUX_STREAM_ID_MPGV_INIT;
    info->id_ac3  = PSMUX_STREAM_ID_AC3_INIT;
    info->id_spu  = PSMUX_STREAM_ID_SPU_INIT;
    info->id_dts  = PSMUX_STREAM_ID_DTS_INIT;
    info->id_lpcm = PSMUX_STREAM_ID_LPCM_INIT;
    info->id_dirac= PSMUX_STREAM_ID_DIRAC_INIT;
}

#endif
