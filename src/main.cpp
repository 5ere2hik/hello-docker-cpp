#include <iostream>
#include "pipeline.h"
#include <fstream>
#include <sstream>
#include <string>

static std::string read_stream_url_from_config(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) return std::string();
    std::string line;
    while (std::getline(ifs, line)) {
        // Look for a line like: stream_url: "rtsp://..."
        auto pos = line.find("stream_url:");
        if (pos != std::string::npos) {
            auto value = line.substr(pos + std::size("stream_url:"));
            // trim
            size_t start = value.find_first_not_of(" \t\"'");
            size_t end = value.find_last_not_of(" \t\"'");
            if (start == std::string::npos || end == std::string::npos) return std::string();
            return value.substr(start, end - start + 1);
        }
    }
    return std::string();
}

int main() {
    std::cout << "Hello from Docker + WSL + C++!" << std::endl;

    std::string config_path = "config.yml"; // placed next to the executable by CMake post-build step
    std::string url = read_stream_url_from_config(config_path);
    if (url.empty()) {
        url = "rtsp://192.168.0.171:8554/mystream"; // fallback default
    }

    openStream(url);
    return 0;
}