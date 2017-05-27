

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "psmuxcommon.h"
#include "psmuxstream.h"
#include "psmux.h"

static guint8 psmux_stream_pes_header_length (PsMuxStream * stream);
static void psmux_stream_write_pes_header (PsMuxStream * stream, guint8 * data);

#define min(a,b)    (((a) < (b)) ? (a) : (b))

/**
 * psmux_stream_new:
 * @pid: a PID
 * @stream_type: the stream type
 *
 * Create a new stream with type = @stream_type, assign stream id accordingly
 *
 * Returns: a new #PsMuxStream.
 */
PsMuxStream *
psmux_stream_new (PsMux * mux, PsMuxStreamType stream_type)
{
  PsMuxStream *stream = new PsMuxStream;
  PsMuxStreamIdInfo *info = &(mux->id_info);

  stream->stream_type = stream_type;
  stream->is_audio_stream = 0;
  stream->is_video_stream = 0;
  stream->stream_id = 0;
  stream->max_buffer_size = 0;

  switch (stream_type) {
      /* MPEG AUDIO */
    case PSMUX_ST_PS_AUDIO_G711A:
    case PSMUX_ST_AUDIO_MPEG1:
    case PSMUX_ST_AUDIO_MPEG2:
      stream->max_buffer_size = 2484;   /* ISO/IEC 13818 2.5.2.4 */
    case PSMUX_ST_AUDIO_AAC:
      if (info->id_mpga > PSMUX_STREAM_ID_MPGA_MAX)
        break;
      stream->stream_id = info->id_mpga++;
      stream->stream_id_ext = 0;
      stream->is_audio_stream = 1;
      break;
      /* MPEG VIDEO */
    case PSMUX_ST_VIDEO_MPEG1:
    case PSMUX_ST_VIDEO_MPEG2:
    case PSMUX_ST_VIDEO_MPEG4:
    case PSMUX_ST_VIDEO_H264:
      if (info->id_mpgv > PSMUX_STREAM_ID_MPGV_MAX)
        break;
      stream->stream_id = info->id_mpgv++;
      stream->stream_id_ext = 0;
      stream->is_video_stream = 1;
      break;
      /* AC3 / A52 */
    case PSMUX_ST_PS_AUDIO_AC3:
      if (info->id_ac3 > PSMUX_STREAM_ID_AC3_MAX)
        break;
      stream->stream_id = PSMUX_PRIVATE_STREAM_1;
      stream->stream_id_ext = info->id_ac3++;
      stream->is_audio_stream = 1;
      /* AC3 requires data alignment */
      stream->pi.flags |= PSMUX_PACKET_FLAG_PES_DATA_ALIGN;
      break;
      /* TODO: SPU missing */
#if 0
    case spu:
      if (info->id_spu > PSMUX_STREAM_ID_SPU_MAX)
        break;
      return info->id_spu++;
#endif
      /* DTS */
    case PSMUX_ST_PS_AUDIO_DTS:
      if (info->id_dts > PSMUX_STREAM_ID_DTS_MAX)
        break;
      stream->stream_id = PSMUX_PRIVATE_STREAM_1;
      stream->stream_id_ext = info->id_dts++;
      stream->is_audio_stream = 1;
      break;
      /* LPCM */
    case PSMUX_ST_PS_AUDIO_LPCM:
      if (info->id_lpcm > PSMUX_STREAM_ID_LPCM_MAX)
        break;
      stream->stream_id = PSMUX_PRIVATE_STREAM_1;
      stream->stream_id_ext = info->id_lpcm++;
      stream->is_audio_stream = 1;
      break;
    case PSMUX_ST_VIDEO_DIRAC:
      if (info->id_dirac > PSMUX_STREAM_ID_DIRAC_MAX)
        break;
      stream->stream_id = PSMUX_EXTENDED_STREAM;
      stream->stream_id_ext = info->id_dirac++;
      stream->is_video_stream = 1;
      break;
    default:
      g_critical ("Stream type 0x%0x not yet implemented", stream_type);
      break;
  }

  if (stream->stream_id == 0) {
    g_critical ("Number of elementary streams of type %04x exceeds maximum",
        stream->stream_type);
    return NULL;
  }

  /* XXX: Are private streams also using stream_id_ext? */
  if (stream->stream_id == PSMUX_EXTENDED_STREAM)
    stream->pi.flags |= PSMUX_PACKET_FLAG_PES_EXT_STREAMID;

  /* Are these useful at all? */
  if (stream->stream_id == PSMUX_PROGRAM_STREAM_MAP ||
      stream->stream_id == PSMUX_PADDING_STREAM ||
      stream->stream_id == PSMUX_PRIVATE_STREAM_2 ||
      stream->stream_id == PSMUX_ECM ||
      stream->stream_id == PSMUX_EMM ||
      stream->stream_id == PSMUX_PROGRAM_STREAM_DIRECTORY ||
      stream->stream_id == PSMUX_DSMCC_STREAM ||
      stream->stream_id == PSMUX_ITU_T_H222_1_TYPE_E)
    stream->pi.flags &= ~PSMUX_PACKET_FLAG_PES_FULL_HEADER;
  else
    stream->pi.flags |= PSMUX_PACKET_FLAG_PES_FULL_HEADER;

  stream->cur_buffer = NULL;
  stream->cur_buffer_consumed = 0;

  stream->cur_pes_payload_size = 0;

  stream->buffer_release = NULL;

  stream->pts = INVALID_TS;
  stream->dts = INVALID_TS;
  stream->last_pts =INVALID_TS;

  /* These fields are set by gstreamer */
  stream->audio_sampling = 0;
  stream->audio_channels = 0;
  stream->audio_bitrate = 0;

  if (stream->max_buffer_size == 0) {
    /* XXX: VLC'S VALUE. Better default? */
    if (stream->is_video_stream)
      stream->max_buffer_size = 400 * 1024;
    else if (stream->is_audio_stream)
      stream->max_buffer_size = 4 * 1024;
    else                        /* Unknown */
      stream->max_buffer_size = 4 * 1024;
  }

  return stream;
}

