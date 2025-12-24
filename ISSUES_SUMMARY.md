# Issues Summary - Quick Reference

## 🔴 Critical Issues (Must Fix)

### 1. Fixed-Size Validation Missing in Decode
- **File**: `src/codec/decode.cpp`
- **Lines**: 227, 244, 248-249
- **Issue**: Doesn't validate pubkey/signature lengths match algorithm
- **Fix**: Add validation after reading pubkey and signatures

### 2. Length Prefix Validation Missing
- **File**: `src/codec/decode.cpp`
- **Class**: `Reader::read_bytes_with_len()`
- **Issue**: Doesn't check if length exceeds remaining buffer
- **Fix**: Add check before reading bytes

## 🟡 Missing Features

### 3. CLI Base64 Output
- **File**: `src/cli/main.cpp` - `cmd_make_tx()`
- **Issue**: Only outputs hex, requirement says "hex/base64"
- **Fix**: Add `--format` option

### 4. Benchmark Graph Missing
- **File**: `benches/verify.cpp`
- **Issue**: No graph generation
- **Fix**: Add CSV output + Python script

### 5. Fuzz Test Missing (Bonus)
- **File**: `tests/fuzz_decode.cpp` (to create)
- **Issue**: Not implemented
- **Fix**: Create libFuzzer target

## 🟠 Incomplete Tests

### 6. SignVerify Test Skipped
- **File**: `tests/integration_roundtrip.cpp` line 37-41
- **Issue**: Test is skipped, needs liboqs integration
- **Fix**: Implement full test with real signatures

### 7. DifferentChainId Test Skipped
- **File**: `tests/replay.cpp` line 7-32
- **Issue**: Test is skipped, needs liboqs integration
- **Fix**: Implement full test with domain separation

## ✅ What's Already Correct

- ✅ Transaction model complete
- ✅ Canonical encoding/decoding (mostly)
- ✅ Domain separation in signing
- ✅ All CLI commands implemented
- ✅ Round-trip test
- ✅ Mutation tests (mostly)
- ✅ README complete
- ✅ Benchmark structure exists

## Quick Fix Priority

1. **First**: Length prefix validation (Issue #2) - Simple, no dependencies
2. **Second**: Fixed-size validation (Issue #1) - Core requirement
3. **Third**: Complete liboqs tests (Issues #6, #7) - Need OpenSSL
4. **Fourth**: CLI base64 + benchmark graph (Issues #3, #4)
5. **Last**: Fuzz test (Issue #5) - Bonus

See `FIX_PLAN.md` for detailed implementation steps.

