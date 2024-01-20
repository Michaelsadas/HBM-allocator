#include<cstdio>
#include<cstring>
#include<iostream>
#include<ap_int.h>
#include "ap_utils.h"

#define BRANCH 32 //mini heap in one branch
#define BRANCH_BIAS 5 //log2(BRANCH)
#define TRUNK 16 //number of branches
#define TRUNK_BIAS 4 //log2(TRUNK)
#define L 4 //number of branches in one HBM stack assuming there are 16 HBM ports
#define B 4  //number of banks depending on HBM
#define HEAP_BIAS 3 //number of log2(spaces in one mini heap) + 1
#define DATA_BIAS 14 //the log2(size of one space)

typedef struct{
    int free_target;
    int addr;
    int size;
    char cmd;
    char id;
} allocator_port;

ap_uint<8> log_2_8bit(ap_uint<8> tmp)
{
//#pragma HLS INLINE
	ap_uint<3> rc =0;
	switch (tmp)
	{
		case 1: rc = 0;break;
		case 2: rc = 1;break;
		case 4: rc = 2;break;
		case 8: rc = 3;break;
		case 16: rc = 4;break;
		case 32: rc = 5;break;
		case 64: rc = 6;break;
		case 128: rc = 7;break;
	}
	return rc;
}

static ap_uint<8> hash[37]={-1, 0, 1, 26, 2, 23, 27, -1, 3, 16, 24, 30, 28, 11, -1, 13, 4, 7, 17, -1, 25, 22, 31, 15, 29, 10, 12, 6, -1, 21, 14, 9, 5, 20, 8, 19, 18};
const unsigned long long magic = 3134165325;
ap_uint<8> log_2_32bit(ap_uint<32> tmp)
{
	ap_uint<32> rc = (((tmp * magic) >> 32) + tmp) >> 6;
	return hash[tmp-((rc<<5)+(rc<<2)+rc)];
}

ap_uint<8> log_2_16bit(ap_uint<16> tmp)
{
#pragma HLS INLINE
	ap_uint<16> rc =0;
	switch (tmp)
	{
		case 1: rc = 0;break;
		case 2: rc = 1;break;
		case 4: rc = 2;break;
		case 8: rc = 3;break;
		case 16: rc = 4;break;
		case 32: rc = 5;break;
		case 64: rc = 6;break;
		case 128: rc = 7;break;
		case 256: rc = 8;break;
		case 512: rc = 9;break;
		case 1024: rc = 10;break;
		case 2048: rc = 11;break;
		case 4096: rc = 12;break;
		case 8192: rc = 13;break;
		case 16384: rc = 14;break;
		case 32768: rc = 15;break;
	}
	return rc;
}

ap_uint<8> log_2_64bit(ap_uint<64> tmp)
{
	#pragma HLS INLINE off
	ap_uint<16> AA,BB,CC,DD;
	ap_uint<8> loc1;
	AA=tmp(15,0);BB=tmp(31,16);CC=tmp(47,32);DD=tmp(63,48);
	if (AA)loc1 = log_2_16bit(AA);
	if (BB)loc1 = log_2_16bit(BB)+16;
	if (CC)loc1 = log_2_16bit(CC)+32;
	if (DD)loc1 = log_2_16bit(DD)+48;
	return loc1;
}

ap_uint<16> log_2_256bit(ap_uint<256> tmp)
{
	#pragma HLS INLINE off
	ap_uint<64> AA,BB,CC,DD;
	ap_uint<16> loc1;
	AA=tmp(63,0);BB=tmp(127,64);CC=tmp(191,128);DD=tmp(255,192);
	if (AA)loc1 = log_2_16bit(AA);
	if (BB)loc1 = log_2_16bit(BB)+64;
	if (CC)loc1 = log_2_16bit(CC)+128;
	if (DD)loc1 = log_2_16bit(DD)+192;
	return loc1;
}

static ap_uint<BRANCH> heap_tree[TRUNK]={-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
};

static ap_uint<TRUNK> top_heap=-1;
static int last_offset[B]={-1, -1, -1, -1}; //20 should be a parameter
static ap_uint<2 * HEAP_BIAS> used_free[512]; //one for free and one for allocate
static int last_loc1[B], last_loc2[B];
static int last_addr[B];

