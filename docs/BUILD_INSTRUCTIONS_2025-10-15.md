# Puzzle71Solver 构建指南 (2025-10-15)

## 快速开始

### 1. 克隆仓库

```bash
# 克隆主仓库
git clone https://github.com/pest88-spec/keycuda.git
cd keycuda

# 切换到分支 003-gpu-1-28
git checkout 003-gpu-1-28

# 初始化并更新 Git submodules (secp256k1)
git submodule update --init --recursive
```

**说明**:
- `git clone`: 克隆主仓库 (包含BitCrack提取的代码)
- `git submodule update --init --recursive`: 初始化secp256k1子模块

**时间**: 约2-3分钟 (取决于网络速度)

---

### 2. 安装依赖

#### Ubuntu/Debian

```bash
# 更新包列表
sudo apt-get update

# 安装构建工具
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config

# 安装CUDA Toolkit (如果未安装)
# 参考: https://developer.nvidia.com/cuda-downloads

# 安装secp256k1依赖
sudo apt-get install -y \
    autoconf \
    automake \
    libtool
```

#### Windows (WSL)

```powershell
# 在WSL中执行上述Ubuntu/Debian命令
wsl bash -c "sudo apt-get update && sudo apt-get install -y build-essential cmake git pkg-config autoconf automake libtool"
```

---

### 3. 构建项目

#### 方法1: 使用Makefile (推荐)

```bash
# 构建Release版本
make build

# 或者构建Debug版本
make debug

# 清理构建文件
make clean
```

#### 方法2: 手动CMake构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置CMake (Release模式)
cmake ../src/KeyhuntCore -DCMAKE_BUILD_TYPE=Release

# 编译 (使用所有CPU核心)
make -j$(nproc)

# 返回项目根目录
cd ..
```

**说明**:
- `CMAKE_BUILD_TYPE=Release`: 优化编译,性能最佳
- `CMAKE_BUILD_TYPE=Debug`: 调试编译,包含调试符号
- `-j$(nproc)`: 并行编译,使用所有CPU核心

**时间**: 约5-10分钟 (首次编译)

---

### 4. 验证构建

```bash
# 检查可执行文件
ls -lh build/Puzzle71Solver

# 运行测试 (如果启用)
cd build && make test
```

**预期输出**:
```
-rwxr-xr-x 1 user user 36M Oct 15 23:00 build/Puzzle71Solver
```

---

## 详细步骤说明

### 步骤1: 克隆仓库

**命令**:
```bash
git clone https://github.com/pest88-spec/keycuda.git
cd keycuda
git checkout 003-gpu-1-28
git submodule update --init --recursive
```

**详细说明**:

1. **`git clone`**:
   - 克隆主仓库到本地
   - 包含所有提取的BitCrack代码 (`src/extracted/bitcrack/`)
   - 包含项目源代码 (`src/`)
   - 包含CMake配置 (`CMakeLists.txt`)

2. **`git checkout 003-gpu-1-28`**:
   - 切换到最新的GPU优化分支
   - 包含P1-PERF-001 CUDA Stream Manager

3. **`git submodule update --init --recursive`**:
   - 初始化`third_party/bitcoin-core-secp256k1/` (CPU验证库)
   - 初始化`third_party/secp256k1-zkp/` (GLV endomorphism库)
   - 递归初始化所有子模块

**为什么需要submodule?**
- secp256k1是完整的密码学库,需要完整代码
- 用于CPU验证GPU计算结果
- 用于GLV endomorphism优化

---

### 步骤2: 安装依赖

**必需依赖**:

| 依赖 | 版本 | 用途 |
|------|------|------|
| CMake | ≥3.18 | 构建系统 |
| GCC/G++ | ≥9.0 | C++编译器 |
| CUDA Toolkit | ≥11.0 | GPU编译 |
| Git | ≥2.0 | 版本控制 |

**可选依赖**:

| 依赖 | 用途 |
|------|------|
| Google Test | 单元测试 (自动下载) |
| nlohmann/json | JSON解析 (自动下载) |

**说明**:
- Google Test和nlohmann/json通过CMake FetchContent自动下载
- 无需手动安装

---

### 步骤3: 构建项目

**CMake配置选项**:

```bash
cmake ../src/KeyhuntCore \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DCMAKE_CUDA_ARCHITECTURES="75;86;89;90"
```

**选项说明**:

| 选项 | 值 | 说明 |
|------|-----|------|
| CMAKE_BUILD_TYPE | Release/Debug | 构建类型 |
| BUILD_TESTS | ON/OFF | 是否构建测试 |
| CMAKE_CUDA_ARCHITECTURES | "75;86;89;90" | CUDA架构 |

**CUDA架构对应**:
- 75: Turing (RTX 2080 Ti)
- 86: Ampere (RTX 3090)
- 89: Ada Lovelace (RTX 4090)
- 90: Hopper (H20, A100)

**编译输出**:
```
[  5%] Built target secp256k1_precomputed
[ 10%] Built target secp256k1
[100%] Built target Puzzle71Solver
```

---

### 步骤4: 运行程序

```bash
# 运行Puzzle71Solver
./build/Puzzle71Solver

