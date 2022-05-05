#include "HuffmanCompressor.h"
#include <iostream>
#include <chrono>

using namespace std;

static void show_help() {
    printf("-------------[Usage]-------------\n");
    printf("./main {e|d} {input_filepath} {output_filepath} {map_filepath} {thread_num}\n");
    printf("e: encode(compress)  d: decode(extract)\n");
    printf("input_filepath: *.txt\n");
    printf("output_filepath: *.bin\n");
    printf("map_filepath: *_map.txt\n");
    printf("[e.g.] ./main e testfile.txt output.bin test_map.txt 4\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    using namespace std::chrono;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;

    HuffmanCompressor hc;
    char* operation = argv[1];
    char* input = argv[2];
    char* output = argv[3];
    char* mapfilepath = argv[4];
    int thread_num = atoi(argv[5]);


    hc.set_filepath(input,output,mapfilepath);
    hc.set_thread_num(thread_num);// or set to 1 to execute sequentially

    auto init_start = Clock::now();
    double compute_time = 0;

    if(*operation=='e'){
        hc.get_encoded_file();
    } else if(*operation=='d'){
        hc.get_decoded_file();
    } else {
        show_help();
        return 0;
    }
    compute_time += duration_cast<dsec>(Clock::now() - init_start).count();
    cout<<"[Total    Time]  "<<compute_time<<endl;
    return 0;
}