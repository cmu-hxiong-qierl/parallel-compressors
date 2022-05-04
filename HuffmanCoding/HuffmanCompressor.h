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
using namespace std;
using namespace std::chrono;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::duration<double> dsec;

struct hnode
{
    char character;
    string code;
    int freq;
    hnode *left;
    hnode *right;
    hnode(){
        left = nullptr;
        right = nullptr;
    }
};

class HuffmanCompressor{
private:
    char* input_filepath;
    char* output_filepath;
    uint32_t rawdata_size;
    unsigned char* rawdata_buffer;
    map<char,int> char_freq_map;
    hnode* hnode_array[26];
    int threads_num = 0;
    struct compare{
        bool operator()(hnode* a, hnode* b){
            return a->freq >= b->freq;
        }
    };
    priority_queue<hnode*,vector<hnode*>,compare> pq;
    unordered_map<unsigned char, huffmanBitSet> char_code_map;
    map<char, string> char_strcode_map;



public:
    void set_thread_num(int thread_num){
        threads_num = thread_num;
    }
    void set_filepath(char* input, char* output);

    void read_raw_file(char* filepath);

    void init_hnode_array();

    void output_encoded_file();

    void build_huffman_tree();

    void traverse_huffman_tree(hnode* node, huffmanBitSet path,string curstr);

    void encode();

    int get_output_bitsize();

    void get_encoded_file();

};

void HuffmanCompressor::set_filepath(char* input, char* output){
    input_filepath = input;
    output_filepath = output;
    rawdata_size = 0;
}

/**
 * @brief read the input file, count frequency of each character
 * @return
 */

int charToInt(char c){
    return c-'a';
}
void HuffmanCompressor::read_raw_file(char* filepath){
    cout<<"read_raw_file "<<filepath<<endl;
    FILE* fp = fopen(filepath,"rb");
    if(fp== nullptr){
        cout<<"Error in open file "<<filepath<<endl;
    }
    fseek(fp,0L,SEEK_END);
    rawdata_size = ftell(fp);
    fseek(fp,0L,SEEK_SET);
    rawdata_buffer = new unsigned char[rawdata_size];
    fread(rawdata_buffer,sizeof(unsigned char), rawdata_size,fp);
    fclose(fp);

    uint32_t all_freq[26] = {0};
    #pragma omp parallel num_threads(threads_num)
    {
        int tid = omp_get_thread_num();
        uint32_t partition_size = rawdata_size/omp_get_num_threads();
        uint32_t start_off = tid * partition_size;
        uint32_t end_off = (tid==omp_get_num_threads()-1)?rawdata_size:start_off+partition_size;
        uint32_t local_freq[26] = {0};
        for(uint32_t idx = start_off;idx<end_off;idx++){
            unsigned char c = rawdata_buffer[idx];
            local_freq[charToInt(c)]++;
        }
        for(int i=0;i<26;i++){
            #pragma omp atomic
            all_freq[i] += local_freq[i];
        }
    }

    for(int i=0;i<26;i++){
        hnode_array[i]->freq += all_freq[i];
    }

//    for(int t = 0;t<threads_num;t++){
//        uint32_t start_off = t * partition_size;
//        uint32_t end_off = min(rawdata_size,start_off+partition_size);
//        memset(freq_array,0,sizeof(uint32_t)*26);
//
//        for(uint32_t i=start_off;i<end_off;i++){
//            char x = rawdata_buffer[i];
//            freq_array[charToInt(x)]++;
//        }
//
//        for(int j=0;j<26;j++){
//            hnode_array[j]->freq += freq_array[j];
//        }
//    }
//    compute_time += duration_cast<dsec>(Clock::now() - init_start).count();
//    cout<<"read_raw_data time= "<<compute_time<<endl;
}

void HuffmanCompressor::init_hnode_array(){
    cout<<"init_hnode_array"<<endl;
    for(int i=0;i<26;i++){
        hnode_array[i]=new hnode;
        hnode_array[i]->character = 'a'+i;// TODO
        hnode_array[i]->freq = 0;
    }
}

int HuffmanCompressor::get_output_bitsize(){
    int sum = 0;
    for(int i=0;i<26;i++){
        char c = hnode_array[i]->character;
        int tmp = hnode_array[i]->freq * (char_strcode_map[c].size());
        sum += tmp;
    }
    return sum;
}


