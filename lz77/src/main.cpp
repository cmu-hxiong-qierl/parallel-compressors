#include "compress.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "CycleTimer.h"

int main(int argc, char *argv[]) {
    int opt = 0;
    char *inputFilename = NULL;
    bool isExtract = false;
    int num_threads = 0;

    // Read command line arguments
    do {
        opt = getopt(argc, argv, "f:t:x");
        switch (opt) {
        case 'f':
            inputFilename = optarg;
            break;
        
        case 'x':
            isExtract = true;
            break;

        case 't':
            num_threads = atoi(optarg);
            break;

        case -1:
            break;

        default:
            break;
        }
    } while (opt != -1);

    if (inputFilename == NULL) {
        printf("Usage: %s -f <filename>\n \t\t[-x for extract]\n \t\t[-t <number of threads, 0 for sequential>] \n", argv[0]);
        return -1;
    }
    
    if (isExtract) {
        if (num_threads == 0) {
            double start_time = CycleTimer::currentSeconds();
            FILE *input = fopen(inputFilename, "rb");
            fseek(input, 0, SEEK_END);
            long fsize = ftell(input);
            fseek(input, 0, SEEK_SET);
            unsigned char* compressed = (unsigned char*)malloc(fsize);
            fread(compressed, fsize, 1, input);
            fclose(input);
            
            unsigned char** original = (unsigned char**)malloc(sizeof(unsigned char**));
            double uncompress_start_time = CycleTimer::currentSeconds();
            long original_size = lz77_uncompress(compressed, original);
            double uncompress_end_time = CycleTimer::currentSeconds();
            std::string path{inputFilename};
            path = path.substr(0,path.find_last_of('.'));
            path.append(strdup(".lz77_ex"));
            FILE *output = fopen(path.c_str(), "wb");
            fwrite(*original, original_size, 1, output);
            double end_time = CycleTimer::currentSeconds();

            printf("Total time: %.3f ms\n", 1000.f * (end_time - start_time));
            printf("Uncompress time: %.3f ms\n", 1000.f * (uncompress_end_time - uncompress_start_time));
        } else {
            double start_time = CycleTimer::currentSeconds();
            FILE *input = fopen(inputFilename, "rb");
            int num_threads;
            int original_size;
            fread(&num_threads, sizeof(int), 1, input);
            fread(&original_size, sizeof(int), 1, input);
            printf("original_size: %d\n", original_size);
            int size_local = original_size / num_threads;
            int size_last = original_size - size_local * (num_threads - 1);
            
            std::string path{inputFilename};
            path = path.substr(0,path.find_last_of('.'));
            path.append(strdup(".lz77_ex"));
            FILE *output = fopen(path.c_str(), "wb");
            for (int i = 0; i < num_threads; i++) {
                int partition_size;
                fread(&partition_size, sizeof(int), 1, input);
                printf("partition %d size: %d\n", i, partition_size);
                unsigned char* compressed = (unsigned char*)malloc(partition_size + sizeof(int) * 2);
                if (i == num_threads - 1) {
                    *(int *)compressed = size_last;
                } else {
                    *(int *)compressed = size_local;
                }
                fread(compressed + sizeof(int), partition_size, 1, input);
                unsigned char** original = (unsigned char**)malloc(sizeof(unsigned char**));
                long original_size_t = lz77_uncompress(compressed, original);
                printf("partition %d original size: %ld\n", i, original_size_t);
                fwrite(*original, original_size_t, 1, output);
                delete compressed;
                delete *original;
                delete original;
            } 
            double end_time = CycleTimer::currentSeconds();
            fclose(input);
            fclose(output);

            printf("Total time: %.3f ms\n", 1000.f * (end_time - start_time));
        }
    } else {
        if (num_threads == 0) {
            double start_time = CycleTimer::currentSeconds();
            FILE *input = fopen(inputFilename, "rb");
            fseek(input, 0, SEEK_END);
            long fsize = ftell(input);
            fseek(input, 0, SEEK_SET);
            unsigned char* original = (unsigned char*)malloc(fsize);
            fread(original, fsize, 1, input);
            fclose(input);

            unsigned char** compressed = (unsigned char**)malloc(sizeof(unsigned char**));
            double compress_start_time = CycleTimer::currentSeconds();
            long compressed_size = lz77_compress(original, compressed, fsize);
            double compress_end_time = CycleTimer::currentSeconds();
            std::string path{inputFilename};
            path.append(strdup(".lz77"));
            FILE *output = fopen(path.c_str(), "wb");
            fwrite(*compressed, compressed_size, 1, output);
            double end_time = CycleTimer::currentSeconds();

            printf("Total time: %.3f ms\n", 1000.f * (end_time - start_time));
            printf("Compress time: %.3f ms\n", 1000.f * (compress_end_time - compress_start_time));
        } else {
            double start_time = CycleTimer::currentSeconds();
            FILE *input = fopen(inputFilename, "rb");
            fseek(input, 0, SEEK_END);
            long fsize = ftell(input);
            fseek(input, 0, SEEK_SET);
            unsigned char* original = (unsigned char*)malloc(fsize);
            fread(original, fsize, 1, input);
            fclose(input);

            unsigned char** compressed = (unsigned char**)malloc(sizeof(unsigned char**) * num_threads);
            double compress_start_time = CycleTimer::currentSeconds();
            int *compressed_sizes = parallel_lz77_compress(original, compressed, fsize, num_threads);
            double compress_end_time = CycleTimer::currentSeconds();
            std::string path{inputFilename};
            path.append(strdup(".lz77_t"));
            FILE *output = fopen(path.c_str(), "wb");
            fwrite(&num_threads, sizeof(int), 1, output);
            int fsize_int = (int)fsize;
            fwrite(&fsize_int, sizeof(int), 1, output);
            for (int i = 0; i < num_threads; i++) {
                fwrite(compressed_sizes + i, sizeof(int), 1, output);
                fwrite(*(compressed + i), compressed_sizes[i], 1, output);
            }
            double end_time = CycleTimer::currentSeconds();

            printf("Total time: %.3f ms\n", 1000.f * (end_time - start_time));
            printf("Compress time: %.3f ms\n", 1000.f * (compress_end_time - compress_start_time));
            fclose(input);
            fclose(output);
        }
    }


    return 0;
}
