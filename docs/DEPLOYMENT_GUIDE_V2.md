# Puzzle71 CUDA Solver v2.0 Deployment Guide

**Version**: 2.0 (Technical Debt Complete)
**Date**: 2025-10-20
**Status**: Production Ready

## Overview

This guide provides comprehensive deployment instructions for the Puzzle71 CUDA Solver v2.0, which has completed technical debt repair and architectural modernization. The system now uses unified modules, constitutional v5.5 compliance, and optimized CUDA kernels for high-performance Bitcoin private key scanning.

## System Requirements

### Hardware Requirements

#### Minimum Requirements
- **GPU**: NVIDIA GPU with Compute Capability 3.5+
- **GPU Memory**: 8GB VRAM minimum
- **System RAM**: 16GB minimum
- **Storage**: 100GB free disk space

#### Recommended Requirements
- **GPU**: NVIDIA RTX 2080 Ti or better (RTX 3090/4090/A100 recommended)
- **GPU Memory**: 16GB+ VRAM
- **System RAM**: 32GB+
- **Storage**: 500GB+ SSD
- **CPU**: Multi-core processor (8+ cores recommended)

### Software Requirements

#### Operating System
- **Linux**: Ubuntu 18.04+ (recommended)
- **Docker**: 20.10+ with GPU support
- **CUDA Toolkit**: 11.0+ or higher

#### Development Tools
- **CMake**: 3.18+
- **C++ Compiler**: GCC 9+ or Clang 10+
- **Git**: 2.25+
- **Python**: 3.8+ (for build scripts)

#### CUDA Dependencies
- **NVIDIA Driver**: 470+ or newer
- **cuDNN**: 8.0+ (optional for enhanced performance)
- **NCCL**: 2.7+ (for multi-GPU support)

## Pre-Deployment Checklist

### 1. Environment Validation

```bash
# Check CUDA installation
nvcc --version
nvidia-smi

# Check GPU capabilities
nvidia-smi --query-gpu=name,compute_cap --format=csv

# Check system resources
free -h
df -h

# Verify Docker GPU support
docker run --rm --gpus all nvidia/cuda:11.0-base nvidia-smi
```

### 2. Dependency Installation

```bash
# Update system packages
sudo apt-get update && sudo apt-get upgrade -y

# Install CUDA Toolkit (if not already installed)
wget https://developer.download.nvidia.com/compute/cuda/11.8.0/local_installers/cuda_11.8.0_520.61.05_linux.run
sudo sh cuda_11.8.0_520.61.05_linux.run

# Install build dependencies
sudo apt-get install -y cmake build-essential git pkg-config

# Install testing dependencies
sudo apt-get install -y libgtest-dev libgmock-dev

# Install additional tools
sudo apt-get install -y jq curl wget htop
```

### 3. Source Code Preparation

```bash
# Clone the repository
git clone <repository-url>
cd PuzzleKeyhunt
git checkout 002-techdebt-repair

# Verify the branch
git status
git log --oneline -5
```

## Build Instructions

### 1. Build Configuration

```bash
# Create build directory
mkdir build && cd build

# Configure CMake with optimization flags
cmake ../src/KeyhuntCore \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CUDA_ARCHITECTURES="75;80;86;89;90" \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON \
    -DENABLE_AGGRESSIVE_OPTIMIZATIONS=ON

# Alternative: Debug build
cmake ../src/KeyhuntCore \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CUDA_ARCHITECTURES="75;80;86;89;90" \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON
```

### 2. Compilation

```bash
# Build with all available cores
make -j$(nproc)

# Verify build success
ls -la Puzzle71Solver
ls -la tests/
```

### 3. Testing and Validation

```bash
# Run unit tests
make test

# Run integration tests
./tests/integration/test_unified_modules_integration

# Run performance benchmarks
make benchmark

# Validate constitutional compliance
./scripts/validate_constitutional_compliance.sh

# Run ECC validation
./tests/validation/test_ecc_validation
```

## Configuration

### 1. Configuration Files

Create the necessary configuration files:

```bash
# Create configuration directories
mkdir -p config data

# Create main configuration
cp examples/puzzle71.yaml.example config/puzzle71.yaml

# Create target addresses file
echo "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa" > data/target_addresses.txt

# Create private key ranges file
echo "2000000000000000000" > data/private_ranges.txt
echo "2000010000000000" >> data/private_ranges.txt
```

