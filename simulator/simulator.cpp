#include "simulator.hpp"
#include <iomanip>
#include <cmath>

using namespace std;

template<size_t N, size_t len>
bitset<len> getSubBitset(bitset<N> &b, int start, int size)
{
    bitset<len> res = 0;
    for(int i = 0; i < size; i ++){
        res[i] = b[start + i];
    }
    return res;
}

Simulator::Simulator(string file_name, long long total_size){
    _filename = file_name; 
    _total_size = total_size;
    instruction temp;
    ifstream file;
    _instr_num = 0;
    string unused;
    file.open(_filename);
    if(!file.is_open()){
		cout << "Cannot open the file." << endl;
        exit(EXIT_FAILURE);
    }
    getline(file, unused);
    file >> temp.order >> temp.alloc >> temp.size >> temp.id >> temp.pc;
    while(file.good()){
        _instr_list.push_back(temp);
        _instr_num ++;
        file >> temp.order >> temp.alloc >> temp.size >> temp.id >> temp.pc;
    }
    if(file.eof()){
        cout << "End of file researched." << endl;
    }else if(file.fail()){
        cout << "Input terminated by data research." << endl;
    }
}

void Simulator::change_file(string new_filename){
    _filename = new_filename; 
    instruction temp;
    ifstream file;
    _instr_num = 0;
    string unused;
    file.open(_filename);
    if(!file.is_open()){
		cout << "Cannot open the file." << endl;
        exit(EXIT_FAILURE);
    }
    getline(file, unused);
    file >> temp.order >> temp.alloc >> temp.size >> temp.id;
    while(file.good()){
        if(_instr_list.size() <= _instr_num) _instr_list.push_back(temp);
        else _instr_list[0] = temp;
        _instr_num ++;
        file >> temp.order >> temp.alloc >> temp.size >> temp.id;
    }
    if(file.eof()){
        cout << "End of file researched." << endl;
    }else if(file.fail()){
        cout << "Input terminated by data research." << endl;
    }
}

