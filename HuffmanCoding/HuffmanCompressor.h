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

#define CHAR_NUM 128
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
    unsigned char* encoded_data_buffer;
    hnode* hnode_array[CHAR_NUM];
    int threads_num = 0;

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
    }

    void set_thread_num(int thread_num){
        threads_num = thread_num;
    }
    void set_filepath(char* input, char* output, char* mapfilepath);

    void read_raw_file(char* filepath);

    void init_hnode_array();

    void output_encoded_file();

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

    int get_local_output_bitsize(uint32_t start_off, uint32_t end_off);
};

void HuffmanCompressor::set_filepath(char* input, char* output, char* mapfilepath){
    input_filepath = input;
    output_filepath = output;
    map_filepath = mapfilepath;
    rawdata_size = 0;
}

/**
 * @brief read the map file and fill all_freq[], and rawdata_size
 * @param filepath
 */
void HuffmanCompressor::read_encode_map(){
    // the first line is the rawdata_size
    cout<<"read_encode_map"<<endl;
    char line_ca[32];
    FILE *mapfile = fopen(map_filepath,"r");

    fscanf(mapfile,"%s",line_ca);
    rawdata_size = stoi(string(line_ca));
    cout<<"rawdata_size="<<rawdata_size<<endl;

    fscanf(mapfile,"%s",line_ca);
    encoded_bytesize = stoi(string(line_ca));
    cout<<"encoded_bytesize="<<encoded_bytesize<<endl;

    fscanf(mapfile,"%s",line_ca);
    encoded_bitsize = stoi(string(line_ca));
    cout<<"encoded_bitsize="<<encoded_bitsize<<endl;

    while(fscanf(mapfile,"%s",line_ca)!=EOF){
        string line = string(line_ca);
        int find_pos = line.find(':', 0);
        string id = line.substr(0, find_pos);
        string freq = line.substr(find_pos + 1);
        hnode_array[stoi(id)]->freq = stoi(freq);
        all_freq[stoi(id)] = stoi(freq);
    }
}

void HuffmanCompressor::read_encoded_file(){
    cout<<"read_encoded_file "<<input_filepath<<endl;
    encoded_data_buffer = new unsigned char[encoded_bytesize];
    memset(encoded_data_buffer,0,encoded_bytesize);
    FILE* fp = fopen(input_filepath,"rb");
    if(fp== nullptr){
        cout<<"Error in open file "<<input_filepath<<endl;
    }
    cout<<"hi"<<endl;
    fread(encoded_data_buffer,sizeof(unsigned char), encoded_bytesize,fp);
    fclose(fp);
}
/**
 * @brief read the input file, count frequency of each character
 * @return
 */
void HuffmanCompressor::read_raw_file(char* filepath){
    FILE* fp = fopen(filepath,"rb");
    if(fp== nullptr){
        cout<<"Error in open file "<<filepath<<endl;
    }
    fseek(fp,0L,SEEK_END);
    rawdata_size = ftell(fp);
    cout<<"read_raw_file, size "<<rawdata_size<<endl;
    fseek(fp,0L,SEEK_SET);
    rawdata_buffer = new unsigned char[rawdata_size];
    fread(rawdata_buffer,sizeof(unsigned char), rawdata_size,fp);
    fclose(fp);

//    cout<<"f"<<endl;
    #pragma omp parallel num_threads(threads_num)
    {
        int tid = omp_get_thread_num();
        uint32_t partition_size = rawdata_size/omp_get_num_threads();
        uint32_t start_off = tid * partition_size;
        uint32_t end_off = (tid==omp_get_num_threads()-1)?rawdata_size:start_off+partition_size;
        uint32_t local_freq[CHAR_NUM] = {0};
//        cout<<"start_off="<<start_off<<" end_off="<<end_off<<endl;
        for(uint32_t idx = start_off;idx<end_off;idx++){
            unsigned char c = rawdata_buffer[idx];
            local_freq[static_cast<int>(c)]++;
        }
        int e;
        for(e=0;e<CHAR_NUM;e++){
            #pragma omp atomic
            all_freq[e] += local_freq[e];
//            cout<<"local freq[e]="<<local_freq[e]<<endl;
//            hnode_array[e]->freq += local_freq[e];
        }
    }
    int m = 0;
    for(m=0;m<CHAR_NUM;m++){
        if(all_freq[m]!=0){
            hnode_array[m]->freq = all_freq[m];
//            cout<<m<<" "<<all_freq[m]<<endl;
        }
    }
//    compute_time += duration_cast<dsec>(Clock::now() - init_start).count();
//    cout<<"read_raw_data time= "<<compute_time<<endl;
}

