# Puzzle71 CUDA Technical Debt Repair System
# Multi-stage Docker build for CI/CD environment
# Based on Ubuntu 20.04 with CUDA 11.8 support

# Build stage
FROM nvidia/cuda:11.8-devel-ubuntu20.04 AS builder

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV CUDA_HOME=/usr/local/cuda
ENV PATH=$PATH:/usr/local/cuda/bin
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    pkg-config \
    libgtest-dev \
    libgmock-dev \
    libyaml-cpp-dev \
    libssl-dev \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install GoogleTest
RUN cd /tmp && \
    git clone https://github.com/google/googletest.git && \
    cd googletest && \
    git checkout release-1.11.0 && \
    cmake -B build && \
    cmake --build build --target install && \
    cd / && rm -rf /tmp/googletest

# Install secp256k1 for CPU reference validation
RUN cd /tmp && \
    git clone https://github.com/bitcoin-core/secp256k1.git && \
    cd secp256k1 && \
    git checkout v0.3.2 && \
    ./autogen.sh && \
    ./configure --enable-module-recovery --disable-tests --disable-benchmark --disable-shared && \
    make -j$(nproc) && \
    make install && \
    cd / && rm -rf /tmp/secp256k1

# Create build directory
WORKDIR /build

# Copy source code
COPY . /src/PuzzleKeyhunt/
WORKDIR /build

# Configure build
RUN cmake /src/PuzzleKeyhunt/src/KeyhuntCore \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON \
    -DCMAKE_CUDA_ARCHITECTURES="75;80;86;89;90" \
    -DENABLE_AGGRESSIVE_OPTIMIZATIONS=ON

# Build the project
RUN make -j$(nproc)

# Run tests to ensure build quality
RUN ctest --output-on-failure

# Production stage
FROM nvidia/cuda:11.8-runtime-ubuntu20.04 AS production

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV CUDA_HOME=/usr/local/cuda
ENV PATH=$PATH:/usr/local/cuda/bin
ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda/lib64

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    wget \
    curl \
    libyaml-cpp0.6 \
    libssl1.1 \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install Python monitoring tools
RUN pip3 install psutil py-cpuinfo

# Create application user
RUN useradd -m -u 1000 puzzle71 && \
    mkdir -p /app/data && \
    mkdir -p /app/logs && \
    mkdir -p /app/telemetry && \
    chown -R puzzle71:puzzle71 /app

# Copy built binaries from builder stage
COPY --from=builder /build/Puzzle71Solver /app/
COPY --from=builder /build/tests/* /app/tests/ 2>/dev/null || true
COPY --from=builder /src/PuzzleKeyhunt/data/ /app/data/
COPY --from=builder /src/PuzzleKeyhunt/scripts/ /app/scripts/

# Set working directory
WORKDIR /app

# Switch to non-root user
USER puzzle71

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD ./Puzzle71Solver --version || exit 1

# Expose data directory
VOLUME ["/app/data"]

# Default command
ENTRYPOINT ["./Puzzle71Solver"]
CMD ["--help"]

# Labels
LABEL maintainer="Puzzle71 Team"
LABEL version="1.0.0"
LABEL description="Puzzle71 CUDA Technical Debt Repair System"
LABEL cuda.version="11.8"
LABEL cmake.version="3.18+"