double Simulator::simulate_hta(int heap_depth, int hbm_bank_num, bool select_en){
    int mini_size = _total_size / heap_depth;
    int layers = (int)log2(heap_depth);
    int branch_size = _total_size / 64;
    int i, j;
    int ans;
    int loc1, loc2;
    int new_loc_f, new_loc_s1, new_loc_s2, ans_f, ans_s, new_loc_g, ans_g;
    int flag;
    int maf = 0;
    int log_bank_num = (int)log2(hbm_bank_num);
    double maf_rate;

    unsigned int *addr = new unsigned int[_instr_num / 2];
    int *level = new int[_instr_num / 2];

    bitset<64> buddy_tree[17];
    bitset<2048> branch_tree[64];
    bitset<1024> branch_tmp;

    // ofstream file;
    // file.open("result.txt");

    _res_list.clear();
    for(i = 0; i < 17; i++){
        for(j = 0; j < min((int)pow(2, i), 64);  j++){
            buddy_tree[i][j] = 1;
        }
    }
    for(i = 0; i < 64; i ++){
        for(j = 0; j < 2048; j++){
            branch_tree[i][j] = 1;
        }
    }
    for(auto iter = _instr_list.begin(); iter != _instr_list.end(); iter ++){
        if(iter->id >= hbm_bank_num) exit(1);
        if(iter->alloc == 2){ // alloc
            ans = layers - ceil(log2((float)max(iter->size, mini_size) / mini_size));
            ans_s = (ans > 6) ? 6 : ans;
            if(ans < log_bank_num || select_en == false || hbm_bank_num == 1){
                loc1 = buddy_tree[ans]._Find_first();
            }else if((loc1 = getSubBitset<64, 64>(buddy_tree[ans], iter->id * pow(2, ans_s - log_bank_num), pow(2, ans_s - log_bank_num))._Find_first()) != 64){
                loc1 += iter->id * pow(2, ans_s - log_bank_num);
            }else{
                loc1 = buddy_tree[ans]._Find_first();
            }
            if(loc1 == 64){
                maf ++;
                addr[iter->order] = -1;
                _res_list[iter->pc].push_back(result(iter->order, addr[iter->order], iter->size, iter->pc));
                // file << dec << right << setfill(' ') << setw(5) << iter->order << hex << right << setfill(' ') << setw(16) << addr[iter->order] << right << setfill(' ') << setw(16) << iter->size << right << setfill(' ') << setw(16) << iter->pc << endl;
            }
            else{
                if(ans <= 6){
                    addr[iter->order] = (loc1 << (layers - ans)) * mini_size;
                    _res_list[iter->pc].push_back(result(iter->order, addr[iter->order], iter->size, iter->pc));
                    // file << dec << right << setfill(' ') << setw(5) << iter->order << hex << right << setfill(' ') << setw(16) << addr[iter->order] << right << setfill(' ') << setw(16) << iter->size << right << setfill(' ') << setw(16) << iter->pc << endl;
                    level[iter->order] = ans;
                    buddy_tree[ans].set(loc1, 0);
                }
                else{
                    branch_tmp.reset();
                    for(i = 0, j = (int)pow(2, ans - 6) - 1; j <= (int)pow(2, ans - 5) - 2; i++, j++){
                        branch_tmp[i] = branch_tree[loc1][j];
                    }
                    loc2 = branch_tmp._Find_first();
                    addr[iter->order] = ((loc1 << (layers - 6)) + (loc2 << (layers - ans))) * mini_size;
                    _res_list[iter->pc].push_back(result(iter->order, addr[iter->order], iter->size, iter->pc));
                    // file << dec << right << setfill(' ') << setw(5) << iter->order << hex << right << setfill(' ') << setw(16) << addr[iter->order] << right << setfill(' ') << setw(16) << iter->size << right << setfill(' ') << setw(16) << iter->pc << endl;
                    level[iter->order] = ans;
                    buddy_tree[6].set(loc1, 0);
                    loc2 = loc2 + (int)pow(2, ans - 6) - 1;
                    branch_tree[loc1].set(loc2, 0);
                }

                if(ans <= 6){
                    new_loc_f = loc1 >> 1;
                    if(ans <= 5){
                        new_loc_s1 = loc1 << 1;
                        new_loc_s2 = (loc1 << 1) + 1;    
                    }else{
                        new_loc_s1 = new_loc_s2 = loc1;
                    }
                    for(ans_f = ans - 1, ans_s = ans + 1; (ans_f >= 1) || (ans_s <= 6); ans_f -= (ans_f != 0), ans_s += (ans_s != 7)){
					    buddy_tree[0] = 0;
					    if(ans_f >= 1){
						    buddy_tree[ans_f].set(new_loc_f, 0);
						    new_loc_f = new_loc_f >> 1;
					    }
					    if(ans_s <= 6){
                            for(i = new_loc_s1; i <= new_loc_s2; i++){
                                buddy_tree[ans_s].set(i, 0);
                            }
					    }
					    if(ans_s <= 5){
						    new_loc_s1 = (new_loc_s1 << 1);
						    new_loc_s2 = (new_loc_s2 << 1) + 1;
					    }
                    }
                	for(ans_s = 7; ans_s <= layers; ans_s ++){
                        for(i = new_loc_s1; i <= new_loc_s2; i++){
                            buddy_tree[ans_s].set(i, 0);
                        }
	    			}
                }
                else{
                    new_loc_g = loc1 >> 1;
				    new_loc_f = (loc2 - 1) >> 1;
				    new_loc_s1 = (loc2 << 1) + 1;
				    new_loc_s2 = (loc2 << 1) + 2;
				    for(ans_g = 5, ans_f = ans - 1, ans_s = ans + 1; (ans_g >= 1) || (ans_f >= 7) || (ans_s <= layers); ans_g -= (ans_g != 0), ans_f -= (ans_f != 6), ans_s += (ans_s != layers + 1))
				    {
				    	buddy_tree[0] = 0;
				    	if(ans_g >= 1){
				    		buddy_tree[ans_g].set(new_loc_g, 0);
				    		new_loc_g = new_loc_g >> 1;
				    	}
				    	if(ans_f >= 7){
				    		branch_tree[loc1].set(new_loc_f, 0);
                            flag = 0;
                            for(i = (int)pow(2, ans_f - 6) - 1; i <= (int)pow(2, ans_f - 5) - 2; i++){
				    			flag += branch_tree[loc1][i];
				    		}
                            if(flag == 0) buddy_tree[ans_f].set(loc1, 0);
				    		new_loc_f = (new_loc_f - 1) >> 1;
				    	}					
				    	if(ans_s <= layers){
                            for(i = new_loc_s1; i <= new_loc_s2; i++){
                                branch_tree[loc1].set(i, 0);
                            }
                            flag = 0;
                            for(i = (int)pow(2, ans_f - 6) - 1; i <= (int)pow(2, ans_f - 5) - 2; i++){
				    			flag += branch_tree[loc1][i];
				    		}
                            if(flag == 0) buddy_tree[ans_f].set(loc1, 0);
				    		//if(branch_tree[loc1].range(2 * exp_2_8bit(ans_s - 6) - 2, exp_2_8bit(ans_s - 6) - 1) == 0){
				    		//	buddy_tree[ans_s].set(loc1, 0);
				    		//}
				    		new_loc_s1 = (new_loc_s1 << 1) + 1;
				    		new_loc_s2 = (new_loc_s2 << 1) + 2;
				    	}
				    }
                }
            }
        }
        else{ // free
            if(addr[iter->order] != -1){
                //cout << "Free " << addr[iter->order] << endl;
                ans = level[iter->order];
                if(ans <= 6){
                    loc1 = addr[iter->order] / (pow(2, layers - ans) * mini_size);
                    buddy_tree[ans].set(loc1, 1);
			        new_loc_f = loc1 >> 1;
			        //new_loc_s1 = loc1 << 1;
			        //new_loc_s2 = (loc1 << 1) + 1;
                    if(ans <= 5){
                        new_loc_s1 = loc1 << 1;
                        new_loc_s2 = (loc1 << 1) + 1;    
                    }else{
                        new_loc_s1 = new_loc_s2 = loc1;
                    }
			        for(ans_f = ans - 1, ans_s = ans + 1; (ans_f >= 1) || (ans_s <= 6); ans_f -= (ans_f != 0), ans_s += (ans_s != 7)){
				        if(ans_f >= 1){
					        if(buddy_tree[ans_f + 1][((loc1 >> 1) << 1) + 1] && buddy_tree[ans_f + 1][(loc1 >> 1) << 1]){
						        buddy_tree[ans_f].set(new_loc_f, 1);
						        loc1 = loc1 >> 1;
						        new_loc_f = new_loc_f >> 1;
					        }
					        else{
						        ans_f = 0;
					        }
				        }
				        if(ans_s <= 6){
                            for(i = new_loc_s1; i <= new_loc_s2; i++){
					            buddy_tree[ans_s][i] = 1;
                            }
				        }
				        if(ans_s <= 5){
					        new_loc_s1 = (new_loc_s1 << 1);
					        new_loc_s2 = (new_loc_s2 << 1) + 1;
				        }
			        }
			        for(ans_s = 7; ans_s <= layers; ans_s ++){
                        for(i = new_loc_s1; i <= new_loc_s2; i++){
					        buddy_tree[ans_s][i] = 1;
                        }
    				    //buddy_tree[ans_s].range(new_loc_s2, new_loc_s1) = -1;
	    		    }
                }
                else{
			        loc2 = addr[iter->order] % branch_size / (pow(2, layers - ans) * mini_size) + pow(2, ans - 6) -1; //+ exp_2_8bit(ans - 6) - 1; //index in the branch
			        loc1 = addr[iter->order] / branch_size;//index in the buddy
                    //loc2 = loc2 - loc1 * (branch_size / mini_size) + pow(2, ans - 6) -1;
			        branch_tree[loc1].set(loc2, 1);
			        buddy_tree[ans].set(loc1, 1);
			        new_loc_f = (loc2 - 1) >> 1;
			        new_loc_s1 = (loc2 << 1) + 1;
			        new_loc_s2 = (loc2 << 1) + 2;
			        for(ans_f = ans - 1, ans_s = ans + 1; (ans_f >= 7) || (ans_s <= layers); ans_f -= (ans_f != 6), ans_s += (ans_s != layers + 1)){
				        if(ans_f >= 7){
					        if(branch_tree[loc1][(((loc2 - 1) >> 1) << 1) + 2] && branch_tree[loc1][(((loc2 - 1) >> 1) << 1) + 1]){
						        branch_tree[loc1].set(new_loc_f, 1);
						        buddy_tree[ans_f].set(loc1, 1);
						        loc2 = (loc2 - 1) >> 1;
						        new_loc_f = (new_loc_f - 1) >> 1;
					        }
					        else{
						        ans_f = 6;
					        }
				        }
				        if(ans_s <= layers){
                            for(i = new_loc_s1; i <= new_loc_s2; i++){
    					        branch_tree[loc1][i] = -1;
                            }
					        new_loc_s1 = new_loc_s1 << 1;
					        new_loc_s2 = (new_loc_s2 << 1) + 1;
				        }				
			        }
			        if(branch_tree[loc1][2] && branch_tree[loc1][1]){
				        branch_tree[loc1].set(0, 1);
				        buddy_tree[6].set(loc1, 1);
				        new_loc_f = loc1 >> 1;
				        for(ans_g = 5; ans_g > 0; ans_g -= (ans_g != 0)){
					        if(buddy_tree[ans_g + 1][((loc1 >> 1) << 1) + 1] && buddy_tree[ans_g + 1][(loc1 >> 1) << 1]){
						        buddy_tree[ans_g].set(new_loc_f, 1);
						        loc1 = loc1 >> 1;
						        new_loc_f = new_loc_f >> 1;
					        }
					        else{
						        ans_g = 0;
					        }
				        }
			        }
			        if(buddy_tree[1][0] && buddy_tree[1][1]){
				        buddy_tree[0] = 1;
			        }
                }
            }
        }
    }
    maf_rate = maf / (_instr_num / 2.0);
    //for(i = 1; i <= _instr_num / 2; i ++){
    //    cout << addr[i] << "\t" << level[i] << endl;
    //}
    delete[] addr;
    delete[] level;
    return maf_rate;
};

