FROM ubuntu:22.04

RUN \
    dpkg --add-architecture i386 && \
    apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
        ca-certificates \
        cmake \
        ninja-build \
        clang \
        gcc-multilib \
        g++-multilib \
        libc6:i386 \
        libstdc++6:i386 \
        libssl-dev:i386 \
        zlib1g-dev:i386 \
    && \
    useradd -m user

COPY docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh

USER user

ENV CC=/usr/bin/clang \
    CXX=/usr/bin/clang++ \
    PATH=~/.local/bin:${PATH}

CMD ["/docker-entrypoint.sh"]
