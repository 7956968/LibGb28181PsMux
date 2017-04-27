// PsMuxExample.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "Gb28181PsMux.h"
#include <string>
#include <stdio.h>
#include <string.h>
#define BUF_LEN (1024*1024)

struct PsMuxContext 
{
    PsMuxContext()
    {
        Idx = PsMux.AddStream(PSMUX_ST_VIDEO_H264);
        pMuxBuf = new guint8[BUF_LEN];
        pts = 0;
        dts = 0;
    }
    ~PsMuxContext()
    {
        delete []pMuxBuf;
    }
    virtual void Process(guint8* buf, int len)
    {
        int MuxOutSize = 0;
        int ret = PsMux.MuxH264SingleFrame(buf, len, pts, dts, Idx, pMuxBuf, &MuxOutSize, BUF_LEN);
        
        if (ret == 0){
            OnPsFrameOut(pMuxBuf, MuxOutSize, pts, dts);
        }
        
        NAL_type Type = getH264NALtype(buf[4]);

        if ((Type == NAL_IDR) || (Type == NAL_PFRAME)){
            pts += 3600;
            dts += 3600;
        }
    }
    
    void testMuxSpsPpsI(guint8* buf, int len)
    {
        int MuxOutSize = 0;
        PsMux.MuxH264SpsPpsIFrame(buf, len, 0, 0, Idx, pMuxBuf, &MuxOutSize, BUF_LEN);
    }

    virtual void OnPsFrameOut(guint8* buf, int len, gint64 pts, gint64 dts) = 0;

private:
    Gb28181PsMux PsMux;
    StreamIdx Idx;
    guint8* pMuxBuf;
    gint64 pts;
    gint64 dts;
};

struct PsProcessSaveFile : public PsMuxContext
{
    PsProcessSaveFile(std::string DstName)
    {
        fp = fopen(DstName.c_str(), "wb+");
    }
    ~PsProcessSaveFile()
    {
        if (fp){
            fclose(fp);
        }
    }
    virtual void OnPsFrameOut(guint8* buf, int len, gint64 pts, gint64 dts)
    {
        if (len > 0 && fp)
        {
            fwrite(buf, len, 1, fp);
            fflush(fp);
        }
    }
    FILE* fp;
};


//����block���NALU,ֱ��MaxSlice,��Ȼһֱ������ȥ
int process_block(guint8* pBlock, int BlockLen, int MaxSlice,  PsMuxContext* PsDst)
{
    static guint8* pStaticBuf = new guint8[BUF_LEN];
    static int StaticBufSize = 0;

    guint8* pCurBlock = NULL;

    int LastBlockLen = 0;

    memcpy(pStaticBuf+StaticBufSize, pBlock, BlockLen);

    LastBlockLen = StaticBufSize+BlockLen;

    guint8* pCurPos = pStaticBuf;

    guint8* NaluStartPos = NULL;
    guint8* NaluEndPos   = NULL;


    //һ�����������NALU����,����SPS PPS ���I֡�ǾͲ��ñ���
    int iSliceNum = 0;

    while (LastBlockLen > 4)
    {
        if ((pCurPos[0] == 0) && (pCurPos[1] == 0) && (pCurPos[2] == 0) && (pCurPos[3] == 1)){

            if (iSliceNum + 1 >= MaxSlice){//�Ѿ��������NALU����,����Ĳ������˰�ʣ�µļ��Ͼ���
                PsDst->Process(pCurPos, LastBlockLen);
                break;
            }

            if (NaluStartPos == NULL){
                NaluStartPos = pCurPos;
            }
            else{
                PsDst->Process(NaluStartPos, pCurPos-NaluStartPos);
                iSliceNum++;
                NaluStartPos = pCurPos;
            }
        }

        pCurPos++;
        LastBlockLen--;
    }

    //��ʣ�µ�,����,�ͺ����ƴ����
    if (NaluStartPos){
        memcpy(pStaticBuf, NaluStartPos, LastBlockLen);
        StaticBufSize = LastBlockLen;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    Gb28181PsMux PsMuxer;
    int Circle = 0;



    PsProcessSaveFile SaveFile("PsMux.mpeg");

    unsigned char pTest[] = {0x00, 0x00, 0x00, 0x01, 0x27, 0x55, 0x66,
        0x00, 0x00, 0x00, 0x01, 0x28, 0x55, 
        0x00, 0x00, 0x00, 0x01, 0x25, 0x66};

    //SaveFile.testMuxSpsPpsI(pTest, sizeof(pTest));

    FILE* fp = fopen(argv[1], "rb");
    if (fp == NULL)
    {
        printf("can't open file %s\n", argv[1]);
        return -1;
    }

    guint8* fReadbuf = new guint8[BUF_LEN];

    while(1)
    {
        int fReadsz = fread(fReadbuf, 1, BUF_LEN, fp);

        if(fReadsz <= 0){

            if (Circle){
                fseek(fp, 0, SEEK_SET);
                continue;
            }
            else{
                break;
            }
        }

        process_block(fReadbuf, fReadsz, 0xffff, &SaveFile);
    }

    delete []fReadbuf;

	return 0;
}

