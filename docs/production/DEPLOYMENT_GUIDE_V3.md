# Puzzle71 CUDA Technical Debt Repair System - Deployment Guide v3.0

**Version**: 3.0.0
**Updated**: 2025-10-21
**Branch**: `002-techdebt-repair`
**Status**: Production Ready

## Overview

This guide provides comprehensive instructions for deploying the Puzzle71 CUDA Technical Debt Repair System in production environments. The system has undergone complete technical debt resolution with enterprise-grade quality standards and constitutional v5.5 compliance.

## System Requirements

### Hardware Requirements

#### Minimum System Requirements
- **GPU**: NVIDIA GPU with Compute Capability 3.5+ (Kepler or newer)
- **GPU Memory**: 8GB VRAM minimum
- **System Memory**: 16GB RAM
- **Storage**: 50GB free disk space
- **Network**: Stable internet connection for initial setup

#### Recommended Production System
- **GPU**: NVIDIA RTX 3090, A100, or H100
- **GPU Memory**: 24GB+ VRAM
- **System Memory**: 64GB+ RAM
- **Storage**: 200GB+ NVMe SSD
- **Network**: 10Gbps for distributed deployments

#### Supported GPU Architectures
- **Turing (7.5+)**: RTX 20-series (RTX 2080 Ti, RTX 2070 Super)
- **Ampere (8.0+)**: RTX 30-series (RTX 3090, RTX 3080 Ti)
- **Hopper (9.0+)**: H100, H20
- **Ada Lovelace (8.9+)**: RTX 40-series (RTX 4090, RTX 4080)

### Software Requirements

#### Core Dependencies
- **Operating System**: Ubuntu 20.04 LTS or Ubuntu 22.04 LTS
- **CUDA Toolkit**: 11.8 or newer
- **NVIDIA Driver**: 525.60.13 or newer
- **CMake**: 3.18 or newer
- **Compiler**: GCC 9+ or Clang 10+

#### Runtime Dependencies
- **libyaml-cpp**: 0.6.0+
- **libssl**: 1.1.1+
- **Python**: 3.8+ (for monitoring scripts)
- **Docker**: 20.10+ (for containerized deployment)

#### Optional Dependencies
- **Jenkins**: 2.400+ (for CI/CD)
- **Prometheus**: 2.40+ (for monitoring)
- **Grafana**: 9.0+ (for visualization)

## Installation Methods

### Method 1: Docker Deployment (Recommended)

#### Prerequisites
```bash
# Install Docker and Docker Compose
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER

# Install NVIDIA Container Toolkit
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list

sudo apt-get update && sudo apt-get install -y nvidia-docker2
sudo systemctl restart docker
```

#### Quick Deployment
```bash
# Clone the repository
git clone <repository-url>
cd PuzzleKeyhunt
git checkout 002-techdebt-repair

# Build and run with Docker Compose
docker-compose -f docker-compose.production.yml up -d

# Verify installation
docker-compose -f docker-compose.production.yml exec puzzle71 ./Puzzle71Solver --version
```

#### Production Docker Compose
```bash
# Production deployment with persistent volumes
docker-compose -f docker-compose.production.yml up -d \
  --build \
  --scale puzzle71-worker=2

# Monitor deployment
docker-compose -f docker-compose.production.yml logs -f
```

### Method 2: Native Installation

#### Step 1: Install CUDA Toolkit
```bash
# Download CUDA 11.8
wget https://developer.download.nvidia.com/compute/cuda/11.8.0/local_installers/cuda_11.8.0_520.61.05_linux.run

# Run installer (accept defaults, install toolkit)
sudo sh cuda_11.8.0_520.61.05_linux.run

# Set environment variables
echo 'export PATH=/usr/local/cuda/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
```

#### Step 2: Install Dependencies
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    libgtest-dev \
    libgmock-dev \
    libyaml-cpp-dev \
    libssl-dev \
    python3 \
    python3-pip

# Install GoogleTest
cd /tmp
git clone https://github.com/google/googletest.git
cd googletest
git checkout release-1.11.0
cmake -B build
cmake --build build --target install
sudo ldconfig
cd /
rm -rf /tmp/googletest
```

#### Step 3: Install secp256k1 (CPU Reference)
```bash
cd /tmp
git clone https://github.com/bitcoin-core/secp256k1.git
cd secp256k1
git checkout v0.3.2
./autogen.sh
./configure --enable-module-recovery --disable-tests --disable-benchmark --disable-shared
make -j$(nproc)
sudo make install
sudo ldconfig
cd /
rm -rf /tmp/secp256k1
```

#### Step 4: Build Puzzle71
```bash
# Clone repository
git clone <repository-url>
cd PuzzleKeyhunt
git checkout 002-techdebt-repair