/**
 * psmux_stream_free:
 * @stream: a #PsMuxStream
 *
 * Free the resources of @stream.
 */
void
psmux_stream_free (PsMuxStream * stream)
{
  if(stream == NULL) return;

  delete stream;
}

int
psmux_stream_mux_frame (PsMuxStream * stream, 
                        guint8 * rawBuf, guint rawBuflen, gint64 pts, gint64 dts,
                                        guint8 * pOutBuf, 
                                        int* pOutSize, 
                                        int maxOutSize)
{
  guint8 pes_hdr_length;
   
  if(stream == NULL)                     return MUX_ERROR;
  if(rawBuf == NULL)                     return MUX_ERROR;
  if(maxOutSize < PSMUX_PES_MAX_HDR_LEN) return MUX_ERROR;

  stream->pts = pts;
  stream->dts = dts;

  *pOutSize = 0;

  /* clear pts/dts flag */
  stream->pi.flags &= ~(PSMUX_PACKET_FLAG_PES_WRITE_PTS_DTS |
      PSMUX_PACKET_FLAG_PES_WRITE_PTS);
  /* update pts/dts flag */
  if (stream->pts != INVALID_TS && stream->dts != INVALID_TS)
    stream->pi.flags |= PSMUX_PACKET_FLAG_PES_WRITE_PTS_DTS;
  else {
    if (stream->pts != -1)
      stream->pi.flags |= PSMUX_PACKET_FLAG_PES_WRITE_PTS;
  }

  pes_hdr_length = psmux_stream_pes_header_length (stream);

  int len = rawBuflen;
  guint8 *cur = rawBuf;

  while (len > 0) {

      stream->cur_pes_payload_size =
          min(len, PSMUX_PES_MAX_PAYLOAD - pes_hdr_length);
      
      if(maxOutSize < pes_hdr_length){
          return MEM_ERROR;
      }
      
      psmux_stream_write_pes_header (stream, pOutBuf);
      maxOutSize -= pes_hdr_length;
      pOutBuf += pes_hdr_length;

      if(maxOutSize < stream->cur_pes_payload_size){
          return MEM_ERROR;
      }

      memcpy (pOutBuf, cur, stream->cur_pes_payload_size);

      cur += stream->cur_pes_payload_size;
      len -= stream->cur_pes_payload_size;     /* number of bytes of payload to write */
      pOutBuf += stream->cur_pes_payload_size;
      maxOutSize -= pes_hdr_length;

      *pOutSize += (pes_hdr_length + stream->cur_pes_payload_size);
  }

  return MUX_OK;
}

