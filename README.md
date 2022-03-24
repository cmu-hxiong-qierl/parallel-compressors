# Parallel Lossless Compressors

Huilin Xiong (hxiong)

Qier Li (qierl)

## Summary

This project aims to implement and develop lossless compression algorithms on multi-core machines, starting from parallelizing LZ77 and Huffman Coding compressions. The project would explore and evaluate the compression ratio and throughput on different workflows, applications and machines. Further analysis and tuning might be conducted referring to modern algorithms such as Facebook's zstd.

## Background

Compression is one of the most widely-used applications in the industry, and is developed over decades. The most recent significant variant is Facebook's zstd, which was released on 31 Agust 2016, providing great compression ratios and reasonable compress/decompress throughputs. LZ4 is also heavily used for its highest throughput among popular variants. 

Our motivation is to extremely parallelize the algorithm to improve the compress/decompress speed of one large file, which could lower the latency. Imagine the following case: transfer one large file through a fast-but-charge-by-traffic network (e.g., 5G network). Applying a parallel compression algorithm could greatly lower the latency of the process of compression and transfer, and save your pocket.

LZ77 and Huffman Coding, the two specific algorithms the project would start from, are the 2 base algorithms repectively of Lempel-Ziv family and Entropy encoding with a long history. LZ77 is theoretically a dictionary coder, maintaining a sliding window during compression. Huffman Coding is a particular type of optimal prefix coder, representing data items with the frequencies of occurrences. Most modern compressors, such as zstd, are based on those 2 algorithms.

## Challenge

#### File Size versus Partition Number
One common and widely-used way to parallelize is partitioning the file into several chunks, compressing each and then combining the compressed ones together. Obviously, this high-level parallelism might leads to a lower compression ratio, especially when some replicated patterns happen across the boundary of chunks so that it would fail to be detected in LZ77 algorithms. As the number of cores increases (e.g., up to 128), the ratio would be extremly compromised.

#### Communication Cost when Parallelizing Huffman Coding

Huffman coding needs to collect the global information at first (the word frequency), which may lead to a great amount of communication cost in the parallel implementation. 

#### Compression Ratio on Different Traces

If the logic of the parallel implementation differs from the sequential one and the parallel one is not deterministic, it is neccessary to test the ratio for multiple times to ensure a stable ratio. Besides, the compression ratio for different files greatly differs, that some compressor even check whether a compressed file is greater than the original one (If so, it would take the original one instead of the compressed one as the output). Therefore, the performance should be evaluated on different traces.

## Resources

#### Machine Resources

Development would be on laptops, while tests would be on GHC machines and PSC bridge machines, to estimate performance of a regular multi-core machine case and an extreme multi-core machine case respectively. GPUs are not in the plan because it's not suitable for the compression.

#### Reference

Several sequential versions of Huffman Encoding and LZ77 would be refered. However, we would implement our own implementations of sequential solution for the benchmark and as the starter code for parallel ones too.
Research papers and open-sourced projects of modern compressor would be refered in the further analysis. 

## Goals and Deliverables

#### Plan to Achieve

1. Sequential implementations of Huffman encoding and LZ77 compression.
2. Parallel versions of these 2 implementations.
3. Performance benchmark and comparison. The project aims to provide a linear speedup of throughput over the number of cores, and a similar compression ratio with the sequential one.
4. Tuning for different machines and different workflows.

#### Hope to Achieve
1. Futher development of the alogrithms based on industrial implementations such as Facebook's zstd and LZ4. Push the performance to the limit (e.g., compare our implementations to pzstd, the official simple implementation of parallel zstd).
2. GPU implementation, and probably the analysis showing why GPU doesn't suit for compression problem.

#### Compromised goal when work goes slow

One correct parallel version of Huffman or LZ77 with reasonable performance.

## Platform Choice

The compression algorithms would be implemented in C for the performance. Tests would be conducted on GHC machines and PSC bridge machines, which represents regular and extreme multi-core machines respectively.

## Schedule

| Time                               | Milestone                                                    |
| ---------------------------------- | ------------------------------------------------------------ |
| Week 1 03/25~04/04                 | Background research and paper reading. Understand algorithms. |
| Week  2 04/04~04/11 (Checkpoint)   | Sequential implementation of Huffman encoding and LZ77 compression. |
| Week  3 04/11~04/18                | Parallielizing Huffman encoding and LZ77 compression.        |
| Week  4 04/18~04/25                | Benchmarks, tuning and analysis.                             |
| Week  5 04/25~04/29 (Final Report) | Further analysis, improvement and prepare for the final report. |

