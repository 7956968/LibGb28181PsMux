#include "../inc/Gb28181PsMux.h"

#include "psmux.h"




//处理具有同一个时间戳的多个帧,比如SPS PPS I帧
struct MuxMultiFrameContext 
{
    MuxMultiFrameContext():Idx(-1),
                            OutBuf(NULL),
                            MaxOutSize(0),
                            OutSize(0),
                            pMux(NULL)
    {
    }
    virtual int MuxOneOfMultiFrame(guint8* buf, int len) = 0;
    StreamIdx Idx;
    guint8* OutBuf;
    int MaxOutSize;
    gint64 pts;
    gint64 dts;
    Gb28181PsMux* pMux;
protected:
    int OutSize;
};

int MuxBlock(unsigned char* buf, int len, int MaxSlice, MuxMultiFrameContext* pContext);

Gb28181PsMux::Gb28181PsMux():m_PsMuxContext(NULL)
{
}

Gb28181PsMux::~Gb28181PsMux()
{
    if (m_PsMuxContext){
        psmux_free(m_PsMuxContext);
    }
}

struct MuxH265VpsSpsPpsIFrameContext : public MuxMultiFrameContext
{
public:
    virtual int MuxOneOfMultiFrame(guint8* buf, int len)
    {
        int ret = 0;
        ret = pMux->MuxH265SingleFrame(buf, len, pts, dts, Idx, OutBuf, &OutSize, MaxOutSize);
        if (ret != MUX_OK)
        {
            return MUX_ERROR;
        }

        OutBuf += OutSize;
        MaxOutSize -= OutSize;
        return MUX_OK;
    }
};

struct MuxH264SPSPPSIFrameContext : public MuxMultiFrameContext
{
public:
    virtual int MuxOneOfMultiFrame(guint8* buf, int len)
    {
        int ret = 0;
        ret = pMux->MuxH264SingleFrame(buf, len, pts, dts, Idx, OutBuf, &OutSize, MaxOutSize);
        if (ret != MUX_OK)
        {
            return MUX_ERROR;
        }
        
        OutBuf += OutSize;
        MaxOutSize -= OutSize;
        return MUX_OK;
    }
};

StreamIdx Gb28181PsMux::AddStream(PsMuxStreamType Type)
{
    if(m_PsMuxContext == NULL){
        m_PsMuxContext = psmux_new();
    }
    PsMuxStream * pStream = psmux_create_stream (m_PsMuxContext, Type);
    m_VecStream.push_back(pStream);
    return (StreamIdx)(m_VecStream.size()-1);
}

int Gb28181PsMux::MuxH265VpsSpsPpsIFrame(guint8* buf, int len, gint64 Pts, gint64 Dts, StreamIdx Idx,
                                         guint8 * outBuf, int* pOutSize, int maxOutSize)
{
    MuxH265VpsSpsPpsIFrameContext context;
    context.Idx = Idx;
    context.pts = Pts;
    context.dts = Dts;
    context.OutBuf = outBuf;
    context.MaxOutSize = maxOutSize;
    context.pMux = this;
    MuxBlock(buf, len, 4, &context);
    if (pOutSize){//计算用去了多少字节
        *pOutSize = context.OutBuf - outBuf;
    }
    return MUX_OK;
}

int Gb28181PsMux::MuxH264SpsPpsIFrame(guint8* buf, int len, gint64 Pts, gint64 Dts, StreamIdx Idx,
                                      guint8 * outBuf, int* pOutSize, int maxOutSize)
{
    MuxH264SPSPPSIFrameContext context;
    context.Idx = Idx;
    context.pts = Pts;
    context.dts = Dts;
    context.OutBuf = outBuf;
    context.MaxOutSize = maxOutSize;
    context.pMux = this;
    MuxBlock(buf, len, 3, &context);
    if (pOutSize){//计算用去了多少字节
        *pOutSize = context.OutBuf - outBuf;
    }
    return MUX_OK;
}

int Gb28181PsMux::MuxH264SingleFrame(guint8* buf, int len, gint64 Pts, gint64 Dts, StreamIdx Idx,
                                     guint8 * outBuf, int* pOutSize, int maxOutSize)
{
    if (Idx >= m_VecStream.size()){
        return MUX_ERROR;
    }
    
    NAL_type Type = getH264NALtype(buf[4]);
    
    if (Type == NAL_other){
        return MUX_ERROR;
    }
    PsMuxStream * pMuxStream = m_VecStream[Idx];

    //default
    m_PsMuxContext->enable_pack_hdr = 0;
    m_PsMuxContext->enable_psm = 0;
    m_PsMuxContext->enable_sys_hdr = 0;
    pMuxStream->pi.flags &= ~PSMUX_PACKET_FLAG_PES_DATA_ALIGN;
    m_PsMuxContext->pts = Pts;

    if (Pts == Dts){
        Dts = INVALID_TS;
    }

    if (Type == NAL_SPS){
        m_PsMuxContext->enable_pack_hdr = 1;
        m_PsMuxContext->enable_psm = 1;
        m_PsMuxContext->enable_sys_hdr = 1;
        Pts = INVALID_TS;
        Dts = INVALID_TS;
    }
    else if (Type == NAL_PPS){
        Pts = INVALID_TS;
        Dts = INVALID_TS;
    }
    else if (Type == NAL_PFRAME){
        m_PsMuxContext->enable_pack_hdr = 1;
        pMuxStream->pi.flags |= PSMUX_PACKET_FLAG_PES_DATA_ALIGN;
    }
    else if (Type == NAL_IDR){
        pMuxStream->pi.flags |= PSMUX_PACKET_FLAG_PES_DATA_ALIGN;
    }

    psmux_mux_frame(m_PsMuxContext, m_VecStream[Idx], buf, len, Pts, Dts, outBuf, pOutSize, maxOutSize);

    return MUX_OK;
}

