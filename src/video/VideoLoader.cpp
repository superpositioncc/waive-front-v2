/*
WAIVE-FRONT
Copyright (C) 2024  Bram Bogaerts, Superposition

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/opt.h"
}

#include "VideoFrameDescription.h"
#include "../util/Logger.cpp"
using namespace Util::Logger;
#include <string>
#include <vector>

#ifdef __APPLE__
#include <unistd.h>
#endif

/**
 * @brief Class to load a video and extract frames and colors from it
 *
 */
class VideoLoader
{
private:
	std::vector<float> colors; /**< Colors extracted from the video */
	bool used = false;		   /**< Whether the video loader has been used */
	bool usedFrame = false;	   /**< Whether the frame has been used */

	/**
	 * @brief Convert a frame to RGB
	 *
	 * @param frame Frame to convert
	 * @param rgb_frame Converted frame
	 * @return int 0 if successful, -1 otherwise
	 */
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

	AVFormatContext *format;			/**< Format context */
	AVCodecParameters *codecParameters; /**< Codec parameters */
	const AVCodec *codec;				/**< Codec */
	AVCodecContext *context;			/**< Codec context */
	AVCodecParserContext *parser;		/**< Parser context */
	AVPacket *packet;					/**< Packet */
	AVFrame *frame;						/**< Frame */
	AVFrame *rgb_frame;					/**< RGB frame */
	uint8_t *data;						/**< Data */
	int videoStreamIndex;				/**< Video stream index */
	int status;							/**< Status */

	int64_t lastFrameTime = 0; /**< Last frame time */
	int64_t frameDuration = 0; /**< Frame duration */

	// For the palettegen filter
	AVFilterGraph *filterGraph;							   /**< Filter graph for the palettegen filter */
	AVFilterContext *bufferSrcContext, *bufferSinkContext; /**< Buffer source and sink contexts */
	const AVFilter *bufferSrc, *bufferSink;				   /**< Buffer source and sink */
	AVFrame *paletteFrame;								   /**< Palette frame */
	AVFilterInOut *outputs, *inputs;					   /**< Filter in/out */

public:
	/**
	 * @brief Construct a new VideoLoader object
	 *
	 */
	VideoLoader()
	{
		av_log_set_level(AV_LOG_QUIET);
		print("VIDEO", "FFmpeg version: " + std::string(av_version_info()));
	}

	/**
	 * @brief Get the status of the video loader
	 *
	 * @return int Status
	 */
	int getStatus()
	{
		return status;
	}

	/**
	 * @brief Rewind the video
	 *
	 */
	void rewind()
	{
		avcodec_flush_buffers(context);
		av_seek_frame(format, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
	}

	/**
	 * @brief Extract colors from a frame
	 *
	 * @param frame Frame to extract colors from
	 * @return std::vector<float> Colors extracted from the frame
	 */
	std::vector<float> extractColors(AVFrame *frame)
	{
		if (av_buffersrc_add_frame(bufferSrcContext, frame) < 0)
		{
			error("VIDEO", "Could not add frame to buffer source");
			return std::vector<float>();
		}
		av_buffersrc_add_frame_flags(bufferSrcContext, nullptr, 0);

		paletteFrame = av_frame_alloc();
		if (!paletteFrame)
		{
			error("VIDEO", "Could not allocate palette frame");
			return std::vector<float>();
		}

		int ret = av_buffersink_get_frame(bufferSinkContext, paletteFrame);
		if (ret == AVERROR(EAGAIN))
		{
			error("VIDEO", "Could not retrieve palette frame from buffer sink");
			return std::vector<float>();
		}

		std::vector<float> colors;
		uint32_t *palette = (uint32_t *)paletteFrame->data[0];

		int nColors = 5;

		for (int i = 0; i < nColors; i++)
		{
			uint32_t color = palette[i];
			uint8_t r = (color >> 16) & 0xFF;
			uint8_t g = (color >> 8) & 0xFF;
			uint8_t b = color & 0xFF;

			colors.push_back(r / 255.0f);
			colors.push_back(g / 255.0f);
			colors.push_back(b / 255.0f);
		}

		return colors;
	}

	/**
	 * @brief Check if the next frame should be retrieved
	 *
	 * @param currentTime Current time
	 * @return bool Whether the next frame should be retrieved
	 */
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

	/**
	 * @brief Get the next frame from the video
	 *
	 * @return VideoFrameDescription Frame description
	 */
	VideoFrameDescription getFrame()
	{
		if (usedFrame)
		{
			getReadyForNextFrame();
		}

		usedFrame = true;
		int data_size = 0;
		bool got_frame = false;

		VideoFrameDescription videoFrameDescription;
		videoFrameDescription.ready = false;

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

							if (colors.size() == 0)
								colors = extractColors(rgb_frame);

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
			{
				videoFrameDescription.ready = true;
				break;
			}
		}

		return videoFrameDescription;
	}

