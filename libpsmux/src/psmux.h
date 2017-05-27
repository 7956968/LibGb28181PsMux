/* MPEG-PS muxer
 * Copyright 2008 Lin YANG <oxcsnicho@gmail.com>
 */

#ifndef __PSMUX_H__
#define __PSMUX_H__

#include "psmuxcommon.h"
#include "psmuxstream.h"

#define PSMUX_MAX_ES_INFO_LENGTH ((1 << 12) - 1)
#define MAX_MUX_STREAM_NUM  (2)


struct PsMux {
    PsMuxStream * streams[MAX_MUX_STREAM_NUM];
    guint nb_streams;
    PsMuxStreamIdInfo id_info; /* carrying the info which ids are used */

    /* timestamps: pts */ 
    gint64 pts;

    guint32 pes_cnt; /* # of pes that has been created */
    guint16 pes_max_payload; /* maximum payload size in pes packets */

    guint64 bit_size;  /* accumulated bit size of processed data */
    guint   bit_rate;  /* bit rate */ 
    gint64  bit_pts; /* last time the bit_rate is updated */

    guint  enable_pack_hdr;
    gint64 pack_hdr_pts; /* last time a pack header is written */

    guint  enable_sys_hdr;
    gint64 sys_hdr_pts; /* last time a system header is written */

    guint  enable_psm;
    gint64 psm_pts; /* last time a psm is written */

    guint8 packet_buf[PSMUX_MAX_PACKET_LEN];

    /* Scratch space for writing ES_info descriptors */
    guint8 es_info_buf[PSMUX_MAX_ES_INFO_LENGTH];

    /* bounds in system header */ 
    guint8 audio_bound;
    guint8 video_bound;
    guint32 rate_bound;
};

#define MUX_ERROR   (1)
#define MEM_ERROR   (2)
#define MUX_WAIT    (3)         //当前帧被缓存,稍后给出数据
#define MUX_OK      (0)

/* create/free new muxer session */
PsMux *		psmux_new 			(void);
void 		psmux_free 			(PsMux *mux);

/* stream management */
PsMuxStream *	psmux_create_stream 		(PsMux *mux, PsMuxStreamType stream_type);


int psmux_mux_frame(PsMux * mux, PsMuxStream * stream, guint8 * rawBuf, 
                    guint rawBuflen, 
                    gint64 pts, 
                    gint64 dts,
                    guint8 * outBuf, 
                    int* OutSize, 
                    int maxOutSize);
#endif
