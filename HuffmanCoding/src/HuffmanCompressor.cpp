#include "HuffmanCompressor.h"

void HuffmanCompressor::set_filepath(char* input, char* output, char* mapfilepath){
    input_filepath = input;
    output_filepath = output;
    map_filepath = mapfilepath;
    rawdata_size = 0;
}

void HuffmanCompressor::read_encode_map(){
    // the first line is the rawdata_size
    char line_ca[300];
    FILE *mapfile = fopen(map_filepath,"r");

    fscanf(mapfile,"%s",line_ca);
    rawdata_size = stoi(string(line_ca));
    cout<<"raw data size = "<<rawdata_size<<endl;

    fscanf(mapfile,"%s",line_ca);
    encoded_bytesize = stoi(string(line_ca));
    cout<<"encoded size = "<<encoded_bytesize<<endl;

    fscanf(mapfile,"%s",line_ca);
    encoded_bitsize = stoi(string(line_ca));

    fscanf(mapfile,"%s",line_ca);
    encode_thread_num = stoi(string(line_ca));

    int line_size = encode_thread_num * 2 * 8;
    char* bitsize_line = new char[line_size];
    fscanf(mapfile,"%s",bitsize_line);
    string str_bitsize = string(bitsize_line);
    thread_output_bit_bound = new uint32_t[encode_thread_num*2];
    string delimiter = ":";
    size_t pos = 0;
    string token;
    int tid = 0;
    while ((pos = str_bitsize.find(delimiter)) != string::npos) {
        token = str_bitsize.substr(0, pos);
        thread_output_bit_bound[tid] = uint32_t(stoi(token));
        tid++;
        str_bitsize.erase(0, pos + delimiter.length());
    }
    delete(bitsize_line);

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
    FILE* fp = fopen(input_filepath,"rb");
    if(fp== nullptr){
        cout<<"Error in open file "<<input_filepath<<endl;
    }
    fseek(fp,0L,SEEK_END);
    uint32_t encoded_file_size = ftell(fp);
    fseek(fp,0L,SEEK_SET);
    if(encoded_file_size!=encoded_bytesize){
        cout<<encoded_file_size<<" "<<encoded_bytesize<<endl;
    }
    encoded_data_buffer = new unsigned char[encoded_file_size];
    memset(encoded_data_buffer,0,encoded_file_size);

    fread(encoded_data_buffer,sizeof(unsigned char), encoded_file_size,fp);
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
    fseek(fp,0L,SEEK_SET);
    rawdata_buffer = new unsigned char[rawdata_size];
    fread(rawdata_buffer,sizeof(unsigned char), rawdata_size,fp);
    fclose(fp);

    #pragma omp parallel num_threads(threads_num)
    {
        int tid = omp_get_thread_num();
        uint32_t partition_size = rawdata_size/omp_get_num_threads();
        uint32_t start_off = tid * partition_size;
        uint32_t end_off = (tid==omp_get_num_threads()-1)?rawdata_size:start_off+partition_size;
        uint32_t local_freq[CHAR_NUM] = {0};
        for(uint32_t idx = start_off;idx<end_off;idx++){
            unsigned char c = rawdata_buffer[idx];
            local_freq[static_cast<int>(c)]++;
        }
        int e;
        for(e=0;e<CHAR_NUM;e++){
            #pragma omp atomic
            all_freq[e] += local_freq[e];
        }
    }
    int m = 0;
    for(m=0;m<CHAR_NUM;m++){
        if(all_freq[m]!=0){
            hnode_array[m]->freq = all_freq[m];
        }
    }
}

