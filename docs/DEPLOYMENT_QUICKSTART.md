# Puzzle71 Deployment Quick Start Guide

**Version 2.0.0** • Production Ready • CUDA-Accelerated Bitcoin Puzzle Solver

## 🚀 Quick Start Options

Choose the deployment method that best fits your needs:

### 1. One-Command Quick Start (Recommended)

```bash
# Clone and run
git clone <repository-url>
cd PuzzleKeyhunt
./scripts/deploy_quickstart.sh
```

### 2. Docker Deployment

```bash
# Automated Docker deployment with monitoring
./scripts/deploy_quickstart.sh --docker --enable-monitoring
```

### 3. Manual Production Deployment

```bash
# Full production deployment
./scripts/deploy_production.sh --environment production --gpu-devices all
```

## 📋 System Requirements

### Minimum Requirements
- **OS**: Linux (Ubuntu 20.04+ recommended)
- **GPU**: NVIDIA GPU with Compute Capability 7.5+
- **CUDA**: CUDA Toolkit 11.0+
- **Memory**: 16GB RAM
- **Storage**: 10GB free disk space
- **CPU**: 4+ cores

### Recommended Production Setup
- **OS**: Ubuntu 22.04 LTS
- **GPU**: RTX 3090+ or A100
- **Memory**: 64GB RAM
- **Storage**: 100GB+ NVMe SSD
- **CPU**: 8+ cores

### Docker Requirements
- Docker Engine 20.10+
- NVIDIA Container Toolkit
- GPU support in Docker

## 🔧 Installation Steps

### Step 1: System Preparation

```bash
# Update system packages
sudo apt update && sudo apt upgrade -y

# Install CUDA Toolkit (if not already installed)
# Visit: https://developer.nvidia.com/cuda-downloads

# Install NVIDIA Container Toolkit (for Docker)
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list

sudo apt update
sudo apt install -y nvidia-docker2
sudo systemctl restart docker

# Install required libraries
sudo apt install -y build-essential cmake ninja-build git libsecp256k1-dev
```

### Step 2: Quick Start Deployment

```bash
# Clone repository
git clone <repository-url>
cd PuzzleKeyhunt

# Run quick start
./scripts/deploy_quickstart.sh

# Or with specific options
./scripts/deploy_quickstart.sh --run-tests --docker
```

### Step 3: Verify Installation

```bash
# Test binary
cd build_quickstart
./Puzzle71Solver --version

# Validate ECC operations
./Puzzle71Solver --validate-ecc --iterations 1000

# Run quick scan with sample data
./Puzzle71Solver \
    --config ../config/quickstart.json \
    --range-file ../data/sample_ranges.txt \
    --target-file ../data/sample_targets.txt \
    --device-id 0
```

## 🐳 Docker Deployment

### Basic Docker Deployment

```bash
# Simple deployment
./scripts/docker/deploy.sh

# With monitoring stack
./scripts/docker/deploy.sh --enable-monitoring

# Custom environment
./scripts/docker/deploy.sh --environment staging --gpu-devices 0,1
```

### Docker Compose

```bash
# Start services
cd deployment/docker
docker-compose up -d

# View logs
docker-compose logs -f puzzle71-solver

# Stop services
docker-compose down
```

### Monitoring Stack

When using `--enable-monitoring`, you get:

- **Prometheus**: http://localhost:9090
- **Grafana**: http://localhost:3000 (admin/puzzle71)
- **Node Exporter**: System metrics collection

## 📊 Configuration

### Basic Configuration

The quick start creates `config/quickstart.json`:

```json
{
    "version": "2.0.0",
    "cuda": {
        "devices": "all",
        "architectures": ["75", "86"],
        "optimization_level": "O3"
    },
    "performance": {
        "target_utilization": 90,
        "batch_size": 1000000,
        "max_threads_per_block": 1024
    },
    "validation": {
        "constitutional_compliance": {
            "enabled": true,
            "version": "5.5"
        }
    }
}
```

### Environment Variables

```bash
# GPU selection
export CUDA_VISIBLE_DEVICES=0,1

# Performance tuning
export PUZZLE71_BATCH_SIZE=2000000
export PUZZLE71_THREADS_PER_BLOCK=1024

# Logging
export PUZZLE71_LOG_LEVEL=INFO
export PUZZLE71_LOG_FILE=/var/log/puzzle71.log
```

## 🎯 Usage Examples

### Basic Private Key Scanning

