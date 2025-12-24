# Installing OpenSSL on Windows

OpenSSL is required for:
- SHA256 hashing (signing messages)
- Ed25519 signatures (hybrid mode)
- Full mutation and replay tests

## Option 1: Using vcpkg (Recommended for CMake projects)

If you have vcpkg installed:

```powershell
# Install vcpkg if you don't have it
# git clone https://github.com/Microsoft/vcpkg.git
# cd vcpkg
# .\bootstrap-vcpkg.bat

# Install OpenSSL
vcpkg install openssl:x64-windows

# Integrate with Visual Studio
vcpkg integrate install
```

Then update CMakeLists.txt to use vcpkg toolchain:
```cmake
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
```

## Option 2: Manual Installation (Pre-built Binaries)

1. Download OpenSSL for Windows:
   - Go to: https://slproweb.com/products/Win32OpenSSL.html
   - Download: "Win64 OpenSSL v3.x.x" (Light version is sufficient)
   - Or use direct link: https://slproweb.com/download/Win64OpenSSL-3_3_1.msi

2. Install the MSI:
   - Run the installer
   - Choose "Copy OpenSSL DLLs to: The OpenSSL binaries (/bin) directory"
   - Install to default location: `C:\Program Files\OpenSSL-Win64`

3. Add to PATH (if not done automatically):
   ```powershell
   # Add to system PATH (requires admin):
   [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Program Files\OpenSSL-Win64\bin", "Machine")
   ```

4. Update CMakeLists.txt to find OpenSSL:
   ```cmake
   # Add this to CMakeLists.txt if OpenSSL is in a custom location:
   set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
   set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
   set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib/libcrypto.lib" "${OPENSSL_ROOT_DIR}/lib/libssl.lib")
   ```

## Option 3: Using Chocolatey (Requires Admin)

Open PowerShell as Administrator and run:
```powershell
choco install openssl -y
```

## Option 4: Build from Source (Advanced)

If you need a specific version or configuration:
1. Download source from: https://www.openssl.org/source/
2. Follow Windows build instructions
3. This is complex and not recommended unless necessary

## Verify Installation

After installation, verify OpenSSL is found:

```powershell
# Check if OpenSSL is in PATH
openssl version

# Rebuild CMake to detect OpenSSL
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
# Should see: "Found OpenSSL: ..." instead of warning
```

## After Installation

1. Rebuild the project:
   ```powershell
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   ```

2. Run tests - all should pass now:
   ```powershell
   ctest --test-dir . -C Release --output-on-failure
   ```

3. All mutation and replay tests should now run (not skip)

## Troubleshooting

### CMake can't find OpenSSL
- Check that `OPENSSL_ROOT_DIR` is set correctly
- Verify `libcrypto.lib` and `libssl.lib` exist in the lib directory
- On Windows, you may need both `.lib` (for linking) and `.dll` (for runtime)

### Linker errors
- Ensure you have both Debug and Release libraries if needed
- Check that the library architecture (x64/x86) matches your build

### Runtime errors (DLL not found)
- Add OpenSSL `bin` directory to PATH
- Or copy DLLs to your executable directory
- Or install to system directory

## Quick Test

After installation, test SHA256:
```powershell
cd build\bin\Release
.\pqc-ledger-cli.exe gen-key --algo pq --out test_keys
# Should work without "OpenSSL not available" errors
```



