#include <iostream>
#include "pipeline.h"
#include <fstream>
#include <sstream>
#include <string>

// RapidYAML
#include <ryml.hpp>
#include <ryml_std.hpp>

static std::string read_stream_url_from_config(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) return std::string();
    std::string yaml((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    // Parse YAML using RapidYAML
    try {
        auto tree = ryml::parse_in_arena(c4::to_csubstr(yaml));
        auto root = tree.rootref();
        auto node = root["stream_url"];
        //return ryml::emitrs_yaml<std::string>(node);
        return std::string(node.val().str, node.val().len);
    } catch (...) {
        // Fall through to return empty on parse errors
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