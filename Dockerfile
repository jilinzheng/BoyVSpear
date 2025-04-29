# Use a Debian base image
FROM debian:stable

# Set environment variables to avoid interactive prompts during apt install
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

# Update package list and install basic build tools and pkg-config
RUN apt update && apt upgrade -y && \
    apt install -y --no-install-recommends \
    build-essential \
    pkg-config # pkg-config is needed to query target library flags

# Add the ARM architecture (armhf) to the container's apt sources
RUN dpkg --add-architecture armhf && apt update

# Install the ARM cross-compiler and the development headers/libraries for SDL2 and SDL2_ttf for the armhf architecture
RUN apt install -y --no-install-recommends \
    crossbuild-essential-armhf \
    libsdl2-dev:armhf \
    libsdl2-ttf-dev:armhf \
    # Add these missing dependency development packages for armhf:
    libasound2-dev:armhf \
    libpulse-dev:armhf \
    libsamplerate0-dev:armhf \
    libgbm-dev:armhf \
    libwayland-dev:armhf \
    libxkbcommon-dev:armhf \
    libharfbuzz-dev:armhf \
    libgraphite2-dev:armhf \
    libdecor-0-dev:armhf \
    libx11-dev:armhf \
    libxext-dev:armhf \
    libxcursor-dev:armhf \
    libxi-dev:armhf \
    libxfixes-dev:armhf \
    libxrandr-dev:armhf \
    libxss-dev:armhf \
    libdrm-dev:armhf

# Clean up apt cache to reduce image size
RUN rm -rf /var/lib/apt/lists/*

# Set the working directory inside the container to /app
WORKDIR /app

# The container is now set up for arm-linux-gnueabihf cross-compilation.
# We will mount the host project directory into /app when running the container.