# Configure build
mkdir build && cd build
cmake ../src/KeyhuntCore \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_BENCHMARKS=ON \
    -DCMAKE_CUDA_ARCHITECTURES="75;80;86;89;90" \
    -DENABLE_AGGRESSIVE_OPTIMIZATIONS=ON

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo make install
```

## Configuration

### Configuration Files

#### Main Configuration (`data/config.yaml`)
```yaml
# Puzzle71 Configuration v3.0
# Production settings optimized for performance and stability

system:
  gpu_device_id: 0
  cuda_streams: 4
  thread_block_size: 256
  max_batch_size: 1000000

performance:
  memory_efficiency_target: 95.0
  gpu_utilization_target: 85.0
  max_memory_usage_gb: 22.0
  auto_tune_performance: true

security:
  validate_ecc_operations: true
  use_cpu_reference: true
  precision_tolerance: 1e-10
  enable_deterministic_mode: true

logging:
  level: INFO
  file: logs/puzzle71.log
  max_size_mb: 100
  backup_count: 5

monitoring:
  enable_telemetry: true
  metrics_interval_sec: 30
  performance_baselines: "data/performance_baselines.json"
  health_check_interval_sec: 60
```

#### Private Key Ranges (`data/private_ranges.txt`)
```
# Private key ranges to scan (hexadecimal format)
# Format: start_hex:end_hex:weight

0000000000000000000000000000000000000000000000000000000000000000:00000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff:10
0000100000000000000000000000000000000000000000000000000000000000:00001ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff:5
```

#### Target Addresses (`data/target_addresses.txt`)
```
# Bitcoin addresses to search for
# One address per line

1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
1dice8EMZmqKvrGE4Qc9bUFf9PX3xaYDp
1BoatSLRHtKNngkdXEeobR76b53LETtpyT
```

### Performance Tuning

#### GPU Architecture Optimization
```yaml
# Architecture-specific settings
gpu_architectures:
  Turing:
    thread_block_size: 256
    shared_memory_size: 48KB
    max_registers_per_thread: 64

  Ampere:
    thread_block_size: 256
    shared_memory_size: 100KB
    max_registers_per_thread: 80

  Hopper:
    thread_block_size: 256
    shared_memory_size: 164KB
    max_registers_per_thread: 80
```

#### Memory Configuration
```yaml
memory:
  # Structure-of-Arrays optimization
  use_soa_layout: true

  # Shared memory settings
  shared_memory_pool_size: 2GB
  max_shared_memory_per_block: 48KB

  # Global memory settings
  global_memory_alignment: 128
  memory_coalescing_enabled: true

  # Memory management
  use_unified_memory: false
  enable_memory_pooling: true
```

## Deployment Strategies

### Single Node Deployment
```bash
# Production deployment on single GPU
./Puzzle71Solver \
  --config data/config.yaml \
  --range-file data/private_ranges.txt \
  --targets-file data/target_addresses.txt \
  --output results/output.json \
  --threads 8
```

### Multi-GPU Deployment
```bash
# Deploy across multiple GPUs
CUDA_VISIBLE_DEVICES=0,1,2,3 ./Puzzle71Solver \
  --config data/config.yaml \
  --multi-gpu \
  --gpu-devices 0,1,2,3 \
  --distributed-mode
```

### Cluster Deployment
```bash
# Kubernetes deployment
kubectl apply -f k8s/puzzle71-deployment.yaml

# Docker Swarm deployment
docker stack deploy -c docker-compose.cluster.yml puzzle71
```

## Monitoring and Operations

### Health Monitoring
```bash
# System health check
./Puzzle71Solver --health-check

# Performance monitoring
./Puzzle71Solver --monitor --interval 30

# Resource usage monitoring
nvidia-smi dmon -s pucvmet -d 1
```

### Log Management
```bash
# View real-time logs
tail -f logs/puzzle71.log

# Rotate logs
logrotate -f /etc/logrotate.d/puzzle71

# Archive old logs
tar -czf logs/archive/puzzle71-$(date +%Y%m%d).tar.gz logs/puzzle71.log.*
```

### Performance Metrics
```bash
# Benchmark current performance
./Puzzle71Solver --benchmark --duration 300

# Compare against baselines
./Puzzle71Solver --validate-performance --baseline data/performance_baselines.json

