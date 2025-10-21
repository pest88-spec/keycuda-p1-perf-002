# Puzzle71 Enhanced Deployment Guide v2.0

**Version**: 2.0.0
**Last Updated**: 2025-10-20
**Author**: Puzzle71 Development Team

## 🚀 Overview

This guide provides comprehensive instructions for deploying Puzzle71 v2.0 in production environments using enhanced Docker containers, automated deployment pipelines, and comprehensive monitoring stacks.

## 📋 Table of Contents

1. [Prerequisites](#prerequisites)
2. [Quick Start](#quick-start)
3. [System Requirements](#system-requirements)
4. [Installation](#installation)
5. [Configuration](#configuration)
6. [Deployment Methods](#deployment-methods)
7. [Monitoring and Observability](#monitoring-and-observability)
8. [Security Considerations](#security-considerations)
9. [Performance Tuning](#performance-tuning)
10. [Troubleshooting](#troubleshooting)
11. [Maintenance](#maintenance)

## ✅ Prerequisites

### Hardware Requirements

**Minimum System Requirements:**
- CPU: 8+ cores (Intel i7/AMD Ryzen 7 or better)
- RAM: 16GB DDR4+
- GPU: NVIDIA GPU with CUDA support (Pascal architecture or newer)
- GPU Memory: 8GB+ VRAM
- Storage: 100GB+ SSD (preferably NVMe)
- Network: 1Gbps+ internet connection

**Recommended System Requirements:**
- CPU: 16+ cores (Intel i9/AMD Ryzen 9)
- RAM: 32GB+ DDR4
- GPU: NVIDIA RTX 3090/4090 or A100/H100
- GPU Memory: 24GB+ VRAM
- Storage: 500GB+ NVMe SSD
- Network: 10Gbps+ internet connection

### Software Requirements

**Operating System:**
- Ubuntu 22.04 LTS (recommended)
- Ubuntu 20.04 LTS (supported)
- CentOS 8+/RHEL 8+ (supported)
- Debian 11+ (supported)

**Required Software:**
- NVIDIA CUDA Toolkit 12.1+
- Docker 24.0+
- Docker Compose 2.0+
- Git 2.30+
- Make 4.2+
- CMake 3.25+
- NVIDIA Container Toolkit

### GPU Requirements

**Supported NVIDIA GPUs:**
- Turing Architecture (RTX 20-series, RTX 20x0 Super, RTX TITAN)
- Ampere Architecture (RTX 30-series, RTX 40-series, A100)
- Hopper Architecture (H100, H200)

**CUDA Compute Capability:**
- Minimum: 6.1 (Pascal)
- Recommended: 7.5+ (Turing+)
- Optimal: 8.6+ (Ampere+)

## 🚀 Quick Start

### 1. Clone Repository

```bash
git clone https://github.com/puzzle71/PuzzleKeyhunt.git
cd PuzzleKeyhunt
```

### 2. Quick Deployment

```bash
# Make deployment script executable
chmod +x scripts/deployment/deploy_production_enhanced.sh

# Run quick deployment
./scripts/deployment/deploy_production_enhanced.sh
```

### 3. Verify Deployment

```bash
# Check service status
docker-compose -f docker-compose.production.enhanced.yml ps

# Check logs
docker-compose -f docker-compose.production.enhanced.yml logs -f puzzle71-solver

# Access services
# Grafana: http://localhost:3000 (admin/puzzle71_admin_password_change_me)
# Prometheus: http://localhost:9090
# Metrics: http://localhost:8080/metrics
```

## 📊 System Requirements

### Detailed Hardware Analysis

**GPU Performance Tiers:**

| GPU Model | Architecture | VRAM | Performance | Power |
|-----------|-------------|------|-------------|-------|
| RTX 2080 Ti | Turing | 11GB | 1.0 Gkeys/s | 250W |
| RTX 3090 | Ampere | 24GB | 2.0 Gkeys/s | 350W |
| RTX 4090 | Ada Lovelace | 24GB | 3.0 Gkeys/s | 450W |
| A100 | Ampere | 40GB | 4.0 Gkeys/s | 250W |
| H100 | Hopper | 80GB | 6.0 Gkeys/s | 700W |

**Memory Requirements:**

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| System RAM | 16GB | 32GB+ |
| GPU VRAM | 8GB | 24GB+ |
| Storage | 100GB | 500GB+ |
| Swap Space | 8GB | 16GB |

### Network Requirements

**Bandwidth Requirements:**
- Minimum: 100 Mbps (for small deployments)
- Recommended: 1 Gbps (for production)
- Optimal: 10 Gbps (for large-scale deployments)

**Port Requirements:**

| Port | Service | Purpose |
|------|---------|---------|
| 8080 | Puzzle71 Solver | Metrics API |
| 8081 | Puzzle71 Solver | Health Check |
| 3000 | Grafana | Dashboard |
| 9090 | Prometheus | Metrics Collection |
| 9091 | Pushgateway | Batch Metrics |
| 9093 | Alertmanager | Alert Management |
| 9100 | Node Exporter | System Metrics |
| 9445 | NVIDIA DCGM | GPU Metrics |
| 5601 | Kibana | Log Analysis |
| 9200 | Elasticsearch | Log Storage |
| 6379 | Redis | Caching |

## 🔧 Installation

### 1. System Preparation

```bash
# Update system packages
sudo apt update && sudo apt upgrade -y

# Install essential packages
sudo apt install -y curl wget git build-essential cmake ninja-build \
    pkg-config python3 python3-pip python3-venv software-properties-common

# Add NVIDIA package repositories
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.1-1_all.deb
sudo dpkg -i cuda-keyring_1.1-1_all.deb
sudo apt-get update
```

### 2. NVIDIA CUDA Installation

```bash
# Install CUDA Toolkit
sudo apt-get -y install cuda-toolkit-12-1 cuda-cudart-dev-12-1

# Add CUDA to PATH
echo 'export PATH=/usr/local/cuda/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

# Verify CUDA installation
nvcc --version
nvidia-smi
```

### 3. Docker Installation

```bash
# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh

# Add user to docker group
sudo usermod -aG docker $USER

# Install Docker Compose
sudo apt install -y docker-compose-plugin

# Verify Docker installation
docker --version
docker compose version
```

### 4. NVIDIA Container Toolkit

```bash
# Add NVIDIA container repository
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg
curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | \
    sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | \
    sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list

# Install NVIDIA container toolkit
sudo apt update
sudo apt install -y nvidia-container-toolkit

# Configure Docker runtime
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker

# Verify NVIDIA Docker support
docker run --rm --gpus all nvidia/cuda:12.1-base nvidia-smi
```

### 5. Application Dependencies

```bash
# Install cryptographic libraries
sudo apt install -y libssl-dev libsecp256k1-dev libjsoncpp-dev \
    libcurl4-openssl-dev zlib1g-dev

# Clone repository with submodules
git clone --recursive https://github.com/puzzle71/PuzzleKeyhunt.git
cd PuzzleKeyhunt

# Create necessary directories
mkdir -p logs monitoring config data results cache
```

## ⚙️ Configuration

### 1. Production Configuration

Create production configuration file:

```yaml
# config/production.yaml
production:
  version: "2.0.0"
  environment: "production"

  gpu:
    device_id: 0
    memory_pool_size: "4GB"
    stream_count: 4
    block_size: 256
    grid_size: 0  # Auto-detect

  performance:
    target_utilization: 95
    memory_efficiency_target: 90
    batch_size: 1000000
    checkpoint_interval: 60

  security:
    enable_monitoring: true
    log_level: "INFO"
    max_log_size: "100MB"
    log_retention_days: 7

  limits:
    max_memory_usage: "12GB"
    max_execution_time: 86400  # 24 hours
    max_results_per_batch: 1000

  monitoring:
    metrics_enabled: true
    metrics_port: 8080
    health_port: 8081
    prometheus_gateway: "http://prometheus-pushgateway:9091"
```

### 2. Docker Compose Configuration

Copy and configure the enhanced Docker Compose file:

```bash
cp docker-compose.production.enhanced.yml docker-compose.production.yml
```

### 3. Monitoring Configuration

**Prometheus Configuration** (`monitoring/prometheus.yml`):

```yaml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'puzzle71-application'
    static_configs:
      - targets: ['puzzle71-solver:8080']
    metrics_path: '/metrics'
    scrape_interval: 5s

  - job_name: 'puzzle71-gpu'
    static_configs:
      - targets: ['nvidia-dcgm-exporter:9445']
    scrape_interval: 10s

  - job_name: 'prometheus'
    static_configs:
      - targets: ['localhost:9090']

  - job_name: 'node-exporter'
    static_configs:
      - targets: ['node-exporter:9100']

rule_files:
  - "puzzle71_alerts.yml"
```

**Alert Rules** (`monitoring/puzzle71_alerts.yml`):

```yaml
groups:
  - name: puzzle71.rules
    rules:
      - alert: HighGPUUsage
        expr: nvidia_gpu_utilization_gpu > 95
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High GPU usage detected"
          description: "GPU utilization is above 95% for more than 5 minutes"

      - alert: HighMemoryUsage
        expr: (node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes * 100 > 90
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High memory usage detected"
          description: "Memory usage is above 90% for more than 5 minutes"

      - alert: ApplicationDown
        expr: up{job="puzzle71-application"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Puzzle71 application is down"
          description: "The Puzzle71 application has been down for more than 1 minute"
```

### 4. Data Files Configuration

**Private Key Ranges** (`data/private_ranges.txt`):

```
# Hexadecimal ranges for private key scanning
# Format: start_hex:end_hex
0000000000000000000000000000000000000000000000000000000000000000:1000000000000000000000000000000000000000000000000000000000000000
```

**Target Addresses** (`data/target_addresses.txt`):

```
# Bitcoin addresses to search for
1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
```

## 🚀 Deployment Methods

### Method 1: Automated Deployment (Recommended)

**Automated Production Deployment:**

```bash
# Run enhanced deployment script
./scripts/deployment/deploy_production_enhanced.sh

# This script will:
# 1. Validate prerequisites
# 2. Build Docker image with optimizations
# 3. Deploy services with health checks
# 4. Setup monitoring stack
# 5. Run performance tests
# 6. Generate deployment reports
```

**CI/CD Pipeline Deployment:**

```bash
# Run automated pipeline
./scripts/ci/automated_deployment_pipeline.sh

# This pipeline includes:
# 1. Configuration validation
# 2. Comprehensive testing
# 3. Security scanning
# 4. Performance testing
# 5. Automated deployment
# 6. Post-deployment verification
```

### Method 2: Manual Deployment

**Step 1: Build Docker Image**

```bash
# Build enhanced production image
docker build \
    --target production \
    --build-arg DEPLOYMENT_VERSION=2.0.0 \
    --tag puzzle71/puzzle71-solver:2.0.0 \
    --tag puzzle71/puzzle71-solver:latest \
    -f Dockerfile.production.enhanced .
```

**Step 2: Deploy Services**

```bash
# Deploy with enhanced Docker Compose
docker-compose -f docker-compose.production.enhanced.yml up -d

# Check deployment status
docker-compose -f docker-compose.production.enhanced.yml ps
```

**Step 3: Verify Services**

```bash
# Wait for services to be healthy
./scripts/deployment/wait_for_services.sh

# Check application logs
docker-compose -f docker-compose.production.enhanced.yml logs -f puzzle71-solver
```

### Method 3: Development Deployment

**Development Environment:**

```bash
# Build development image
docker build --target development -f Dockerfile.production.enhanced -t puzzle71-dev .

# Run development container
docker run --gpus all -it --rm \
    -v $(pwd)/src:/opt/puzzle71/src \
    -v $(pwd)/config:/opt/puzzle71/config \
    -p 8080:8080 \
    puzzle71-dev
```

## 📊 Monitoring and Observability

### 1. Grafana Dashboards

Access Grafana at `http://localhost:3000` with credentials:
- Username: `admin`
- Password: `puzzle71_admin_password_change_me` (change this!)

**Available Dashboards:**
- **Puzzle71 Overview**: Main application metrics
- **GPU Performance**: GPU utilization, memory, temperature
- **System Resources**: CPU, memory, disk, network
- **Application Health**: Error rates, response times
- **Pipeline Status**: CI/CD pipeline metrics

### 2. Prometheus Metrics

Key metrics available at `http://localhost:9090`:

**Application Metrics:**
- `puzzle71_throughput_keys_per_second`: Processing throughput
- `puzzle71_gpu_utilization_percent`: GPU usage
- `puzzle71_memory_usage_bytes`: Memory consumption
- `puzzle71_errors_total`: Error count
- `puzzle71_uptime_seconds`: Application uptime

**GPU Metrics:**
- `nvidia_gpu_utilization_gpu`: GPU utilization
- `nvidia_gpu_memory_used_bytes`: GPU memory usage
- `nvidia_gpu_temperature_gpu`: GPU temperature
- `nvidia_gpu_power_usage_watts`: Power consumption

### 3. Log Management

**Kibana Dashboard** (`http://localhost:5601`):
- Centralized log aggregation
- Real-time log analysis
- Error tracking and alerting
- Performance correlation

**Log Sources:**
- Application logs: `puzzle71.log`
- System logs: Docker containers
- Access logs: HTTP requests
- Error logs: Exceptions and failures

### 4. Alerting

**AlertManager** (`http://localhost:9093`):
- Configurable alert rules
- Multiple notification channels
- Alert grouping and silencing
- Incident management integration

**Alert Channels:**
- Email notifications
- Slack integration
- PagerDuty integration
- Custom webhooks

## 🛡️ Security Considerations

### 1. Container Security

**Security Hardening:**
- Non-root user execution
- Read-only filesystem where possible
- Minimal attack surface
- Resource limits enforcement
- Seccomp and AppArmor profiles

**Docker Security:**
```bash
# Scan images for vulnerabilities
docker scan puzzle71/puzzle71-solver:2.0.0

# Run with security options
docker run --security-opt no-new-privileges:true \
           --cap-drop ALL \
           --cap-add CHOWN,SETGID,SETUID \
           puzzle71/puzzle71-solver:2.0.0
```

### 2. Network Security

**Network Isolation:**
- Custom bridge networks
- Port exposure control
- Inter-service communication control
- External access restrictions

**TLS Configuration:**
```yaml
# Enable TLS in configuration
security:
  enable_tls: true
  tls_cert_path: "/opt/puzzle71/certs/server.crt"
  tls_key_path: "/opt/puzzle71/certs/server.key"
```

### 3. Data Protection

**Encryption at Rest:**
- Disk encryption (LUKS)
- Encrypted volumes
- Secure key management

**Secrets Management:**
```bash
# Use environment variables for secrets
export PUZZLE71_SECRET_KEY="your-secret-key-here"
export REDIS_PASSWORD="secure-redis-password"

# Or use Docker secrets
echo "your-secret" | docker secret create puzzle71_secret -
```

### 4. Access Control

**Authentication:**
- Grafana admin password change
- API key authentication
- LDAP/Active Directory integration

**Authorization:**
- Role-based access control
- Principle of least privilege
- Regular access audits

## ⚡ Performance Tuning

### 1. GPU Optimization

**CUDA Configuration:**
```yaml
gpu:
  device_id: 0
  memory_pool_size: "4GB"
  stream_count: 4
  block_size: 256
  grid_size: 0  # Auto-detect
```

**Performance Tips:**
- Use appropriate batch sizes (1M+ keys)
- Optimize memory allocation patterns
- Enable CUDA streams for concurrent execution
- Monitor GPU temperature and throttling

### 2. System Optimization

**Memory Management:**
```bash
# Optimize system memory
echo 'vm.swappiness=10' >> /etc/sysctl.conf
echo 'vm.dirty_ratio=15' >> /etc/sysctl.conf
sysctl -p
```

**CPU Optimization:**
```bash
# Set CPU governor to performance
echo 'performance' | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Disable CPU idle states
echo '1' | sudo tee /sys/devices/system/cpu/cpu*/cpuidle/state*/disable
```

### 3. Docker Performance

**Resource Limits:**
```yaml
deploy:
  resources:
    limits:
      memory: 16G
      cpus: '8'
    reservations:
      devices:
        - driver: nvidia
          count: 1
          capabilities: [gpu]
```

**Storage Optimization:**
```bash
# Use Docker overlay2 with optimal settings
{
  "storage-driver": "overlay2",
  "storage-opts": [
    "overlay2.override_kernel_check=true",
    "overlay2.maxsize=10G"
  ]
}
```

### 4. Network Performance

**Bandwidth Optimization:**
- Use 10Gbps+ network interfaces
- Enable jumbo frames (9000 MTU)
- Optimize TCP settings

**Latency Optimization:**
- Minimize network hops
- Use local caching (Redis)
- Optimize DNS resolution

## 🔧 Troubleshooting

### 1. Common Issues

**GPU Not Detected:**
```bash
# Check GPU availability
nvidia-smi

# Check NVIDIA Docker support
docker run --rm --gpus all nvidia/cuda:12.1-base nvidia-smi

# Reinstall NVIDIA Container Toolkit
sudo apt install --reinstall nvidia-container-toolkit
sudo systemctl restart docker
```

**Application Won't Start:**
```bash
# Check logs
docker-compose -f docker-compose.production.enhanced.yml logs puzzle71-solver

# Check configuration
docker-compose -f docker-compose.production.enhanced.yml config

# Check resource limits
docker stats
```

**Performance Issues:**
```bash
# Monitor GPU utilization
watch -n 1 nvidia-smi

# Check system resources
htop
iotop
nethogs

# Profile application
nvprof --metrics all ./Puzzle71Solver
```

### 2. Health Check Failures

**Service Health:**
```bash
# Check all services health
docker-compose -f docker-compose.production.enhanced.yml exec puzzle71-solver curl -f http://localhost:8081/health

# Manual health check
curl -f http://localhost:8080/metrics
```

**Memory Issues:**
```bash
# Check memory usage
docker stats --no-stream

# Check for memory leaks
valgrind --tool=memcheck --leak-check=full ./Puzzle71Solver
```

### 3. Network Issues

**Service Connectivity:**
```bash
# Test service communication
docker network ls
docker network inspect puzzle71_puzzle71-network

# Check port exposure
netstat -tlnp | grep :8080
```

**DNS Resolution:**
```bash
# Test DNS resolution
docker exec puzzle71-solver nslookup redis
docker exec puzzle71-solver ping prometheus
```

### 4. Log Analysis

**Application Logs:**
```bash
# Real-time log monitoring
docker-compose -f docker-compose.production.enhanced.yml logs -f puzzle71-solver

# Search for errors
grep -i error /opt/puzzle71/logs/puzzle71.log

# Monitor performance metrics
tail -f /opt/puzzle71/logs/metrics.log
```

**System Logs:**
```bash
# Docker logs
journalctl -u docker.service

# System logs
journalctl -f --since "1 hour ago"
```

## 🔄 Maintenance

### 1. Regular Maintenance Tasks

**Daily:**
- Check service health status
- Monitor performance metrics
- Review error logs
- Verify backup integrity

**Weekly:**
- Update container images
- Rotate log files
- Clean up unused Docker resources
- Performance tuning reviews

**Monthly:**
- Security updates
- Configuration audits
- Capacity planning
- Documentation updates

### 2. Backup and Recovery

**Data Backup:**
```bash
# Backup configuration
tar -czf backup_config_$(date +%Y%m%d).tar.gz config/

# Backup results
tar -czf backup_results_$(date +%Y%m%d).tar.gz results/

# Backup Docker volumes
docker run --rm -v puzzle71-results:/data -v $(pwd):/backup alpine tar czf /backup/results_backup_$(date +%Y%m%d).tar.gz -C /data .
```

**Service Recovery:**
```bash
# Restart services
docker-compose -f docker-compose.production.enhanced.yml restart

# Full service recovery
docker-compose -f docker-compose.production.enhanced.yml down
docker-compose -f docker-compose.production.enhanced.yml up -d

# Emergency recovery
./scripts/deployment/emergency_recovery.sh
```

### 3. Updates and Upgrades

**Application Updates:**
```bash
# Update to new version
git pull origin main
docker-compose -f docker-compose.production.enhanced.yml pull
docker-compose -f docker-compose.production.enhanced.yml up -d
```

**System Updates:**
```bash
# Update system packages
sudo apt update && sudo apt upgrade -y

# Update Docker
sudo apt install docker-ce docker-ce-cli containerd.io docker-compose-plugin

# Update NVIDIA drivers
sudo apt install --reinstall nvidia-driver-535
```

### 4. Performance Monitoring

**Key Performance Indicators:**
- GPU utilization > 90%
- Memory usage < 80%
- Throughput > 1M keys/sec
- Error rate < 0.1%
- Response time < 100ms

**Alert Thresholds:**
```yaml
# Alert configuration
alerts:
  gpu_utilization_high:
    threshold: 95%
    duration: 5m

  memory_usage_high:
    threshold: 90%
    duration: 5m

  error_rate_high:
    threshold: 1%
    duration: 2m

  throughput_low:
    threshold: 500k keys/sec
    duration: 10m
```

## 📚 Additional Resources

### Documentation
- [API Reference v2.0](API_REFERENCE_V2.md)
- [Architecture Overview](ARCHITECTURE_OVERVIEW.md)
- [Performance Benchmarks](PERFORMANCE_BENCHMARKS.md)
- [Security Guide](SECURITY_GUIDE.md)

### Community Support
- GitHub Issues: [PuzzleKeyhunt Issues](https://github.com/puzzle71/PuzzleKeyhunt/issues)
- Discord Server: [Puzzle71 Community](https://discord.gg/puzzle71)
- Documentation Wiki: [Puzzle71 Wiki](https://github.com/puzzle71/PuzzleKeyhunt/wiki)

### Tools and Utilities
- [NVIDIA System Management Interface](https://developer.nvidia.com/nvidia-system-management-interface)
- [Docker Monitoring](https://docs.docker.com/config/daemon/prometheus/)
- [Grafana Dashboards](https://grafana.com/grafana/dashboards/)
- [Prometheus Querying](https://prometheus.io/docs/prometheus/latest/querying/)

---

## 📞 Support

For deployment issues and questions:

- **Technical Support**: support@puzzle71.io
- **Security Issues**: security@puzzle71.io
- **Documentation Feedback**: docs@puzzle71.io

**Response Times:**
- Critical Issues: 1 hour
- High Priority: 4 hours
- Normal Priority: 24 hours
- Documentation: 48 hours

---

*This guide is maintained by the Puzzle71 Development Team and updated regularly with the latest deployment best practices and security recommendations.*