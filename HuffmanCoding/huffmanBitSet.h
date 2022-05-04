using namespace std;
#include<iostream>
#include <stdio.h>
#include <string.h>

const uint32_t block_len = 32;

class huffmanBitSet{
private:
    uint32_t len;
    unsigned char *data;

public:
    huffmanBitSet(){
        len = 0;
        data = new unsigned char[block_len];
        memset(data,0,block_len);
    }

    uint32_t length(){
        return len;
    }

    bool append(unsigned char c){
        if(len>=block_len*8){
            cout<<"Error in block overflow"<<endl;
        }
        uint32_t idxByte = len/8;
        uint32_t idxBit = len%8;
        if(c=='1'){
            data[idxByte] |= (1<<idxBit);
        } else {
            data[idxByte] &= (~(1<<idxBit));
        }
        len++;
        return true;
    }

    bool remove_last(){
        len--;
        return true;
    }

    bool operator[](uint32_t idx) const {
        uint32_t byte_offset = idx/8;
        uint32_t bit_offset = idx%8;
        bool res = static_cast<bool>((data[byte_offset]>>bit_offset) & 1);
        return res;
    }
};