# Puzzle71 Production Deployment Guide v2.0

**Version**: 2.0.0
**Date**: 2025-10-20
**Status**: Production Ready

## Overview

This guide provides comprehensive instructions for deploying Puzzle71 v2.0 in production environments using Docker containers with automated monitoring, backup, and alerting capabilities.

## Quick Start

### Prerequisites

- **Docker Engine** 20.10+ with GPU support
- **Docker Compose** 2.0+
- **NVIDIA Container Toolkit**
- **NVIDIA GPU** with Compute Capability 3.5+
- **System Memory**: 16GB+ minimum
- **Storage**: 100GB+ SSD for optimal performance
- **Linux**: Ubuntu 20.04+ (recommended)

### Installation Commands

```bash
# 1. Install NVIDIA Container Toolkit
distribution=$(. /etc/os-release;echo $ID$VERSION_ID)
curl -s -L https://nvidia.github.io/nvidia-docker/gpgkey | sudo apt-key add -
curl -s -L https://nvidia.github.io/nvidia-docker/$distribution/nvidia-docker.list | sudo tee /etc/apt/sources.list.d/nvidia-docker.list

sudo apt-get update && sudo apt-get install -y nvidia-docker2
sudo systemctl restart docker

# 2. Clone repository and configure
git clone <repository-url>
cd PuzzleKeyhunt
git checkout 002-techdebt-repair

# 3. Setup production environment
mkdir -p config data results logs monitoring

# 4. Copy configuration templates
cp config/production.yaml.template config/production.yaml
# Edit configuration as needed

# 5. Deploy production services
./scripts/deployment/deploy_production.sh
```

## Detailed Deployment Process

### 1. Environment Preparation

#### System Configuration

```bash
# Create puzzle71 user (optional but recommended)
sudo useradd -m -s /bin/bash puzzle71
sudo usermod -aG docker puzzle71

# Create necessary directories
sudo mkdir -p /opt/puzzle71/{config,data,results,logs,monitoring,backups}
sudo chown -R puzzle71:puzzle71 /opt/puzzle71

# Set appropriate permissions
chmod 755 /opt/puzzle71
chmod 700 /opt/puzzle71/{results,logs,backups}
chmod 644 /opt/puzzle71/config/*
chmod 644 /opt/puzzle71/data/*
```

#### GPU Configuration

```bash
# Verify GPU availability
nvidia-smi

# Set GPU persistence mode
sudo nvidia-smi -pm 1

# Optimize GPU performance settings
sudo nvidia-smi -i 0 -ac 8770,1215  # Example for RTX 3090

# Verify Docker GPU support
docker run --rm --gpus all nvidia/cuda:12.1-base nvidia-smi
```

### 2. Configuration Setup

#### Production Configuration

Edit `config/production.yaml` with your specific settings:

```yaml
# Key sections to customize:
deterministic_config:
  kernel_launch:
    grid_dim: 2048        # Adjust based on GPU
    block_dim: 512        # Adjust based on GPU
    points_per_thread: 16 # Adjust for performance

performance:
  targets:
    gpu_utilization_percent: 90
    memory_efficiency_percent: 95
    throughput_mkeys_per_second: 1000

logging:
  level: "INFO"
  file_config:
    path: "/opt/puzzle71/logs/puzzle71.log"
```

#### Data Files Setup

```bash
# Private key ranges (hex format)
echo "2000000000000000000" > data/private_ranges.txt
echo "2000010000000000" >> data/private_ranges.txt

# Target Bitcoin addresses
echo "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa" > data/target_addresses.txt
echo "1BitcoinEaterAddressDDonations" >> data/target_addresses.txt

# Validate configuration
python3 -c "import yaml; yaml.safe_load(open('config/production.yaml'))"
```

### 3. Service Deployment

#### Automated Deployment

```bash
# Run complete production deployment
./scripts/deployment/deploy_production.sh

# The script will:
# - Validate prerequisites
# - Build production Docker image
# - Setup monitoring configuration
# - Deploy all services
# - Wait for health checks
# - Verify deployment
```

#### Manual Deployment Steps

1. **Build Docker Image**
   ```bash
   docker build -t puzzle71-solver:2.0.0 -f Dockerfile.production .
   ```

2. **Deploy Services**
   ```bash
   docker-compose -f docker-compose.production.yml up -d
   ```

3. **Verify Services**
   ```bash
   docker-compose -f docker-compose.production.yml ps

   # Check service health
   docker-compose -f docker-compose.production.yml exec puzzle71-solver /opt/puzzle71/health_check.sh
   ```

## Service Architecture

### Container Services

