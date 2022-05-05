



# Parallel Lossless Compressors

Huilin Xiong (hxiong)

Qier Li (qierl)



The project parallizes Huffman Coding and LZ77 compression algorithms with OpenMP, and conducts general analysis on the benchmarking and general industrial implementations.

### Brief

Compression is one of the most widely-used applications in the industry, and is developed over decades. The most recent significant variant is Facebook's zstd, which was released on 31 Agust 2016, providing great compression ratios and reasonable compress/decompress throughputs. LZ4 is also heavily used for its highest throughput among popular variants. 

Our motivation is to extremely parallelize the algorithm to improve the compress/decompress speed of one large file, which could lower the latency. Imagine the following case: transfer one large file through a fast-but-charge-by-traffic network (e.g., 5G network). Applying a parallel compression algorithm could greatly lower the latency of the process of compression and transfer, and save your pocket.

LZ77 and Huffman Coding are the 2 base algorithms repectively of Lempel-Ziv family and Entropy encoding with a long history. LZ77 is theoretically a dictionary coder, maintaining a sliding window during compression. Huffman Coding is a particular type of optimal prefix coder, representing data items with the frequencies of occurrences. Most modern compressors, such as zstd, are based on those 2 algorithms.

This project has learned and implemented this two most important lossless data-compression algorithm （sequential version), then designed and implemented parallel versions for each using OpenMP. The project further analyzed performance including compression ratio and throughput/speedups on PSC machines. By such a programming practice, the project developed some conclusions and assumptions for the parallism strategy and potentials for general compression algorithms and Facebook's zstd.



### Reference and Resource

This project heavily refers to *Mastering Algorithms with C: Useful Techniques from Sorting to Encryption*, especially implementing the sequential versions of algorithms.

The algorithms is developed to be executable and configurable with timer in **C/C++, OpenMP**, and then test on mainly two types of files, that literature such as *War and Peace* and source codes such as Angular.js. 

The tests is run on PSC machines sequentially and parallelly from num_threads 1 to 128.



### Huffman Coding





| 10wap(32M) |                          |                                 |                       |                 |               |
| ---------- | ------------------------ | ------------------------------- | --------------------- | --------------- | ------------- |
| Algorithm  | Thread Num               | Input Time (ms)                 | Compression Time (ms) | Total Time (ms) | Total Speedup |
| Huffman    | 1                        | 28.332                          | 1658.56               | 1702.26         | 1.000         |
|            | 4                        | 13.6374                         | 667.689               | 696.701         | 2.443         |
|            | 8                        | 11.8753                         | 209.68                | 236.976         | 7.183         |
|            | 16                       | 11.407                          | 107.579               | 153.547         | 11.086        |
|            | 32                       | 11.8537                         | 57.2743               | 84.5705         | 20.128        |
|            | 64                       | 23.626                          | 46.0987               | 85.2528         | 19.967        |
|            | Raw File Bytes: 32022870 | Compressed File Bytes: 17987649 |                       |                 |               |
|            | Compression Ratio        | 0.562 (其实应该取倒数，1.779)   |                       |                 |               |
|            | Decode Time              | 616.528ms                       |                       |                 |               |

| angular.js (21M) |                         |                                |                       |                 |               |
| ---------------- | ----------------------- | ------------------------------ | --------------------- | --------------- | ------------- |
| Algorithm        | Thread Num              | Input Time (ms)                | Compression Time (ms) | Total Time (ms) | Total Speedup |
| Huffman          | 1                       | 20.782                         | 1037.930              | 1087.510        | 1.000         |
|                  | 4                       | 11.148                         | 271.698               | 294.180         | 3.697         |
|                  | 8                       | 8.660                          | 132.590               | 152.936         | 7.111         |
|                  | 16                      | 8.530                          | 68.518                | 88.530          | 12.284        |
|                  | 32                      | 18.716                         | 37.405                | 67.559          | 16.097        |
|                  | 64                      | 14.617                         | 23.218                | 49.677          | 21.892        |
|                  | Raw File Bytes 22046527 | Compressed File Bytes:13294321 |                       |                 |               |
|                  | Compression Ratio       | 0.603(1.658)                   |                       |                 |               |
|                  | Decode Time             | 381.781 ms                     |                       |                 |               |



### LZ77

#### Intro

LZ77, short for Lempel-Ziv algorithm, is a lossless data-compression algorithm created by Lempel and Ziv in 1977. LZ77 has been adopted for many modern complicated algorithms such as Facebook's zstd.