# Generate performance report
./Puzzle71Solver --report --output performance_report.json
```

## Security Considerations

### Access Control
```bash
# Create dedicated user
sudo useradd -r -s /bin/false puzzle71
sudo chown -R puzzle71:puzzle71 /opt/puzzle71

# File permissions
chmod 600 data/config.yaml
chmod 644 data/target_addresses.txt
chmod 644 data/private_ranges.txt
```

### Network Security
```yaml
# Firewall configuration
firewall:
  allowed_ports:
    - 22    # SSH
    - 8080  # Web interface (optional)
    - 9090  # Metrics endpoint (optional)

  blocked_ports:
    - 3306  # MySQL (if not used)
    - 5432  # PostgreSQL (if not used)
```

### Data Protection
```yaml
# Encryption settings
encryption:
  encrypt_checkpoints: true
  encrypt_output: true
  key_rotation_interval_days: 90

  # Backup encryption
  backup_encryption: AES-256
  backup_retention_days: 30
```

## Troubleshooting

### Common Issues

#### CUDA Device Not Found
```bash
# Check GPU availability
nvidia-smi

# Check CUDA installation
nvcc --version

# Check driver version
nvidia-smi --query-gpu=driver_version --format=csv
```

#### Memory Issues
```bash
# Check GPU memory usage
nvidia-smi --query-gpu=memory.used,memory.total --format=csv

# Monitor memory usage during execution
watch -n 1 'nvidia-smi --query-gpu=memory.used,memory.total,utilization.gpu --format=csv'
```

#### Performance Issues
```bash
# Check GPU utilization
nvidia-smi dmon -s u

# Profile kernel execution
nvprof ./Puzzle71Solver [options]

# Detailed profiling with Nsight Compute
nsys profile ./Puzzle71Solver [options]
```

### Debug Mode
```bash
# Enable debug logging
./Puzzle71Solver --log-level DEBUG --debug-output debug.log

# Run with sanitizers
export CUDA_LAUNCH_BLOCKING=1
./Puzzle71Solver [options]

# Enable error checking
export CUDA_LAUNCH_BLOCKING=1
export CUDA_ERROR_LEVEL=2
```

## Maintenance

### Regular Maintenance Tasks
```bash
# Weekly maintenance script
#!/bin/bash
# maintenance.sh

# Update baselines
./Puzzle71Solver --update-baselines

# Clean old logs
find logs/ -name "*.log" -mtime +7 -delete

# Validate configuration
./Puzzle71Solver --validate-config

# Performance health check
./Puzzle71Solver --health-check --thresholds 0.9
```

### Backup Procedures
```bash
# Backup configuration
tar -czf backup/config-$(date +%Y%m%d).tar.gz data/

# Backup results
tar -czf backup/results-$(date +%Y%m%d).tar.gz results/

# Backup logs
tar -czf backup/logs-$(date +%Y%m%d).tar.gz logs/
```

### Update Procedures
```bash
# Update system
sudo apt-get update && sudo apt-get upgrade

# Update CUDA (if needed)
# Follow NVIDIA's upgrade guide

# Update application
git pull origin 002-techdebt-repair
cd build
make clean
cmake ../src/KeyhuntCore -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest --output-on-failure
```

## Performance Benchmarks

### Expected Performance by GPU Architecture

| GPU Model | Architecture | Throughput (keys/sec) | Memory Efficiency | GPU Utilization |
|-----------|--------------|----------------------|------------------|-----------------|
| RTX 2080 Ti | Turing 7.5 | 1.0G | 92% | 85% |
| RTX 3090 | Ampere 8.6 | 2.0G | 94% | 88% |
| A100 | Ampere 8.0 | 3.5G | 95% | 90% |
| H100 | Hopper 9.0 | 4.0G | 96% | 92% |

### Performance Validation
```bash
# Validate performance meets targets
./Puzzle71Solver --validate-performance \
  --min-throughput 1000000 \
  --min-memory-efficiency 90.0 \
  --min-gpu-utilization 80.0
```

## Support

### Getting Help
- **Documentation**: See `/docs/` directory for detailed technical documentation
- **Issues**: Report issues via GitHub issue tracker
- **Community**: Join our Discord/Slack community for support
- **Enterprise**: Contact enterprise-support@puzzle71.com for enterprise support

### Contributing
- **Code**: Follow contribution guidelines in `CONTRIBUTING.md`
- **Testing**: Ensure all tests pass before submitting
- **Documentation**: Update documentation for new features

---

**Deployment Guide v3.0**
**Last Updated**: 2025-10-21
**Next Review**: 2025-11-21