| Service | Purpose | Port | Resources |
|---------|---------|------|-----------|
| **puzzle71-solver** | Main Bitcoin puzzle solver | 8080 | 16GB RAM, 8 CPU cores |
| **redis** | Caching and session storage | 6379 | 512MB RAM, 0.5 CPU cores |
| **prometheus** | Metrics collection | 9090 | 1GB RAM, 1 CPU core |
| **grafana** | Visualization dashboard | 3000 | 512MB RAM, 0.5 CPU cores |
| **node-exporter** | System metrics | 9100 | 128MB RAM, 0.2 CPU cores |

### Data Flow

```
Private Key Ranges → Puzzle71 Solver → Bitcoin Addresses
                      ↓
                 Metrics → Prometheus → Grafana
                      ↓
                   Results → JSON Files → Backup
```

## Monitoring and Observability

### Health Monitoring

#### Automated Health Checks

```bash
# Run single health check
./scripts/deployment/monitor_production.sh check

# Start continuous monitoring
./scripts/deployment/monitor_production.sh continuous 30 true

# Health check metrics are available at:
# - http://localhost:8080/metrics (application metrics)
# - http://localhost:9100/metrics (system metrics)
# - http://localhost:9090 (Prometheus)
# - http://localhost:3000 (Grafana)
```

#### Health Check Metrics

- **GPU Health**: Temperature, utilization, memory usage
- **System Health**: CPU, memory, disk usage, load average
- **Application Health**: Throughput, response time, error count
- **Service Health**: Container status, endpoint availability

### Alerting Configuration

#### Alert Thresholds (configurable)

| Metric | Warning | Critical |
|--------|---------|----------|
| GPU Temperature | >80°C | >85°C |
| GPU Utilization | <70% | <50% |
| Memory Usage | >80% | >90% |
| Disk Usage | >85% | >90% |
| Throughput | <500 keys/s | <100 keys/s |
| Response Time | >5000ms | >10000ms |

#### Custom Alert Integration

Edit `scripts/deployment/monitor_production.sh` to add custom alert destinations:

```bash
# Example: Add Slack webhook
send_slack_alert() {
    local message="$1"
    curl -X POST -H 'Content-type: application/json' \
        --data "{\"text\":\"$message\"}" \
        "$SLACK_WEBHOOK_URL"
}
```

## Backup and Recovery

### Automated Backups

```bash
# Run manual backup
./scripts/deployment/backup_production.sh

# Schedule automated backups (cron)
crontab -e
# Add: 0 2 * * * /path/to/PuzzleKeyhunt/scripts/deployment/backup_production.sh
```

### Backup Contents

- **Configuration files** with checksums
- **Data files** (private key ranges, target addresses)
- **Results and logs** with timestamps
- **Docker volumes** (Redis, Prometheus, Grafana data)
- **Docker images** for disaster recovery

### Recovery Process

```bash
# 1. Stop services
docker-compose -f docker-compose.production.yml down

# 2. Restore from backup (latest backup)
BACKUP_FILE="/opt/puzzle71/backups/puzzle71_backup_latest.tar.gz"
tar xzf "$BACKUP_FILE" -C /opt/puzzle71/

# 3. Restart services
docker-compose -f docker-compose.production.yml up -d

# 4. Verify recovery
./scripts/deployment/monitor_production.sh check
```

## Performance Optimization

### GPU Optimization

```bash
# Set optimal GPU performance profile
sudo nvidia-smi -i 0 -pm 1              # Persistence mode
sudo nvidia-smi -i 0 -ac 8770,1215      # Clock speeds (GPU-specific)
sudo nvidia-smi -i 0 -pl 300            # Power limit (Watts)

# Monitor GPU performance
watch -n 1 nvidia-smi
```

### System Optimization

```bash
# Optimize kernel parameters
echo 'vm.swappiness=10' | sudo tee -a /etc/sysctl.conf
echo 'vm.dirty_ratio=15' | sudo tee -a /etc/sysctl.conf
sudo sysctl -p

# Set process limits
echo '* soft nofile 65536' | sudo tee -a /etc/security/limits.conf
echo '* hard nofile 65536' | sudo tee -a /etc/security/limits.conf
```

### Application Tuning

Edit `config/production.yaml` for optimal performance:

```yaml
deterministic_config:
  kernel_launch:
    grid_dim: 4096        # Increase for better utilization
    block_dim: 256        # Optimize for GPU architecture
    points_per_thread: 32 # Adjust for memory constraints

performance:
  checkpoint:
    interval_seconds: 900 # 15 minutes for more frequent saves
```

## Security Considerations

### Container Security

- **Non-root user**: Services run as limited `puzzle71` user
- **Minimal attack surface**: Only essential packages installed
- **Read-only volumes**: Configuration and data mounted read-only where possible
- **Resource limits**: CPU and memory constraints enforced
- **Network isolation**: Services isolated in Docker networks

