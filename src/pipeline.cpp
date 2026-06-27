#include <iostream>
#include <string>
#include "pipeline.h"
#include <thread>
#include <chrono>

// FFmpeg headers
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}

void openStream(const std::string &url)
{
    std::cout << "Opening RTSP stream: " << url << std::endl;

    // Initialize network components (required for RTSP)
    avformat_network_init();

    AVFormatContext* fmt_ctx = nullptr;
    AVDictionary* opts = nullptr;

    // Prefer TCP for RTSP to avoid packet loss
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    // Set a reasonable timeout (in microseconds)
    av_dict_set(&opts, "stimeout", "5000000", 0); // 5 seconds

    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    int ret = avformat_open_input(&fmt_ctx, url.c_str(), nullptr, &opts);
    if (ret < 0) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Failed to open input: " << errbuf << std::endl;
        av_dict_free(&opts);
        avformat_network_deinit();
        return;
    }

    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        av_strerror(ret, errbuf, sizeof(errbuf));
        std::cerr << "Failed to find stream info: " << errbuf << std::endl;
        avformat_close_input(&fmt_ctx);
        av_dict_free(&opts);
        avformat_network_deinit();
        return;
    }

    std::cout << "Successfully opened stream. Format: "
              << (fmt_ctx->iformat ? fmt_ctx->iformat->long_name : "unknown")
              << std::endl;

    // Find the first video stream
    int video_stream_index = -1;
    for (unsigned i = 0; i < fmt_ctx->nb_streams; ++i) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = static_cast<int>(i);
            break;
        }
    }

    if (video_stream_index < 0) {
        std::cerr << "No video stream found in input." << std::endl;
        avformat_close_input(&fmt_ctx);
        av_dict_free(&opts);
        avformat_network_deinit();
        return;
    }

    const AVCodecParameters* codecpar = fmt_ctx->streams[video_stream_index]->codecpar;
    std::cout << "Video stream index: " << video_stream_index << std::endl;
    std::cout << "Codec ID: " << codecpar->codec_id << " (expected HEVC => " << AV_CODEC_ID_HEVC << ")" << std::endl;
    std::cout << "Width x Height: " << codecpar->width << " x " << codecpar->height << std::endl;

    // Read packets from the stream for a short demo period
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) {
        std::cerr << "Failed to allocate AVPacket." << std::endl;
        avformat_close_input(&fmt_ctx);
        av_dict_free(&opts);
        avformat_network_deinit();
        return;
    }

    const int max_packets = 200; // read up to this many packets in this demo
    int packets_read = 0;

    std::cout << "Starting read loop (will read up to " << max_packets << " packets)" << std::endl;

    while (packets_read < max_packets) {
        ret = av_read_frame(fmt_ctx, pkt);
        if (ret < 0) {
            // end of stream or error
            if (ret == AVERROR_EOF) {
                std::cout << "End of stream reached." << std::endl;
            } else {
                av_strerror(ret, errbuf, sizeof(errbuf));
                std::cerr << "Error reading frame: " << errbuf << std::endl;
            }
            break;
        }

        if (pkt->stream_index == video_stream_index) {
            // For now just print some basic packet info. In a real app you'd send
            // the packet to a decoder or write it to a file/socket.
            std::cout << "Packet: pts=" << pkt->pts << " dts=" << pkt->dts
                      << " size=" << pkt->size << std::endl;
            ++packets_read;
        }

        av_packet_unref(pkt);

        // Be cooperative: sleep a tiny bit to avoid busy-looping
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    std::cout << "Read loop finished, closing stream." << std::endl;

    // Clean up
    avformat_close_input(&fmt_ctx);
    av_dict_free(&opts);
    av_packet_free(&pkt);
    avformat_network_deinit();
}