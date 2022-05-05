#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <queue>
#include <stdio.h>
#include <unordered_map>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
#include <chrono>
#include "huffmanBitSet.h"

#define CHAR_NUM 256
using namespace std;
using namespace std::chrono;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::duration<double> dsec;


struct hnode
{
    int id;// char id in ascii
    int freq;
    hnode *left;
    hnode *right;
    hnode(){
        left = nullptr;
        right = nullptr;
        freq = 0;
    }
};

class HuffmanCompressor{
private:
    char* input_filepath;
    char* output_filepath;
    char* map_filepath;
    uint32_t rawdata_size;
    unsigned char* rawdata_buffer;

    uint32_t encoded_bytesize;
    uint32_t encoded_bitsize;
    int encode_thread_num;
    unsigned char* encoded_data_buffer;
    hnode* hnode_array[CHAR_NUM];
    int threads_num = 1;
    uint32_t* thread_output_bitsize;
    uint32_t* thread_output_bit_bound;

    struct compare{
        bool operator()(hnode* a, hnode* b){
            return a->freq >= b->freq;
        }
    };
    priority_queue<hnode*,vector<hnode*>,compare> pq;
    map<int, huffmanBitSet> char_code_map;
    map<int, string> char_strcode_map;
    uint32_t all_freq[CHAR_NUM] = {0};


public:
    ~HuffmanCompressor(){
        if(pq.size()==0){
            return;
        }
        queue<hnode*> q;
        q.push(pq.top());
        while(!q.empty()){
            hnode* node = q.front();
            q.pop();
            if(node->left!=nullptr)
                q.push(node->left);
            if(node->right!= nullptr)
                q.push(node->right);
            delete(node);
        }
        if(encoded_data_buffer!=NULL)
            delete(encoded_data_buffer);
        if(rawdata_buffer!=NULL)
            delete(rawdata_buffer);
    }

    void set_thread_num(int thread_num){
        threads_num = thread_num;
    }
    void set_filepath(char* input, char* output, char* mapfilepath);

    void read_raw_file(char* filepath);

    void init_hnode_array();

    void output_encoded_file();

    void generate_encoded_file();

    void output_encode_map();

    void build_huffman_tree();

    void traverse_huffman_tree(hnode* node, huffmanBitSet path,string curstr);

    void encode();

    int get_output_bitsize();

    void get_encoded_file();

    void get_decoded_file();

    void read_encode_map();

    void read_encoded_file();

    void decode();

    void output_decoded_file();

    uint32_t get_local_output_bitsize(uint32_t start_off, uint32_t end_off);

    void generate_encoded_file_parallel();
};
