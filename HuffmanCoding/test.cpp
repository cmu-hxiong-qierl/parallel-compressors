//
// Created by A gasoline on 5/2/22.
//
#include <iostream>
#include <queue>
#include "test.h"

using namespace std;

struct node{
    char x;
    int y;
};

struct compare{
    bool operator()(node a, node b){
        return a.y>b.y;
    }
};
int main(){
//    priority_queue<node,vector<node>,compare> pq;
//    node a;
//    a.y= 10;
//    node b;
//    b.y = 20;
//    pq.push(a);
//    pq.push(b);
    bitset<4> b = bitset<4>(string("0110"));
    cout<<b<<endl;
    int x = 10;
    char y = 'a';
    cout<<sizeof(y)<<endl;
}