static guint8
psmux_stream_pes_header_length (PsMuxStream * stream)
{
  guint8 packet_len;

  /* Calculate the length of the header for this stream */

  /* start_code prefix + stream_id + pes_packet_length = 6 bytes */
  packet_len = 6;

  if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_FULL_HEADER) {
    /* For a PES 'full header' we have at least 3 more bytes, 
     * and then more based on flags */
    packet_len += 3;
    if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_WRITE_PTS_DTS) {
      packet_len += 10;
    } else if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_WRITE_PTS) {
      packet_len += 5;
    }
    if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_EXT_STREAMID) {
      /* Need basic extension flags (1 byte), plus 2 more bytes for the 
       * length + extended stream id */
      packet_len += 3;
    }
  }

  return packet_len;
}
static void
psmux_stream_write_pes_header (PsMuxStream * stream, guint8 * data)
{
  guint16 length_to_write;
  guint8 hdr_len = psmux_stream_pes_header_length (stream);

  /* start_code prefix + stream_id + pes_packet_length = 6 bytes */
  data[0] = 0x00;
  data[1] = 0x00;
  data[2] = 0x01;
  data[3] = stream->stream_id;
  data += 4;

  length_to_write = hdr_len - 6 + stream->cur_pes_payload_size;

  psmux_put16 (&data, length_to_write);

  if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_FULL_HEADER) {
    guint8 flags = 0;

    /* Not scrambled, original, not-copyrighted, data_alignment specified by flag */
    flags = 0x88;
    if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_DATA_ALIGN)
      flags |= 0x04;            /* Enable data_alignment_indicator */
    *data++ = flags;

    /* Flags */
    flags = 0;
    if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_WRITE_PTS_DTS)
      flags |= 0xC0;
    else if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_WRITE_PTS)
      flags |= 0x80;
    if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_EXT_STREAMID)
      flags |= 0x01;            /* Enable PES_extension_flag */
    *data++ = flags;

    /* Header length is the total pes length, 
     * minus the 9 bytes of start codes, flags + hdr_len */
    if(hdr_len < 9) return;
    *data++ = (hdr_len - 9);

    if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_WRITE_PTS_DTS) {
      psmux_put_ts (&data, 0x3, stream->pts);
      psmux_put_ts (&data, 0x1, stream->dts);
    } else if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_WRITE_PTS) {
      psmux_put_ts (&data, 0x2, stream->pts);
    }

    if (stream->pi.flags & PSMUX_PACKET_FLAG_PES_EXT_STREAMID) {
      guint8 ext_len;

      flags = 0x0f;             /* preceeding flags all 0 | (reserved bits) | PES_extension_flag_2 */
      *data++ = flags;

      ext_len = 1;              /* Only writing 1 byte into the extended fields */
      *data++ = 0x80 | ext_len; /* marker | PES_extension_field_length */
      *data++ = 0x80 | stream->stream_id_ext;   /* stream_id_extension_flag | extended_stream_id */
    }
  }
}


/**
 * psmux_stream_get_es_descrs:
 * @stream: a #PsMuxStream
 * @buf: a buffer to hold the ES descriptor
 * @len: the length used in @buf
 *
 * Write an Elementary Stream Descriptor for @stream into @buf. the number of
 * bytes consumed in @buf will be updated in @len.
 *
 * @buf and @len must be at least #PSMUX_MIN_ES_DESC_LEN.
 */
