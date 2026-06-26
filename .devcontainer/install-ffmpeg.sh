#!/usr/bin/env bash
set -euo pipefail

# Install FFmpeg development packages and the ffprobe CLI on Ubuntu 24.04
sudo apt update
sudo apt install -y libavformat-dev libavcodec-dev libavutil-dev pkg-config

echo "FFmpeg development packages and ffprobe installed."
