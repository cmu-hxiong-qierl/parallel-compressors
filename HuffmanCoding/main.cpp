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
        // encode file:  ./main d test.txt out.bin map.txt 4
        hc.get_encoded_file();
    } else {
        // decode file:  ./main d out.bin map.txt 4
        hc.get_decoded_file();
    }

//    cout<<"hi"<<endl;
    compute_time += duration_cast<dsec>(Clock::now() - init_start).count();
    cout<<"***** compute time= "<<compute_time<<endl;
    cout<<"OK"<<endl;
    return 0;
}