### 2. Configuration Validation

```bash
# Validate configuration file
./build/tools/config_validator config/puzzle71.yaml

# Check required configuration sections
./scripts/validate_config.sh config/puzzle71.yaml
```

## Docker Deployment

### 1. Production Docker Image

```dockerfile
# Dockerfile.production
FROM nvidia/cuda:11.8-devel-ubuntu20.04

# Set environment variables
ENV CUDA_ARCHITECTURES=75;80;86;89;90
ENV CMAKE_BUILD_TYPE=Release
ENV BUILD_TESTS=OFF
ENV BUILD_BENCHMARKS=ON

# Install system dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    git \
    pkg-config \
    libgtest-dev \
    libgmock-dev \
    jq \
    curl \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Create build directory
WORKDIR /app

# Copy source code
COPY . .

# Build the application
RUN mkdir build && \
    cd build && \
    cmake ../src/KeyhuntCore \
        -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
        -DCMAKE_CUDA_ARCHITECTURES=$CUDA_ARCHITECTURES \
        -DBUILD_TESTS=$BUILD_TESTS \
        -DBUILD_BENCHMARKS=$BUILD_BENCHMARKS \
        -DENABLE_AGGRESSIVE_OPTIMIZATIONS=ON && \
    make -j$(nproc)

# Create non-root user
RUN useradd -m -u 1000 puzzle71 && \
    chown -R puzzle71:puzzle71 /app
USER puzzle71

# Set working directory
WORKDIR /app/build

# Expose ports (if needed)
EXPOSE 8080

# Default command
CMD ["./Puzzle71Solver", "--help"]
```

### 2. Build Docker Image

```bash
# Build production image
docker build -f Dockerfile.production -t puzzle71-solver:v2.0 .

# Verify image
docker images | grep puzzle71-solver
```

### 3. Docker Compose Deployment

```yaml
# docker-compose.yml
version: '3.8'

services:
  puzzle71-solver:
    image: puzzle71-solver:v2.0
    container_name: puzzle71-solver
    restart: unless-stopped
    environment:
      - CUDA_VISIBLE_DEVICES=0
      - LOG_LEVEL=INFO
    volumes:
      - ./config:/app/config:ro
      - ./data:/app/data:ro
      - ./results:/app/results
      - ./logs:/app/logs
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]
    command: >
      --config /app/config/puzzle71.yaml
      --keyspace-file /app/data/private_ranges.txt
      --target-addresses /app/data/target_addresses.txt
      --output /app/results/results.json
      --verbose

  monitoring:
    image: nvidia/cuda:11.8-base
    container_name: puzzle71-monitoring
    restart: unless-stopped
    environment:
      - CUDA_VISIBLE_DEVICES=0
    volumes:
      - ./scripts:/app/scripts:ro
      - ./logs:/app/logs:ro
    command: >
      bash -c "while true; do
        nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total --format=csv,noheader,nounits >> /app/logs/gpu_metrics.log
        sleep 10
      done"
    depends_on:
      - puzzle71-solver
```

### 4. Run Docker Deployment

```bash
# Start with Docker Compose
docker-compose up -d

# Monitor logs
docker-compose logs -f puzzle71-solver

# Check status
docker-compose ps
```

## Production Deployment

### 1. System Service Setup

```bash
# Create systemd service file
sudo tee /etc/systemd/system/puzzle71-solver.service > /dev/null <<EOF
[Unit]
Description=Puzzle71 CUDA Solver Service
After=network.target docker.service
Requires=docker.service

[Service]
Type=simple
ExecStart=/usr/bin/docker-compose -f /opt/puzzle71/docker-compose.yml up
ExecStop=/usr/bin/docker-compose -f /opt/puzzle71/docker-compose.yml down
TimeoutStartSec=300
TimeoutStopSec=120
Restart=always
RestartSec=10
User=puzzle71
Group=puzzle71
WorkingDirectory=/opt/puzzle71

[Install]
WantedBy=multi-user.target
EOF

# Reload systemd and enable service
sudo systemctl daemon-reload
sudo systemctl enable puzzle71-solver.service
```

