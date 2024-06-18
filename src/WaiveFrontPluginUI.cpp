#ifndef WAIVE_FRONT_PLUGIN_UI_CPP
#define WAIVE_FRONT_PLUGIN_UI_CPP

#include "DistrhoUI.hpp"
#include "ViewerWindow.cpp"
#include "themes/CinderTheme.cpp"
#include "Application.hpp"
#include "DearImGui.hpp"
#include <iostream>

extern "C"
{
#include "libavutil/imgutils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include "fonts/SpaceMono_Regular.cpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class WaiveFrontPluginUI : public UI
{
public:
    float parameters[Parameters::NumParameters];
    // ResizeHandle fResizeHandle;

    WaiveFrontPluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);

        openViewerWindow();

        ImGuiIO &io = ImGui::GetIO();
        regular = io.Fonts->AddFontFromMemoryCompressedTTF(SpaceMono_Regular_compressed_data, SpaceMono_Regular_compressed_size, 48.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    }

protected:
    // Function to convert AVFrame to RGB
    int convert_to_rgb(AVFrame *frame, AVFrame **rgb_frame)
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

    void onFileSelected(const char *filename)
    {
        // print ffmpeg version
        std::cout << "FFmpeg version: " << av_version_info() << std::endl;

        AVFormatContext *format = avformat_alloc_context();
        if (avformat_open_input(&format, filename, nullptr, nullptr) < 0)
        {
            std::cerr << "Could not open video file" << std::endl;
            return;
        }

        if (avformat_find_stream_info(format, nullptr) < 0)
        {
            std::cerr << "Could not find video stream info" << std::endl;
            return;
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
            std::cerr << "Could not find video stream" << std::endl;
            return;
        }

        AVCodecParameters *codec_parameters = format->streams[video_stream_index]->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codec_parameters->codec_id);
        if (!codec)
        {
            std::cerr << "Codec not found" << std::endl;
            return;
        }

        AVCodecContext *context = avcodec_alloc_context3(codec);
        if (avcodec_parameters_to_context(context, codec_parameters) < 0)
        {
            std::cerr << "Could not copy codec parameters to context" << std::endl;
            return;
        }

        if (avcodec_open2(context, codec, nullptr) < 0)
        {
            std::cerr << "Could not open codec" << std::endl;
            return;
        }

        AVCodecParserContext *parser = av_parser_init(codec->id);
        if (!parser)
        {
            std::cerr << "Parser not found" << std::endl;
            return;
        }

        AVPacket *packet = av_packet_alloc();
        if (!packet)
        {
            std::cerr << "Could not allocate video packet" << std::endl;
            return;
        }

        AVFrame *frame = av_frame_alloc();
        if (!frame)
        {
            std::cerr << "Could not allocate video frame" << std::endl;
            return;
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
                    // print data size
                    std::cout << "Data size: " << data_size << std::endl;

                    int ret = av_parser_parse2(parser, context, &packet->data, &packet->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

                    if (ret < 0)
                    {
                        std::cerr << "Error while parsing" << std::endl;
                        return;
                    }

                    data += ret;
                    data_size -= ret;

                    if (packet->size)
                    {
                        ret = avcodec_send_packet(context, packet);

                        if (ret < 0)
                        {
                            std::cerr << "Error while sending packet" << std::endl;
                            return;
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
                                std::cerr << "Error while receiving frame" << std::endl;
                                return;
                            }

                            AVFrame *rgb_frame;
                            if (convert_to_rgb(frame, &rgb_frame) < 0)
                            {
                                std::cerr << "Could not convert frame to RGB" << std::endl;
                                return;
                            }

                            printf("Frame %3d has width %d and height %d\n", context->frame_num, rgb_frame->width, rgb_frame->height);

                            viewerWindow->getViewerWidget()->setFrame(rgb_frame->data[0], rgb_frame->width, rgb_frame->height);

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

        std::cout << "Done" << std::endl;
    }

    void parameterChanged(uint32_t index, float value) override
    {
        parameters[index] = value;

        repaint();
    }

    void onImGuiDisplay() override
    {
        if (!initialized)
        {
            initialized = true;

            Window &window = getWindow();
            window.setOffsetY(window.getOffsetY() + 720 / 2 + 100);

            cinderTheme(ImGui::GetStyle());
        }

        if (fileBrowserHandle != nullptr)
        {
            if (fileBrowserIdle(fileBrowserHandle))
            {
                const char *filename = fileBrowserGetPath(fileBrowserHandle);

                if (filename != nullptr)
                {
                    onFileSelected(filename);
                }

                fileBrowserClose(fileBrowserHandle);
                fileBrowserHandle = nullptr;

                getWindow().focus();
            }
        }

        const float width = getWidth();
        const float height = getHeight();
        const float margin = 20.0f * getScaleFactor();

        ImGui::SetNextWindowSizeConstraints(ImVec2(512, 0), ImVec2(width - 2 * margin, height - 2 * margin));

        ImGui::PushFont(regular);

        ImGui::Begin("WAIVE-FRONT V2", nullptr, ImGuiWindowFlags_NoResize);

        if (ImGui::SliderFloat("Threshold", &parameters[Threshold], 0.0f, 1.0f))
            setParameterValue(0, parameters[Threshold]);

        if (ImGui::SliderFloat("Width", &parameters[Width], 0.0f, 1.0f))
            setParameterValue(1, parameters[Width]);

        if (ImGui::Button("Select Video"))
        {
            FileBrowserOptions options;
            options.startDir = "/Users/Bram/Documents/WAIVE";
            // openFileBrowser(options);

            if (fileBrowserHandle != nullptr)
            {
                fileBrowserClose(fileBrowserHandle);
            }

            fileBrowserHandle = fileBrowserCreate(false, getWindow().getNativeWindowHandle(), 1, options);
        }

        ImGui::End();

        ImGui::PopFont();

        if (viewerWindow != nullptr)
        {
            viewerWindow->repaint();
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaiveFrontPluginUI)

private:
    ViewerWindow *viewerWindow = nullptr;
    bool initialized = false;
    FileBrowserHandle fileBrowserHandle = nullptr;

    ImFont *regular;

    void openViewerWindow()
    {
        std::cout << "Opening viewer window" << std::endl;
        Application &app = getApp();

        if (viewerWindow == nullptr)
        {
            viewerWindow = new ViewerWindow(app, parameters);
        }
    }
};

UI *createUI()
{
    return new WaiveFrontPluginUI();
}

END_NAMESPACE_DISTRHO

#endif
