#pragma once

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include "VideoFrameDescription.h"
#include "Global.h"
#include <string>

class VideoLoader
{
public:
	VideoLoader()
	{
		log("VIDEO", "FFmpeg version: " + std::string(av_version_info()));
	}

	VideoFrameDescription loadVideo(const std::string &videoPath)
	{
		VideoFrameDescription videoFrameDescription;

		AVFormatContext *format = avformat_alloc_context();
		if (avformat_open_input(&format, videoPath.c_str(), nullptr, nullptr) < 0)
		{
			error("VIDEO", "Could not open video file");
			return videoFrameDescription;
		}

		if (avformat_find_stream_info(format, nullptr) < 0)
		{
			error("VIDEO", "Could not find video stream info");
			return videoFrameDescription;
		}

		int video_stream_index = -1;
		for (unsigned int i = 0; i < format->nb_streams; i++)
		{
			if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				video_stream_index = i;
				break;
			}
		}

		if (video_stream_index == -1)
		{
			error("VIDEO", "Could not find video stream");
			return videoFrameDescription;
		}

		AVCodecParameters *codec_parameters = format->streams[video_stream_index]->codecpar;
		const AVCodec *codec = avcodec_find_decoder(codec_parameters->codec_id);
		if (!codec)
		{
			error("VIDEO", "Codec not found");
			return videoFrameDescription;
		}

		AVCodecContext *context = avcodec_alloc_context3(codec);
		if (avcodec_parameters_to_context(context, codec_parameters) < 0)
		{
			error("VIDEO", "Could not copy codec parameters to context");
			return videoFrameDescription;
		}

		if (avcodec_open2(context, codec, nullptr) < 0)
		{
			error("VIDEO", "Could not open codec");
			return videoFrameDescription;
		}

		AVCodecParserContext *parser = av_parser_init(codec->id);
		if (!parser)
		{
			error("VIDEO", "Parser not found");
			return videoFrameDescription;
		}

		AVPacket *packet = av_packet_alloc();
		if (!packet)
		{
			error("VIDEO", "Could not allocate video packet");
			return videoFrameDescription;
		}

		AVFrame *frame = av_frame_alloc();
		if (!frame)
		{
			error("VIDEO", "Could not allocate video frame");
			return videoFrameDescription;
		}

		uint8_t *data;
		int data_size = 0;
		bool got_frame = false;

		while (av_read_frame(format, packet) >= 0)
		{
			if (packet->stream_index == video_stream_index)
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

							AVFrame *rgb_frame;
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

		av_parser_close(parser);
		avcodec_free_context(&context);
		av_frame_free(&frame);
		av_packet_free(&packet);
		avformat_close_input(&format);

		log("VIDEO", "Video loaded: " + videoPath);

		return videoFrameDescription;
	}

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
};
