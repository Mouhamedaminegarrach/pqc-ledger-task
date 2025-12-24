# Fix Plan: PQC Ledger Task Issues

## Overview
This document outlines the fixes needed to fully comply with the requirements.

## Priority 1: Critical Issues (Must Fix)

### Issue 1.1: Fixed-Size Validation in Decode
**Problem**: `decode()` does not validate that pubkey and signature lengths match expected algorithm sizes.

**Requirement**: "Enforce fixed sizes: PQ pubkey length must match algorithm's expected length. PQ signature length must match expected length."

**Location**: `src/codec/decode.cpp` - `decode()` function

**Current State**:
- Line 227: Reads pubkey with `read_bytes_with_len()` but doesn't validate size
- Line 244: Reads PQ signature but doesn't validate size
- Line 248-249: Reads hybrid signatures but doesn't validate sizes

**Fix Plan**:
1. Add helper function to get expected sizes for algorithm (or use existing `get_pubkey_size()` and `get_signature_size()`)
2. After reading pubkey (line 227), validate length:
   ```cpp
   // Validate pubkey size (default to Dilithium3, or detect from context)
   auto expected_pubkey_size = crypto::get_pubkey_size("Dilithium3");
   if (expected_pubkey_size.is_ok() && 
       tx.from_pubkey.size() != expected_pubkey_size.value()) {
       return Result<Transaction>::Err(Error(ErrorCode::InvalidPublicKey,
           "Public key size mismatch: expected " + 
           std::to_string(expected_pubkey_size.value()) + 
           ", got " + std::to_string(tx.from_pubkey.size())));
   }
   ```
3. After reading PQ signature (line 244), validate length:
   ```cpp
   auto expected_sig_size = crypto::get_signature_size("Dilithium3");
   if (expected_sig_size.is_ok() && 
       sig_bytes.size() != expected_sig_size.value()) {
       return Result<Transaction>::Err(Error(ErrorCode::InvalidSignature,
           "PQ signature size mismatch: expected " + 
           std::to_string(expected_sig_size.value()) + 
           ", got " + std::to_string(sig_bytes.size())));
   }
   ```
4. For hybrid mode (line 248-249), validate both:
   - Ed25519 signature must be 64 bytes
   - PQ signature must match algorithm size

**Note**: Since decode doesn't know the algorithm, we have two options:
- Option A: Always validate against default (Dilithium3) - simpler but less flexible
- Option B: Add algorithm identifier to transaction encoding - more complex but more flexible

**Recommendation**: Option A for now (validate against Dilithium3), document that decode assumes Dilithium3.

**Files to Modify**:
- `src/codec/decode.cpp`

**Dependencies**: Requires `crypto::get_pubkey_size()` and `crypto::get_signature_size()` which are already implemented.

---

### Issue 1.2: Length Prefix Validation
**Problem**: `read_bytes_with_len()` doesn't verify that the length matches remaining buffer.

**Requirement**: "Length prefixes must match remaining buffer."

**Location**: `src/codec/decode.cpp` - `Reader::read_bytes_with_len()`

**Current State**:
```cpp
std::vector<uint8_t> read_bytes_with_len() {
    uint16_t len = read_u16_be();
    return read_bytes(len);
}
```

**Fix Plan**:
1. Check if length exceeds remaining bytes before reading:
   ```cpp
   std::vector<uint8_t> read_bytes_with_len() {
       uint16_t len = read_u16_be();
       if (len > remaining()) {
           throw std::runtime_error("Length prefix exceeds remaining buffer: " + 
                                    std::to_string(len) + " > " + 
                                    std::to_string(remaining()));
       }
       return read_bytes(len);
   }
   ```

**Files to Modify**:
- `src/codec/decode.cpp` - Reader class

**Error Handling**: Should return `ErrorCode::InvalidLengthPrefix` or `ErrorCode::MismatchedLength`

---

## Priority 2: Missing Features

### Issue 2.1: CLI Base64 Output Option
**Problem**: `make-tx` only outputs hex, but requirement says "hex/base64"

**Location**: `src/cli/main.cpp` - `cmd_make_tx()`

**Fix Plan**:
1. Add `--format` or `--output-format` option (default: hex)
2. Support both "hex" and "base64" formats
3. Use `codec::encode_to_hex()` or `codec::encode_to_base64()` based on format

