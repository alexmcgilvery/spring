FROM docker.io/ubuntu:24.04
ENV ENGINE_PLATFORM=amd64-windows

RUN apt-get update \
 && apt-get upgrade --yes \
 && apt-get install --no-install-recommends --yes \
    g++-mingw-w64-x86-64-posix git ninja-build ccache pipx p7zip-full binutils zstd \
    libvulkan-dev \
 && rm -rf /var/lib/apt/lists/*

# Using `pipx --global` would be better but it's available only since pipx 1.5
RUN PIPX_HOME=/opt/pipx PIPX_BIN_DIR=/usr/local/bin pipx install cmake==3.27.*

WORKDIR /build
RUN mkdir src cache out artifacts && chmod a+rwx cache out artifacts

# Fetch library dependencies and configure resolution
RUN git clone --depth=1 https://github.com/beyond-all-reason/mingwlibs64.git
ENV MINGWLIBS=/build/mingwlibs64

# Vulkan headers are platform-independent, but Ubuntu's Vulkan import library
# targets Linux. Generate the MinGW import library for the Windows loader.
RUN cp -r /usr/include/vulkan /usr/include/vk_video "${MINGWLIBS}/include/" \
 && { \
      echo 'LIBRARY vulkan-1.dll'; \
      echo 'EXPORTS'; \
      sed -nE 's/.*VKAPI_CALL (vk[A-Za-z0-9_]+)\(.*/\1/p' \
        /usr/include/vulkan/vulkan_core.h \
        /usr/include/vulkan/vulkan_win32.h; \
    } > /tmp/vulkan-1.def \
 && x86_64-w64-mingw32-dlltool \
      --input-def /tmp/vulkan-1.def \
      --dllname vulkan-1.dll \
      --output-lib "${MINGWLIBS}/lib/libvulkan-1.a" \
 && rm /tmp/vulkan-1.def

# Set up default cmake toolchain
COPY amd64-windows/toolchain.cmake .
ENV CMAKE_TOOLCHAIN_FILE=/build/toolchain.cmake

# Configure ccache caching
COPY all/ccache.conf .
ENV CCACHE_CONFIGPATH=/build/ccache.conf
ENV CMAKE_CXX_COMPILER_LAUNCHER=ccache
ENV CMAKE_C_COMPILER_LAUNCHER=ccache
