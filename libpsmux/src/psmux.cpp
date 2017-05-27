/* MPEG-PS muxer
 * Copyright 2008 Lin YANG <oxcsnicho@gmail.com>
 */

#include <string.h>
#include <stdio.h>

#include "psmuxcommon.h"
#include "psmuxstream.h"
#include "psmux.h"
#include "crc.h"

int psmux_write_pack_header (PsMux * mux, guint8 * outBuf, int* pOutSize, int maxOutSize);
int psmux_write_system_header (PsMux * mux, guint8 * outBuf, int* pOutSize, int maxOutSize);
int psmux_write_program_stream_map (PsMux * mux, guint8 * outBuf, int* pOutSize, int maxOutSize);

/**
 * psmux_new:
 *
 * Create a new muxer session.
 *
 * Returns: A new #PsMux object.
 */
PsMux *
psmux_new ()
{
  PsMux *mux;

  mux = new PsMux;

  mux->pts = INVALID_TS;                /* uninitialized values */
  mux->pack_hdr_pts = INVALID_TS;
  mux->sys_hdr_pts = INVALID_TS;
  mux->psm_pts = INVALID_TS;

  mux->bit_pts = 0;

  mux->pes_max_payload = PSMUX_PES_MAX_PAYLOAD;
  mux->bit_rate = 400 * 1024;   /* XXX: better default values? */
  mux->rate_bound = 2 * 1024;   /* 2* bit_rate / (8*50). XXX: any better default? */
  
  mux->video_bound = 0;
  mux->audio_bound = 0;
  mux->rate_bound = 0;

  mux->nb_streams = 0;

  psmux_stream_id_info_init (&mux->id_info);

  return mux;
}

/**
 * psmux_free:
 * @mux: a #PsMux
 *
 * Free all resources associated with @mux. After calling this function @mux can
 * not be used anymore.
 */
void
psmux_free (PsMux * mux)
{
  if (mux == NULL) return;

  /* Free all streams */
  for (unsigned int i = 0; i < mux->nb_streams; i++){
      psmux_stream_free (mux->streams[i]);
  }

  delete mux;
}

/**
 * psmux_create_stream:
 * @mux: a #PsMux
 * @stream_type: a #PsMuxStreamType
 *
 * Create a new stream of @stream_type in the muxer session @mux.
 *
 * Returns: a new #PsMuxStream.
 */
PsMuxStream *
psmux_create_stream (PsMux * mux, PsMuxStreamType stream_type)
{
  if(mux == NULL)     return NULL;
  if(mux->nb_streams >= MAX_MUX_STREAM_NUM) return NULL;

  PsMuxStream *stream = NULL;

  stream = psmux_stream_new (mux, stream_type);

  mux->streams[mux->nb_streams++] = stream;

  if (stream->is_video_stream) {
    mux->video_bound++;
    if (mux->video_bound > 32)
      g_critical ("Number of video es exceeds upper limit");
  } else if (stream->is_audio_stream) {
    mux->audio_bound++;
    if (mux->audio_bound > 64)
      g_critical ("Number of audio es exceeds upper limit");
  }

  return stream;
}

/**
 * psmux_write_stream_packet:
 * @mux: a #PsMux
 * @stream: a #PsMuxStream
 *
 * Write a packet of @stream.
 *
 * Returns: TRUE if the packet could be written.
 */