double Simulator::simulate_kwta(int heap_depth, int hbm_bank_num, bool select_en){
    int space_num = heap_depth / _mini_space_size;
    int unit_size = _total_size / heap_depth;
    int leaf_num = min(128, (int)pow(2, (int)(log2(space_num) / 2)));
    int branch_num = space_num / leaf_num;
    int branch_num_in_one_hbm = branch_num / hbm_bank_num;

    short *alloc_count = new short[space_num];
    short *free_count = new short[space_num];
    unsigned *addr = new unsigned int[_instr_num / 2];
    int *last_loc1 = new int[hbm_bank_num];
    int *last_loc2 = new int[hbm_bank_num];
    int *last_addr = new int[hbm_bank_num];
    int *last_offset = new int[hbm_bank_num];

    int i, j, maf = 0, loc1, loc2, offset = -1;
    int tmp_loc1, tmp_loc2, tmp_offset;
    double maf_rate;
    int use_id;

    bitset<128> *heap_tree = new bitset<128>[branch_num];
    bitset<512> top_heap; //max is set to be 512

    // ofstream file;
    // file.open("result.txt");

    _res_list.clear();
    for(i = 0; i < branch_num; i++){
        for(j = 0; j < 128; j++){
            heap_tree[i][j] = 1;
        }
    }
    for(i = 0; i < hbm_bank_num; i++){
        last_offset[i] = -1;
    }
    for(i = 0; i < 512; i++){
        top_heap[i] = 1;
    }
    for(i = 0; i < space_num; i++){
        alloc_count[i] = free_count[i] = 0;
    }
    for(auto iter = _instr_list.begin(); iter != _instr_list.end(); iter ++){
        if(iter->id >= hbm_bank_num) exit(1);
        if(iter->alloc == 2){ //alloc
            if(top_heap._Find_first() == branch_num){
                maf ++;
                addr[iter->order] = -1;
            }
            else{
                if(select_en && hbm_bank_num != 1){
                    if(getSubBitset<512, 256>(top_heap, iter->id * branch_num_in_one_hbm, branch_num_in_one_hbm)._Find_first() != 256){
                        use_id = iter->id;
                    }
                }else{
                    for(i = 0; i < branch_num_in_one_hbm; i++){
                        if(getSubBitset<512, 256>(top_heap, i * branch_num_in_one_hbm, branch_num_in_one_hbm)._Find_first() != 256){
                            use_id = i;
                            break;
                        }
                    }
                }

                offset = last_offset[use_id];
                loc1 = last_loc1[use_id];
                loc2 = last_loc2[use_id];
                if(offset != -1){
                    addr[iter->order] = last_addr[use_id] * unit_size;
                    _res_list[iter->pc].push_back(result(iter->order, addr[iter->order], iter->size, iter->pc));
                    // file << dec << right << setfill(' ') << setw(5) << iter->order << hex << right << setfill(' ') << setw(16) << addr[iter->order] << right << setfill(' ') << setw(16) << iter->size << right << setfill(' ') << setw(16) << iter->pc << endl;
                    last_addr[use_id] ++;
                    alloc_count[offset] ++;
                    if(alloc_count[offset] == _mini_space_size){
                        heap_tree[loc1].set(loc2, 0);
                        last_offset[use_id] = -1;
                        if(heap_tree[loc1]._Find_first() == leaf_num){ // change
                            top_heap.set(loc1, 0);
                        }
                    }
                    // inspire1 = alloc_count[offset];
                }
                else{
                    last_loc1[use_id] = loc1 = getSubBitset<512, 256>(top_heap, use_id * branch_num_in_one_hbm, branch_num_in_one_hbm)._Find_first() + use_id * branch_num_in_one_hbm;
                    last_loc2[use_id] = loc2 = heap_tree[loc1]._Find_first();
                    last_offset[use_id] = offset = loc1 * leaf_num + loc2;
                    alloc_count[offset] = 1;
                    last_addr[use_id] = offset * _mini_space_size + 1;
                    addr[iter->order] = (last_addr[use_id] - 1) * unit_size;
                    _res_list[iter->pc].push_back(result(iter->order, addr[iter->order], iter->size, iter->pc));
                    // file << dec << right << setfill(' ') << setw(5) << iter->order << hex << right << setfill(' ') << setw(16) << addr[iter->order] << right << setfill(' ') << setw(16) << iter->size << right << setfill(' ') << setw(16) << iter->pc << endl;

                }
            }
        }
        else{ //free
            if(addr[iter->order] != -1){
                // cout << "Free " << addr[iter->order] << endl;
                tmp_offset = addr[iter->order] / (_mini_space_size * unit_size);
                tmp_loc1 = addr[iter->order] / (leaf_num * _mini_space_size * unit_size);
                tmp_loc2 = tmp_offset - tmp_loc1 * leaf_num;
                free_count[tmp_offset] ++;
                // inspire1 = free_count[tmp_offset];
                // inspire2 = alloc_count[tmp_offset];
                if(free_count[tmp_offset] == _mini_space_size){
                    heap_tree[tmp_loc1].set(tmp_loc2, 1);
                    top_heap.set(tmp_loc1, 1);
                    alloc_count[tmp_offset] = free_count[tmp_offset] = 0;
                }
            }
        }
    }
    maf_rate = maf / (_instr_num / 2.0);
    delete[] alloc_count;
    delete[] free_count;
    delete[] addr;
    delete[] heap_tree;
    delete[] last_loc1;
    delete[] last_loc2;
    delete[] last_addr;
    delete[] last_offset;
    // file.close();
    return maf_rate;
};