void
psmux_stream_get_es_descrs (PsMuxStream * stream, guint8 * buf, guint16 * len)
{
  guint8 *pos;

  if(stream == NULL) return;

  if (buf == NULL) {
    if (len != NULL)
      *len = 0;
    return;
  }

  /* Based on the stream type, write out any descriptors to go in the 
   * PMT ES_info field */
  pos = buf;

  /* tag (registration_descriptor), length, format_identifier */
  switch (stream->stream_type) {
    case PSMUX_ST_AUDIO_AAC:
      /* FIXME */
      break;
    case PSMUX_ST_VIDEO_MPEG4:
      /* FIXME */
      break;
    case PSMUX_ST_VIDEO_H264:
      *pos++ = 0x05;
      *pos++ = 8;
      *pos++ = 0x48;            /* 'H' */
      *pos++ = 0x44;            /* 'D' */
      *pos++ = 0x4D;            /* 'M' */
      *pos++ = 0x56;            /* 'V' */
      /* FIXME : Not sure about this additional_identification_info */
      *pos++ = 0xFF;
      *pos++ = 0x1B;
      *pos++ = 0x44;
      *pos++ = 0x3F;
      break;
    case PSMUX_ST_VIDEO_DIRAC:
      *pos++ = 0x05;
      *pos++ = 4;
      *pos++ = 0x64;            /* 'd' */
      *pos++ = 0x72;            /* 'r' */
      *pos++ = 0x61;            /* 'a' */
      *pos++ = 0x63;            /* 'c' */
      break;
    case PSMUX_ST_PS_AUDIO_AC3:
    {
      *pos++ = 0x05;
      *pos++ = 4;
      *pos++ = 0x41;            /* 'A' */
      *pos++ = 0x43;            /* 'C' */
      *pos++ = 0x2D;            /* '-' */
      *pos++ = 0x33;            /* '3' */

      /* audio_stream_descriptor () | ATSC A/52-2001 Annex A
       *
       * descriptor_tag       8 uimsbf
       * descriptor_length    8 uimsbf
       * sample_rate_code     3 bslbf
       * bsid                 5 bslbf
       * bit_rate_code        6 bslbf
       * surround_mode        2 bslbf
       * bsmod                3 bslbf
       * num_channels         4 bslbf
       * full_svc             1 bslbf
       * langcod              8 bslbf
       * [...]
       */
      *pos++ = 0x81;
      *pos++ = 0x04;

      /* 3 bits sample_rate_code, 5 bits hardcoded bsid (default ver 8) */
      switch (stream->audio_sampling) {
        case 48000:
          *pos++ = 0x08;
          break;
        case 44100:
          *pos++ = 0x28;
          break;
        case 32000:
          *pos++ = 0x48;
          break;
        default:
          *pos++ = 0xE8;
          break;                /* 48, 44.1 or 32 Khz */
      }

      /* 1 bit bit_rate_limit, 5 bits bit_rate_code, 2 bits suround_mode */
      switch (stream->audio_bitrate) {
        case 32:
          *pos++ = 0x00 << 2;
          break;
        case 40:
          *pos++ = 0x01 << 2;
          break;
        case 48:
          *pos++ = 0x02 << 2;
          break;
        case 56:
          *pos++ = 0x03 << 2;
          break;
        case 64:
          *pos++ = 0x04 << 2;
          break;
        case 80:
          *pos++ = 0x05 << 2;
          break;
        case 96:
          *pos++ = 0x06 << 2;
          break;
        case 112:
          *pos++ = 0x07 << 2;
          break;
        case 128:
          *pos++ = 0x08 << 2;
          break;
        case 160:
          *pos++ = 0x09 << 2;
          break;
        case 192:
          *pos++ = 0x0A << 2;
          break;
        case 224:
          *pos++ = 0x0B << 2;
          break;
        case 256:
          *pos++ = 0x0C << 2;
          break;
        case 320:
          *pos++ = 0x0D << 2;
          break;
        case 384:
          *pos++ = 0x0E << 2;
          break;
        case 448:
          *pos++ = 0x0F << 2;
          break;
        case 512:
          *pos++ = 0x10 << 2;
          break;
        case 576:
          *pos++ = 0x11 << 2;
          break;
        case 640:
          *pos++ = 0x12 << 2;
          break;
        default:
          *pos++ = 0x32 << 2;
          break;                /* 640 Kb/s upper limit */
      }

      /* 3 bits bsmod, 4 bits num_channels, 1 bit full_svc */
      switch (stream->audio_channels) {
        case 1:
          *pos++ = 0x01 << 1;
          break;                /* 1/0 */
        case 2:
          *pos++ = 0x02 << 1;
          break;                /* 2/0 */
        case 3:
          *pos++ = 0x0A << 1;
          break;                /* <= 3 */
        case 4:
          *pos++ = 0x0B << 1;
          break;                /* <= 4 */
        case 5:
          *pos++ = 0x0C << 1;
          break;                /* <= 5 */
        case 6:
        default:
          *pos++ = 0x0D << 1;
          break;                /* <= 6 */
      }

      *pos++ = 0x00;

      break;
    }
    case PSMUX_ST_PS_AUDIO_DTS:
      /* FIXME */
      break;
    case PSMUX_ST_PS_AUDIO_LPCM:
      /* FIXME */
      break;
    default:
      break;
  }

  if (len)
    *len = (pos - buf);
}

/**
 * psmux_stream_get_pts:
 * @stream: a #PsMuxStream
 *
 * Return the PTS of the last buffer that has had bytes written and
 * which _had_ a PTS in @stream.
 *
 * Returns: the PTS of the last buffer in @stream.
 */
guint64
psmux_stream_get_pts (PsMuxStream * stream)
{
  if(stream == NULL) return -1;

  return stream->last_pts;
}
