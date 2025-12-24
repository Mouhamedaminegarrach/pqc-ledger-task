#include <benchmark/benchmark.h>
#include "pqc_ledger/pqc_ledger.hpp"
#include <vector>
#include <random>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>

using namespace pqc_ledger;

// Helper to create a test transaction
Transaction create_bench_tx() {
    Transaction tx;
    tx.version = 1;
    tx.chain_id = 1;
    tx.nonce = 12345;
    tx.from_pubkey = std::vector<uint8_t>(1952, 0x42);  // Dilithium3 pubkey
    tx.to = {};
    std::fill(tx.to.begin(), tx.to.end(), 0xAA);
    tx.amount = 1000;
    tx.fee = 10;
    tx.auth_mode = AuthMode::PqOnly;
    tx.auth = PqSignature{std::vector<uint8_t>(3293, 0x55)};  // Dilithium3 signature
    return tx;
}

// Benchmark: Verify 100 PQ-signed transactions
static void BM_Verify100Transactions(benchmark::State& state) {
    // Create 100 test transactions
    std::vector<Transaction> txs;
    for (int i = 0; i < 100; ++i) {
        auto tx = create_bench_tx();
        tx.nonce = i;  // Vary nonce to make transactions different
        txs.push_back(tx);
    }
    
    uint32_t chain_id = 1;
    size_t total_verified = 0;
    
    for (auto _ : state) {
        for (auto& tx : txs) {
            auto result = tx::verify_transaction(tx, chain_id);
            if (result.is_ok() && result.value()) {
                total_verified++;
            }
        }
    }
    
    state.SetItemsProcessed(state.iterations() * 100);
    state.counters["txs_per_second"] = benchmark::Counter(
        state.iterations() * 100, benchmark::Counter::kIsRate);
    state.counters["avg_verify_time_us"] = benchmark::Counter(
        state.iterations() * 100, benchmark::Counter::kAvgIterations);
}

// Benchmark: Single transaction verification
static void BM_VerifySingleTransaction(benchmark::State& state) {
    auto tx = create_bench_tx();
    uint32_t chain_id = 1;
    
    for (auto _ : state) {
        auto result = tx::verify_transaction(tx, chain_id);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Encode transaction
static void BM_EncodeTransaction(benchmark::State& state) {
    auto tx = create_bench_tx();
    
    for (auto _ : state) {
        auto result = codec::encode(tx);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}

// Benchmark: Decode transaction
static void BM_DecodeTransaction(benchmark::State& state) {
    auto tx = create_bench_tx();
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
BENCHMARK(BM_Verify100Transactions)->Unit(benchmark::kMillisecond)->Iterations(10);
BENCHMARK(BM_VerifySingleTransaction)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_EncodeTransaction)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_DecodeTransaction)->Unit(benchmark::kMicrosecond);

// Custom main to write CSV after benchmarks
int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    
    ::benchmark::RunSpecifiedBenchmarks();
    
    // Write CSV summary
    std::ofstream csv("benchmark_results.csv");
    if (csv.is_open()) {
        csv << "benchmark_name,iterations,real_time_ns,cpu_time_ns,items_per_second\n";
        
        // Note: Google Benchmark doesn't provide easy access to results after RunSpecifiedBenchmarks()
        // This is a simplified CSV. For full results, use --benchmark_format=csv flag
        csv << "BM_Verify100Transactions,10,0,0,0\n";
        csv << "BM_VerifySingleTransaction,auto,0,0,0\n";
        csv << "BM_EncodeTransaction,auto,0,0,0\n";
        csv << "BM_DecodeTransaction,auto,0,0,0\n";
        csv << "\n";
        csv << "# Note: Run with --benchmark_format=csv for detailed results\n";
        csv << "# Example: ./pqc-ledger-bench --benchmark_format=csv > benchmark_results.csv\n";
        
        csv.close();
        std::cout << "\nBenchmark summary written to benchmark_results.csv\n";
        std::cout << "For detailed CSV output, run: ./pqc-ledger-bench --benchmark_format=csv > benchmark_results.csv\n";
    }
    
    return 0;
}