int
psmux_mux_frame(PsMux * mux, PsMuxStream * stream, guint8 * rawBuf, 
                                                    guint rawBuflen, 
                                                    gint64 pts, 
                                                    gint64 dts,
                                                    guint8 * pOutBuf, 
                                                    int* pOutSize, 
                                                    int maxOutSize)
{
    if(mux    == NULL)   return MUX_ERROR;
    if(stream == NULL)   return MUX_ERROR;
    if(rawBuf == NULL)   return MUX_ERROR;
    if(rawBuflen == 0)   return MUX_ERROR;
    if(pOutBuf == NULL)  return MUX_ERROR;
    if(maxOutSize == 0)  return MUX_ERROR;

    {
        gint64 ts = psmux_stream_get_pts (stream);
        if (ts != INVALID_TS)
            mux->pts = ts;
    }

    int muxState = MUX_OK;

    *pOutSize = 0;

    if (mux->enable_pack_hdr) {

        if (mux->pts != INVALID_TS && mux->pts > mux->bit_pts
            && mux->pts - mux->bit_pts > PSMUX_BITRATE_CALC_INTERVAL) {
                /* XXX: smoothing the rate? */
                mux->bit_rate = (mux->bit_size * 8 * CLOCKBASE) / (mux->pts - mux->bit_pts);
                mux->bit_size = 0;
                mux->bit_pts = mux->pts;
        }
        
        int muxHdrSize = 0;
        int muxState = psmux_write_pack_header (mux, pOutBuf, &muxHdrSize, maxOutSize);
        if(muxState != MUX_OK) return muxState;
        
        maxOutSize -= muxHdrSize;
        *pOutSize  += muxHdrSize;
        pOutBuf    += muxHdrSize;
        mux->pack_hdr_pts = mux->pts;
    }

    if (mux->enable_sys_hdr) {
        int muxSysHdrSize = 0;
        int muxState = psmux_write_system_header (mux, pOutBuf, &muxSysHdrSize, maxOutSize);
        if(muxState != MUX_OK) return muxState;

        maxOutSize -= muxSysHdrSize;
        *pOutSize  += muxSysHdrSize;
        pOutBuf    += muxSysHdrSize;
        mux->sys_hdr_pts = mux->pts;
    }

    if (mux->enable_psm) {
        int muxPsmSize = 0;
        int muxState = psmux_write_program_stream_map (mux, pOutBuf, &muxPsmSize, maxOutSize);
        if(muxState != MUX_OK) return muxState;

        maxOutSize -= muxPsmSize;
        *pOutSize  += muxPsmSize;
        pOutBuf    += muxPsmSize;
        mux->psm_pts = mux->pts;
    }

    int PesSize = 0;
    if (psmux_stream_mux_frame (stream, rawBuf, rawBuflen, pts, dts, pOutBuf, &PesSize, maxOutSize) != MUX_OK) {
        return MUX_ERROR;
    }

    *pOutSize  += PesSize;

    mux->pes_cnt += 1;

    return muxState;
}

int psmux_write_pack_header (PsMux * mux, guint8 * outBuf, int* pOutSize, int maxOutSize)
{
  bits_buffer_t bw;
  guint64 scr = mux->pts;       /* XXX: is this correct? necessary to put any offset? */
  if (mux->pts == -1)
    scr = 0;

  /* pack_start_code */
  bits_initwrite (&bw, 14, mux->packet_buf);
  bits_write (&bw, 24, PSMUX_START_CODE_PREFIX);
  bits_write (&bw, 8, PSMUX_PACK_HEADER);

  /* scr */
  bits_write (&bw, 2, 0x1);
  bits_write (&bw, 3, (scr >> 30) & 0x07);
  bits_write (&bw, 1, 1);
  bits_write (&bw, 15, (scr >> 15) & 0x7fff);
  bits_write (&bw, 1, 1);
  bits_write (&bw, 15, scr & 0x7fff);
  bits_write (&bw, 1, 1);
  bits_write (&bw, 9, 0);       /* system_clock_reference_extension: set to 0 (like what VLC does) */
  bits_write (&bw, 1, 1);

  {
    /* Scale to get the mux_rate, rounding up */
    guint mux_rate = (mux->bit_rate + 8 * 50 - 1) / (8 * 50);
        //gst_util_uint64_scale (mux->bit_rate + 8 * 50 - 1, 1, 8 * 50);
    if (mux_rate > mux->rate_bound / 2)
      mux->rate_bound = mux_rate * 2;
    bits_write (&bw, 22, mux_rate);     /* program_mux_rate */
    bits_write (&bw, 2, 3);
  }

  bits_write (&bw, 5, 0x1f);
  bits_write (&bw, 3, 0);       /* pack_stuffing_length */

  if (maxOutSize < 14) return MEM_ERROR;

  memcpy(outBuf, bw.p_data, 14);
  
  if(pOutSize) *pOutSize = 14;

  return MUX_OK;
}

