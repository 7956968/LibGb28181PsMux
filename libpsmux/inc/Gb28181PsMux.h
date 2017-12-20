#include <vector>
#include "PsMuxStreamType.h"
#include "psmuxdef.h"



typedef unsigned int StreamIdx;

struct PsMux;
struct PsMuxStream;


#define MAX_SPSPPSI_SIZE	(1024*1024)


class Gb28181PsMux
{
public:
    Gb28181PsMux();

    ~Gb28181PsMux();
    
    //根据指定类型增加一个流,返回流索引
    StreamIdx AddStream(PsMuxStreamType Type);

    int MuxH265VpsSpsPpsIFrame(guint8* pBlock, int BlockLen, gint64 Pts, gint64 Dts, StreamIdx Idx,
        guint8 * outBuf, int* pOutSize, int maxOutSize);

    //输入SPS+PPS+I帧,必须以00 00 00 01或者00 00 01开头
    int MuxH264SpsPpsIFrame(guint8* pBlock, int BlockLen, gint64 Pts, gint64 Dts, StreamIdx Idx,
        guint8 * outBuf, int* pOutSize, int maxOutSize);

    //输入单个H264/5帧,必须以00 00 00 01或者00 00 01开头,SPS PPS 和 I帧不能连在一起
    int MuxH264SingleFrame(guint8* pFrame, int FrameLen, gint64 Pts, gint64 Dts, StreamIdx Idx,
        guint8 * outBuf, int* pOutSize, int maxOutSize);

    int MuxH265SingleFrame(guint8* pFrame, int FrameLen, gint64 Pts, gint64 Dts, StreamIdx Idx,
        guint8 * outBuf, int* pOutSize, int maxOutSize);

    int MuxAudioFrame(guint8* pFrame, int FrameLen, gint64 Pts, gint64 Dts, StreamIdx Idx,
        guint8 * outBuf, int* pOutSize, int maxOutSize);

private:
    PsMux *m_PsMuxContext;
    std::vector<PsMuxStream *> m_VecStream;

    guint8* m_SpsPpsIBuf;
	int m_SpsPpsIBufSize;
};

//帧类型定义
enum NAL_type
{
    NAL_IDR,
    NAL_SPS,
    NAL_PPS,
    NAL_SEI,
    NAL_PFRAME,
    NAL_VPS,
    NAL_SEI_PREFIX,
    NAL_SEI_SUFFIX,
    NAL_other,
    NAL_TYPE_NUM
};

//判断是否是264或者265帧,如果是顺便把NalTypeChar设置一下
bool isH264Or265Frame(guint8* buf, unsigned char* NalTypeChar);
NAL_type getH264NALtype(guint8 c);
NAL_type getH265NALtype(guint8 c);