#### Breakdown



<img src="lz77_intro.png" alt="lz77_intro" style="zoom: 50%;" />

​				the slide refered from 15-853 lesson

LZ77 compression process is a simple sliding-window process: from the start to the end, continually matching the longest sequence from buffer to the current window, and record the sequence in buffer as a (offset, length, ending character) tuple.

The sequential process contains the following steps (assuming the original file and encoded file could fit in memory): 

1. Read the file into memory.
2. Initialize the sliding windows and starts encoding.
3. Write the encoded content to another file.

For most storage (disk, SSD...), reading/writing will not benefit from parallism (or even worse with parallism), so step 1 and step 3 are not parallelizable.

Generally the parallelizable of the process is the step 2. And as the file size grows, the ratio of time of step 2 increases. It could be concluded that large file is more parallelizable. And a linear-speedup of compression (throughput) is expected for large files.

The strategy of parallism is to **partition consecutive parts of original files, and then encode the partitions each as sequentially** and indicates the parallism in the headers. The pitfall for ratio/speedup is the following:

1. Partition might break a potential matched sequence and require more encoding tuples.
2. Extra padding is needed, as encoding is bit-level and partition and headers are byte-level.
3. Extra headers is needed, to indicates the number of partitions and positions. 

Above 2 pitfalls just requires a few bytes to encode, which is negligible and the following tests prove it.

#### Deliverables

A executable program is developed and configurable based on the following strategy. The decoding process is not parallelized because it is comparably fast.

Inside the path lz77/src, there's a Makefile to compile a executable file LZ77.

````bash
% ./lz77 -h
./lz77: invalid option -- 'h'
Usage: ./lz77 -f <filename>
                [-x for extract]
                [-t <number of threads, 0 for sequential>] 
````

Tag -t indicates the number of threads.

The tuple is configurable in the source file compress.h as following:

````c++
#define LZ77_TYPE_BITS   1 // indicates a matched sequence or a single character
#define LZ77_WINOFF_BITS 12 // [0, 4096]
#define LZ77_BUFLEN_BITS 5 // [0, 32]
#define LZ77_NEXT_BITS   8 // [0, 256]

#define LZ77_WINDOW_SIZE  4096 // must in [2, 2^LZ77_WINOFF_BITS]
#define LZ77_BUFFER_SIZE  32 // must in [2, 2^LZ77_BUFLEN_BITS]
````

As the window size and buffer size increases, the length of the matched sequence is likely to be longer, but it requires more bits to represent each tuple and also more time to compress. And it's highly trace-dependent. Based on the test files, the default configuration achieves a reasonable throughput and compression ratio.

#### Result

The records of benchmarking is shown below:

Correctness is checked by md5sum and diff command.

Compression ratio negligible for sequential and parallel results.

When the number of threads is below 16, the speedup is nearly linear to num_threads.

When the number of threads reach 128, the speedup stops increasing.

For files that has more duplicated content (angular.js), the compression ratio is much better.

| war&peace(32M) |                         |                                |                 |               |
| -------------- | ----------------------- | ------------------------------ | --------------- | ------------- |
| Algorithm      | Thread Num              | Compress Time                  | Total Time (ms) | Total Speedup |
| lz77           | 1                       | 21807.544                      | 21844.794       | 1.000         |
|                | 4                       | 5467.133                       | 5498.592        | 3.973         |
|                | 8                       | 2753.437                       | 2792.868        | 7.822         |
|                | 16                      | 1558.221                       | 1595.659        | 13.690        |
|                | 32                      | 962.479                        | 1001.362        | 21.815        |
|                | 64                      | 422.977                        | 453.289         | 48.192        |
|                | 128                     | 325.274                        | 357.07          | 61.178        |
|                | Raw File Bytes 32022870 | Compressed File Bytes:20156523 |                 |               |
|                | Compression Ratio       | 1.59                           |                 |               |
|                | Decode Time             | 851.717ms                      |                 |               |

| angular.js (21M) |                         |                               |                 |               |
| ---------------- | ----------------------- | ----------------------------- | --------------- | ------------- |
| Algorithm        | Thread Num              | Compress Time                 | Total Time (ms) | Total Speedup |
| lz77             | 1                       | 9298.655                      | 9319.683        | 1.000         |
|                  | 4                       | 2458.891                      | 2476.388        | 3.763         |
|                  | 8                       | 1225.719                      | 1252.558        | 7.441         |
|                  | 16                      | 644.235                       | 665.019         | 14.014        |
|                  | 32                      | 338.801                       | 361.162         | 25.805        |
|                  | 64                      | 185.644                       | 211.809         | 44.000        |
|                  | 128                     | 201.124                       | 220.842         | 42.201        |
|                  | Raw File Bytes 22046527 | Compressed File Bytes:9631119 |                 |               |
|                  | Compression Ratio       | 2.29                          |                 |               |
|                  | Decode Time             | 438.476 ms                    |                 |               |