**Files to Modify**:
- `src/cli/main.cpp`

---

### Issue 2.2: Benchmark Graph Generation
**Problem**: Benchmark runs but doesn't generate a graph as required.

**Requirement**: "print average verify time over multiple iteration and create a graph"

**Location**: `benches/verify.cpp`

**Fix Plan**:
1. Add CSV output to benchmark:
   ```cpp
   // At end of benchmark, write CSV
   std::ofstream csv("benchmark_results.csv");
   csv << "iteration,avg_time_us,txs_per_second\n";
   // ... write results
   ```
2. Create Python script to generate graph:
   ```python
   # scripts/generate_benchmark_graph.py
   import matplotlib.pyplot as plt
   import pandas as pd
   # Read CSV and create graph
   ```
3. Or use Google Benchmark's JSON output and convert to graph

**Files to Create/Modify**:
- `benches/verify.cpp` - Add CSV output
- `scripts/generate_benchmark_graph.py` - New file for graph generation
- Update `benches/CMakeLists.txt` to run script after benchmark

**Alternative**: Use Google Benchmark's built-in JSON/CSV export if available.

---

### Issue 2.3: Fuzz Test (Bonus)
**Problem**: Fuzz test for decoder is not implemented (bonus requirement).

**Fix Plan**:
1. Use libFuzzer or similar fuzzing framework
2. Create fuzz target:
   ```cpp
   // tests/fuzz_decode.cpp
   extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
       std::vector<uint8_t> input(data, data + size);
       auto result = codec::decode(input);
       // Should never crash, always return Result
       return 0;
   }
   ```
3. Add to CMakeLists.txt with fuzzing support

**Files to Create**:
- `tests/fuzz_decode.cpp`
- Update `tests/CMakeLists.txt`

**Note**: This is a bonus requirement, lower priority.

---

## Priority 3: Incomplete Tests

### Issue 3.1: Complete SignVerify Test
**Problem**: `integration_roundtrip.cpp::SignVerify` is skipped, needs liboqs integration.

**Location**: `tests/integration_roundtrip.cpp` line 37-41

**Fix Plan**:
1. Remove `GTEST_SKIP()`
2. Implement full test:
   ```cpp
   TEST(IntegrationRoundtrip, SignVerify) {
       // 1. Generate keypair
       auto keypair = crypto::generate_keypair("Dilithium3");
       ASSERT_TRUE(keypair.is_ok());
       const auto& [pubkey, privkey] = keypair.value();
       
       // 2. Create unsigned transaction
       Transaction tx;
       tx.version = 1;
       tx.chain_id = 1;
       tx.nonce = 12345;
       tx.from_pubkey = pubkey;
       // ... set other fields
       
       // 3. Sign transaction
       auto sign_result = tx::sign_transaction(tx, privkey, "Dilithium3");
       ASSERT_TRUE(sign_result.is_ok());
       
       // 4. Verify transaction
       auto verify_result = tx::verify_transaction(tx, 1);
       ASSERT_TRUE(verify_result.is_ok());
       EXPECT_TRUE(verify_result.value()) << "Valid signature should verify";
       
       // 5. Mutate signature and verify it fails
       auto& pq_sig = std::get<PqSignature>(tx.auth);
       pq_sig.sig[0] ^= 0xFF;  // Flip a bit
       auto verify_fail = tx::verify_transaction(tx, 1);
       ASSERT_TRUE(verify_fail.is_ok());
       EXPECT_FALSE(verify_fail.value()) << "Mutated signature should fail";
   }
   ```

**Files to Modify**:
- `tests/integration_roundtrip.cpp`

**Dependencies**: Requires OpenSSL for SHA256 (already handled conditionally)

---

### Issue 3.2: Complete DifferentChainId Test
**Problem**: `replay.cpp::DifferentChainId` is skipped, needs liboqs integration.

**Location**: `tests/replay.cpp` line 7-32

