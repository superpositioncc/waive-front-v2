#pragma once

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include "VideoFrameDescription.h"
#include "../util/Logger.cpp"
using namespace Util::Logger;
#include <string>

class VideoLoader
{
private:
	int convertToRGB(AVFrame *frame, AVFrame **rgb_frame)
	{
		// Allocate an AVFrame structure
		*rgb_frame = av_frame_alloc();
		if (*rgb_frame == NULL)
		{
			fprintf(stderr, "Could not allocate RGB frame\n");
			return -1;
		}

		// Set up the parameters for the output RGB frame
		(*rgb_frame)->format = AV_PIX_FMT_RGB24;
		(*rgb_frame)->width = frame->width;
		(*rgb_frame)->height = frame->height;

		// Allocate buffer for the RGB frame
		int ret = av_frame_get_buffer(*rgb_frame, 32); // align to 32 bytes
		if (ret < 0)
		{
			fprintf(stderr, "Could not allocate buffer for RGB frame\n");
			av_frame_free(rgb_frame);
			return ret;
		}

		// Initialize SWS context for software scaling
		struct SwsContext *sws_ctx = sws_getContext(
			frame->width, frame->height, (enum AVPixelFormat)frame->format,
			frame->width, frame->height, AV_PIX_FMT_RGB24,
			SWS_BILINEAR, NULL, NULL, NULL);

		if (sws_ctx == NULL)
		{
			fprintf(stderr, "Could not initialize sws context\n");
			av_frame_free(rgb_frame);
			return -1;
		}

		// create an array of new linesizes
		(*rgb_frame)->linesize[0] = frame->width * 3;

		// Convert the image from its native format to RGB
		ret = sws_scale(sws_ctx, (uint8_t const *const *)frame->data,
						frame->linesize, 0, frame->height,
						(*rgb_frame)->data, (*rgb_frame)->linesize);

		if (ret <= 0)
		{
			fprintf(stderr, "Error while converting to RGB\n");
			av_frame_free(rgb_frame);
			sws_freeContext(sws_ctx);
			return -1;
		}

		// Free the SWS context
		sws_freeContext(sws_ctx);

		return 0;
	}

	AVFormatContext *format;
	AVCodecParameters *codecParameters;
	const AVCodec *codec;
	AVCodecContext *context;
	AVCodecParserContext *parser;
	AVPacket *packet;
	AVFrame *frame;
	AVFrame *rgb_frame;
	uint8_t *data;
	int videoStreamIndex;
	int status;

	int64_t lastFrameTime = 0;
	int64_t frameDuration = 0;

public:
	VideoLoader()
	{
		av_log_set_level(AV_LOG_QUIET);
		print("VIDEO", "FFmpeg version: " + std::string(av_version_info()));
	}

	int getStatus()
	{
		return status;
	}

	void rewind()
	{
		avcodec_flush_buffers(context);
		av_seek_frame(format, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
	}

	bool shouldGetNextFrame(int64_t currentTime)
	{
		if (frameDuration == 0)
		{
			frameDuration = float(format->streams[videoStreamIndex]->r_frame_rate.den) / float(format->streams[videoStreamIndex]->r_frame_rate.num) * 1000000.0f;
		}

		if (
			currentTime >= lastFrameTime + frameDuration ||
			currentTime + 1000000 / 120 >= lastFrameTime + frameDuration)
		{
			lastFrameTime = currentTime;
			return true;
		}
		return false;
	}

	VideoFrameDescription getFrame()
	{
		// deallocate previous frame
		av_frame_unref(frame);
		av_frame_unref(rgb_frame);
		av_packet_unref(packet);

		int data_size = 0;
		bool got_frame = false;

		VideoFrameDescription videoFrameDescription;

		while (true)
		{
			if (av_read_frame(format, packet) < 0)
			{
				// End of video, seek to start
				rewind();
				continue;
			}

			if (packet->stream_index == videoStreamIndex)
			{
				data = packet->data;
				data_size = packet->size;

				while (data_size > 0)
				{
					int ret = av_parser_parse2(parser, context, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

					if (ret < 0)
					{
						error("VIDEO", "Error while parsing");
						return videoFrameDescription;
					}

					data += ret;
					data_size -= ret;

					if (packet->size)
					{
						ret = avcodec_send_packet(context, packet);

						if (ret < 0)
						{
							error("VIDEO", "Error while sending packet");
							return videoFrameDescription;
						}

						while (ret >= 0)
						{
							ret = avcodec_receive_frame(context, frame);

							if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
							{
								break;
							}
							else if (ret < 0)
							{
								error("VIDEO", "Error while receiving frame");
								return videoFrameDescription;
							}

							if (convertToRGB(frame, &rgb_frame) < 0)
							{
								error("VIDEO", "Could not convert frame to RGB");
								return videoFrameDescription;
							}

							videoFrameDescription.width = rgb_frame->width;
							videoFrameDescription.height = rgb_frame->height;
							videoFrameDescription.data = rgb_frame->data[0];

							got_frame = true;
							break;
						}
					}

					if (got_frame)
						break;
				}
			}

			if (got_frame)
				break;
		}

		return videoFrameDescription;
	}

	int loadVideo(const std::string &videoPath)
	{
		close();

		format = avformat_alloc_context();
		if (avformat_open_input(&format, videoPath.c_str(), nullptr, nullptr) < 0)
		{
			error("VIDEO", "Could not open video file");
			return -1;
		}

		if (avformat_find_stream_info(format, nullptr) < 0)
		{
			error("VIDEO", "Could not find video stream info");
			return -1;
		}

		videoStreamIndex = -1;
		for (unsigned int i = 0; i < format->nb_streams; i++)
		{
			if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				videoStreamIndex = i;
				break;
			}
		}

		if (videoStreamIndex == -1)
		{
			error("VIDEO", "Could not find video stream");
			return -1;
		}

		codecParameters = format->streams[videoStreamIndex]->codecpar;
		codec = avcodec_find_decoder(codecParameters->codec_id);
		if (!codec)
		{
			error("VIDEO", "Codec not found");
			return -1;
		}

		context = avcodec_alloc_context3(codec);
		if (avcodec_parameters_to_context(context, codecParameters) < 0)
		{
			error("VIDEO", "Could not copy codec parameters to context");
			return -1;
		}

		if (avcodec_open2(context, codec, nullptr) < 0)
		{
			error("VIDEO", "Could not open codec");
			return -1;
		}

		parser = av_parser_init(codec->id);
		if (!parser)
		{
			error("VIDEO", "Parser not found");
			return -1;
		}

		packet = av_packet_alloc();
		if (!packet)
		{
			error("VIDEO", "Could not allocate video packet");
			return -1;
		}

		frame = av_frame_alloc();
		if (!frame)
		{
			error("VIDEO", "Could not allocate video frame");
			return -1;
		}

		print("VIDEO", "Video loaded: " + videoPath);

		status = 1;

		return 0;
	}

	void close()
	{
		av_parser_close(parser);
		avcodec_free_context(&context);
		av_frame_free(&frame);
		av_packet_free(&packet);
		avformat_close_input(&format);
	}
};
