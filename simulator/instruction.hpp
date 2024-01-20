#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <fstream>
#include <bitset>
#include <unordered_map>

using namespace std;

class Instruction
{
public:
    Instruction(){};
    Instruction(string example_type, int pc_num, int mini_size, int max_size);
    ~Instruction(){};

    struct instruction{
        int order;
        short alloc;
        int size;
        int id;
        int pc;
        instruction(int order, short alloc, int size, int id, int pc) : 
            order(order), 
            alloc(alloc),
            size(size),
            id(id),
            pc(pc) {}

        instruction(){}
    };

    void generate_instruction(string filename);
    int _kmeans_class = 5;
    int _ACM_loop = 8;
    int _pc_num;
    // dou

private:
    string _filename;
    unordered_map<int, vector<instruction>> _instr_map;
};

#endif