### 2. Production Configuration

```yaml
# config/production.yaml
deterministic_config:
  version: "5.5"
  kernel_launch:
    grid_dim: 2048
    block_dim: 512
    points_per_thread: 16
    shared_mem_bytes: 98304
  rng:
    algorithm: "xorshift64"
    base_seed: 0x123456789ABCDEF0
    derivation: "replay_seed + blockIdx.x * blockDim.x + threadIdx.x"
  memory_layout:
    scalar_format: "little_endian_u32x8"
    point_format: "jacobian_projective"

performance:
  checkpoint_interval_sec: 1800
  telemetry_interval_sec: 1
  benchmark:
    warmup_iterations: 5
    measurement_iterations: 10
    discard_warmup: true

operator:
  default_id: "production"
  default_purpose: "bitcoin-puzzle-solving"
  require_explicit: true

digest:
  algorithm: "SHA-256"
  sla_ms: 250
  alert_on_violation: true

# Production-specific settings
production:
  max_runtime_hours: 24
  auto_checkpoint: true
  monitor_performance: true
  alert_thresholds:
    gpu_utilization_min: 80
    memory_efficiency_min: 90
    error_rate_max: 0.1
```

### 3. Monitoring and Logging

```bash
# Create log directories
sudo mkdir -p /opt/puzzle71/{logs,results,config,data}
sudo chown puzzle71:puzzle71 /opt/puzzle71

# Setup log rotation
sudo tee /etc/logrotate.d/puzzle71 << EOF
/opt/puzzle71/logs/*.log {
    daily
    rotate 30
    compress
    delaycompress
    missingok
    notifempty
    create 644 puzzle71 puzzle71
    postrotate
        docker-compose -f /opt/puzzle71/docker-compose.yml restart
    endscript
}
EOF
```

## Performance Tuning

### 1. GPU Optimization

```bash
# Set GPU performance mode
sudo nvidia-smi -i 0 -pm 1

# Disable power saving
sudo nvidia-smi -i 0 -ac 8740

# Set memory clock
sudo nvidia-smi -i 0 -ac 8776

# Verify settings
nvidia-smi -i 0 --query-gpu=power.limit,clocks.memory,clocks.graphics --format=csv
```

### 2. System Optimization

```bash
# Increase process limits
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 65536" | sudo tee -a /etc/security/limits.conf

# Optimize kernel parameters
echo "vm.swappiness=10" | sudo tee -a /etc/sysctl.conf
echo "vm.dirty_ratio=15" | sudo tee -a /etc/sysctl.conf
echo "vm.dirty_background_ratio=5" | sudo tee -a /etc/sysctl.conf

# Apply kernel parameters
sudo sysctl -p
```

### 3. CUDA Optimization

```bash
# Set CUDA environment variables
export CUDA_VISIBLE_DEVICES=0
export CUDA_DEVICE_ORDER=PCI_BUS_ID
export CUDA_CACHE_DISABLE=0
export CUDA_CACHE_MAXSIZE=2147483648

# Optimize for production
export CUDA_LAUNCH_BLOCKING=1
export CUDA_MANAGED_FORCE_DEVICE_ALLOC=1
```

## Multi-GPU Deployment

### 1. Multi-GPU Configuration

```yaml
# config/multi-gpu.yaml
deterministic_config:
  version: "5.5"
  kernel_launch:
    grid_dim: 4096
    block_dim: 256
    points_per_thread: 8
    shared_mem_bytes: 49152
  multi_gpu:
    enabled: true
    device_count: 4
    load_balancing: "round_robin"
    communication: "nccl"

performance:
  multi_gpu_optimization: true
  inter_gpu_bandwidth: "max"
  load_balancing_strategy: "dynamic"
```

### 2. Docker Compose Multi-GPU

