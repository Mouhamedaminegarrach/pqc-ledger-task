# Phase 2 Complete: Incomplete Tests

## ✅ Completed Tests

### 1. SignVerify Test ✅
**File**: `tests/integration_roundtrip.cpp`
**Status**: Fully implemented

**What it tests**:
- Keypair generation with liboqs
- Transaction signing with Dilithium3
- Signature verification
- Mutated signature rejection

**Implementation**:
- Generates real keypair using `crypto::generate_keypair()`
- Creates unsigned transaction
- Signs transaction using `tx::sign_transaction()`
- Verifies signature succeeds
- Mutates signature and verifies it fails

**OpenSSL Check**: Test skips gracefully if OpenSSL is not available (needed for SHA256)

---

### 2. DifferentChainId Test ✅
**File**: `tests/replay.cpp`
**Status**: Fully implemented

**What it tests**:
- Domain separation via chain_id
- Replay attack prevention
- Correct verification with matching chain_id

**Implementation**:
- Generates keypair and signs transaction for chain_id = 1
- Attempts verification with chain_id = 2 (should fail)
- Verifies with correct chain_id = 1 (should succeed)

**OpenSSL Check**: Test skips gracefully if OpenSSL is not available

---

### 3. Fixed-Size Validation Tests ✅
**File**: `tests/mutation.cpp`
**Status**: New tests added

**Tests Added**:
1. **WrongPubkeySize**: Tests decode rejects transactions with wrong pubkey size
2. **WrongSignatureSize**: Tests decode rejects transactions with wrong PQ signature size
3. **WrongEd25519SignatureSize**: Tests decode rejects hybrid transactions with wrong Ed25519 signature size
4. **LengthPrefixExceedsBuffer**: Tests decode rejects when length prefix exceeds remaining buffer

**Coverage**:
- ✅ Pubkey size validation (Dilithium3 = 1952 bytes)
- ✅ PQ signature size validation (Dilithium3 = 3293 bytes)
- ✅ Ed25519 signature size validation (64 bytes in hybrid mode)
- ✅ Length prefix validation (exceeds buffer check)

---

## GTest Setup

### FetchContent Integration ✅
**File**: `CMakeLists.txt`

**Changes**:
- Added FetchContent to automatically download GTest if not found
- Falls back to system GTest if available
- Downloads GTest v1.14.0 from GitHub

**Benefits**:
- No manual GTest installation required
- Tests can be built automatically
- Works on Windows, Linux, macOS

### Test CMakeLists.txt Updates ✅
**File**: `tests/CMakeLists.txt`

**Changes**:
- Removed `find_package(GTest REQUIRED)` (handled by parent)
- Added helper function `link_gtest()` to handle both find_package and FetchContent targets
- All test targets now use the helper function

---

## Test Status

### All Tests Implemented:
- ✅ `EncodeDecodeEncode` - Round-trip encoding test
- ✅ `SignVerify` - Full signing and verification test
- ✅ `DomainSeparation` - Domain separation logic test
- ✅ `DifferentChainId` - Chain replay prevention test
- ✅ `FlipAmountByte` - Mutation test
- ✅ `FlipFeeByte` - Mutation test
- ✅ `FlipNonceByte` - Mutation test
- ✅ `FlipSignatureByte` - Mutation test
- ✅ `FlipLengthPrefix` - Mutation test
- ✅ `TrailingBytes` - Mutation test
- ✅ `WrongPubkeySize` - Fixed-size validation test (NEW)
- ✅ `WrongSignatureSize` - Fixed-size validation test (NEW)
- ✅ `WrongEd25519SignatureSize` - Fixed-size validation test (NEW)
- ✅ `LengthPrefixExceedsBuffer` - Length validation test (NEW)
- ✅ All hex/base64 encoding tests (from Phase 1)

**Total Test Cases**: 25+ test cases

---

## Dependencies

### Required for Full Test Execution:
1. **liboqs**: ✅ Already integrated and working
2. **OpenSSL**: ⚠️ Optional - tests skip gracefully if not available
3. **GTest**: ✅ Automatically downloaded via FetchContent

### Test Behavior:
- Tests that require OpenSSL will skip with a clear message if OpenSSL is not available
- Tests that require liboqs will work (already integrated)
- All tests compile and can be built

---

## Next Steps: Phase 3

1. **CLI Base64 Output** (Issue 2.1)
   - Add `--format` option to `make-tx` command
   - Support both hex and base64 output

2. **Benchmark Graph Generation** (Issue 2.2)
   - Add CSV output to benchmark
   - Create Python script for graph generation

---

## Build Status

- ✅ All test code compiles
- ✅ GTest setup complete (FetchContent)
- ⚠️ Tests require OpenSSL to run fully (but skip gracefully if not available)
- ✅ All critical fixes from Phase 1 are tested

---

## Files Modified

1. `CMakeLists.txt` - Added FetchContent for GTest
2. `tests/CMakeLists.txt` - Updated to use FetchContent GTest
3. `tests/integration_roundtrip.cpp` - Completed SignVerify test
4. `tests/replay.cpp` - Completed DifferentChainId test
5. `tests/mutation.cpp` - Added 4 new fixed-size validation tests