void HuffmanCompressor::init_hnode_array(){
//    cout<<"init_hnode_array"<<endl;
    for(int i=0;i<CHAR_NUM;i++){
        hnode_array[i]=new hnode;
        hnode_array[i]->id = i;
//        cout<<i<<" "<<hnode_array[i]->character<<endl;
        hnode_array[i]->freq = 0;
    }
}

int HuffmanCompressor::get_output_bitsize(){
    int sum = 0;
    for(int i=0;i<CHAR_NUM;i++){
        if(all_freq[i]!=0){
            int tmp = all_freq[i] * (char_strcode_map[i].size());
            sum += tmp;
        }
    }
    return sum;
}

int HuffmanCompressor::get_local_output_bitsize(uint32_t start_off, uint32_t end_off){
    int sum = 0;
    for(uint32_t i=start_off;i<end_off;i++){
        unsigned char c = rawdata_buffer[i];
        sum += char_strcode_map[c].size();
    }
    return sum;
}

void HuffmanCompressor::output_encoded_file(){
//    cout<<"output begin"<<endl;
    uint32_t output_bitsize = get_output_bitsize();
    int output_bytesize = output_bitsize/8+1;
    cout<<"Target output bitsize = "<<output_bitsize<<endl;
    unsigned char* output_buffer = new unsigned char[output_bytesize];
    memset(output_buffer,0,output_bytesize);
    uint32_t output_bit_offset = 0;

    // sequential version that works
    uint32_t byte_offset=0, bit_offset_in_byte=0;
    for(uint32_t k = 0; k < rawdata_size;  k++) {
        int char_id = static_cast<int>(rawdata_buffer[k]);
        huffmanBitSet cur_bitSet = char_code_map[char_id];
        uint32_t bitset_len = cur_bitSet.length();
//        cout<<rawdata_buffer[k]<<" "<<char_strcode_map[char_id]<<" "<<bitset_len<<endl;
        for (uint32_t i = 0; i < bitset_len; i++) {
            // write to the output_buffer one bit by one bit
            byte_offset = (i+output_bit_offset)/8; // In which Byte of the output
            bit_offset_in_byte = (i+output_bit_offset)%8; // the bit offset in that byte
            output_buffer[byte_offset] |= (static_cast<int>(cur_bitSet[i]) << bit_offset_in_byte);
//            cout<<cur_bitSet[i]<<" "<<bit_offset_in_byte<<" "<< bit_offset_in_byte<<endl;
        }
        output_bit_offset += bitset_len;
    }
    if(output_bit_offset!=output_bitsize){
        cout << "Error in output_bitsize"<<endl;
    }
    cout<<output_buffer<<endl;

    FILE* fp = fopen(output_filepath,"wb");
    if(fp== nullptr) {
        cout << "Error in open file " << output_filepath << endl;
    }

    fwrite(output_buffer,sizeof(char),output_bytesize,fp);
    fclose(fp);
    encoded_bytesize = output_bytesize;
    encoded_bitsize = output_bitsize;

    delete[] output_buffer;
}

