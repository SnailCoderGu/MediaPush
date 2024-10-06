# MediaPush
使用ffmpeg ,directshow,x624,fdkaac,rtmp等技术，推流到平台

v1.2.0
1. 增加了使用librtmp api方式推流 RTMP_PUSH_USER_FFMPEG定义来区分不同的推流方式
2. 增加了界面输入rtmp推流地址

v1.1.0
1. 加入了ffmpeg推送rtmp
2. 可以启动推流，关闭推流
3. ffmpeg库集成了librtmp
4. 推送地址源码固定设置方式

v1.0.1
1. 添加ffmpeg库，库支持fdk-aac
2. 支持ffmpeg 的api编码音频aac


v1.0.1
1. 添加ffmpeg库，库支持libx264
2. 添加独立的libx264静态库
3. 支持ffmpeg 的api编码264
4. 支持x264库原生api编码264

v1.0.0
使用qt技术，ui，音视频技术，采集rgb，转yuv，采集pcm

1. 支持摄像头预览和yuv数据采集
2. 支持摄像头设备切换
3. 支持麦克风数据采集
4. 支持麦克风采集活动量实时监测