void HuffmanCompressor::init_hnode_array(){
    for(int i=0;i<CHAR_NUM;i++){
        hnode_array[i]=new hnode;
        hnode_array[i]->id = i;
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

uint32_t HuffmanCompressor::get_local_output_bitsize(uint32_t start_off, uint32_t end_off){
    uint32_t sum = 0;
    for(uint32_t i=start_off;i<end_off;i++){
        int c = static_cast<int>(rawdata_buffer[i]);
        sum += char_strcode_map[c].size();
    }
    return sum;
}

void HuffmanCompressor::generate_encoded_file(){
    encoded_bitsize = get_output_bitsize();
    encoded_bytesize = encoded_bitsize/8+1;
    encoded_data_buffer = new unsigned char[encoded_bytesize];
    memset(encoded_data_buffer,0, encoded_bytesize);
    uint32_t output_bit_offset = 0;

    // sequential version that works
    uint32_t byte_offset=0, bit_offset_in_byte=0;
    for(uint32_t k = 0; k < rawdata_size;  k++) {
        int char_id = static_cast<int>(rawdata_buffer[k]);
        huffmanBitSet cur_bitSet = char_code_map[char_id];
        uint32_t bitset_len = cur_bitSet.length();
        for (uint32_t i = 0; i < bitset_len; i++) {
            // write to the output_buffer one bit by one bit
            byte_offset = (i+output_bit_offset)/8; // In which Byte of the output
            bit_offset_in_byte = (i+output_bit_offset)%8; // the bit offset in that byte
            encoded_data_buffer[byte_offset] |= (static_cast<int>(cur_bitSet[i]) << bit_offset_in_byte);
        }
        output_bit_offset += bitset_len;
    }
    if(output_bit_offset!=encoded_bitsize){
        cout << "Error in output_bitsize"<<endl;
    }
}

void HuffmanCompressor::generate_encoded_file_parallel(){
    unsigned char* thread_output_ptr[threads_num];
    thread_output_bitsize = new uint32_t[threads_num];

    #pragma omp parallel num_threads(threads_num)
    {
        int tid = omp_get_thread_num();
        uint32_t partition_size = rawdata_size / omp_get_num_threads();
        uint32_t start_off = tid * partition_size;
        uint32_t end_off = (tid == omp_get_num_threads() - 1) ? rawdata_size : start_off + partition_size;

        uint32_t local_output_bitsize = get_local_output_bitsize(start_off, end_off);
        thread_output_bitsize[tid] = local_output_bitsize;
        uint32_t local_output_bytesize = local_output_bitsize/8 + 1;
        #pragma omp atomic
        encoded_bytesize += local_output_bytesize;

        unsigned char* local_output_buffer = new unsigned char[local_output_bytesize];
        thread_output_ptr[tid] = local_output_buffer;

        uint32_t local_output_bit_offset = 0;
        uint32_t byte_offset = 0, bit_offset_in_byte = 0;

        for (uint32_t k = start_off; k < end_off; k++) {
            int char_id = static_cast<int>(rawdata_buffer[k]);
            huffmanBitSet cur_bitSet = char_code_map[char_id];
            uint32_t bitset_len = cur_bitSet.length();
            for (uint32_t i = 0; i < bitset_len; i++) {
                // write to the output_buffer one bit by one bit
                byte_offset = (i + local_output_bit_offset) / 8; // In which Byte of the output
                bit_offset_in_byte = (i + local_output_bit_offset) % 8; // the bit offset in that byte
                local_output_buffer[byte_offset] |= (cur_bitSet[i] << bit_offset_in_byte);
            }
            local_output_bit_offset += bitset_len;
        }
    }
    encoded_data_buffer = new unsigned char[encoded_bytesize];
    memset(encoded_data_buffer,0, encoded_bytesize);

    thread_output_bit_bound = new uint32_t[2*threads_num];
    uint32_t all_byteoff = 0;
    for(int t=0;t<threads_num;t++){
        // move each partition together
        thread_output_bit_bound[2*t] = all_byteoff*8;
        uint32_t local_bitsize = thread_output_bitsize[t];
        uint32_t local_bytesize = thread_output_bitsize[t] / 8 + 1;
        memmove(encoded_data_buffer+all_byteoff,thread_output_ptr[t],local_bytesize);
        thread_output_bit_bound[2*t+1] = thread_output_bit_bound[2*t] + local_bitsize;
        all_byteoff += local_bytesize;
    }
    if(encoded_bytesize != all_byteoff){
        cout<<"encoded_bytesize!=all_byteoff?"<<endl;
    }
    encoded_bitsize = encoded_bytesize*8;
}

void HuffmanCompressor::output_encoded_file(){
    FILE* fp = fopen(output_filepath,"wb");
    if(fp== nullptr) {
        cout << "Error in open file " << output_filepath << endl;
    }
    if(encoded_data_buffer== nullptr){
        cout << "Error that encoded data buffer is empty?" << endl;
    }
    fwrite(encoded_data_buffer,sizeof(char),encoded_bytesize,fp);
    fclose(fp);
}

void HuffmanCompressor::output_encode_map(){
    // store the frequency_map as {char:freq} in a .txt
    FILE* fp = fopen(map_filepath,"wb");
    fprintf(fp,"%d\n",rawdata_size);
    fprintf(fp,"%d\n",encoded_bytesize);
    fprintf(fp,"%d\n",encoded_bitsize);
    fprintf(fp,"%d\n",threads_num);
    for(int i=0;i<threads_num*2;i++){
        fprintf(fp,"%d:",thread_output_bit_bound[i]);
    }
    fprintf(fp,"\n");
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
}


void HuffmanCompressor::decode(){
    rawdata_buffer = new unsigned char[rawdata_size+1];
    memset(rawdata_buffer,0,rawdata_size+1);
    hnode* root = pq.top();
    hnode* cur_node = root;

    uint32_t rawdata_off = 0;
    uint32_t byte_off = 0;
    uint32_t bit_off = 0;

    int tid = 0;
    uint32_t cur_bound = thread_output_bit_bound[2*tid+1];
    for(uint32_t encoded_bit_off = 0; encoded_bit_off<encoded_bitsize;encoded_bit_off++){
        if(cur_node->left== nullptr && cur_node->right==nullptr){
            rawdata_buffer[rawdata_off++]=static_cast<char>(cur_node->id);
            cur_node = root;
        }
        if(encoded_bit_off == cur_bound){
            tid++;
            if(tid<encode_thread_num){
                encoded_bit_off = thread_output_bit_bound[2*tid];
                cur_bound = thread_output_bit_bound[2*tid+1];
                cur_node = root;
            } else {
                break;
            }
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
        cout<<"Error in rawdata: rawdata_off="<<rawdata_off<<" rawdata_size="<<rawdata_size<<endl;
        rawdata_size = rawdata_off;
        unsigned char* new_rawdata_buffer = new unsigned char[rawdata_size+1];
        memcpy(new_rawdata_buffer,rawdata_buffer,rawdata_size);
        delete(rawdata_buffer);
        rawdata_buffer = new_rawdata_buffer;
    }
}


void HuffmanCompressor::build_huffman_tree(){
    // add to priority queue
    for(int i=0;i<CHAR_NUM;i++){
        if(all_freq[i]!=0){
            pq.push(hnode_array[i]);
        }
    }
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
    FILE* fp = fopen(output_filepath, "wb");
    if(fp== nullptr) {
        cout << "Error in open file " << output_filepath << endl;
    }
    fwrite(rawdata_buffer,sizeof(char),rawdata_size,fp);
    fclose(fp);
}


void HuffmanCompressor::get_encoded_file(){
    auto init_start = Clock::now();
    double compute_time = 0;

    init_hnode_array();
    read_raw_file(input_filepath);
    compute_time = duration_cast<dsec>(Clock::now() - init_start).count();
    cout<<"[Input    Time]  "<<compute_time<<endl;

    build_huffman_tree();
    encode();

    init_start = Clock::now();
    generate_encoded_file_parallel();
    compute_time = duration_cast<dsec>(Clock::now() - init_start).count();
    cout<<"[Compress Time]  "<<compute_time<<endl;

    init_start = Clock::now();
    output_encoded_file();
    output_encode_map();
    compute_time = duration_cast<dsec>(Clock::now() - init_start).count();
    cout<<"[Output   Time]  "<<compute_time<<endl;
    cout<<"raw file size: "<<rawdata_size<<endl;
    cout<<"compressed file size: "<<encoded_bytesize<<endl;
}

void HuffmanCompressor::get_decoded_file(){
    init_hnode_array();
    read_encode_map();
    read_encoded_file();
    build_huffman_tree();
    decode();
    output_decoded_file();
}


