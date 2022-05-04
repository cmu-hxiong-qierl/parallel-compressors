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

#include "HuffmanCoding/huffmanBitSet.h"
#include "HuffmanCoding/HuffmanCompressor.h"
using namespace std;

    void HuffmanCompressor::set_filepath(char* input, char* output){
        input_filepath = input;
        output_filepath = output;
    }

    /**
     * @brief read the input file, count frequency of each character
     * @return
     */
    void HuffmanCompressor::read_input(){
        ifstream ifs;
        ifs.open(input_filepath,ios::in);
        char c;
        while((c=ifs.get())!=EOF){
            hnode_array[(int)c]->freq++;
        }
    }

    void HuffmanCompressor::init_hnode_array(){
        for(int i=0;i<26;i++){
            hnode_array[i]=new hnode;
            hnode_array[i]->character = 'a'+i;// TODO
            hnode_array[i]->freq = 0;
        }
    }


    void HuffmanCompressor::output_encoded_file(){
        // output each character
        unsigned char* output_buffer = new unsigned char[1000];// TODO
        memset(output_buffer,0,1000);
        uint32_t output_bit_offset = 0;
        uint32_t byte_offset, bit_offset_in_byte;

        ifstream ifs;
        ifs.open(input_filepath,ios::in);
        char c;
        while((c=ifs.get())!=EOF) {
            huffmanBitSet cur_bitSet = char_code_map[c];
            uint32_t bitset_len = cur_bitSet.length();
            for (int i = 0; i < bitset_len; i++) {
                // write to the output_buffer one bit by one bit
                byte_offset = (i+output_bit_offset)/8; // In which Byte of the output
                bit_offset_in_byte = (i+output_bit_offset)%8; // the bit offset in that byte
                output_buffer[byte_offset] |= (cur_bitSet[i]<<bit_offset_in_byte);
            }
            output_bit_offset += bitset_len;
        }
        // output to the file
        FILE* fp = fopen(output_filepath,"wb");
        if(fp== nullptr) {
            cout << "Error in open file " << output_filepath << endl;
        }

        fwrite(output_buffer,sizeof(unsigned char),output_bit_offset,fp);
        // free pointer;
        fclose(fp);
    }

    void HuffmanCompressor::traverse_huffman_tree(hnode* node, huffmanBitSet path){
        if(node->left== nullptr && node->right== nullptr){
            char_code_map[node->character] = path;
            return;
        }
        path.append('0');
        traverse_huffman_tree(node->left,path);
        path.remove_last();
        traverse_huffman_tree(node->right,path);
        path.remove_last();
    }

    /**
     * @brief Traverse all node and encode, store encode in char_code_map
     */
    void HuffmanCompressor::encode(){
        hnode* root = pq.top();
        if(root== nullptr){
            cout<<"root is null?"<<endl;
            return;
        }
        huffmanBitSet path;
        traverse_huffman_tree(root,path);
    }

    void HuffmanCompressor::build_huffman_tree(){
        init_hnode_array();
        read_input();
        // add to priority queue
        for(int i=0;i<26;i++){
            pq.push(hnode_array[i]);
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
    void HuffmanCompressor::get_encoded_map_file(){
        build_huffman_tree();
        encode();
        output_encoded_file();
    }