double Simulator::simulate_line(int hbm_bank_num, bool select_en){
    long long bank_size = _total_size / hbm_bank_num;
    int *a = new int[hbm_bank_num];
    int *offset = new int[hbm_bank_num];
    unsigned int *addr = new unsigned int[_instr_num / 2];
    int *alloc_count = new int[hbm_bank_num];
    int *free_count = new int[hbm_bank_num];
    int i, j, id;
    int maf = 0;
    float maf_rate;

    // ofstream file;
    // file.open("result.txt");

    _res_list.clear();
    for(i = 0; i < hbm_bank_num; i++){
        a[i] = 0;
        offset[i] = 0;
        alloc_count[i] = 0;
        free_count[i] = 0;
    }
    
    for(auto iter = _instr_list.begin(); iter != _instr_list.end(); iter ++){
        if(iter->alloc == 2){ //malloc
            id = hbm_bank_num;
            if(iter->id >= hbm_bank_num) iter->id = 0;//exit(1);
            if(offset[iter->id] + iter->size <= bank_size){
                id = iter->id;
            }else{
                for(j = 0; i < hbm_bank_num; j++){
                    if(offset[j] + iter->size <= bank_size){
                        id = j;
                        break;
                    }
                }
            }
            if(id == hbm_bank_num){
                maf ++;
                addr[iter->order] = -1;
                _res_list[iter->pc].push_back(result(iter->order, addr[iter->order], iter->size, iter->pc));
                // file << dec << right << setfill(' ') << setw(5) << iter->order << hex << right << setfill(' ') << setw(16) << addr[iter->order] << right << setfill(' ') << setw(16) << iter->size << right << setfill(' ') << setw(16) << iter->pc << endl;
            }else{
                addr[iter->order] = offset[id] + id * bank_size;
                _res_list[iter->pc].push_back(result(iter->order, addr[iter->order], iter->size, iter->pc));
                // file << dec << right << setfill(' ') << setw(5) << iter->order << hex << right << setfill(' ') << setw(16) << addr[iter->order] << right << setfill(' ') << setw(16) << iter->size << right << setfill(' ') << setw(16) << iter->pc << endl;
                offset[id] += iter->size;
                alloc_count[id] ++;
            }
        }
        else{ //free
            if(addr[iter->order] != -1){
                int inspire = addr[iter->order];
                id = addr[iter->order] / bank_size;
                free_count[id] ++;
                if(free_count[id] == alloc_count[id]){
                    free_count[id] = alloc_count[id] = 0;
                    offset[id] = 0;
                }
            }
        }
    }
    maf_rate = maf / (_instr_num / 2.0);
    delete[] a;
    delete[] offset;
    delete[] addr;
    delete[] alloc_count;
    delete[] free_count;
    // file.close();
    return maf_rate;
};