void HuffmanCompressor::output_encoded_file(){
    // output each character
//    cout<<"output begin"<<endl;
    uint32_t output_bitsize = get_output_bitsize();
    cout<<"Target output bitsize = "<<output_bitsize<<endl;
    unsigned char* output_buffer = new unsigned char[output_bitsize/8+1];// TODO
    memset(output_buffer,0,output_bitsize/8+1);
    uint32_t output_bit_offset = 0;
    uint32_t byte_offset=0, bit_offset_in_byte=0;

    omp_set_num_threads(threads_num);
//    uint32_t partition_size = (rawdata_size+threads_num-1)/threads_num;
//    auto init_start = Clock::now();
//    double compute_time = 0;

    for(uint32_t k = 0; k<rawdata_size;k++) {
        char c = rawdata_buffer[k];
        huffmanBitSet cur_bitSet = char_code_map[c];
        uint32_t bitset_len = cur_bitSet.length();
//        #pragma omp parallel for
        for (uint32_t i = 0; i < bitset_len; i++) {
            // write to the output_buffer one bit by one bit
            byte_offset = (i+output_bit_offset)/8; // In which Byte of the output
            bit_offset_in_byte = (i+output_bit_offset)%8; // the bit offset in that byte
            output_buffer[byte_offset] |= (cur_bitSet[i]<<bit_offset_in_byte);
        }
        output_bit_offset += bitset_len;
    }

//    for(int t=0;t<threads_num;t++){
//        uint32_t start_off = t * partition_size;
//        uint32_t end_off = min(start_off + partition_size, rawdata_size);
//        local_output_size =
//        auto local_output_buffer = new uint32_t[local_output_size];
//        for(uint32_t i = start_off; i<end_off;i++) {
//            char c = rawdata_buffer[i];
//            huffmanBitSet cur_bitSet = char_code_map[c];
//            uint32_t bitset_len = cur_bitSet.length();
//            for (uint32_t i = 0; i < bitset_len; i++) {
//                // write to the output_buffer one bit by one bit
//                byte_offset = (i+output_bit_offset)/8; // In which Byte of the output
//                bit_offset_in_byte = (i+output_bit_offset)%8; // the bit offset in that byte
//                output_buffer[byte_offset] |= (cur_bitSet[i]<<bit_offset_in_byte);
//            }
//            output_bit_offset += bitset_len;
//        }
//    }

    FILE* fp = fopen(output_filepath,"wb");
    if(fp== nullptr) {
        cout << "Error in open file " << output_filepath << endl;
    }

    fwrite(output_buffer,sizeof(unsigned char),output_bit_offset/8+1,fp);
    // TODO free pointer;
    fclose(fp);
}

void HuffmanCompressor::traverse_huffman_tree(hnode* node, huffmanBitSet path, string curstr){
    if(node->left== nullptr && node->right== nullptr){
        char_code_map[node->character] = path;
        char_strcode_map[node->character] = curstr;
        return;
    }
    path.append('0');
    curstr+='0';
    traverse_huffman_tree(node->left,path,curstr);
    path.remove_last();
    curstr = curstr.substr(0,curstr.size()-1);
    path.append('1');
    curstr+='1';
    traverse_huffman_tree(node->right,path,curstr);
    curstr = curstr.substr(0,curstr.size()-1);
    path.remove_last();
}

/**
 * @brief Traverse all node and encode, store encode in char_code_map
 */
void HuffmanCompressor::encode() {
//    cout << "encode begin" << endl;
    hnode *root = pq.top();
    if (root == nullptr) {
        cout << "root is null?" << endl;
        return;
    }
    huffmanBitSet path;
    string s;
    traverse_huffman_tree(root, path, s);

//    auto iter = char_strcode_map.begin();
//    for (auto &t: char_strcode_map) {
//        cout << t.first << " " << t.second << endl;
//    }
}

void HuffmanCompressor::build_huffman_tree(){
//    cout<<"build_huffman_tree"<<endl;
    // add to priority queue
    for(int i=0;i<26;i++){
        pq.push(hnode_array[i]);
    }
//    cout<<"build 1"<<endl;
    // get every top two hnode, merge them until only one node
    while(pq.size()>1){
        hnode* top1 = pq.top();
        pq.pop();
        hnode* top2 = pq.top();
        pq.pop();
        // merge them into one node
        hnode* new_hnode = new hnode;
        new_hnode->freq = top1->freq + top2->freq;
        new_hnode->left = top1;
        new_hnode->right = top2;
        pq.push(new_hnode);
    }
}

void HuffmanCompressor::get_encoded_file(){
//    cout<<"get begin"<<endl;
    auto init_start = Clock::now();
    double compute_time = 0;

    init_hnode_array();
    read_raw_file(input_filepath);
    compute_time = duration_cast<dsec>(Clock::now() - init_start).count();
    cout<<"*** read_raw_file time= "<<compute_time<<endl;

    init_start = Clock::now();
    build_huffman_tree();
    compute_time = duration_cast<dsec>(Clock::now() - init_start).count();
    cout<<"*** build_huffman_tree time= "<<compute_time<<endl;

    init_start = Clock::now();
    encode();
    compute_time = duration_cast<dsec>(Clock::now() - init_start).count();
    cout<<"*** encode time= "<<compute_time<<endl;

    init_start = Clock::now();
    output_encoded_file();
    compute_time = duration_cast<dsec>(Clock::now() - init_start).count();
    cout<<"*** output time= "<<compute_time<<endl;
}


