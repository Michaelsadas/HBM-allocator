#include<cstdio>
#include<cstring>
#include<iostream>
#include<ap_int.h>
#include "ap_utils.h"

#define BANK 3355443
#define BANK_SHIFT 21
#define N 10

typedef struct{
    int free_target;
    int addr;
    int size;
    char cmd;
    char id;
} allocator_port;

static ap_uint <25> offsets[N] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static ap_uint <16> alloc_count[N] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static ap_uint<16> free_count[N] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

volatile void line_allocator(volatile allocator_port *alloc)
{
    #pragma HLS interface ap_hs port=alloc
    ap_uint<25> free_target = 0, size = 0; //20 should be a parameter
    ap_uint<25> loc;
    ap_uint<8> cmd = 0;
    ap_uint<8> id = 0;
	 ap_uint<8> i;
    ap_uint<25> id_res;
    ap_uint<8> used_id;
	section0:{
#pragma HLS protocol fixed
		cmd = alloc->cmd;
		size = alloc->size;
		free_target = alloc->free_target;
		id= alloc->id;
	}
	if (cmd == 2){
		id_res = 4;
		if(offsets[id] + size <= BANK){
			loc = offsets[id];
			id_res = id;
		}
		else{
			if(offsets[0] + size <= BANK) {id_res = 0;}
			else if(offsets[1] + size <= BANK) {id_res = 1;}
			else if(offsets[2] + size <= BANK) {id_res = 2;}
			else if(offsets[3] + size <= BANK) {id_res = 3;}
			else if(offsets[4] + size <= BANK) {id_res = 4;}
			else if(offsets[5] + size <= BANK) {id_res = 5;}
			else if(offsets[6] + size <= BANK) {id_res = 6;}
			else if(offsets[7] + size <= BANK) {id_res = 7;}
			else if(offsets[8] + size <= BANK) {id_res = 8;}
			else if(offsets[9] + size <= BANK) {id_res = 9;}
		}

		if(id_res == 4){
			#pragma HLS protocol fixed
			alloc->addr = -1;
		}
		else{
			section2:{
				#pragma HLS protocol fixed
				alloc->addr = offsets[id_res] + (id_res << BANK_SHIFT);
			}
			offsets[id_res] += size;
			alloc_count[id_res] ++;
		}
	}
	if (cmd == 3){
		id_res = free_target >> BANK_SHIFT;
		free_count[id_res] ++;
		if(free_count[id_res] == alloc_count[id_res]){
			free_count[id_res] = alloc_count[id_res] = 0;
			offsets[id_res] = 0;
		}
	}

}

int main(){
	return 0;
}
