#include "HuffmanCompressor.h"
#include <iostream>
#include <chrono>

using namespace std;
int main(int argc, char *argv[])
{
    // ./main "file_to_be_encode" "encoded_file"
    using namespace std::chrono;
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::duration<double> dsec;

    HuffmanCompressor hc;
    char* input = argv[1];
    char* output = argv[2];
    int thread_num = atoi(argv[3]);

    hc.set_filepath(input,output);
    hc.set_thread_num(thread_num);// or set to 1 to execute sequentially

    auto init_start = Clock::now();
    double compute_time = 0;

    hc.get_encoded_file();
    compute_time += duration_cast<dsec>(Clock::now() - init_start).count();
//    hc.get_encoded_map();
//    char* decoded_file = argv[3];
//    hc.set_decode_filepath(decoded_file);
//    hc.get_decoded_file();
    cout<<"***** compute time= "<<compute_time<<endl;
    cout<<"OK"<<endl;
    return 0;
}