double criteria_get(double maf, int area){
    return maf * area;
}

void Simulator::trial(int hbm_bank_num, bool select_en){
    bool tag = 0;
    ofstream file;
    file.open("result.txt");
    if(!file.is_open()){
		cout << "Cannot open the result.txt." << endl;
        exit(EXIT_FAILURE);
    }
    evaluation tmp;
    evaluation rec;
    vector<evaluation> trial_res;
    int max_size = 0;
    int min_size = _total_size;
    int mini_id = 1;
    int max_heap_kwta;
    int max_heap_hta;
    double criteria = criteria_get(1, _hta[65536]);

    //TODO TO TEST THE MINIMUM SIZE OF THE KWTA AND HTA
    for(auto iter = _instr_list.begin(); iter != _instr_list.end(); iter ++){
        max_size = iter->size > max_size ? iter->size : max_size;
        min_size = max(1, min(iter->size, min_size));
    }

    tmp.maf = simulate_line(hbm_bank_num, select_en);
    tmp.util = _line[hbm_bank_num]; //TODO TO USE A HASH TABLE TO CONTAIN THE INFORMATION OF UTILIZATION
    tmp.name = "line";
    tmp.depth = hbm_bank_num;
    trial_res.push_back(tmp);

    max_heap_kwta = min(65536, (int)pow(2, (int)log2(_total_size / max_size)));
    max_heap_hta = max(64, min(65536, (int)pow(2, (int)log2(_total_size / min_size))));
    if(max_heap_kwta >= 16){
        for(int i = 16; i <= max_heap_kwta; i = i * 2){
            tmp.maf = simulate_kwta(i, hbm_bank_num, select_en);
            if(tmp.maf == 0){
                file << "kwta" << i << endl;
                file.close();
                return;
            }
            tmp.util = _kwta[i];
            tmp.name = "kwta";
            tmp.depth = i;
            trial_res.push_back(tmp);
        }
    }
    for(int i = 64; i <= max_heap_hta; i = i * 2){
        tmp.maf = simulate_hta(i, hbm_bank_num, select_en);
        if(tmp.maf == 0){
            file << "hta" << i << endl;
            file.close();
            return;
        }
        tmp.util = _hta[i];
        tmp.name = "hta";
        tmp.depth = i;
        trial_res.push_back(tmp);
    }

    for(auto iter = trial_res.begin(); iter != trial_res.end(); iter ++){
        if(criteria < criteria_get(iter->maf, iter->util)){
            criteria = criteria_get(iter->maf, iter->util);
            rec = *iter;
        }
    }
    file << rec.name << rec.depth << endl;
    file.close();
    //TODO USE THE HAT TO TEST THE MAF
}