```yaml
# docker-compose.multi-gpu.yml
version: '3.8'

services:
  puzzle71-solver-gpu0:
    image: puzzle71-solver:v2.0
    environment:
      - CUDA_VISIBLE_DEVICES=0
      - GPU_ID=0
      - TOTAL_GPUS=4
    volumes:
      - ./config:/app/config:ro
      - ./data:/app/data:ro
      - ./results:/app/results
    command: >
      --config /app/config/multi-gpu.yaml
      --gpu-id 0
      --total-gpus 4
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              device_ids: ['0']
              capabilities: [gpu]

  puzzle71-solver-gpu1:
    image: puzzle71-solver:v2.0
    environment:
      - CUDA_VISIBLE_DEVICES=1
      - GPU_ID=1
      - TOTAL_GPUS=4
    volumes:
      - ./config:/app/config:ro
      - ./data:/app/data:ro
      - ./results:/app/results
    command: >
      --config /app/config/multi-gpu.yaml
      --gpu-id 1
      --total-gpus 4
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              device_ids: ['1']
              capabilities: [gpu]

  # Add more services for additional GPUs...
```

## Security Hardening

### 1. Container Security

```bash
# Create non-root user in Docker
RUN groupadd -r puzzle71 && useradd -r -g puzzle71 puzzle71

# Set proper file permissions
RUN chmod -R 755 /app && chown -R puzzle71:puzzle71 /app

# Remove unnecessary packages
RUN apt-get autoremove -y && apt-get clean
```

### 2. Network Security

```yaml
# docker-compose.security.yml
version: '3.8'

services:
  puzzle71-solver:
    image: puzzle71-solver:v2.0
    networks:
      - puzzle71-internal
    deploy:
      resources:
        limits:
          memory: 16G
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]
    security_opt:
      - no-new-privileges:true
    cap_drop:
      - ALL
    cap_add:
      - CHOWN
      - SETGID
      - SETUID

networks:
  puzzle71-internal:
    driver: bridge
    internal: true
```

### 3. File System Security

```bash
# Secure configuration files
chmod 600 config/*.yaml
chmod 644 data/target_addresses.txt
chmod 644 data/private_ranges.txt

# Secure results directory
chmod 700 results/
chmod 700 logs/

# Set up proper ownership
chown puzzle71:puzzle71 config/ data/ results/ logs/
```

## Troubleshooting

### Common Issues

#### 1. CUDA Out of Memory

```bash
# Check GPU memory usage
nvidia-smi

# Reduce batch size in configuration
# Edit config/puzzle71.yaml and reduce points_per_thread

# Monitor memory during execution
watch -n 1 nvidia-smi
```

#### 2. Poor GPU Utilization

```bash
# Check GPU utilization
nvidia-smi dmon -s u

# Verify kernel launch parameters
# Ensure grid_dim and block_dim are optimal for your GPU

# Profile kernels
./scripts/analyze_profiling.sh 0 eccScalarMulKernel
```

#### 3. Configuration Errors

```bash
# Validate configuration
./scripts/validate_config.sh config/puzzle71.yaml

# Check configuration syntax
python3 -c "import yaml; yaml.safe_load(open('config/puzzle71.yaml'))"

# Check required sections
grep -E "deterministic_config|performance|operator" config/puzzle71.yaml
```

#### 4. Performance Regression

```bash
# Run performance validation
./scripts/validate_constitutional_compliance.sh

# Check recent changes
git log --oneline -10

# Rebuild with optimizations
make clean && make -j$(nproc)
```

### Debug Mode

```bash
# Enable debug logging
export PUZZLE71_DEBUG=1
export LOG_LEVEL=DEBUG

# Run with verbose output
./Puzzle71Solver --verbose --debug

# Use CUDA debugging tools
cuda-gdb ./build/Puzzle71Solver
cuda-memcheck ./build/Puzzle71Solver
```

## Monitoring and Maintenance

### 1. Health Checks

```bash
#!/bin/bash
# health_check.sh

echo "=== Puzzle71 Solver Health Check ==="

# Check if service is running
if ! pgrep -f "Puzzle71Solver" > /dev/null; then
    echo "❌ Puzzle71Solver is not running"
    exit 1
fi

# Check GPU status
if ! nvidia-smi > /dev/null 2>&1; then
    echo "❌ NVIDIA GPU not available"
    exit 1
fi

# Check memory usage
MEMORY_USAGE=$(nvidia-smi --query-gpu=memory.used,memory.total --format=csv,noheader,nounits)
echo "✅ GPU Memory Usage: $MEMORY_USAGE"

# Check configuration
if [ ! -f "config/puzzle71.yaml" ]; then
    echo "❌ Configuration file missing"
    exit 1
fi

echo "✅ All health checks passed"
```

