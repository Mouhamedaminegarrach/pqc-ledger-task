#!/usr/bin/env python3
"""
Generate graph from benchmark CSV results.

Usage:
    python scripts/generate_benchmark_graph.py [benchmark_results.csv]

If no file is provided, looks for benchmark_results.csv in current directory.
"""

import sys
import csv
import matplotlib.pyplot as plt
import os
from pathlib import Path

def parse_benchmark_csv(filename):
    """Parse Google Benchmark CSV output."""
    results = []
    
    if not os.path.exists(filename):
        print(f"Error: File {filename} not found")
        print("Run benchmark with: ./pqc-ledger-bench --benchmark_format=csv > benchmark_results.csv")
        return None
    
    with open(filename, 'r') as f:
        # Skip header if present
        reader = csv.reader(f)
        header = next(reader, None)
        
        # Google Benchmark CSV format:
        # name,iterations,real_time,cpu_time,time_unit,bytes_per_second,items_per_second,label,error_occurred,error_message
        for row in reader:
            if not row or row[0].startswith('#'):
                continue
            
            try:
                name = row[0]
                iterations = int(row[1]) if len(row) > 1 else 0
                real_time = float(row[2]) if len(row) > 2 and row[2] else 0
                cpu_time = float(row[3]) if len(row) > 3 and row[3] else 0
                time_unit = row[4] if len(row) > 4 else 'ns'
                items_per_second = float(row[6]) if len(row) > 6 and row[6] else 0
                
                # Convert time to microseconds for consistency
                if time_unit == 'ns':
                    real_time_us = real_time / 1000.0
                    cpu_time_us = cpu_time / 1000.0
                elif time_unit == 'us':
                    real_time_us = real_time
                    cpu_time_us = cpu_time
                elif time_unit == 'ms':
                    real_time_us = real_time * 1000.0
                    cpu_time_us = cpu_time * 1000.0
                else:
                    real_time_us = real_time
                    cpu_time_us = cpu_time
                
                results.append({
                    'name': name,
                    'iterations': iterations,
                    'real_time_us': real_time_us,
                    'cpu_time_us': cpu_time_us,
                    'items_per_second': items_per_second
                })
            except (ValueError, IndexError) as e:
                print(f"Warning: Skipping invalid row: {row} ({e})")
                continue
    
    return results

def generate_graph(results, output_file='benchmark_graph.png'):
    """Generate graph from benchmark results."""
    if not results:
        print("No valid results to graph")
        return
    
    # Filter for the main benchmark (Verify100PQSignedTransactions or Verify100Transactions)
    main_bench = [r for r in results if 'Verify100PQSignedTransactions' in r['name'] or 'Verify100Transactions' in r['name']]
    other_benches = [r for r in results if 'Verify100PQSignedTransactions' not in r['name'] and 'Verify100Transactions' not in r['name']]
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # Graph 1: Verify 100 Transactions - Average Time Per Transaction
    if main_bench:
        bench = main_bench[0]
        # Calculate average time per transaction
        avg_time_per_tx_us = bench['real_time_us'] / 100.0
        avg_time_per_tx_ms = avg_time_per_tx_us / 1000.0
        total_time_ms = bench['real_time_us'] / 1000.0
        
        # Show both total time and average per tx
        categories = ['Total (100 txs)', 'Avg per tx']
        times = [total_time_ms, avg_time_per_tx_ms]
        colors = ['steelblue', 'coral']
        
        bars = ax1.bar(categories, times, color=colors, alpha=0.7)
        ax1.set_ylabel('Time (milliseconds)')
        ax1.set_title('PQ Signature Verification: 100 Transactions', fontweight='bold')
        ax1.grid(axis='y', alpha=0.3)
        
        # Add value labels on bars
        for bar, time in zip(bars, times):
            height = bar.get_height()
            ax1.text(bar.get_x() + bar.get_width()/2., height,
                    f"{time:.3f} ms",
                    ha='center', va='bottom', fontweight='bold', fontsize=10)
        
        # Add annotation for average verify time
        ax1.text(0.5, 0.95, f'Average verify time: {avg_time_per_tx_us:.2f} μs per transaction',
                transform=ax1.transAxes, ha='center', va='top',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5),
                fontsize=11, fontweight='bold')
    
    # Graph 2: All Benchmarks Comparison
    if other_benches:
        names = [b['name'].replace('BM_', '') for b in other_benches]
        times = [b['real_time_us'] for b in other_benches]
        
        ax2.barh(names, times, color='coral', alpha=0.7)
        ax2.set_xlabel('Time (microseconds)')
        ax2.set_title('Benchmark Comparison')
        ax2.grid(axis='x', alpha=0.3)
        
        # Add value labels
        for i, (name, time) in enumerate(zip(names, times)):
            ax2.text(time, i, f" {time:.2f} μs", va='center')
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Graph saved to {output_file}")
    
    # Print summary with average verify time prominently displayed
    print("\n" + "="*60)
    print("=== Benchmark Summary ===")
    print("="*60)
    if main_bench:
        bench = main_bench[0]
        # Calculate average verify time per transaction
        avg_time_per_tx_us = bench['real_time_us'] / 100.0
        avg_time_per_tx_ms = avg_time_per_tx_us / 1000.0
        
        print(f"\nVerify 100 PQ-Signed Transactions:")
        print(f"   Total time for 100 txs: {bench['real_time_us'] / 1000.0:.2f} ms")
        print(f"   *** Average verify time per tx: {avg_time_per_tx_us:.2f} us ({avg_time_per_tx_ms:.3f} ms) ***")
        print(f"   Iterations: {bench['iterations']}")
        if bench['items_per_second'] > 0:
            print(f"   Throughput: {bench['items_per_second']:.0f} transactions/sec")
    
    if other_benches:
        print(f"\nOther Benchmarks (for comparison):")
        for bench in other_benches:
            name = bench['name'].replace('BM_', '')
            # Use 'us' instead of μ symbol to avoid encoding issues on Windows
            print(f"   {name}: {bench['real_time_us']:.2f} us")
    
    print("="*60)

def main():
    if len(sys.argv) > 1:
        csv_file = sys.argv[1]
    else:
        csv_file = 'benchmark_results.csv'
    
    print(f"Reading benchmark results from {csv_file}...")
    results = parse_benchmark_csv(csv_file)
    
    if results:
        generate_graph(results)
    else:
        print("\nTo generate benchmark CSV:")
        print("  ./build/bin/Release/pqc-ledger-bench --benchmark_format=csv > benchmark_results.csv")
        print("\nThen run this script again.")

if __name__ == '__main__':
    main()

