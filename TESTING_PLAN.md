# PQC Ledger Task - Comprehensive Testing Plan (C++)

## Prerequisites

Before starting, ensure you have:

- **C++17 compatible compiler** (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.15+** installed
- **liboqs** built (in `third_party/liboqs/build/`)
- **OpenSSL** (optional, for full test execution)
- **GTest** (will be downloaded automatically via FetchContent)
- **Google Benchmark** (optional, for benchmarks)

---

## PHASE 1: Build & Compilation Tests

### Test 1.1: Clean Build
```bash
cd technical-task-Amine
# Remove build directory
rm -rf build
# Or on Windows:
# Remove-Item -Recurse -Force build

# Create fresh build directory
mkdir build
cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release
```

**Expected**: Should compile without errors or warnings

**Verify**:
- Library `pqc_ledger.lib` (or `.a` on Linux) is created
- CLI executable `pqc-ledger-cli` is created (if BUILD_CLI is ON)
- No compilation errors

---

### Test 1.2: Format Check
```bash
# Check if code is properly formatted
# Using clang-format (if installed)
find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror

# Or on Windows PowerShell:
Get-ChildItem -Recurse -Include *.cpp,*.hpp | ForEach-Object { clang-format --dry-run --Werror $_.FullName }
```

**Expected**: Code is properly formatted (no changes needed)

**Note**: We have `.clang-format` file in the project root

---

### Test 1.3: Static Analysis (Optional)
```bash
# Using clang-tidy (if installed)
cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy -p . ../src/**/*.cpp
```

**Expected**: No critical warnings

---

## PHASE 2: Unit Tests

### Test 2.1: Build Tests
```bash
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --config Release --target test_codec_encoding
cmake --build . --config Release --target test_integration_roundtrip
cmake --build . --config Release --target test_mutation
cmake --build . --config Release --target test_replay
```

**Expected**: All test executables compile successfully

---

### Test 2.2: Run Codec Encoding Tests
```bash
cd build
./tests/Release/test_codec_encoding.exe
# Or on Linux:
# ./tests/test_codec_encoding
```

**Expected**: All hex/base64 encoding/decoding tests pass

**Tests**:
- Hex round-trip encoding/decoding
- Base64 round-trip encoding/decoding
- Transaction encoding/decoding with hex/base64
- Invalid input handling

---

### Test 2.3: Run Integration Tests
```bash
cd build
./tests/Release/test_integration_roundtrip.exe
```

**Expected**: 
- EncodeDecodeEncode test passes
- SignVerify test passes (if OpenSSL available, otherwise skips)

**Tests**:
- Round-trip encoding/decoding
- Full signing and verification workflow

---

### Test 2.4: Run Mutation Tests
```bash
cd build
./tests/Release/test_mutation.exe
```

**Expected**: All mutation tests pass

**Tests**:
- FlipAmountByte
- FlipFeeByte
- FlipNonceByte
- FlipSignatureByte
- FlipLengthPrefix
- TrailingBytes
- WrongPubkeySize
- WrongSignatureSize
- WrongEd25519SignatureSize
- LengthPrefixExceedsBuffer

---

### Test 2.5: Run Replay Tests
```bash
cd build
./tests/Release/test_replay.exe
```

**Expected**: All replay tests pass

**Tests**:
- DomainSeparation (should pass)
- DifferentChainId (should pass if OpenSSL available)

---

### Test 2.6: Run All Tests with CTest
```bash
cd build
ctest --test-dir . -C Release --output-on-failure
```

**Expected**: All tests pass

---

## PHASE 3: Functional Tests (CLI)

### Test 3.1: Generate Key Pair
```bash
cd build
mkdir -p test_keys
./bin/Release/pqc-ledger-cli.exe gen-key --algo pq --out test_keys
```

**Expected**:
- Creates `test_keys/pubkey.bin`
- Creates `test_keys/privkey.bin`
- No errors

**Verify**:
```bash
# Check files exist and have correct sizes
# Dilithium3 pubkey should be 1952 bytes
# Private key size depends on liboqs implementation
```

---

### Test 3.2: Create Transaction (Hex Output)
```bash
cd build
# Use a test address (64 hex chars = 32 bytes)
TEST_TO="0000000000000000000000000000000000000000000000000000000000000000"
./bin/Release/pqc-ledger-cli.exe make-tx \
    --to $TEST_TO \
    --amount 1000 \
    --fee 10 \
    --nonce 1 \
    --chain 1 \
    --pubkey test_keys/pubkey.bin
```

**Expected**: Outputs hex-encoded transaction (no errors)

---

### Test 3.3: Create Transaction (Base64 Output)
```bash
cd build
./bin/Release/pqc-ledger-cli.exe make-tx \
    --to $TEST_TO \
    --amount 1000 \
    --fee 10 \
    --nonce 1 \
    --chain 1 \
    --pubkey test_keys/pubkey.bin \
    --format base64
```

**Expected**: Outputs base64-encoded transaction (no errors)

---

### Test 3.4: Sign Transaction
```bash
cd build
# First create unsigned tx and save to file
TX_HEX=$(./bin/Release/pqc-ledger-cli.exe make-tx \
    --to $TEST_TO \
    --amount 1000 \
    --fee 10 \
    --nonce 1 \
    --chain 1 \
    --pubkey test_keys/pubkey.bin)

# Sign the transaction
SIGNED_TX=$(./bin/Release/pqc-ledger-cli.exe sign-tx \
    --tx $TX_HEX \
    --pq-key test_keys/privkey.bin)

echo $SIGNED_TX > signed_tx.txt
```

**Expected**: 
- Transaction is signed successfully
- Output is different from input (signature added)
- No errors

**Note**: Requires OpenSSL for SHA256

---

### Test 3.5: Verify Transaction
```bash
cd build
SIGNED_TX=$(cat signed_tx.txt)
./bin/Release/pqc-ledger-cli.exe verify-tx \
    --tx $SIGNED_TX \
    --chain 1
```

**Expected**:
- Output: `valid: true`
- Output: `from_address: <hex address>`
- Exit code: 0

---

### Test 3.6: Verify Transaction (Wrong Chain ID) - Chain Replay Prevention
```bash
cd build
# Create and sign transaction for chain_id = 1
TX=$(./bin/Release/pqc-ledger-cli.exe make-tx \
    --to $TEST_TO \
    --amount 1000 \
    --fee 10 \
    --nonce 1 \
    --chain 1 \
    --pubkey test_keys/pubkey.bin)

SIGNED_TX=$(./bin/Release/pqc-ledger-cli.exe sign-tx \
    --tx $TX \
    --pq-key test_keys/privkey.bin)

# ❌ CRITICAL: Try to verify SAME signed tx on chain 2 (MUST FAIL)
./bin/Release/pqc-ledger-cli.exe verify-tx \
    --tx $SIGNED_TX \
    --chain 2
```

**Expected**:
- Output: `valid: false`
- Output: `error: signature verification failed` (or similar error message)
- Exit code: 1
- **Must NOT accept transaction signed for chain 1 when verifying on chain 2**

**This tests domain separation and prevents replay attacks across chains.**

---

## PHASE 4: Error Handling Tests (No Panic Verification)

### Test 4.1: Invalid Transaction Decoding
```bash
cd build
# Try to decode invalid hex
echo "invalid_hex_data" | ./bin/Release/pqc-ledger-cli.exe verify-tx --tx - --chain 1
```

**Expected**: 
- Error message about invalid transaction
- Exit code: 1
- **Must NOT crash/panic** - should return structured error

---

### Test 4.2: Completely Invalid Input (No Panic Test)
```bash
cd build
# Test with completely invalid input (random characters)
./bin/Release/pqc-ledger-cli.exe verify-tx --tx "ZZZ_INVALID_&*@#" --chain 1

# Test with empty input
./bin/Release/pqc-ledger-cli.exe verify-tx --tx "" --chain 1

# Test with truncated transaction (too short)
./bin/Release/pqc-ledger-cli.exe verify-tx --tx "0a1b" --chain 1

# Test with non-hex characters
./bin/Release/pqc-ledger-cli.exe verify-tx --tx "GGGGGGGGGGGGGGGG" --chain 1
```

**Expected**: 
- All should return error messages (not crash/panic)
- Exit code: 1
- **CRITICAL**: No segmentation faults, no exceptions, no crashes
- Structured error messages returned

**This verifies the requirement: "no panics in decode/verify path"**

---

### Test 4.3: Mutation Tests at CLI Level
```bash
cd build
# Get a valid signed transaction
SIGNED_TX=$(cat signed_tx.txt)

# Manually flip one byte in the transaction
# Example: Change character at position 40 (if using bash)
# On Windows PowerShell, use substring manipulation:
$tx = Get-Content signed_tx.txt
$mutated = $tx.Substring(0,40) + "b" + $tx.Substring(41)

# Try to verify mutated transaction
./bin/Release/pqc-ledger-cli.exe verify-tx --tx $mutated --chain 1
```

**Expected**:
- Output: `valid: false`
- Exit code: 1
- **Must NOT crash/panic** - should fail gracefully
- Error message about signature verification failure

**Alternative mutation tests**:
```bash
# Flip a byte in the amount field (if you know the offset)
# Flip a byte in the signature
# Flip a byte in the length prefix
```

**All should fail verification without crashing.**

---

### Test 4.4: Missing Arguments
```bash
cd build
# Test each command with missing required arguments
./bin/Release/pqc-ledger-cli.exe gen-key
./bin/Release/pqc-ledger-cli.exe make-tx
./bin/Release/pqc-ledger-cli.exe sign-tx
./bin/Release/pqc-ledger-cli.exe verify-tx
```

**Expected**: All show usage/error messages

---

### Test 4.5: Invalid Format Option
```bash
cd build
./bin/Release/pqc-ledger-cli.exe make-tx \
    --to $TEST_TO \
    --amount 1000 \
    --fee 10 \
    --nonce 1 \
    --chain 1 \
    --pubkey test_keys/pubkey.bin \
    --format invalid
```

**Expected**: Error message about invalid format

---

## PHASE 5: Benchmark Tests

### Test 5.1: Build Benchmarks
```bash
cd build
cmake .. -DBUILD_BENCHMARKS=ON
cmake --build . --config Release --target pqc-ledger-bench
```

**Expected**: Benchmark executable compiles successfully

**Note**: Requires Google Benchmark to be installed

---

### Test 5.2: Run Benchmarks
```bash
cd build
./bin/Release/pqc-ledger-bench.exe
```

**Expected**: 
- Benchmarks run without errors
- Output shows timing information
- No crashes

---

### Test 5.3: Generate Benchmark CSV and Graph
```bash
cd build
# Run benchmark with CSV output
./bin/Release/pqc-ledger-bench.exe --benchmark_format=csv > benchmark_results.csv

# Verify CSV was created and has data
cat benchmark_results.csv | head -5

# Generate graph (requires Python 3 and matplotlib)
# Install matplotlib if needed: pip install matplotlib
python ../scripts/generate_benchmark_graph.py benchmark_results.csv
```

**Expected**:
- `benchmark_results.csv` is created with benchmark data
- CSV contains columns: name, iterations, real_time, cpu_time, etc.
- `benchmark_graph.png` is created
- Graph shows:
  - Average verification time for 100 transactions (in milliseconds)
  - Comparison of all benchmark results
  - Clear visualization of performance metrics

**Graph Script Location**: `scripts/generate_benchmark_graph.py`

**Graph Requirements** (from spec):
- Must show "average verify time over multiple iterations"
- Must create a graph visualization
- Must be reproducible

**Verify Graph Content**:
- Graph should show verification time for 100 transactions
- Should display average time clearly
- Should be saved as PNG file

---

## PHASE 6: Integration Tests

### Test 6.1: Full Workflow Test
```bash
cd build
mkdir -p test_workflow
cd test_workflow

# 1. Generate keys
../bin/Release/pqc-ledger-cli.exe gen-key --algo pq --out .

# 2. Create transaction
TX_HEX=$(../bin/Release/pqc-ledger-cli.exe make-tx \
    --to "0000000000000000000000000000000000000000000000000000000000000000" \
    --amount 1000 \
    --fee 10 \
    --nonce 1 \
    --chain 1 \
    --pubkey pubkey.bin)

# 3. Sign transaction
SIGNED_TX=$(../bin/Release/pqc-ledger-cli.exe sign-tx \
    --tx $TX_HEX \
    --pq-key privkey.bin)

# 4. Verify transaction
../bin/Release/pqc-ledger-cli.exe verify-tx \
    --tx $SIGNED_TX \
    --chain 1

# 5. Verify round-trip encoding
# Decode and re-encode should produce same result
```

**Expected**: All steps complete without errors

---

### Test 6.2: Round-Trip Encoding Test
```bash
cd build
# Create transaction
TX1=$(./bin/Release/pqc-ledger-cli.exe make-tx \
    --to "0000000000000000000000000000000000000000000000000000000000000000" \
    --amount 1000 \
    --fee 10 \
    --nonce 1 \
    --chain 1 \
    --pubkey test_keys/pubkey.bin)

# Sign
TX2=$(./bin/Release/pqc-ledger-cli.exe sign-tx \
    --tx $TX1 \
    --pq-key test_keys/privkey.bin)

# Verify
./bin/Release/pqc-ledger-cli.exe verify-tx --tx $TX2 --chain 1

# Decode, modify nothing, re-encode should be identical
# (This tests the canonical encoding)
```

**Expected**: Verification succeeds

---

## PHASE 7: Edge Cases & Stress Tests

### Test 7.1: Large Nonce Values
```bash
cd build
./bin/Release/pqc-ledger-cli.exe make-tx \
    --to "0000000000000000000000000000000000000000000000000000000000000000" \
    --amount 1000 \
    --fee 10 \
    --nonce 18446744073709551615 \
    --chain 1 \
    --pubkey test_keys/pubkey.bin
```

**Expected**: Handles max u64 value correctly

---

### Test 7.2: Maximum Amount/Fee
```bash
cd build
./bin/Release/pqc-ledger-cli.exe make-tx \
    --to "0000000000000000000000000000000000000000000000000000000000000000" \
    --amount 18446744073709551615 \
    --fee 18446744073709551615 \
    --nonce 1 \
    --chain 1 \
    --pubkey test_keys/pubkey.bin
```

**Expected**: Handles max u64 values correctly

---

### Test 7.3: Multiple Transactions
```bash
cd build
# Create and sign 10 transactions with different nonces
for i in {1..10}; do
    TX=$(./bin/Release/pqc-ledger-cli.exe make-tx \
        --to "0000000000000000000000000000000000000000000000000000000000000000" \
        --amount 1000 \
        --fee 10 \
        --nonce $i \
        --chain 1 \
        --pubkey test_keys/pubkey.bin)
    
    SIGNED=$(./bin/Release/pqc-ledger-cli.exe sign-tx \
        --tx $TX \
        --pq-key test_keys/privkey.bin)
    
    ./bin/Release/pqc-ledger-cli.exe verify-tx --tx $SIGNED --chain 1
done
```

**Expected**: All 10 transactions verify successfully

---

## PHASE 8: Validation Tests

### Test 8.1: Version Validation
Create a test that tries to decode a transaction with version != 1

**Expected**: Decode fails with InvalidVersion error

---

### Test 8.2: Trailing Bytes Validation
Create a test that appends extra bytes to a valid transaction

**Expected**: Decode fails with TrailingBytes error

---

### Test 8.3: Fixed-Size Validation
Create transactions with wrong pubkey/signature sizes

**Expected**: Decode fails with InvalidPublicKey or InvalidSignature error

---

### Test 8.4: Domain Separation Code Review
**Purpose**: Verify domain separation is implemented correctly in source code

**Location**: Check `src/tx/signing.cpp` and `src/crypto/hash.cpp`

**Verify signing message construction**:
```cpp
// In src/crypto/hash.cpp - create_signing_message()
// Should construct: SHA256("TXv1" || chain_id_be || canonical_encode(tx_without_sigs))

// Check:
1. Domain prefix "TXv1" is included
2. chain_id is converted to big-endian bytes
3. Transaction is encoded WITHOUT signatures (using encode_for_signing)
4. All parts are concatenated before hashing
```

**Expected Code Pattern**:
```cpp
// In src/crypto/hash.cpp
Result<std::vector<uint8_t>> create_signing_message(uint32_t chain_id,
                                                     const std::vector<uint8_t>& tx_data) {
    const std::string domain_prefix = "TXv1";  // ✅ Must be "TXv1"
    
    // Convert chain_id to big-endian bytes
    std::vector<uint8_t> chain_id_be(4);
    chain_id_be[0] = static_cast<uint8_t>((chain_id >> 24) & 0xFF);
    // ... ✅ Must be big-endian
    
    // Concatenate: "TXv1" || chain_id_be || tx_data
    std::vector<std::vector<uint8_t>> parts;
    parts.push_back(std::vector<uint8_t>(domain_prefix.begin(), domain_prefix.end()));
    parts.push_back(std::move(chain_id_be));
    parts.push_back(tx_data);
    
    return sha256_concat(parts);  // ✅ SHA256 of concatenated parts
}
```

**In src/tx/signing.cpp - verify_transaction()**:
```cpp
// Should use the SAME chain_id parameter (not tx.chain_id) for domain separation
auto msg_result = crypto::create_signing_message(chain_id, encoded_result.value());
// ✅ Uses parameter chain_id, not tx.chain_id - this enables replay prevention
```

**Verification Steps**:
1. Open `src/crypto/hash.cpp` and verify `create_signing_message()` implementation
2. Open `src/tx/signing.cpp` and verify `verify_transaction()` uses parameter `chain_id`
3. Verify `encode_for_signing()` excludes signatures (check `src/codec/encode.cpp`)

**Expected**: Code matches spec exactly - domain separation prevents replay attacks

---

## Test Results Summary

After running all tests, you should have:

- ✅ All compilation tests pass
- ✅ All unit tests pass (25+ test cases)
- ✅ All CLI commands work correctly
- ✅ All error cases handled properly (no panics)
- ✅ Benchmarks run successfully with graph generation
- ✅ Full workflow works end-to-end
- ✅ Chain replay prevention verified
- ✅ Domain separation verified

---

## CRITICAL Tests Checklist

These tests are **mandatory** and verify core requirements:

### ✅ No Panic Requirement
- [ ] **Test 4.2**: Invalid input doesn't crash (completely invalid, empty, truncated)
- [ ] **Test 4.3**: Mutated transactions fail gracefully without crashing
- [ ] All decode/verify paths return structured errors (no exceptions thrown)

### ✅ Chain Replay Prevention
- [ ] **Test 3.6**: Transaction signed for chain 1 fails verification on chain 2
- [ ] **Test 8.4**: Domain separation code review confirms "TXv1" + chain_id in signing message

### ✅ Strict Decoding
- [ ] **Test 8.1**: Version != 1 is rejected
- [ ] **Test 8.2**: Trailing bytes are rejected
- [ ] **Test 8.3**: Wrong pubkey/signature sizes are rejected
- [ ] Mutation tests: All invalid inputs are rejected

### ✅ Benchmark Graph (Mandatory Requirement)
- [ ] **Test 5.3**: Graph is generated successfully
- [ ] Graph shows average verify time for 100 transactions
- [ ] Graph is saved as PNG file
- [ ] Script location: `scripts/generate_benchmark_graph.py` (exists and works)

---

## Benchmark Graph Script Documentation

**Location**: `scripts/generate_benchmark_graph.py`

**Purpose**: Generate visualization graph from benchmark CSV results (mandatory requirement)

**Dependencies**:
- Python 3
- matplotlib: `pip install matplotlib`

**Usage**:
```bash
# Step 1: Run benchmark and save CSV
./bin/Release/pqc-ledger-bench.exe --benchmark_format=csv > benchmark_results.csv

# Step 2: Generate graph
python scripts/generate_benchmark_graph.py benchmark_results.csv
```

**Output**:
- `benchmark_graph.png` - Graph visualization
- Console output with summary statistics

**Graph Features**:
- Shows average verification time for 100 transactions
- Compares all benchmark results
- Saves as high-resolution PNG (150 DPI)
- Includes value labels on bars

**Verification**:
- Script exists at `scripts/generate_benchmark_graph.py`
- Script parses Google Benchmark CSV format
- Script generates matplotlib graphs
- Graph shows "average verify time over multiple iterations" as required

---

## Quick Test Script

Create a `run_all_tests.sh` (or `.bat` for Windows) to automate testing:

```bash
#!/bin/bash
set -e

echo "=== Phase 1: Build Tests ==="
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

echo "=== Phase 2: Unit Tests ==="
ctest --test-dir . -C Release --output-on-failure

echo "=== Phase 3: CLI Tests ==="
# ... add CLI test commands

echo "=== All Tests Passed! ==="
```

---

## Notes

- Some tests require OpenSSL (tests will skip gracefully if not available)
- Benchmark tests require Google Benchmark
- All tests should be run in Release mode for accurate performance measurements
- On Windows, use `.exe` extension for executables
- On Linux/Mac, executables have no extension

