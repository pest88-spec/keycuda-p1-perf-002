# OpenSSL Installation Guide for PuzzleKeyhunt

**Purpose**: Resolve OpenSSL dependency for P2-001 compilation  
**Platform**: Windows 10/11  
**Date**: 2025-10-13

---

## Problem Description

CMake configuration fails with the following error:

```
CMake Error: Could NOT find OpenSSL, try to set the path to OpenSSL root folder in the
  system variable OPENSSL_ROOT_DIR (missing: OPENSSL_CRYPTO_LIBRARY OPENSSL_INCLUDE_DIR)
```

This blocks compilation of the P2-001 code duplication optimization changes.

---

## Solution Options

### Option 1: Install OpenSSL via Chocolatey (Recommended)

**Prerequisites**: Chocolatey package manager installed

**Steps**:

1. Open PowerShell as Administrator

2. Install OpenSSL:
```powershell
choco install openssl
```

3. Verify installation:
```powershell
openssl version
```

4. Set environment variable (if needed):
```powershell
$env:OPENSSL_ROOT_DIR = "C:\Program Files\OpenSSL-Win64"
[System.Environment]::SetEnvironmentVariable("OPENSSL_ROOT_DIR", "C:\Program Files\OpenSSL-Win64", "Machine")
```

5. Restart PowerShell and retry CMake:
```powershell
cd build
cmake ..
```

---

### Option 2: Install OpenSSL Manually

**Steps**:

1. Download OpenSSL installer from:
   - Official: https://slproweb.com/products/Win32OpenSSL.html
   - Choose: Win64 OpenSSL v3.x.x (latest stable)

2. Run installer:
   - Install to: `C:\Program Files\OpenSSL-Win64`
   - Select: "Copy OpenSSL DLLs to the Windows system directory"

3. Set environment variables:
```powershell
# Open System Properties > Environment Variables
# Add new System Variable:
Variable name: OPENSSL_ROOT_DIR
Variable value: C:\Program Files\OpenSSL-Win64
```

4. Add to PATH:
```powershell
# Add to System PATH:
C:\Program Files\OpenSSL-Win64\bin
```

5. Restart PowerShell and verify:
```powershell
openssl version
```

6. Retry CMake:
```powershell
cd build
cmake ..
```

---

### Option 3: Use vcpkg (Alternative)

**Prerequisites**: vcpkg package manager installed

**Steps**:

1. Install OpenSSL via vcpkg:
```powershell
vcpkg install openssl:x64-windows
```

2. Integrate vcpkg with CMake:
```powershell
vcpkg integrate install
```

3. Configure CMake with vcpkg toolchain:
```powershell
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

---

### Option 4: Specify OpenSSL Path Manually in CMake

If OpenSSL is already installed but CMake can't find it:

```powershell
cd build
cmake .. -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64" `
         -DOPENSSL_INCLUDE_DIR="C:\Program Files\OpenSSL-Win64\include" `
         -DOPENSSL_CRYPTO_LIBRARY="C:\Program Files\OpenSSL-Win64\lib\libcrypto.lib"
```

---

## Verification Steps

After installation, verify OpenSSL is correctly configured:

### 1. Check OpenSSL Version
```powershell
openssl version
```

Expected output:
```
OpenSSL 3.x.x xx xxx xxxx (Library: OpenSSL 3.x.x xx xxx xxxx)
```

### 2. Check Environment Variables
```powershell
$env:OPENSSL_ROOT_DIR
```

Expected output:
```
C:\Program Files\OpenSSL-Win64
```

### 3. Check CMake Detection
```powershell
cd build
cmake .. --debug-find
```

Look for lines like:
```
-- Found OpenSSL: C:/Program Files/OpenSSL-Win64/lib/libcrypto.lib (found version "3.x.x")
```

### 4. Compile Test
```powershell
cd build
cmake ..
make -j4
```

Expected: No OpenSSL-related errors

---

## Troubleshooting

### Issue 1: "openssl: command not found"

**Cause**: OpenSSL bin directory not in PATH

**Solution**:
```powershell
# Add to PATH temporarily
$env:PATH += ";C:\Program Files\OpenSSL-Win64\bin"

# Or permanently via System Properties > Environment Variables
```

### Issue 2: "OPENSSL_ROOT_DIR not set"

**Cause**: Environment variable not configured

**Solution**:
```powershell
[System.Environment]::SetEnvironmentVariable("OPENSSL_ROOT_DIR", "C:\Program Files\OpenSSL-Win64", "Machine")
```

Restart PowerShell after setting.

### Issue 3: "libcrypto.lib not found"

**Cause**: OpenSSL libraries not in expected location

**Solution**:
```powershell
# Check if libraries exist
Test-Path "C:\Program Files\OpenSSL-Win64\lib\libcrypto.lib"

# If not, reinstall OpenSSL or specify correct path
cmake .. -DOPENSSL_CRYPTO_LIBRARY="C:\path\to\libcrypto.lib"
```

### Issue 4: "Version mismatch"

**Cause**: Multiple OpenSSL versions installed

**Solution**:
```powershell
# Uninstall old versions
choco uninstall openssl

# Reinstall latest version
choco install openssl

# Or specify exact version
cmake .. -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64"
```

---

## Post-Installation Steps

After successfully installing OpenSSL:

### 1. Clean Build Directory
```powershell
cd build
Remove-Item * -Recurse -Force
```

### 2. Reconfigure CMake
```powershell
cmake ..
```

### 3. Compile Project
```powershell
make -j4
```

### 4. Run Tests
```powershell
ctest --output-on-failure
```

### 5. Commit Changes
```powershell
cd ..
bash scripts/commit-p2-001.sh
```

---

## Alternative: Remove OpenSSL Dependency

If OpenSSL installation is problematic, consider using alternative hash libraries:

### Option A: Use Windows CryptoAPI

Modify `CMakeLists.txt` to use Windows native crypto:

```cmake
if(WIN32)
    # Use Windows CryptoAPI instead of OpenSSL
    target_link_libraries(${PROJECT_NAME} PRIVATE bcrypt)
else()
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto)
endif()
```

### Option B: Use Embedded Hash Implementation

Use header-only hash libraries:
- SHA256: https://github.com/System-Glitch/SHA256
- RIPEMD160: https://github.com/ARMmbed/mbedtls

---

## References

- OpenSSL Official: https://www.openssl.org/
- OpenSSL Windows Binaries: https://slproweb.com/products/Win32OpenSSL.html
- Chocolatey OpenSSL: https://community.chocolatey.org/packages/openssl
- vcpkg OpenSSL: https://github.com/microsoft/vcpkg/tree/master/ports/openssl

---

**Last Updated**: 2025-10-13  
**Status**: Ready for use  
**Next Step**: Install OpenSSL and retry compilation

