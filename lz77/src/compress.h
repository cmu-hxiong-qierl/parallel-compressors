// configurations based on Mastering Algorithms with C: Useful Techniques from Sorting to Encryption
#define LZ77_TYPE_BITS   1
#define LZ77_WINOFF_BITS 12 // 4096
#define LZ77_BUFLEN_BITS 5 // 32
#define LZ77_NEXT_BITS   8 // 256

#define LZ77_WINDOW_SIZE  4096 // must in [2, 2^LZ77_WINOFF_BITS]
#define LZ77_BUFFER_SIZE  32 // must in [2, 2^LZ77_BUFLEN_BITS]

#define LZ77_PHRASE_BITS  (LZ77_TYPE_BITS + LZ77_WINOFF_BITS + LZ77_NEXT_BITS + LZ77_BUFLEN_BITS)
#define LZ77_SYMBOL_BITS  (LZ77_TYPE_BITS + LZ77_NEXT_BITS)

int lz77_compress(const unsigned char *original, unsigned char **compressed, int size);
int lz77_uncompress(const unsigned char *compressed, unsigned char **original);

// **compressed has size of num_threads
int* parallel_lz77_compress(const unsigned char *original, unsigned char **compressed, int size, int num_threads);
int parallel_lz77_uncompress(const unsigned char *compressed, unsigned char **original, int num_threads);