int Gb28181PsMux::MuxH265SingleFrame(guint8* buf, int len, gint64 Pts, gint64 Dts, StreamIdx Idx,
                       guint8 * outBuf, int* pOutSize, int maxOutSize)
{
    if (Idx >= m_VecStream.size()){
        return MUX_ERROR;
    }

    PsMuxStream * pMuxStream = m_VecStream[Idx];

    NAL_type Type = getH265NALtype(buf[4]);
    if (Type == NAL_other){
        return MUX_ERROR;
    }

    //default
    {
        m_PsMuxContext->enable_pack_hdr = 0;
        m_PsMuxContext->enable_psm = 0;
        m_PsMuxContext->enable_sys_hdr = 0;
    }

    pMuxStream->pi.flags |= PSMUX_PACKET_FLAG_PES_DATA_ALIGN;

    if (Type == NAL_VPS){
        m_PsMuxContext->enable_pack_hdr = 1;
        m_PsMuxContext->enable_psm = 1;
        m_PsMuxContext->enable_sys_hdr = 1;
    }
    else if (Type == NAL_PPS){
        m_PsMuxContext->enable_pack_hdr = 1;
    }
    else if (Type == NAL_PFRAME){
        m_PsMuxContext->enable_pack_hdr = 1;
    }

    psmux_mux_frame(m_PsMuxContext, m_VecStream[Idx], buf, len, Pts, Dts, outBuf, pOutSize, maxOutSize);

    return MUX_OK;
}

int Gb28181PsMux::MuxAudioFrame(guint8* buf, int len, gint64 Pts, gint64 Dts, StreamIdx Idx,
                  guint8 * outBuf, int* pOutSize, int maxOutSize)
{
    if (Idx >= m_VecStream.size()){
        return 1;
    }

    PsMuxStream * pMuxStream = m_VecStream[Idx];

    m_PsMuxContext->enable_pack_hdr = 1;
    m_PsMuxContext->enable_psm = 1;
    m_PsMuxContext->enable_sys_hdr = 0;

    pMuxStream->pi.flags |= PSMUX_PACKET_FLAG_PES_DATA_ALIGN;

    int MuxSize = 0;
    psmux_mux_frame(m_PsMuxContext, m_VecStream[Idx], buf, len, Pts, Dts, outBuf, pOutSize, maxOutSize);
    return MUX_OK;
}

//遍历block拆分NALU,直到MaxSlice,不然一直遍历下去
int MuxBlock(guint8* pBlock, int BlockLen, int MaxSlice, MuxMultiFrameContext* pContext)
{
    guint8* pCurPos = pBlock;
    int LastBlockLen = BlockLen;

    guint8* NaluStartPos = NULL;

    if(pContext == NULL) return MUX_ERROR;

    //一段数据里最多NALU个数,这样SPS PPS 后的I帧那就不用遍历
    int iSliceNum = 0;

    while (LastBlockLen > 4)
    {
        if ((pCurPos[0] == 0) && (pCurPos[1] == 0) && (pCurPos[2] == 0) && (pCurPos[3] == 1)){

            iSliceNum++;
     
            if (NaluStartPos == NULL){
                NaluStartPos = pCurPos;
            }
            else{
                pContext->MuxOneOfMultiFrame(NaluStartPos, pCurPos-NaluStartPos);
                NaluStartPos = pCurPos;
            }

            if (iSliceNum >= MaxSlice){//已经到达最大NALU个数,下面的不用找了把剩下的加上就是
                pContext->MuxOneOfMultiFrame(pCurPos, LastBlockLen);
                break;
            }
        }
        
        pCurPos++;
        LastBlockLen--;
    }

    return MUX_OK;
}

NAL_type getH264NALtype(unsigned char c)
{
    switch(c & 0x1f){
        case 6:
            return NAL_SEI;
            break;
        case 7:
            return NAL_SPS;
            break;
        case 8:
            return NAL_PPS;
            break;
        case 5:
            return NAL_IDR;
            break;
        case 1:
            return NAL_PFRAME;
            break;
        default:
            return NAL_other;
            break;
    }
    return NAL_other;
}

NAL_type getH265NALtype(unsigned char c)
{
    int type = (c & 0x7E)>>1;

    if(type == 33)
        return NAL_SPS;

    if(type == 34)
        return NAL_PPS;

    if(type == 32)
        return NAL_VPS;

    if(type == 39)
        return NAL_SEI_PREFIX;

    if(type == 40)
        return NAL_SEI_SUFFIX;

    if((type >= 1) && (type <=9))
        return NAL_PFRAME;

    if((type >= 16) && (type <=21))
        return NAL_IDR;

    return NAL_other;
}
