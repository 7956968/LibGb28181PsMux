1 项目概述
---

PS打包在实际项目中很难直接调试,而SIP很容易抓包调试,实际中经常有人问PS如何打包,而各自的实现也不尽相同,在对接时经常会遇到困扰,我们将PS打包部分独立出来以求更多的人完善。

例子中有VLC播放的SDP。打开本机的7000端口, 播放RTP payload 96的PS流。

海康，大华，东方网力等大部分平台都测试过。



2 示例代码：<br>
---
Gb28181PsMux PsMuxer;<br>
添加两个流<br>
StreamIdx h264Idx = PsMuxer.AddStream(PSMUX_ST_VIDEO_H264);<br>
StreamIdx g711Idx = PsMuxer.AddStream(PSMUX_ST_PS_AUDIO_G711A);<br>

const int psFrameMax = 1024\*1024;<br>
char* psFrameOutBuf = new char[psFrameMax];<br>
int psFrameSize = 0;<br>

封装音频,psFrameSize为实际ps包的大小<br>
int r = PsMuxer.MuxAudioFrame(g711buf, g711len, pts, dts, g711Idx, psFrameOutBuf, &psFrameSize, psFrameMax);<br>
if(r == MUX_OK && psFrameSize > 0){<br>
    ...<br>
}<br><br>
PsMuxer.MuxH264SingleFrame(h263buf, h264len, pts, dts, g711Idx, psFrameOutBuf, &psFrameSize, psFrameMax);<br>
if(r == MUX_OK && psFrameSize > 0){<br>
    ...<br>
}<br>

3 项目现状
---
2021/06/01 因为从发布以来，没有一个人提交更改，我很生气，停更，再见