void HuffmanCompressor::output_encode_map(){
    // store the frequency_map as {char:freq} in a .txt
    FILE* fp = fopen(map_filepath,"wb");
    fprintf(fp,"%d\n",rawdata_size);
    fprintf(fp,"%d\n",encoded_bytesize);
    fprintf(fp,"%d\n",encoded_bitsize);
    for(int i=0;i<CHAR_NUM;i++){
        if(all_freq[i]!=0){
            fprintf(fp,"%d:%d\n",i,all_freq[i]);
        }
    }
}
void HuffmanCompressor::traverse_huffman_tree(hnode* node, huffmanBitSet path, string curstr){
    if(node->left== nullptr && node->right== nullptr){
        huffmanBitSet cur_path(path);
        char_code_map[node->id] = cur_path;
        char_strcode_map[node->id] = curstr;
//        cout<<static_cast<char>(node->id)<<" ";
//        for(uint32_t idx=0;idx<path.length();idx++){
//            cout<<cur_path[idx];
//        }
        return;
    }
    path.append(0);
    curstr+='0';
    traverse_huffman_tree(node->left,path,curstr);
    path.remove_last();
    curstr = curstr.substr(0,curstr.size()-1);
    path.append(1);
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
//    for(auto &t: char_code_map){
//        huffmanBitSet path = t.second;
//        cout<<static_cast<char>(t.first)<<" ";
//        for(uint32_t idx=0;idx<path.length();idx++){
//            cout<<path[idx];
//        }
//        cout<<" ";
//    }
//    for (auto &t: char_strcode_map) {
//        cout << static_cast<char>(t.first) << " " << t.second << endl;
//    }
}

void HuffmanCompressor::decode(){
    rawdata_buffer = new unsigned char[rawdata_size+1];
    memset(rawdata_buffer,0,rawdata_size+1);
    hnode* root = pq.top();
    hnode* cur_node = root;

    uint32_t rawdata_off = 0;
    uint32_t byte_off = 0;
    uint32_t bit_off = 0;

    for(uint32_t encoded_bit_off = 0; encoded_bit_off<encoded_bitsize;encoded_bit_off++){
        if(cur_node->left== nullptr && cur_node->right==nullptr){
//            cout<<rawdata_off<<" "<<static_cast<char>(cur_node->id)<<endl;
            rawdata_buffer[rawdata_off++]=static_cast<char>(cur_node->id);
//            cout<<static_cast<char>(cur_node->id)<<endl;
            cur_node = root;
        }
        byte_off = encoded_bit_off/8;
        bit_off = encoded_bit_off%8;
        if((encoded_data_buffer[byte_off]>>bit_off) & 1 ){
            // this bit is 1
            if(cur_node==nullptr || cur_node->right==nullptr){
                cout<<"Error that right is null?"<<endl;
            } else {
                cur_node = cur_node -> right;
            }
        } else {
            // this bit is 0
            if(cur_node==nullptr || cur_node->left==nullptr){
                cout<<"Error that left is null?"<<endl;
            } else {
                cur_node = cur_node -> left;
            }
        }
    }
    if(cur_node->left== nullptr && cur_node->right==nullptr){
        rawdata_buffer[rawdata_off++]=static_cast<char>(cur_node->id);
    }
    if(rawdata_off!=rawdata_size){
        cout<<"Error that rawdata overflow: rawdata_off="<<rawdata_off<<" rawdata_size="<<rawdata_size<<endl;
    }
}

void HuffmanCompressor::build_huffman_tree(){
//    cout<<"build_huffman_tree"<<endl;
    // add to priority queue
    for(int i=0;i<CHAR_NUM;i++){
        if(all_freq[i]!=0){
            pq.push(hnode_array[i]);
        }
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

void HuffmanCompressor::output_decoded_file(){
    cout<<"output decoded file to "<<output_filepath<<endl;
    FILE* fp = fopen(output_filepath, "wb");
    if(fp== nullptr) {
        cout << "Error in open file " << output_filepath << endl;
    }
    fwrite(rawdata_buffer,sizeof(char),rawdata_size,fp);
    fclose(fp);
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

    output_encode_map();
}

void HuffmanCompressor::get_decoded_file(){
    init_hnode_array();
    read_encode_map();// fill all_freq[], hnode_array[i]
    read_encoded_file();// put encoded data into encoded_data_buffer
    build_huffman_tree();// use pq build the tree
    decode();// decode according hfmtree
    output_decoded_file();
}