# 或使用Makefile
make run
```

**配置文件**:
- `data/config.txt`: CUDA配置
- `data/private_ranges.txt`: 私钥范围
- `data/target_addresses.txt`: 目标地址

---

## 常见问题

### Q1: `git submodule update` 失败

**问题**: 网络连接失败,无法下载submodule

**解决方案**:
```bash
# 使用国内镜像 (如果可用)
git config --global url."https://gitclone.com/github.com/".insteadOf https://github.com/

# 或手动克隆
cd third_party
git clone https://github.com/bitcoin-core/secp256k1.git bitcoin-core-secp256k1
git clone https://github.com/ElementsProject/secp256k1-zkp.git secp256k1-zkp
```

### Q2: CMake找不到CUDA

**问题**: `Could not find CUDA`

**解决方案**:
```bash
# 设置CUDA路径
export CUDA_HOME=/usr/local/cuda
export PATH=$CUDA_HOME/bin:$PATH
export LD_LIBRARY_PATH=$CUDA_HOME/lib64:$LD_LIBRARY_PATH

# 重新运行CMake
cd build && cmake ..
```

### Q3: 编译错误 `nvcc not found`

**问题**: CUDA编译器未安装或未在PATH中

**解决方案**:
```bash
# 检查nvcc
which nvcc

# 如果未找到,安装CUDA Toolkit
# Ubuntu: https://developer.nvidia.com/cuda-downloads
```

### Q4: 链接错误 `undefined reference to secp256k1_*`

**问题**: secp256k1库未正确构建

**解决方案**:
```bash
# 清理并重新构建
rm -rf build
mkdir build && cd build
cmake .. && make -j$(nproc)
```

---

## 完整构建脚本

```bash
#!/bin/bash
set -e

# 1. 克隆仓库
git clone https://github.com/pest88-spec/keycuda.git
cd keycuda
git checkout 003-gpu-1-28

# 2. 初始化submodules
git submodule update --init --recursive

# 3. 安装依赖 (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake git pkg-config autoconf automake libtool

# 4. 构建项目
mkdir build && cd build
cmake ../src/KeyhuntCore -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 5. 验证构建
ls -lh Puzzle71Solver

echo "✅ 构建完成!"
```

---

## 总结

**克隆步骤**:
1. `git clone` - 克隆主仓库
2. `git checkout 003-gpu-1-28` - 切换分支
3. `git submodule update --init --recursive` - 初始化secp256k1

**编译步骤**:
1. 安装依赖 (CMake, GCC, CUDA)
2. `mkdir build && cd build` - 创建构建目录
3. `cmake ..` - 配置CMake
4. `make -j$(nproc)` - 编译

**总时间**: 约10-15分钟 (首次构建)

---

**文档版本**: 1.0  
**最后更新**: 2025-10-15  
**适用分支**: 003-gpu-1-28

