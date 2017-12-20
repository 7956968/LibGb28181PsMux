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
    
    //����ָ����������һ����,����������
    StreamIdx AddStream(PsMuxStreamType Type);

    int MuxH265VpsSpsPpsIFrame(guint8* pBlock, int BlockLen, gint64 Pts, gint64 Dts, StreamIdx Idx,
        guint8 * outBuf, int* pOutSize, int maxOutSize);

    //����SPS+PPS+I֡,������00 00 00 01����00 00 01��ͷ
    int MuxH264SpsPpsIFrame(guint8* pBlock, int BlockLen, gint64 Pts, gint64 Dts, StreamIdx Idx,
        guint8 * outBuf, int* pOutSize, int maxOutSize);

    //���뵥��H264/5֡,������00 00 00 01����00 00 01��ͷ,SPS PPS �� I֡��������һ��
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

//֡���Ͷ���
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

//�ж��Ƿ���264����265֡,�����˳���NalTypeChar����һ��
bool isH264Or265Frame(guint8* buf, unsigned char* NalTypeChar);
NAL_type getH264NALtype(guint8 c);
NAL_type getH265NALtype(guint8 c);

