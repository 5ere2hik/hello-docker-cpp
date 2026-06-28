#include <iostream>
#include <string>
#include "pipeline.h"
#include <thread>
#include <chrono>

// FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
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

    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Could not find decoder for codec id " << codecpar->codec_id << std::endl;
        avformat_close_input(&fmt_ctx);
        av_dict_free(&opts);
        avformat_network_deinit();
        return;
    }

    AVCodecContext* dec_ctx = avcodec_alloc_context3(codec);
    if (!dec_ctx) {
        std::cerr << "Failed to allocate decoder context." << std::endl;
        avformat_close_input(&fmt_ctx);
        av_dict_free(&opts);
        avformat_network_deinit();
        return;
    }

    if (avcodec_parameters_to_context(dec_ctx, codecpar) < 0) {
        std::cerr << "Failed to copy codec parameters to decoder context." << std::endl;
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        av_dict_free(&opts);
        avformat_network_deinit();
        return;
    }

    if (avcodec_open2(dec_ctx, codec, nullptr) < 0) {
        std::cerr << "Failed to open decoder." << std::endl;
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        av_dict_free(&opts);
        avformat_network_deinit();
        return;
    }

    // Read packets from the stream for a short demo period
    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    if (!pkt || !frame) {
        std::cerr << "Failed to allocate packet/frame." << std::endl;
        av_frame_free(&frame);
        av_packet_free(&pkt);
        avcodec_free_context(&dec_ctx);
        avformat_close_input(&fmt_ctx);
        av_dict_free(&opts);
        avformat_network_deinit();
        return;
    }

    const int max_packets = 400; // read up to this many packets in this demo
    int packets_read = 0;
    bool have_keyframe = false;

    std::cout << "Starting read loop (will read up to " << max_packets << " packets)" << std::endl;

    while (packets_read < max_packets) {
        ret = av_read_frame(fmt_ctx, pkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                std::cout << "End of stream reached." << std::endl;
            } else {
                av_strerror(ret, errbuf, sizeof(errbuf));
                std::cerr << "Error reading frame: " << errbuf << std::endl;
            }
            break;
        }

        if (pkt->stream_index == video_stream_index) {
            std::cout << "Packet: pts=" << pkt->pts << " dts=" << pkt->dts
                      << " size=" << pkt->size << std::endl;

            const bool is_keyframe = (pkt->flags & AV_PKT_FLAG_KEY) != 0;
            if (!have_keyframe && !is_keyframe) {
                std::cout << "Skipping packet until keyframe arrives." << std::endl;
            } else {
                if (!have_keyframe && is_keyframe) {
                    std::cout << "Keyframe received. Starting decode." << std::endl;
                    have_keyframe = true;
                }

                if (have_keyframe) {
                    ret = avcodec_send_packet(dec_ctx, pkt);
                    if (ret < 0) {
                        av_strerror(ret, errbuf, sizeof(errbuf));
                        std::cerr << "Failed to send packet to decoder: " << errbuf << std::endl;
                    } else {
                        while (true) {
                            ret = avcodec_receive_frame(dec_ctx, frame);
                            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                                break;
                            }
                            if (ret < 0) {
                                av_strerror(ret, errbuf, sizeof(errbuf));
                                std::cerr << "Failed to receive frame from decoder: " << errbuf << std::endl;
                                break;
                            }

                            std::cout << "Decoded frame: width=" << frame->width
                                      << " height=" << frame->height
                                      << " pts=" << frame->pts << std::endl;
                        }
                    }
                }
            }

            ++packets_read;
        }

        av_packet_unref(pkt);

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    avcodec_send_packet(dec_ctx, nullptr);
    while (true) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            av_strerror(ret, errbuf, sizeof(errbuf));
            std::cerr << "Failed during decoder flush: " << errbuf << std::endl;
            break;
        }

        std::cout << "Flushed frame: width=" << frame->width
                  << " height=" << frame->height
                  << " pts=" << frame->pts << std::endl;
    }

    std::cout << "Read loop finished, closing stream." << std::endl;

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_dict_free(&opts);
    avformat_network_deinit();
}