void Simulator::print_res(string file_name2){
    ofstream file, file2;
    file2.open(file_name2);

    for(int i = 0; i < _pc_num; i ++){
        int n = _res_list[i].size();
        string file_name = "./generate/PE" + to_string(i) + ".v";
        file.open(file_name);
		file << "module PE" << dec << i << "(" << endl;
		file << "    input           mm_clk," << endl;
		file << "    input           mm_aresetn," << endl;
		file << "    input           start," << endl;
		file << "    // input   [32:0]  addr," << endl;
		file << "    output reg      finish," << endl;
		file << "    output reg[31:0]time_rec," << endl;
		file << "    //" << endl;
		file << "    output  [5:0]   m_axi_awid," << endl;
		file << "    output  [32:0]  m_axi_awaddr," << endl;
		file << "    output  [7:0]   m_axi_awlen," << endl;
		file << "    output          m_axi_awvalid," << endl;
		file << "    input           m_axi_awready," << endl;
		file << "    //" << endl;
		file << "    output  [255:0] m_axi_wdata," << endl;
		file << "    output  [31:0]  m_axi_wstrb," << endl;
		file << "    output          m_axi_wlast," << endl;
		file << "    output          m_axi_wvalid," << endl;
		file << "    input           m_axi_wready," << endl;
		file << "    //" << endl;
		file << "    input   [5:0]   m_axi_bid, // unused" << endl;
		file << "    input   [1:0]   m_axi_bresp, // unused" << endl;
		file << "    input           m_axi_bvalid," << endl;
		file << "    output          m_axi_bready," << endl;
		file << "    //" << endl;
		file << "    output  [5:0]   m_axi_arid," << endl;
		file << "    output  [32:0]  m_axi_araddr," << endl;
		file << "    output  [7:0]   m_axi_arlen," << endl;
		file << "    output          m_axi_arvalid," << endl;
		file << "    input           m_axi_arready," << endl;
		file << "    //" << endl;
		file << "    input   [5:0]   m_axi_rid," << endl;
		file << "    input   [255:0] m_axi_rdata," << endl;
		file << "    input   [1:0]   m_axi_rresp, // unused" << endl;
		file << "    input           m_axi_rlast," << endl;
		file << "    input           m_axi_rvalid," << endl;
		file << "    output          m_axi_rready" << endl;
		file << ");" << endl;
		file << "" << endl;
		file << "localparam IDLE         = 2'd0," << endl;
		file << "           WRITE        = 2'd1," << endl;
		file << "           READ         = 2'd2," << endl;
		file << "           FINISH       = 2'd3;" << endl;
		file << "" << endl;
		file << "localparam TOTAL        = 16'd" << dec << n << ";" << endl;
		file << "" << endl;
		file << "reg   [1:0]   ctrl_state;" << endl;
		file << "reg           start_dly;" << endl;
		file << "wire          start_sig;" << endl;
		file << "" << endl;
		file << "reg           ddr_write_enable;" << endl;
		file << "reg   [31:0]  ddr_write_addr;" << endl;
		file << "reg   [23:0]  ddr_write_cnt;" << endl;
		file << "reg           ddr_write_stop;" << endl;
		file << "wire          ddr_write_ready;" << endl;
		file << "reg   [255:0] ddr_write_data;" << endl;
		file << "reg           ddr_write_data_valid;" << endl;
		file << "wire          write_finish;" << endl;
		file << "" << endl;
		file << "reg           ddr_read_enable;" << endl;
		file << "reg   [31:0]  ddr_read_addr;" << endl;
		file << "reg   [23:0]  ddr_read_cnt;" << endl;
		file << "reg   [3:0]   ddr_read_id;" << endl;
		file << "wire          rd_instr_resp;" << endl;
		file << "" << endl;
		file << "reg           ddr_read_stop;" << endl;
		file << "wire          ddr_read_valid;" << endl;
		file << "wire  [255:0] ddr_read_data;" << endl;
		file << "wire  [3:0]   ddr_read_id_out;" << endl;
		file << "wire          read_finish;" << endl;
		file << "" << endl;
		file << "reg   [32:0]  addr[5:0]; // = {32'h8000, 32'hc000, 32'hc800};" << endl;
		file << "reg           type[5:0]; // = {1'b1, 1'b0, 1'b1};" << endl;
		file << "reg   [23:0]  size[5:0]; // = {1'h400, 1'h800, 1'h200};" << endl;
		file << "reg   [15:0]  cnt;" << endl;
		file << "" << endl;
		file << "assign start_sig = start & (~start_dly);" << endl;
		file << "" << endl;
		file << "always @(posedge mm_clk) " << endl;
		file << "    start_dly <= start;" << endl;
		file << "" << endl;
		file << "always @(posedge mm_clk) " << endl;
		file << "    finish <= (ctrl_state == FINISH);        " << endl;
		file << "" << endl;
		file << "always @(posedge mm_clk) begin" << endl;
		file << "    if(~mm_aresetn)begin" << endl;
		file << "        ddr_write_enable = 0;" << endl;
		file << "        ddr_write_addr = 0; " << endl;
		file << "        ddr_write_cnt = 0;  " << endl;
		file << "        ddr_write_stop = 0;            " << endl;
		file << "        ddr_write_data = 3;  " << endl;
		file << "        ddr_write_data_valid = 0;           " << endl;
		file << "        ddr_read_enable = 0;           " << endl;
		file << "        ddr_read_addr = 0; " << endl;
		file << "        ddr_read_cnt = 0; " << endl;
		file << "        ddr_read_id = 0;  " << endl;
		file << "        ddr_read_stop = 0;     " << endl;
		file << "        ctrl_state = IDLE;   " << endl;
		file << "        cnt = 0;  " << endl;
		file << "        time_rec = 0;" << endl;
		file << "" << endl;
        for(int j = 0; j < n; j ++){
            file << "        addr[" << dec << j << "] = 32'h" << hex << _res_list[i][j].addr << ";" << endl;
            file << "        type[" << dec << j << "] = 1'b" << hex << j % 2 << ";" << endl;
            file << "        size[" << dec << j << "] = 24'h" << hex << _res_list[i][j].size << ";" << endl;
        }
		file << "    end" << endl;
		file << "    else begin" << endl;
		file << "        case(ctrl_state)" << endl;
		file << "            IDLE:" << endl;
		file << "            begin" << endl;
		file << "                cnt <= 0;" << endl;
		file << "                if(start_sig) begin" << endl;
		file << "                    ddr_write_stop <= 0;" << endl;
		file << "                    ddr_write_data_valid <= 1;" << endl;
		file << "                    ddr_write_data <= 3;" << endl;
		file << "                    time_rec <= 0;" << endl;
		file << "                    cnt <= cnt + 1;" << endl;
		file << "                    if(cnt == TOTAL)" << endl;
		file << "                        ctrl_state <= FINISH;" << endl;
		file << "                    else if(type[cnt]) begin //write command" << endl;
		file << "                        ddr_write_cnt <= size[cnt];" << endl;
		file << "                        ddr_write_addr <= addr[cnt];" << endl;
		file << "                        ddr_write_enable <= 1;" << endl;
		file << "                        ctrl_state <= WRITE;" << endl;
		file << "                    end" << endl;
		file << "                    else begin //read command" << endl;
		file << "                        ddr_read_cnt <= size[cnt];" << endl;
		file << "                        ddr_read_addr <= addr[cnt];" << endl;
		file << "                        ddr_read_enable <= 1;" << endl;
		file << "                        ctrl_state <= READ;                        " << endl;
		file << "                    end" << endl;
		file << "                end" << endl;
		file << "            end" << endl;
		file << "            WRITE:" << endl;
		file << "            begin" << endl;
		file << "                time_rec <= time_rec + 1;" << endl;
		file << "                ddr_write_data <= ddr_write_data + 1;" << endl;
		file << "                ddr_write_enable <= 0;" << endl;
		file << "                if(write_finish) begin" << endl;
		file << "                    cnt <= cnt + 1;" << endl;
		file << "                    if(cnt == TOTAL)" << endl;
		file << "                        ctrl_state <= FINISH;" << endl;
		file << "                    else if(type[cnt]) begin //write command" << endl;
		file << "                        ddr_write_cnt <= size[cnt];" << endl;
		file << "                        ddr_write_addr <= addr[cnt];" << endl;
		file << "                        ddr_write_enable <= 1;" << endl;
		file << "                        ctrl_state <= WRITE;" << endl;
		file << "                    end" << endl;
		file << "                    else begin //read command" << endl;
		file << "                        ddr_read_cnt <= size[cnt];" << endl;
		file << "                        ddr_read_addr <= addr[cnt];" << endl;
		file << "                        ddr_read_enable <= 1;" << endl;
		file << "                        ctrl_state <= READ;                        " << endl;
		file << "                    end" << endl;
		file << "                end" << endl;
		file << "            end" << endl;
		file << "            READ:" << endl;
		file << "            begin" << endl;
		file << "                time_rec <= time_rec + 1;" << endl;
		file << "                ddr_read_enable <= 0;" << endl;
		file << "                if(read_finish) begin" << endl;
		file << "                    cnt <= cnt + 1;" << endl;
		file << "                    if(cnt == TOTAL)" << endl;
		file << "                        ctrl_state <= FINISH;" << endl;
		file << "                    else if(type[cnt]) begin //write command" << endl;
		file << "                        ddr_write_cnt <= size[cnt];" << endl;
		file << "                        ddr_write_addr <= addr[cnt];" << endl;
		file << "                        ddr_write_enable <= 1;" << endl;
		file << "                        ctrl_state <= WRITE;" << endl;
		file << "                    end" << endl;
		file << "                    else begin //read command" << endl;
		file << "                        ddr_read_cnt <= size[cnt];" << endl;
		file << "                        ddr_read_addr <= addr[cnt];" << endl;
		file << "                        ddr_read_enable <= 1;" << endl;
		file << "                        ctrl_state <= READ;                        " << endl;
		file << "                    end" << endl;
		file << "                end" << endl;
		file << "            end" << endl;
		file << "            FINISH:" << endl;
		file << "            begin" << endl;
		file << "                ctrl_state <= IDLE;" << endl;
		file << "            end" << endl;
		file << "        endcase" << endl;
		file << "    end" << endl;
		file << "end  " << endl;
		file << "" << endl;
		file << "ddr_driver new_driver(" << endl;
		file << "	.mm_clk(mm_clk)," << endl;
		file << "	.mm_aresetn(mm_aresetn)," << endl;
		file << "	.ddr_write_enable(ddr_write_enable)," << endl;
		file << "	.ddr_write_addr(ddr_write_addr)," << endl;
		file << "	.ddr_write_cnt(ddr_write_cnt)," << endl;
		file << "	.ddr_write_stop(ddr_write_stop)," << endl;
		file << "	.ddr_write_ready(ddr_write_ready)," << endl;
		file << "	.ddr_write_data(ddr_write_data)," << endl;
		file << "	.ddr_write_data_valid(ddr_write_data_valid)," << endl;
		file << "	.write_finish(write_finish)," << endl;
		file << "	.ddr_read_enable(ddr_read_enable)," << endl;
		file << "	.ddr_read_addr(ddr_read_addr)," << endl;
		file << "	.ddr_read_cnt(ddr_read_cnt)," << endl;
		file << "	.ddr_read_id(ddr_read_id)," << endl;
		file << "	.rd_instr_resp(rd_instr_resp)," << endl;
		file << "	.ddr_read_stop(ddr_read_stop)," << endl;
		file << "	.ddr_read_valid(ddr_read_valid)," << endl;
		file << "	.ddr_read_data(ddr_read_data)," << endl;
		file << "	.ddr_read_id_out(ddr_read_id_out)," << endl;
		file << "	.read_finish(read_finish)," << endl;
		file << "" << endl;
		file << "	.m_axi_awid(m_axi_awid)," << endl;
		file << "	.m_axi_awaddr(m_axi_awaddr)," << endl;
		file << "	.m_axi_awlen(m_axi_awlen)," << endl;
		file << "	.m_axi_awvalid(m_axi_awvalid)," << endl;
		file << "	.m_axi_awready(m_axi_awready)," << endl;
		file << "	.m_axi_wdata(m_axi_wdata)," << endl;
		file << "	.m_axi_wstrb(m_axi_wstrb)," << endl;
		file << "	.m_axi_wlast(m_axi_wlast)," << endl;
		file << "	.m_axi_wvalid(m_axi_wvalid)," << endl;
		file << "	.m_axi_wready(m_axi_wready)," << endl;
		file << "	.m_axi_bid(m_axi_bid), // unused" << endl;
		file << "	.m_axi_bresp(m_axi_bresp), // unused" << endl;
		file << "	.m_axi_bvalid(m_axi_bvalid)," << endl;
		file << "	.m_axi_bready(m_axi_bready)," << endl;
		file << "	.m_axi_arid(m_axi_arid)," << endl;
		file << "	.m_axi_araddr(m_axi_araddr)," << endl;
		file << "	.m_axi_arlen(m_axi_arlen)," << endl;
		file << "	.m_axi_arvalid(m_axi_arvalid)," << endl;
		file << "	.m_axi_arready(m_axi_arready)," << endl;
		file << "	.m_axi_rid(m_axi_rid)," << endl;
		file << "	.m_axi_rdata(m_axi_rdata)," << endl;
		file << "	.m_axi_rresp(m_axi_rresp), // unused" << endl;
		file << "	.m_axi_rlast(m_axi_rlast)," << endl;
		file << "	.m_axi_rvalid(m_axi_rvalid)," << endl;
		file << "    .m_axi_rready(m_axi_rready)" << endl;
		file << ");" << endl;
		file << "" << endl;
		file << "endmodule" << endl;
        // for(auto iter = _res_list.begin(); iter != _res_list.end(); iter ++){
            // if(iter->pc == i){
        for(int j = 0; j < n; j++){
            file2 << dec << right << setfill(' ') << setw(5) << _res_list[i][j].order << hex << right << setfill(' ') << setw(16) << _res_list[i][j].addr;
            file2 << right << setfill(' ') << setw(16) << _res_list[i][j].size << right << setfill(' ') << setw(16) << _res_list[i][j].pc << endl;
        }
        file2 << endl;
        file.close();
    }
    // file.close();
}