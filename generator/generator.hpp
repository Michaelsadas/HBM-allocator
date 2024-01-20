#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <fstream>

using namespace std;

class Generator
{
public:
    Generator(){};
    Generator(string type, string file_name, int heap_depth);
    ~Generator(){};

    void generate_allocator();
    void generate_hta();
    void generate_kwta();
    void generate_line();
    void change_type(string new_type) { _type = new_type; }
    void change_file(string new_filename) { _filename = new_filename; }
    void change_depth(int new_depth) { _heap_depth = new_depth; }
    void change_bank(int bank) { _bank = bank; }
    void change_mini_heap(int mini_heap) { _mini_heap_size = mini_heap; }
private:
    string _type;
    string _filename;
    int _heap_depth;
    int _total = 33554432;
    int _bank;
    int _mini_heap_size = 4;
};

#endif