```bash
# Scan a specific range
./Puzzle71Solver \
    --range-start 0x20000000000000000 \
    --range-end 0x3ffffffffffffffff \
    --target-address 1Puzzle71xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx \
    --device-id 0

# Use range file
./Puzzle71Solver \
    --range-file data/my_ranges.txt \
    --target-file data/my_targets.txt \
    --config config/quickstart.json
```

### Validation and Testing

```bash
# ECC validation (compare with CPU reference)
./Puzzle71Solver --validate-ecc --iterations 10000

# Deterministic replay validation
./Puzzle71Solver --validate-replay --test-cases 1000

# Constitutional compliance validation
./Puzzle71Solver --validate-constitutional --strict-mode

# Performance benchmarking
./Puzzle71Solver --benchmark --duration 300 --device-id 0
```

### Advanced Configuration

```bash
# Multi-GPU scanning
./Puzzle71Solver \
    --config config/production.json \
    --range-file data/puzzle71_ranges.txt \
    --target-file data/puzzle71_targets.txt \
    --multi-gpu \
    --gpu-devices 0,1,2,3

# With checkpointing
./Puzzle71Solver \
    --config config/production.json \
    --checkpoint-file data/checkpoint.dat \
    --auto-resume

# Verbose logging
./Puzzle71Solver \
    --config config/production.json \
    --log-level DEBUG \
    --log-file logs/puzzle71_debug.log \
    --verbose
```

## 🔍 Monitoring and Health

### Health Checks

```bash
# Built-in health check
./Puzzle71Solver --health-check

# Docker health check
docker exec puzzle71-solver /opt/puzzle71/scripts/docker-entrypoint.sh --health-check

# System health
./scripts/health_check.sh
```

### Performance Monitoring

```bash
# Real-time metrics
tail -f monitoring/metrics/metrics_$(date +%Y%m%d).jsonl

# GPU utilization
watch -n 1 nvidia-smi

# Application logs
tail -f logs/puzzle71.log
```

### Prometheus Metrics

When using Docker with monitoring, metrics are available at:
- **Endpoint**: http://localhost:8080/metrics
- **Grafana Dashboard**: http://localhost:3000

Key metrics:
- `puzzle71_gpu_utilization_percent`
- `puzzle71_memory_usage_percent`
- `puzzle71_keys_per_second`
- `puzzle71_validation_errors_total`

## 🛠️ Troubleshooting

### Common Issues

#### Build Failures
```bash
# Clean build directory
rm -rf build_*
./scripts/deploy_quickstart.sh --skip-build=false

# Check CUDA installation
nvcc --version
nvidia-smi
```

#### GPU Not Detected
```bash
# Check GPU availability
nvidia-smi
lspci | grep -i nvidia

# Check CUDA runtime
./Puzzle71Solver --device-query
```

#### Performance Issues
```bash
# Check GPU utilization
nvidia-smi dmon -s u

# Profile application
./Puzzle71Solver --profile --device-id 0

# Check memory usage
./Puzzle71Solver --memory-stats
```

#### Docker Issues
```bash
# Check Docker GPU support
docker run --rm --gpus all nvidia/cuda:12.2-base nvidia-smi

# View container logs
docker logs puzzle71-solver

# Restart container
docker restart puzzle71-solver
```

### Getting Help

1. **Check logs**: `tail -f logs/puzzle71.log`
2. **Run health check**: `./Puzzle71Solver --health-check`
3. **Validate configuration**: `./Puzzle71Solver --validate-config`
4. **Consult documentation**: See the Documentation section below

## 📚 Documentation

- **[Technical Summary](TECHNICAL_DEBT_IMPLEMENTATION_SUMMARY.md)** - Complete implementation overview
- **[API Reference v2.0](docs/API_REFERENCE_V2.md)** - Comprehensive API documentation
- **[Deployment Guide v2.0](docs/DEPLOYMENT_GUIDE_V2.md)** - Production deployment guide
- **[Constitutional Constraints v5.5](docs/puzzle71_constraints_v5.5.md)** - Compliance requirements

## 🔗 Next Steps

1. **Run Validation**: `./Puzzle71Solver --validate-ecc --iterations 10000`
2. **Test Scanning**: Use sample data files to test basic functionality
3. **Configure Ranges**: Create your own range and target files
4. **Deploy to Production**: Use production deployment script for production use
5. **Set Up Monitoring**: Configure monitoring stack for production environments

## 📞 Support

For issues and questions:

1. Check the troubleshooting section above
2. Review the comprehensive documentation
3. Check GPU compatibility and CUDA installation
4. Validate system requirements

---

**🎯 Happy Hunting!** Puzzle71 v2.0.0 - Production Ready Bitcoin Puzzle Solver