int
psmux_write_system_header (PsMux * mux, guint8 * outBuf, int* pOutSize, int maxOutSize)
{
  bits_buffer_t bw;
  int len = 12 + mux->nb_streams * 3;

  /* system_header_start_code */
  bits_initwrite (&bw, len, mux->packet_buf);

  /* system_header start code */
  bits_write (&bw, 24, PSMUX_START_CODE_PREFIX);
  bits_write (&bw, 8,  PSMUX_SYSTEM_HEADER);

  bits_write (&bw, 16, len - 6);    /* header_length */
  bits_write (&bw, 1, 1);       /* marker */
  bits_write (&bw, 22, mux->rate_bound);        /* rate_bound */
  bits_write (&bw, 1, 1);       /* marker */
  bits_write (&bw, 6, mux->audio_bound);        /* audio_bound */
  bits_write (&bw, 1, 0);       /* fixed_flag */
  bits_write (&bw, 1, 0);       /* CSPS_flag */
  bits_write (&bw, 1, 0);       /* system_audio_lock_flag */
  bits_write (&bw, 1, 0);       /* system_video_lock_flag */
  bits_write (&bw, 1, 1);       /* marker */
  bits_write (&bw, 5, mux->video_bound);        /* video_bound */
  bits_write (&bw, 1, 0);       /* packet_rate_restriction_flag */
  bits_write (&bw, 7, 0x7f);    /* reserved_bits */

  for (guint i = 0; i < mux->nb_streams; i++){
      PsMuxStream *stream = mux->streams[i];

      bits_write (&bw, 8, stream->stream_id);     /* stream_id */
      bits_write (&bw, 2, 0x3);   /* reserved */
      bits_write (&bw, 1, stream->is_video_stream);       /* buffer_bound_scale */
      bits_write (&bw, 13, stream->max_buffer_size / (stream->is_video_stream ? 1024 : 128));     /* buffer_size_bound */
  }

  if (maxOutSize < len) return MEM_ERROR;

  memcpy(outBuf, bw.p_data, len);

  if(pOutSize) *pOutSize = len;

  return MUX_OK;
}

int
psmux_write_program_stream_map (PsMux * mux, guint8 * outBuf, int* pOutSize, int maxOutSize)
{
  gint psm_size = 16, es_map_size = 0;
  bits_buffer_t bw;
  guint16 len;
  guint8 *pos;

  /* pre-write the descriptor loop */
  pos = mux->es_info_buf;

  for (guint i = 0; i < mux->nb_streams; i++){
      PsMuxStream *stream = mux->streams[i];
      len = 0;

      *pos++ = stream->stream_type;
      *pos++ = stream->stream_id;

      psmux_stream_get_es_descrs (stream, pos + 2, &len);
      psmux_put16 (&pos, len);

      es_map_size += len + 4;
      pos += len;
  }

  psm_size += es_map_size;
  bits_initwrite (&bw, psm_size, mux->packet_buf);

  /* psm start code */
  bits_write (&bw, 24, PSMUX_START_CODE_PREFIX);
  bits_write (&bw, 8, PSMUX_PROGRAM_STREAM_MAP);

  bits_write (&bw, 16, psm_size - 6);   /* psm_length */
  bits_write (&bw, 1, 1);       /* current_next_indicator */
  bits_write (&bw, 2, 0xF);     /* reserved */
  bits_write (&bw, 5, 0x1);     /* psm_version = 1 */
  bits_write (&bw, 7, 0xFF);    /* reserved */
  bits_write (&bw, 1, 1);       /* marker */

  bits_write (&bw, 16, 0);      /* program_stream_info_length */
  /* program_stream_info empty */

  bits_write (&bw, 16, es_map_size);    /* elementary_stream_map_length */
  memcpy (bw.p_data + bw.i_data, mux->es_info_buf, es_map_size);

  /* CRC32 */
  {
    guint32 crc = calc_crc32 (mux->packet_buf, psm_size - 4);
    guint8 *pos = mux->packet_buf + psm_size - 4;
    psmux_put32 (&pos, crc);
  }

  if (maxOutSize < psm_size) return MEM_ERROR;

  memcpy(outBuf, bw.p_data, psm_size);

  if(pOutSize) *pOutSize = psm_size;

  return MUX_OK;
}