volatile void kwta_allocator(volatile allocator_port *alloc)
{
    #pragma HLS RESOURCE variable=heap_tree core=RAM_T2P_BRAM
    #pragma HLS interface ap_hs port=alloc
    #pragma HLS allocation instances=log_2_64bit limit=2 function

    section0:{
        #pragma HLS protocol fixed
        ap_uint<32> free_target = 0; //20 should be a parameter
        ap_uint<8> cmd = 0;
        ap_uint<8> id = 0;
        ap_uint<8> used_id;
        int loc1 = 0, loc2 = 0; //20 should be a parameter
        static ap_uint<HEAP_BIAS> cur_used = 0, cur_free = 0; //TODO
        static ap_int<32> offset=-1;

        section1:{
            #pragma HLS protocol fixed
            cmd = alloc->cmd;
            free_target = alloc->free_target;
            id = alloc->id;
        }
        if (cmd == 2)
        {
            #pragma HLS protocol fixed
                
            if(top_heap==0){
                #pragma HLS protocol fixed
                alloc->addr = -1;
            }
            else{
                #pragma HLS protocol fixed
                if(top_heap(id * L + L - 1, id * L) != 0) //if the target memory is available
                {
                    #pragma HLS protocol fixed
                    used_id = id;
                }
                else // find another available memory
                {
                    if(top_heap(L - 1, 0) != 0) used_id = 0;
                    if(top_heap(2 * L - 1, L) != 0) used_id = 1;
                    if(top_heap(3 * L - 1, 2 * L) != 0) used_id = 2;
                    if(top_heap(4 * L - 1, 3 * L) != 0) used_id = 3;
                }

                offset = last_offset[used_id];
                loc1 = last_loc1[used_id];
                loc2 = last_loc2[used_id];
                if(offset(19,19) == 0)
                {
                    section5:{
						#pragma HLS protocol fixed
						alloc->addr = last_addr[used_id] << DATA_BIAS;   // if output_addr<0, it means the allocation turns out to be a failure.
					}
                    last_addr[used_id] ++;
                    cur_used = used_free[offset].range(2 * HEAP_BIAS - 1, HEAP_BIAS);
                    used_free[offset].range(2 * HEAP_BIAS - 1, HEAP_BIAS) = cur_used + 1;
                    if(cur_used == (1 << HEAP_BIAS) - 1){
                        heap_tree[loc1].set(loc2, 0);
                        last_offset[used_id] = -1;
                        if(heap_tree[loc1] == 0){
                            top_heap.set(loc1, 0);
                        }
                    }
                }
                else
                {
					last_loc1[used_id] = loc1 = log_2_16bit((ap_uint<16>)(top_heap(used_id * L + L - 1, used_id * L)-(top_heap(used_id * L + L - 1, used_id * L)&(top_heap(used_id * L + L - 1, used_id * L)-1)))) + used_id * L; //TODO
					last_loc2[used_id] = loc2 = log_2_64bit((ap_uint<64>)(heap_tree[loc1]-(heap_tree[loc1]&(heap_tree[loc1]-1)))); //TODO
					last_offset[used_id] = offset = (loc1 << BRANCH_BIAS) + loc2;
					used_free[offset].range(2 * HEAP_BIAS - 1, HEAP_BIAS) = 1;
					last_addr[used_id] = (offset << HEAP_BIAS) + 1;
                    section6:{
                        #pragma HLS protocol fixed
                        alloc->addr = (last_addr[used_id] - 1) << DATA_BIAS;
                    }               
                }
            }
        }

        else if (cmd == 3)
        {
            int tmp_loc1, tmp_loc2, tmp_offset; //TODO
            tmp_loc2 = free_target.range(BRANCH_BIAS + DATA_BIAS + HEAP_BIAS - 1, DATA_BIAS + HEAP_BIAS);
            tmp_loc1 = free_target.range(TRUNK_BIAS + BRANCH_BIAS + DATA_BIAS + HEAP_BIAS - 1, BRANCH_BIAS + DATA_BIAS + HEAP_BIAS);
            tmp_offset = free_target.range(TRUNK_BIAS + BRANCH_BIAS + DATA_BIAS + HEAP_BIAS - 1, DATA_BIAS + HEAP_BIAS);
            cur_free = used_free[tmp_offset].range(HEAP_BIAS - 1,0) + 1;
            if(cur_free == used_free[tmp_offset].range(2 * HEAP_BIAS - 1, HEAP_BIAS))
            {
				heap_tree[tmp_loc1].set(tmp_loc2,1);
				top_heap.set(tmp_loc1,1);
				used_free[tmp_offset]= 0;                
            }
            else 
                used_free[tmp_offset].range(HEAP_BIAS - 1,0) = cur_free;
        }

        else
        {
			io_section6:{
		    	#pragma HLS protocol fixed
			    alloc->addr = -1;
			}            
        }
    }

}

int main(){
	return 0;
}
