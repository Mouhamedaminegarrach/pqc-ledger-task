#include <benchmark/benchmark.h>
#include "pqc_ledger/pqc_ledger.hpp"
#include <vector>
#include <random>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>

using namespace pqc_ledger;

// Helper to create and sign a real PQ transaction
std::pair<Transaction, bool> create_and_sign_bench_tx(size_t nonce) {
    // Generate keypair once (reused for all transactions)
    static auto keypair_result = crypto::generate_keypair("Dilithium3");
    static bool keypair_initialized = false;
    
    if (!keypair_initialized) {
        if (!keypair_result.is_ok()) {
            return {{}, false};
        }
        keypair_initialized = true;
    }
    
    const auto& [pubkey, privkey] = keypair_result.value();
    
    // Create unsigned transaction
    Transaction tx;
    tx.version = 1;
    tx.chain_id = 1;
    tx.nonce = nonce;
    tx.from_pubkey = pubkey;
    tx.to = {};
    std::fill(tx.to.begin(), tx.to.end(), 0xAA);
    tx.amount = 1000 + nonce;  // Vary amount slightly
    tx.fee = 10;
    tx.auth_mode = AuthMode::PqOnly;
    tx.auth = PqSignature{{}};  // Empty signature for unsigned tx
    
    // Sign transaction
    auto sign_result = tx::sign_transaction(tx, privkey, "Dilithium3");
    if (!sign_result.is_ok()) {
        return {{}, false};
    }
    
    return {tx, true};
}

// Benchmark: Verify 100 PQ-signed transactions
// This is the main benchmark requirement
static void BM_Verify100PQSignedTransactions(benchmark::State& state) {
    // Pre-create 100 signed transactions (done once, outside timing)
    std::vector<Transaction> txs;
    txs.reserve(100);
    
    for (size_t i = 0; i < 100; ++i) {
        auto [tx, success] = create_and_sign_bench_tx(i);
        if (!success) {
            state.SkipWithError("Failed to create signed transaction");
            return;
        }
        txs.push_back(tx);
    }
    
    uint32_t chain_id = 1;
    size_t total_verified = 0;
    size_t total_iterations = 0;
    
    // Time the verification of 100 transactions over multiple iterations
    for (auto _ : state) {
        for (auto& tx : txs) {
            auto result = tx::verify_transaction(tx, chain_id);
            if (result.is_ok() && result.value()) {
                total_verified++;
            }
        }
        total_iterations++;
    }
    
    // Set counters for reporting
    state.SetItemsProcessed(state.iterations() * 100);
    state.counters["txs_per_second"] = benchmark::Counter(
        state.iterations() * 100, benchmark::Counter::kIsRate);
    
    // Note: Average verify time per transaction will be calculated from the benchmark results
    // The real_time() reported by Google Benchmark is per iteration (100 transactions)
    // So average per tx = real_time() / 100 (converted to microseconds)
}

// Benchmark: Single transaction verification (for comparison)
static void BM_VerifySingleTransaction(benchmark::State& state) {
    auto [tx, success] = create_and_sign_bench_tx(0);
    if (!success) {
        state.SkipWithError("Failed to create signed transaction");
        return;
    }
    
    uint32_t chain_id = 1;
    
    for (auto _ : state) {
        auto result = tx::verify_transaction(tx, chain_id);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Encode transaction (for comparison)
static void BM_EncodeTransaction(benchmark::State& state) {
    auto [tx, success] = create_and_sign_bench_tx(0);
    if (!success) {
        state.SkipWithError("Failed to create signed transaction");
        return;
    }
    
    for (auto _ : state) {
        auto result = codec::encode(tx);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Decode transaction (for comparison)
static void BM_DecodeTransaction(benchmark::State& state) {
    auto [tx, success] = create_and_sign_bench_tx(0);
    if (!success) {
        state.SkipWithError("Failed to create signed transaction");
        return;
    }
    
    auto encoded = codec::encode(tx);
    if (encoded.is_err()) {
        state.SkipWithError("Failed to encode transaction for decode benchmark");
        return;
    }
    
    for (auto _ : state) {
        auto result = codec::decode(encoded.value());
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Register benchmarks
// Main requirement: Verify 100 PQ-signed transactions (reproducible with fixed iterations)
BENCHMARK(BM_Verify100PQSignedTransactions)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(10)  // Fixed iterations for reproducibility (like cargo bench)
    ->Repetitions(3)  // Run 3 times and report average
    ->ReportAggregatesOnly(false);  // Show all runs for transparency

// Additional benchmarks for comparison
BENCHMARK(BM_VerifySingleTransaction)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_EncodeTransaction)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_DecodeTransaction)->Unit(benchmark::kMicrosecond);

// Custom main to print average verify time and generate CSV
int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    
    std::cout << "\n=== PQC Ledger Benchmark: Verify 100 PQ-Signed Transactions ===\n";
    std::cout << "Running with fixed iterations for reproducibility...\n\n";
    
    ::benchmark::RunSpecifiedBenchmarks();
    
    std::cout << "\n=== Benchmark Complete ===\n";
    std::cout << "For detailed CSV output, run:\n";
    std::cout << "  ./pqc-ledger-bench --benchmark_format=csv > benchmark_results.csv\n";
    std::cout << "\nTo generate graph:\n";
    std::cout << "  python scripts/generate_benchmark_graph.py benchmark_results.csv\n";
    
    return 0;
}

