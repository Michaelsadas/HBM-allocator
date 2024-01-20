#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <fstream>
#include <bitset>
#include <unordered_map>

using namespace std;

class Simulator
{
public:
    Simulator(){};
    Simulator(string file_name, long long total_size);
    ~Simulator(){};

    struct instruction{
        int order;
        short alloc;
        int size;
        int id;
        int pc;
    };
    struct evaluation{
        string name;
        double maf;
        int util;
        int depth;
    };
    struct result{
        int order;
        unsigned int addr;
        int size;
        int pc;
        result(int order, unsigned int addr, int size, int pc) : 
            order(order), 
            addr(addr),
            size(size),
            pc(pc) {}
        result(){}
    };

    double simulate_hta(int heap_depth, int hbm_bank_num, bool select_en); //65536, 4, 1
    double simulate_kwta(int heap_depth, int hbm_bank_num, bool select_en); //65536, 4, 1
    double simulate_line(int hbm_bank_num, bool select_en); //4, 1
    void trial(int hbm_bank_size, bool select_en);
    void change_file(string new_filename);
    void change_total_size(long long total_size) { _total_size = total_size; }
    void change_mini_space(int mini_heap) { _mini_space_size = mini_heap; }
    void print_res(string file_name);
    int _pc_num = 16;

private:
    string _filename;
    long long _total_size;
    int _mini_space_size = 4;
    int _instr_num;
    unordered_map<int,int> m;
    vector<instruction> _instr_list;
    // vector<result> _res_list;
    unordered_map<int, vector<result>> _res_list;
    unordered_map<int, int> _line{{1, 406}, {2, 726}, {4, 1177}, {5, 1543}, {6, 830}, {7, 971}, {8, 1049}, {9, 1108}, {10, 1187}};
    unordered_map<int, int> _kwta{{32, 2014}, {64, 2416}, {128, 2493}, {256, 2935}, {512, 3121}, {1024, 3137}, {2048, 3048}, {4096, 3680}, {8192, 3680}, {16384, 4240}, {32768, 4262}, {65536, 5991}};
    unordered_map<int, int> _hta{{32, 4028}, {128, 6594}, {256, 7038}, {512, 7099}, {1024, 7312}, {2048, 7785}, {4096, 8752}, {8192, 11234}, {16384, 15623}, {32768, 18205}, {65536, 22363}};
};

#endif