	/**
	 * @brief Get the colors extracted from the video
	 *
	 * @return std::vector<float> Colors extracted from the video
	 */
	std::vector<float> getColors()
	{
		return colors;
	}

	/**
	 * @brief Load a video
	 *
	 * @param videoPath Path to the video
	 * @return int 0 if successful, -1 otherwise
	 */
	int loadVideo(const std::string &videoPath)
	{
		if (used)
		{
			getReadyForNextLoad();
		}

		used = true;
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

		filterGraph = avfilter_graph_alloc();
		if (!filterGraph)
		{
			error("VIDEO", "Could not allocate filter graph");
			return -1;
		}

		bufferSrc = avfilter_get_by_name("buffer");
		bufferSink = avfilter_get_by_name("buffersink");
		if (!bufferSrc || !bufferSink)
		{
			error("VIDEO", "Could not get filter");
			return -1;
		}

		outputs = avfilter_inout_alloc();
		inputs = avfilter_inout_alloc();
		if (!outputs || !inputs)
		{
			error("VIDEO", "Could not allocate filter in/out");
			return -1;
		}

		char args[512];
		snprintf(args, sizeof(args),
				 "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
				 context->width, context->height, AV_PIX_FMT_RGB24,
				 format->streams[videoStreamIndex]->time_base.num, format->streams[videoStreamIndex]->time_base.den,
				 context->sample_aspect_ratio.num, context->sample_aspect_ratio.den);

		if (avfilter_graph_create_filter(&bufferSrcContext, bufferSrc, "in", args, nullptr, filterGraph) < 0)
		{
			error("VIDEO", "Could not create filter");
			return -1;
		}
		if (avfilter_graph_create_filter(&bufferSinkContext, bufferSink, "out", nullptr, nullptr, filterGraph) < 0)
		{
			error("VIDEO", "Could not create filter");
			return -1;
		}
		if (!bufferSrcContext || !bufferSinkContext)
		{
			error("VIDEO", "Could not create filter");
			return -1;
		}

		enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_RGB32, AV_PIX_FMT_NONE};
		int ret = av_opt_set_int_list(bufferSinkContext, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

		if (ret < 0)
		{
			error("VIDEO", "Could not set pixel formats");
			return -1;
		}

		outputs->name = av_strdup("in");
		outputs->filter_ctx = bufferSrcContext;
		outputs->pad_idx = 0;
		outputs->next = nullptr;

		inputs->name = av_strdup("out");
		inputs->filter_ctx = bufferSinkContext;
		inputs->pad_idx = 0;
		inputs->next = nullptr;

		if (avfilter_graph_parse_ptr(filterGraph, "palettegen=max_colors=5", &inputs, &outputs, nullptr) < 0)
		{
			error("VIDEO", "Could not parse filter graph");
			return -1;
		}

		if (avfilter_graph_config(filterGraph, nullptr) < 0)
		{
			error("VIDEO", "Could not configure filter graph");
			return -1;
		}

		status = 1;

		return 0;
	}

	/**
	 * @brief Free resources and get ready for the next load
	 *
	 */
	void getReadyForNextLoad()
	{
		av_parser_close(parser);
		avcodec_free_context(&context);
		av_frame_free(&frame);
		av_packet_free(&packet);
		avformat_close_input(&format);

		av_frame_free(&rgb_frame);
		av_frame_free(&paletteFrame);
		avfilter_graph_free(&filterGraph);
		avfilter_inout_free(&inputs);
		avfilter_inout_free(&outputs);

		colors.clear();
	}

	/**
	 * @brief Free resources and get ready for the next frame
	 *
	 */
	void getReadyForNextFrame()
	{
		av_frame_unref(frame);
		av_frame_unref(rgb_frame);
		av_packet_unref(packet);
		av_frame_unref(paletteFrame);
	}
};