### 2. Performance Monitoring

```bash
# Continuous monitoring script
#!/bin/bash
# monitor_performance.sh

while true; do
    TIMESTAMP=$(date +%Y-%m-%d_%H:%M:%S)

    # GPU utilization
    GPU_UTIL=$(nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits)

    # Memory usage
    MEMORY_USED=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits)
    MEMORY_TOTAL=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader,nounits)

    # Log metrics
    echo "$TIMESTAMP,gpu_utilization,$GPU_UTIL,memory_used,$MEMORY_USED,memory_total,$MEMORY_TOTAL" >> logs/performance_monitoring.log

    # Alert on issues
    if [ "$GPU_UTIL" -lt 80 ]; then
        echo "⚠️  Low GPU utilization: $GPU_UTIL%" | logger -t puzzle71
    fi

    sleep 60
done
```

### 3. Log Analysis

```bash
# Analyze performance logs
tail -f logs/performance_monitoring.log | grep -E "(Low|High|Error|Alert)"

# Analyze error logs
grep -i error logs/*.log | tail -20

# Generate performance report
./scripts/generate_performance_report.sh logs/ performance_report_$(date +%Y%m%d).html
```

## Backup and Recovery

### 1. Configuration Backup

```bash
#!/bin/bash
# backup_config.sh

BACKUP_DIR="/backup/puzzle71/$(date +%Y%m%d)"
mkdir -p "$BACKUP_DIR"

# Backup configuration files
cp -r config/ "$BACKUP_DIR/"
cp -r data/ "$BACKUP_DIR/"
cp docker-compose*.yml "$BACKUP_DIR/" 2>/dev/null || true

# Backup important scripts
cp -r scripts/ "$BACKUP_DIR/"
cp -r docs/ "$BACKUP_DIR/"

echo "Configuration backed up to: $BACKUP_DIR"
```

### 2. Results Backup

```bash
# Backup scan results
rsync -av --progress results/ "/backup/puzzle71/results/$(date +%Y%m%d)/"

# Backup logs
rsync -av --progress logs/ "/backup/puzzle71/logs/$(date +%Y%m%d)/"

# Create backup manifest
echo "Backup created: $(date)" > "/backup/puzzle71/$(date +%Y%m%d)/manifest.txt"
```

## Scaling and High Availability

### 1. Horizontal Scaling

```yaml
# docker-compose.scale.yml
version: '3.8'

services:
  puzzle71-solver:
    image: puzzle71-solver:v2.0
    environment:
      - INSTANCE_ID=${INSTANCE_ID}
      - CLUSTER_SIZE=${CLUSTER_SIZE}
    deploy:
      replicas: 4
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]

  load-balancer:
    image: nginx:alpine
    ports:
      - "80:80"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf:ro
    depends_on:
      - puzzle71-solver
```

### 2. High Availability Setup

```bash
# Health check for HA
#!/bin/bash
# ha_health_check.sh

NODES=("node1" "node2" "node3" "node4")

for node in "${NODES[@]}"; do
    ssh "$node" "systemctl is-active --quiet puzzle71-solver" && echo "✅ $node: Healthy" || echo "❌ $node: Unhealthy"
done

# Check cluster status
docker node ls 2>/dev/null || echo "Docker Swarm not initialized"
```

## Support and Maintenance

### Contact Information

- **Documentation**: [Technical Implementation Summary](TECHNICAL_DEBT_IMPLEMENTATION_SUMMARY.md)
- **Performance Guide**: [Performance Monitoring Guide](PERFORMANCE_MONITORING_GUIDE.md)
- **Compliance**: [Constitutional Compliance](puzzle71_constraints_v5.5.md)
- **Issues**: GitHub Issues repository
- **Community**: Discord server (if available)

### Maintenance Schedule

- **Daily**: Performance monitoring and log rotation
- **Weekly**: Security updates and patch management
- **Monthly**: Performance baselines review and updates
- **Quarterly**: Full system audit and optimization review

---

**Version**: 2.0 (Technical Debt Complete)
**Last Updated**: 2025-10-20
**Next Review**: As needed for maintenance and updates