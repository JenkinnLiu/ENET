#ifndef _RTMP_H_
#define _RTMP_H_
#include <cstdint>
#include <string>
#include <string.h>
#include <QtGlobal>
#include <memory>


static const int RTMP_VERSION = 0X3;
static const int RTMP_SET_CHUNK_SIZE = 0x1;
static const int RTMP_ABORT_MESSAGE = 0x2;
static const int RTMP_ACK = 0x3;
static const int RTMP_ACK_SIZE = 0x5;
static const int RTMP_BANDWIDTH_SIZE = 0x6;
static const int RTMP_AUDIO = 0x08;
static const int RTMP_VIDEO = 0x09;
static const int RTMP_NOTIFY = 0x12;
static const int RTMP_INVOKE = 0x14;

//chunk类型
static const int RTMP_CHUNK_TYPE_0 = 0;
static const int RTMP_CHUNK_TYPE_1 = 1;
static const int RTMP_CHUNK_TYPE_2 = 2;
static const int RTMP_CHUNK_TYPE_3 = 3;

//RTMP消息ID

static const int RTMP_CHUNK_CONTROL_ID = 2;
static const int RTMP_CHUNK_INVOKE_ID = 3;
static const int RTMP_CHUNK_AUDIO_ID = 4;
static const int RTMP_CHUNK_VIDEO_ID = 5;
static const int RTMP_CHUNK_DATA_ID = 6;

//编码类型
static const int RTMP_CODEC_ID_H264 = 7;
static const int RTMP_CODEC_ID_AAC = 10;

//元数据类型ID
static const int RTMP_AVC_SEQUENCE_HEADER = 0x18;
static const int RTMP_AAC_SEQUENCE_HEADER = 0x19; //rtmp协议文档


struct MediaInfo
{
    quint8  video_codec_id = RTMP_CODEC_ID_H264;
    quint8  video_framerate = 0;
    quint32 video_width = 0;
    quint32 video_height = 0;
    std::shared_ptr<quint8> sps;
    std::shared_ptr<quint8> pps;
    std::shared_ptr<quint8> sei;
    quint32 sps_size = 0;
    quint32 pps_size = 0;
    quint32 sei_size = 0;

    quint8  audio_codec_id = RTMP_CODEC_ID_AAC;
    quint32 audio_channel = 0;
    quint32 audio_samplerate = 0;
    quint32 audio_frame_len = 0;
    std::shared_ptr<quint8>  audio_specific_config;
    quint32 audio_specific_config_size = 0;
};

class Rtmp
{
public:
	virtual ~Rtmp() {};

	void SetChunkSize(uint32_t size)
	{
		if (size > 0 && size <= 60000) {
			max_chunk_size_ = size;
		}
	}

	void SetPeerBandwidth(uint32_t size)
	{ peer_bandwidth_ = size; }

	uint32_t GetChunkSize() const 
	{ return max_chunk_size_; }

	uint32_t GetAcknowledgementSize() const
	{ return acknowledgement_size_; }

	uint32_t GetPeerBandwidth() const
	{ return peer_bandwidth_; }

	virtual int ParseRtmpUrl(std::string url)
	{
		char ip[100] = { 0 };
		char streamPath[500] = { 0 };
		char app[100] = { 0 };
		char streamName[400] = { 0 };
		uint16_t port = 0;

		if (strstr(url.c_str(), "rtmp://") == nullptr) {
			return -1;
		}

#if defined(__linux) || defined(__linux__)
		if (sscanf(url.c_str() + 7, "%[^:]:%hu/%s", ip, &port, streamPath) == 3)
#elif defined(WIN32) || defined(_WIN32)
		if (sscanf_s(url.c_str() + 7, "%[^:]:%hu/%s", ip, 100, &port, streamPath, 500) == 3)
#endif
		{
			port_ = port;
		}
#if defined(__linux) || defined(__linux__)
		else if (sscanf(url.c_str() + 7, "%[^/]/%s", ip, streamPath) == 2)
#elif defined(WIN32) || defined(_WIN32)
		else if (sscanf_s(url.c_str() + 7, "%[^/]/%s", ip, 100, streamPath, 500) == 2)
#endif
		{
			port_ = 1935;
		}
		else {
			return -1;
		}

		ip_ = ip;
		stream_path_ += "/";
		stream_path_ += streamPath;

#if defined(__linux) || defined(__linux__)
		if (sscanf(stream_path_.c_str(), "/%[^/]/%s", app, streamName) != 2)
#elif defined(WIN32) || defined(_WIN32)
		if (sscanf_s(stream_path_.c_str(), "/%[^/]/%s", app, 100, streamName, 400) != 2)
#endif
		{
			return -1;
		}

		app_ = app;
		stream_name_ = streamName;
		return 0;
	}

	std::string GetStreamPath() const
	{ return stream_path_; }

	std::string GetApp() const
	{ return app_; }

	std::string GetStreamName() const
	{ return stream_name_; }

	uint16_t port_ = 1935;
	std::string ip_;
	std::string app_;
	std::string stream_name_;
	std::string stream_path_;

	uint32_t peer_bandwidth_ = 5000000;
	uint32_t acknowledgement_size_ = 5000000;
	uint32_t max_chunk_size_ = 128;
};
#endif