**Fix Plan**:
1. Remove `GTEST_SKIP()`
2. Implement full test:
   ```cpp
   TEST(Replay, DifferentChainId) {
       // 1. Generate keypair
       auto keypair = crypto::generate_keypair("Dilithium3");
       ASSERT_TRUE(keypair.is_ok());
       const auto& [pubkey, privkey] = keypair.value();
       
       // 2. Create and sign transaction for chain_id = 1
       Transaction tx;
       tx.version = 1;
       tx.chain_id = 1;
       tx.from_pubkey = pubkey;
       // ... set other fields
       
       auto sign_result = tx::sign_transaction(tx, privkey, "Dilithium3");
       ASSERT_TRUE(sign_result.is_ok());
       
       // 3. Try to verify with chain_id = 2 (should fail)
       auto verify_wrong = tx::verify_transaction(tx, 2);
       ASSERT_TRUE(verify_wrong.is_ok());
       EXPECT_FALSE(verify_wrong.value()) << 
           "Verification with wrong chain_id should fail";
       
       // 4. Verify with correct chain_id = 1 (should succeed)
       auto verify_correct = tx::verify_transaction(tx, 1);
       ASSERT_TRUE(verify_correct.is_ok());
       EXPECT_TRUE(verify_correct.value()) << 
           "Verification with correct chain_id should succeed";
   }
   ```

**Files to Modify**:
- `tests/replay.cpp`

---

## Priority 4: Verification & Testing

### Issue 4.1: Verify Error Code Usage
**Problem**: Need to verify all length prefix errors use correct error codes.

**Action**: Review `decode.cpp` to ensure:
- Length prefix mismatches → `ErrorCode::InvalidLengthPrefix` or `ErrorCode::MismatchedLength`
- Buffer overflow errors → Appropriate error code
- All error paths return structured errors (no panics)

**Files to Review**:
- `src/codec/decode.cpp`
- `include/pqc_ledger/error.hpp` - Verify all needed error codes exist

---

### Issue 4.2: Test Fixed-Size Validation
**Problem**: Need tests to verify fixed-size validation works.

**Action**: Add tests to `tests/mutation.cpp`:
- Test with wrong pubkey size
- Test with wrong signature size
- Test with wrong Ed25519 signature size in hybrid mode

**Files to Modify**:
- `tests/mutation.cpp`

---

## Implementation Order

### Phase 1: Critical Fixes (Do First)
1. ✅ Fix Issue 1.2: Length prefix validation (simpler, no dependencies)
2. ✅ Fix Issue 1.1: Fixed-size validation in decode (core requirement)

### Phase 2: Complete Tests
3. ✅ Fix Issue 3.1: Complete SignVerify test
4. ✅ Fix Issue 3.2: Complete DifferentChainId test
5. ✅ Fix Issue 4.2: Add tests for fixed-size validation

### Phase 3: Missing Features
6. ✅ Fix Issue 2.1: CLI base64 output option
7. ✅ Fix Issue 2.2: Benchmark graph generation

### Phase 4: Bonus & Polish
8. ✅ Fix Issue 2.3: Fuzz test (bonus)
9. ✅ Fix Issue 4.1: Verify error code usage

---

## Testing Checklist

After implementing fixes, verify:

- [ ] Decode rejects transactions with wrong pubkey size
- [ ] Decode rejects transactions with wrong signature size
- [ ] Decode rejects transactions with length prefix > remaining buffer
- [ ] SignVerify test passes with real liboqs signatures
- [ ] DifferentChainId test passes (domain separation works)
- [ ] CLI make-tx supports --format hex/base64
- [ ] Benchmark generates CSV and graph
- [ ] All mutation tests pass
- [ ] No panics in decode/verify paths (all errors are structured)

---

## Notes

1. **Algorithm Detection**: The decode function doesn't know which algorithm to expect. Current approach: validate against default (Dilithium3). Alternative: add algorithm identifier to transaction (breaking change, not recommended).

2. **OpenSSL Dependency**: Some features require OpenSSL. Current implementation handles this gracefully with conditional compilation. Tests that require OpenSSL will need it installed.

3. **GTest Dependency**: Tests require GTest. Need to install or use FetchContent.

4. **Benchmark Graph**: Can use Google Benchmark's built-in reporting or create custom CSV + Python script.

---

## Estimated Time

- Phase 1 (Critical): 2-3 hours
- Phase 2 (Tests): 2-3 hours  
- Phase 3 (Features): 1-2 hours
- Phase 4 (Bonus): 1-2 hours

**Total**: ~6-10 hours

