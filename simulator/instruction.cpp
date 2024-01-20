#include "instruction.hpp"

using namespace std;

Instruction::Instruction(string example_type, int pc_num, int mini_size, int max_size){
    int step = (max_size - mini_size) / pc_num;
    _pc_num = pc_num;
    if(example_type == "Attention"){
        for(int i = 0; i < pc_num; i++){
            _instr_map[i].push_back(instruction(__INT_MAX__, 2, mini_size + i * step, i / 4, i)); //0
            _instr_map[i].push_back(instruction(__INT_MAX__, 2, mini_size + i * step, i / 4, i)); //1
            _instr_map[i].push_back(instruction(__INT_MAX__, 2, mini_size + i * step, i / 4, i)); //2
            _instr_map[i].push_back(instruction(__INT_MAX__, 2, mini_size + i * step, i / 4, i)); //3
            _instr_map[i].push_back(instruction(0, 0, mini_size + i * step, i / 4, i));
            _instr_map[i].push_back(instruction(1, 0, mini_size + i * step, i / 4, i));
            _instr_map[i].push_back(instruction(__INT_MAX__, 2, mini_size + i * step, i / 4, i)); //4
            _instr_map[i].push_back(instruction(3, 0, mini_size + i * step, i / 4, i)); 
            _instr_map[i].push_back(instruction(__INT_MAX__, 2, mini_size + i * step, i / 4, i)); //5
            _instr_map[i].push_back(instruction(2, 0, mini_size + i * step, i / 4, i));
            _instr_map[i].push_back(instruction(6, 0, mini_size + i * step, i / 4, i));  
            _instr_map[i].push_back(instruction(__INT_MAX__, 2, mini_size + i * step, i / 4, i)); //6
            _instr_map[i].push_back(instruction(8, 0, mini_size + i * step, i / 4, i));
            _instr_map[i].push_back(instruction(11, 0, mini_size + i * step, i / 4, i));  
        }
    }else if(example_type == "AVM"){
        for(int i = 0; i < pc_num; i++){
            for(int j = 0; j < _ACM_loop; j++){
                _instr_map[i].push_back(instruction(__INT_MAX__, 2, mini_size + i * step, i / 4, i));
                _instr_map[i].push_back(instruction(j * 2, 0, mini_size + i * step, i / 4, i));
            }
        }
    }else if(example_type == "Kmeans"){
        for(int i = 0; i < pc_num; i++){
            for(int j = 0; j < _kmeans_class; j++){
                _instr_map[i].push_back(instruction(__INT_MAX__, 2, mini_size + i * step, i / 4, i));
            }
            for(int j = 0; j < _kmeans_class; j++){
                _instr_map[i].push_back(instruction(j, 0, mini_size + i * step, i / 4, i));
            }
        }
    }else{
        cout << "The target examples cannot be supported." << endl;
        exit(1);
    }
}

void Instruction::generate_instruction(string filename){
    int alloc_order = 0;
    ofstream file;
    file.open(filename);
    int size = _instr_map[0].size();
// order       alloc       size        cross_bar_id        PC_id  
    file << dec << right << setfill(' ') << setw(5) << "order" << right << setfill(' ') << setw(16) << "alloc" << right << setfill(' ') << setw(16) << "size";
    file << right << setfill(' ') << setw(16) << "cross_bar_id" << right << setfill(' ') << setw(16) << "PC_id" << endl;
    for(int j = 0; j < size; j ++){
        for(int i = 0; i < _pc_num; i ++){
            if(_instr_map[i][j].alloc == 2){
                _instr_map[i][j].order = alloc_order;
                file << dec << right << setfill(' ') << setw(5) << alloc_order << right << setfill(' ') << setw(16) << _instr_map[i][j].alloc << right << setfill(' ') << setw(16) << _instr_map[i][j].size;
                file << right << setfill(' ') << setw(16) << _instr_map[i][j].id << right << setfill(' ') << setw(16) << _instr_map[i][j].pc << endl;
                alloc_order ++;
            }else{
                file << dec << right << setfill(' ') << setw(5) << _instr_map[i][_instr_map[i][j].order].order << right << setfill(' ') << setw(16) << _instr_map[i][j].alloc << right << setfill(' ') << setw(16) << _instr_map[i][j].size;
                file << right << setfill(' ') << setw(16) << _instr_map[i][j].id << right << setfill(' ') << setw(16) << _instr_map[i][j].pc << endl;
            }
        }
    }
    file.close();

}