### Host Security

```bash
# Configure firewall
sudo ufw enable
sudo ufw allow 22/tcp    # SSH
sudo ufw allow 3000/tcp  # Grafana (internal only)
sudo ufw allow 9090/tcp  # Prometheus (internal only)

# File permissions
chmod 600 config/production.yaml
chmod 700 results/ logs/ backups/
```

### Operational Security

- **Configuration secrets**: Store sensitive data in environment variables
- **Access control**: Limit Grafana access to internal networks
- **Audit logging**: All operations logged with timestamps
- **Regular updates**: Keep Docker images and dependencies updated

## Troubleshooting

### Common Issues

#### GPU Not Detected

```bash
# Check NVIDIA driver
nvidia-smi

# Check Docker GPU support
docker run --rm --gpus all nvidia/cuda:12.1-base nvidia-smi

# Reinstall NVIDIA Container Toolkit if needed
sudo apt-get remove --purge nvidia-docker2
sudo apt-get install nvidia-docker2
sudo systemctl restart docker
```

#### Services Not Starting

```bash
# Check Docker logs
docker-compose -f docker-compose.production.yml logs puzzle71-solver
docker-compose -f docker-compose.production.yml logs redis
docker-compose -f docker-compose.production.yml logs prometheus

# Check service health
docker-compose -f docker-compose.production.yml ps
```

#### Performance Issues

```bash
# Monitor GPU utilization
watch -n 1 nvidia-smi dmon -s u

# Check system resources
htop
iotop

# Profile application performance
./scripts/deployment/monitor_production.sh continuous 5 false
```

#### Memory Issues

```bash
# Check GPU memory usage
nvidia-smi --query-gpu=memory.used,memory.total --format=csv

# Check system memory
free -h

# Reduce batch size in configuration if needed
# Edit config/production.yaml and reduce points_per_thread
```

### Debug Mode

Enable debug logging for troubleshooting:

```bash
# Set debug mode in configuration
sed -i 's/level: "INFO"/level: "DEBUG"/' config/production.yaml

# Or override with environment variable
docker-compose -f docker-compose.production.yml exec puzzle71-solver \
    /opt/puzzle71/bin/Puzzle71Solver --debug --verbose
```

## Maintenance

### Regular Tasks

#### Daily

- Check service health: `./scripts/deployment/monitor_production.sh check`
- Review logs for errors: `tail -f logs/puzzle71.log`
- Monitor performance metrics

#### Weekly

- Backup configuration and results: `./scripts/deployment/backup_production.sh`
- Update Docker images: `docker-compose pull && docker-compose up -d`
- Review and rotate log files

#### Monthly

- Security updates: `sudo apt-get update && sudo apt-get upgrade`
- Performance baseline review
- Backup retention management

### Scaling Considerations

#### Multi-GPU Deployment

```yaml
# docker-compose.multi-gpu.yml
services:
  puzzle71-solver-gpu0:
    environment:
      - CUDA_VISIBLE_DEVICES=0
      - GPU_ID=0
      - TOTAL_GPUS=2

  puzzle71-solver-gpu1:
    environment:
      - CUDA_VISIBLE_DEVICES=1
      - GPU_ID=1
      - TOTAL_GPUS=2
```

#### High Availability

```yaml
# Load balancer configuration
services:
  nginx:
    image: nginx:alpine
    ports:
      - "80:80"
    volumes:
      - ./nginx.conf:/etc/nginx/nginx.conf:ro
```

## Support and Resources

### Documentation

- **[Technical Implementation Summary](TECHNICAL_DEBT_IMPLEMENTATION_SUMMARY.md)** - Complete technical details
- **[API Reference v2.0](API_REFERENCE_V2.md)** - Complete API documentation
- **[Constitutional Compliance](puzzle71_constraints_v5.5.md)** - System constraints and requirements

### Useful Commands

```bash
# Service management
docker-compose -f docker-compose.production.yml start
docker-compose -f docker-compose.production.yml stop
docker-compose -f docker-compose.production.yml restart

# Log viewing
docker-compose -f docker-compose.production.yml logs -f puzzle71-solver
docker-compose -f docker-compose.production.yml logs -f

# Resource monitoring
docker stats
docker-compose -f docker-compose.production.yml exec puzzle71-solver top
```

### Getting Help

- **Issues**: Create GitHub issues with detailed logs and system information
- **Performance**: Use monitoring dashboard to identify bottlenecks
- **Logs**: Always include recent logs when requesting support

---

**Version**: 2.0.0
**Last Updated**: 2025-10-20
**Next Review**: As needed for maintenance and updates