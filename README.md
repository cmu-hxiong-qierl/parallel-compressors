# Parallel Lossless Compressors

Huilin Xiong (hxing)

Qier Li (qierl)

## Summary

This project aims to implement and develop lossless compression algorithms on multi-core machines, starting from parallelizing LZ77 and Huffman Coding. The project would explore and evaluate ratio and throughput on different workflows and applications. Further analysis and tuning might be conducted referring to modern algorithms such as Facebook's zstd for different types of machines.

## Background

Compression is one of the most widely-used application in industry, and is developed over decades. The most recent significant variant is Facebook's zstd, which was released on 31 Agust 2016, providing great compression ratios and reasonable compress/decompress throughputs. Lz4 is also heavily used for its best throughput among popular variants. 

Our motivation is to extremely parallelize the compress/decompress speed of one large file, which could lower the latency. Imagine the following case: transfer one large file through a fast-but-charge-by-traffic network (e.g., 5G network). Applying a parallel compress algorithm could greatly lower the latency of the process of compress and transfer, and save your pocket.

LZ77 and Huffman Coding, the two specific algorithm the project would start from, are the 2 base algorithms repectively of Lempel-Ziv family and Entropy encoding with a long history. LZ77 is theoretically a dictionary coder, maintaining a sliding window during compression. Huffman Coding is a particular type of optimal prefix coder, representing data item with the frequency of occurrence. Most modern compressors, such as zstd, are based on those 2 algorithms.

## Challenge

#### File Size versus Partition Number

One common and widely-used way to parallelize is partition the file into several chunks, compress each and combine the compressed one together. Obviously this high-level parallelism might leads to a lower compression ratio, especially when some patterns happen across the boundary of chunks so that it would fail to be detected in LZ77 algorithms. As the number of cores increases (e.g., up to 128), the ratio would be extremly compromised.

#### Communication Cost when Parallelizing Huffman Coding

Huffman coding needs to collect the global information at first (the work frequency), which may lead to much communication cost in parallel implementation. 

#### Compression Ratio on Different Traces

If the logics of the parallel implementation differs from the sequential one, or the parallel one is not deterministic, it is neccessary to test the ratio for multiple times. Besides, the compression ratio for different files greatly differs, that some compressor even check whether a compressed file is greater than the original one (If so, it would take the original one as the compressed file). The performance should be evaluated on different traces too.

## Resources

#### Machine Resources

Development would be on laptops, while test would be on GHC machines and PSC bridge machines, for a regular multi-core machine case and an extreme multi-core machine case respectively. GPUs are not in the plan because it's not suitable for the compression.

#### Reference

Several sequential versions of Huffman Encoding and LZ77 would be refered. We would implement our own implementations of sequential solution for the benchmark and as the starter code for parallel ones too.
Research papers and open-sourced projects of modern compressor would be refered in the further analysis. 

## Goals and Deliverables

##### Plan to Achieve

1. Sequential implementation of Huffman encoding and LZ77 compression.
2. Parallel versions of these 2 implementations.
3. Performance benchmarks and comparison. The project aims to provide linear speedup of throughput over number of cores and the similar compress ratio with the sequential one.
4. Tuning for different machines and different workflows.

##### Hope to Achieve

1. Futher development of the alogrithms based on industrial implementations such as Facebook's zstd and LZ4. Push the performance to the limit (e.g., compare our implementations to pzstd the official simple implementation of parallel zstd).
2. GPU implementation, and probably the analysis showing why GPU doesn't suit for compression problem.

##### Compromised goal when goes slow

One correct parallel version of Huffman or LZ77 with reasonable performance.

## Platform Choice

The compression algorithms would be implemented in C for performance. Tests would be conducted on GHC machines and PSC bridge machines, which represents regular and extreme multi-core machines.

## Schedule

| Time                               | Milestone                                                    |
| ---------------------------------- | ------------------------------------------------------------ |
| Week 1 03/25~04/04                 | Background research and paper reading. Understand algorithms. |
| Week  2 04/04~04/11 (Checkpoint)   | Sequential implementation of Huffman encoding and LZ77 compression. |
| Week  3 04/11~04/18                | Parallielizing Huffman encoding and LZ77 compression.        |
| Week  4 04/18~04/25                | Benchmarks, tuning and analysis.                             |
| Week  5 04/25~04/29 (Final Report) | Further analysis, improvement and prepare for the final report. |