##### Analysis

One of the reason that speedup can't increase linearly when number of threads increase is **uneven distribution**. Although we partitions the file into parts of same size, the **matching process** might requires different amount of scans for different positions. When the current sequence unmatches, the scan stops and continute to the next section. And it's nature for files having different amount of duplicated sequence at different positions.

By recording each time of OpenMP thread, we proves that uneven workload distribution is one of the reason that it fails to achieve linear speedup.

<img src="uneven.png" alt="uneven" style="zoom:50%;" />

The reason that more duplicated content has better the compression ratio is the nature of LZ77 algorithms. With more duplicated content, tuples could reprsents longer sequence in the original files.

### Comparison for Huffman vs. LZ77 and among different traces

#### Speedup

The speedups for each algorithm on 3 main test traces are shown below:

<img src="text_small.jpg" alt="text_small" style="zoom:50%;" />

<img src="text_big.jpg" alt="text_big" style="zoom:50%;" />



<img src="source_code.jpg" alt="text_small" style="zoom:50%;" />

It could be concluded:

1. LZ77 is genearally more parallelizable than Huffman, probably for its higher ratio of parallelizable part and huffman's overhead of synchronization.
2. Larger files has better speedups than smaller files when number of threads is big, probably for its greater ratio of parallelizable part comparing to the overhead.
3. The performance of parallism is highly trace-dependent. Literature files shows better speedups for LZ77 than source codes, probably for its more even workload distributions. On the other hand, Huffman doesn't suffer from uneven workload distribution.

#### Compression ratio

LZ77 shows a much better compression ratio (2.29) than Huffman Coding (1.66) on angular.js, while similar ratio (1.59) with Huffman Coding (1.78) on *war and peace*.

It's because the nature of the different algorithms: LZ77 performs better when content is more duplicated, while Huffman Coding performs better when the byte patterns show greatly different frequencies.

### Why not GPU/CUDA?

The reason why we don't implement parallel versions of algorithms on CUDA is clear.

##### Memory bound and communication overhead

Compression algorithms requires a great amount of memory, that it reads the whole original file into memory and writes backs the whole encoded file.

If apply CUDA-version for the algorithms, the communication between CPU and GPU includes the whole original file and the whole encoded file.

Let's check the throughput of industrial implementations:

![industry_performance](industry_performance.png)

Considering the bandwith of bus (which is probably what limits the communication between CPU and GPU:

PCI-E Version 3.x:

8 GT/s

- **×1:** 985 MB/s
- **×16:** 15.75 GB/s



It's absolutely not reasonable to develop a GPU version of compression algorithms as the bandwith of communication overhead is nearly the throughput of the sequential compression algorithms. Even though our implementation is slow enough to gain speedup with the communication overhead, it's meaningless to do so.

##### Algorithms are not highly parallelizable (with great number of threads)

As mentioned above, the 2 algorithms are **not** highly parallelizable, especially the Huffman Coding. For both algorithms the ratio of parallelizable part limits the speedups. 

What's more, the overhead might outweight the benefits of parallism. For Huffman Coding, it requires extra synchronization (althought the atomic operation is not so costly) to record the frequency of characters, so the overhead increases as the number of threads increase. For LZ77, partitioning breaks character sequences which might lead to a worst compression ratio.

In the tests we conducted, for general file (size 3MB ~ 30MB), the speedup stops increasing or increases very little when the number of threads reach around 32~128 for both algorithms. There is no reason to implement a CUDA-version for compression algorithms as the threads of CUDA are numbers of thousands.





### Conclusion

xxx





---------------------------------------





Huffman analysis + parallel design, etc…	- Xiong

Huffman family’s implementation versus our simple one (e.g., 新huffman用了什么新技术/算法）- Xiong

Lz77 analysis + parallel design, etc…	- Qier

Modern lz family’s implementation versus our simple one (e.g., lz4用了什么新技术/算法）- Qier

Comparison between Huffman and lz77 - Qier

Why NOT GPU? - Qier

性能总结+zstd分析 – Qier



