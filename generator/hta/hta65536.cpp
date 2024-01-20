#include<cstdio>
#include<cstring>
#include<iostream>
#include<ap_int.h>
#include "ap_utils.h"
#include "hta.h"

#define N 65536
#define LAYERS 16
#define MAU_SIZE 512
#define SHIFT 9
#define TREE_SHIFT 0 // 16 - log2(N)
#define BRANCH_SIZE 2048
#define branch_level 64

typedef struct{
int size;
int addr;
int free_target;
char id;
char cmd;
} allocator_port;

//the top layer of the tree is regards as layer 0
static ap_uint<64> buddy_tree[17] = {0x1, 0x3, 0xF, 0xFF, 0xFFFF, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF};
static ap_uint<BRANCH_SIZE> branch_tree[64]={-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
};

static ap_uint<64> buddy_mask[64] ={0,	0,	0x1,	0x3,	0xf,	0xff,	0xffff,	0xffff,	0xffff,	0xffff,	0xffff,	0xffff,	0xffff,	0xffff,	0xffff,	0xffff,	
0,	0,	0x2,	0xc,	0xf0,	0xff00,	0xffff0000,	0xffff0000,	0xffff0000,	0xffff0000,	0xffff0000,	0xffff0000,	0xffff0000,	0xffff0000,	0xffff0000,	0xffff0000,	
0,	0,	0x4,	0x30,	0xf00,	0xff0000,	0xffff00000000,	0xffff00000000,	0xffff00000000,	0xffff00000000,	0xffff00000000,	0xffff00000000,	0xffff00000000,	0xffff00000000,	0xffff00000000,	0xffff00000000,	
0,	0,	0x8,	0xc0,	0xf000,	0xff000000,	0xffff000000000000,	0xffff000000000000,	0xffff000000000000,	0xffff000000000000,	0xffff000000000000,	0xffff000000000000,	0xffff000000000000,	0xffff000000000000,	0xffff000000000000,	0xffff000000000000};//64 = 4 * 16
static ap_uint<64> mask[10]={1,3, 0xF, 0xFF, 0xFFFF, 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFFFFFFFFFF,0,0};
static ap_uint<BRANCH_SIZE> mask_mask[16];
static ap_uint<BRANCH_SIZE> full = -1;

static ap_uint<BRANCH_SIZE> branch_mask[16];

static ap_uint<16> loc_shift[17] = {0,	0,	0,	0,	0,	0,	0,	1,	3,	7, 15, 31, 63, 127, 255, 511, 1023};

static ap_uint<8> addr_layer_map[N];//HTA1024

volatile void hta_allocator(volatile allocator_port *alloc)//, volatile ap_uint<64> // *port1, volatile ap_uint<64> // *port2)
{
#pragma HLS ARRAY_PARTITION variable=branch_tree cyclic factor=2 dim=1
#pragma HLS ARRAY_PARTITION variable=buddy_tree cyclic factor=2 dim=1
#pragma HLS interface ap_hs port=alloc

#pragma HLS allocation instances=log_2_64bit limit=1 function
{
	#pragma HLS protocol fixed
	static bool a = true;
	if(a){
		branch_mask[7] = (full << 1) & (full >> 2045); 
		branch_mask[8] = (full << 3) & (full >> 2041); 
		branch_mask[9] = (full << 7) & (full >> 2033); 
		branch_mask[10] = (full << 15) & (full >> 2017); 
		branch_mask[11] = (full << 31) & (full >> 1985); 
		branch_mask[12] = (full << 63) & (full >> 1921); 
		branch_mask[13] = (full << 127) & (full >> 1793); 
		branch_mask[14] = (full << 255) & (full >> 1537); 
		branch_mask[15] = (full << 511) & (full >> 1025); 
		branch_mask[16] = (full << 1023) & (full >> 1); 
		mask_mask[1] = full >> 2046;
		mask_mask[2] = full >> 2044;
		mask_mask[3] = full >> 2040;
		mask_mask[4] = full >> 2032;
		mask_mask[5] = full >> 2016;
		mask_mask[6] = full >> 1984;
		mask_mask[7] = full >> 1920;
		mask_mask[8] = full >> 1792;
		mask_mask[9] = full >> 1536;
		mask_mask[10] = full >> 1024;
		a = false;
	}

	ap_wait();
	volatile int status = 0;
	int size = 0;
	ap_uint<16> tmp_size = 0, inv_size = 0;
	ap_uint<32> free_target = 0;
	int output_addr = 0;
	ap_uint<5> id = 0, cnt = 0;
	ap_uint<16> loc1 = 0, loc2 = 0;
	ap_uint<16> new_loc_s1 = 0, new_loc_s2 = 0, new_loc_f = 0, new_loc_g = 0;
	ap_uint<64> TMP_0 = 0;
	ap_uint<2048> TMP_1 = 0;
	ap_uint<5> ans, ans_f, ans_s, ans_g;
	bool sets;

	volatile int cmd = 0;
	io_section1:{
		#pragma HLS protocol fixed
		cmd = alloc->cmd;
		size = alloc->size;
		id = alloc->id;
		free_target = alloc->free_target;
		tmp_size = (size >> SHIFT) - 1;
		inv_size = tmp_size.reverse();
	}

	ans = 0;
	if(cmd == 2)
	{
		#pragma HLS protocol fixed
		if(size <= MAU_SIZE) {
			ans = LAYERS - 1;
		}
		else{
			TMP_0 = inv_size & (~(inv_size - 1));
			ans = log_2_64bit(TMP_0) - TREE_SHIFT;
		}

		ans_s = (ans > 6) ? (ap_uint<5>)6 : ans;

		if(ans <= 1){
			TMP_0 = buddy_tree[ans] & (~(buddy_tree[ans] - 1));
			loc1 = log_2_64bit(TMP_0);
		}
		else if((TMP_0 = buddy_tree[ans] & buddy_mask[id * 16 + ans_s]) != 0){
			TMP_0 = TMP_0 & (~(TMP_0 - 1));
			loc1 = log_2_64bit(TMP_0);
		}
		else{
			TMP_0 = buddy_tree[ans] & (~(buddy_tree[ans] - 1));
			loc1 = log_2_64bit(TMP_0);
		}

		if(TMP_0){
			if(ans <= 6){
				output_addr = loc1 << (LAYERS - ans);
				alloc->addr = output_addr << SHIFT;
				buddy_tree[ans].set(loc1, 0);
			}
			else{
				TMP_1 = branch_tree[loc1] & branch_mask[ans];
				TMP_1 = TMP_1&(~(TMP_1-1));
				loc2 = log_2_2048bit(TMP_1);
				output_addr = (loc1 << (LAYERS - 6)) + ((loc2 - loc_shift[ans])<< (LAYERS - ans));
				alloc->addr = output_addr << SHIFT;
				buddy_tree[6].set(loc1, 0);
				branch_tree[loc1].set(loc2, 0);
			}
			addr_layer_map[output_addr] = ans;

			//maintainance
			if(ans <= 6){
				TMP_0 = ~(mask[0] << loc1);
				loc2 = loc1 << 1;
				loc1 >>= 1;
				cnt = 1;
				buddy_tree[0] = 0;
				for(ans_f= ans+(ans==0)-1, ans_s=ans+1; (ans_f >= 1)||(ans_s <= 6); ans_f-=(ans_f!=0), ans_s+=(ans_s!=7)){
					#pragma HLS dependence variable=buddy_tree intra false
					#pragma HLS pipeline
					if(ans_f >= 1){
						buddy_tree[ans_f].set(loc1, 0);
						loc1 >>= 1;
					}
					if(ans_s <= 6){
						TMP_0 = ~(mask[cnt] << loc2); //TODO
						buddy_tree[ans_s] = buddy_tree[ans_s] & TMP_0;
						loc2 <<= 1;
						cnt ++;
					}
				}
				//TMP_1 = TMP_0;
				for(ans_s = 7; ans_s <=LAYERS; ans_s ++){
					#pragma HLS dependence variable=buddy_tree intra false
					#pragma HLS pipeline
					buddy_tree[ans_s] &= TMP_0;
				}
			}
			else{
				new_loc_g = loc1 >> 1;
				cnt = 1;
				new_loc_f = (loc2 - 1) >> 1;
				new_loc_s1 = (loc2 << 1) + 1;
				//new_loc_s2 = (loc2 << 1) + 2;
				buddy_tree[0] = 0;
				for(ans_g = 5, ans_f = ans - 1, ans_s = ans + 1; (ans_g >= 1) || (ans_f >= 7) || (ans_s <= LAYERS); ans_g -= (ans_g != 0), ans_f -= (ans_f != 6), ans_s += (ans_s != LAYERS + 1))
				{
					#pragma HLS dependence variable=buddy_tree intra false
					#pragma HLS pipeline

					if(ans_g >= 1){
						buddy_tree[ans_g].set(new_loc_g, 0);
						new_loc_g = new_loc_g >> 1;
					}
					if(ans_f >= 7){
						branch_tree[loc1].set(new_loc_f, 0);
						buddy_tree[ans_f].set(loc1, (branch_tree[loc1] & branch_mask[ans] != 0));
						new_loc_f = (new_loc_f - 1) >> 1;
					}					
					if(ans_s <= LAYERS){
						branch_tree[loc1] = branch_tree[loc1] & (~(mask_mask[cnt] << new_loc_s1));
						buddy_tree[ans_s].set(loc1, (branch_tree[loc1] & branch_mask[ans] != 0));
						new_loc_s1 = (new_loc_s1 << 1) + 1;
						cnt ++;
					}
				}
			}
		}
		else{
			alloc->addr = -1;
			return;
		}
	}
	if(cmd == 3){
		free_target = free_target >> SHIFT;
		ans = addr_layer_map[free_target];
		if(ans <= 6){
			TMP_0 = (mask[0] << loc1);
			loc1 = free_target.range(LAYERS - 1, LAYERS - ans);
			buddy_tree[ans].set(loc1, 1);
			new_loc_f = loc1 >> 1;
			new_loc_s1 = loc1 << 1;
			cnt = 1;
			for(ans_f= ans+(ans==0)-1, ans_s = ans + 1; (ans_f >= 1) || (ans_s <= 6); ans_f -= (ans_f != 0), ans_s += (ans_s != 7)){
				#pragma HLS dependence variable=buddy_tree intra false
				#pragma HLS pipeline
				if(ans_f >= 1){
					buddy_tree[ans_f].set(new_loc_f, buddy_tree[ans_f + 1]((new_loc_f << 1) + 1, (new_loc_f << 1) + 1) && buddy_tree[ans_f + 1](new_loc_f << 1, new_loc_f << 1));
					new_loc_f = new_loc_f >> 1;
				}
				if(ans_s <= 6){
					TMP_0 = (mask[cnt] << new_loc_s1);
					buddy_tree[ans_s] = buddy_tree[ans_s] | TMP_0;
					new_loc_s1 = (new_loc_s1 << 1);
					cnt ++;
				}
			}
			//TMP_1 = TMP_0;
			for(ans_s = 7; ans_s <=LAYERS; ans_s ++){
				#pragma HLS dependence variable=buddy_tree intra false
				#pragma HLS pipeline
				buddy_tree[ans_s] |= TMP_0;
			}
		}
		else{
			loc2 = free_target.range(LAYERS - 7, LAYERS - ans) + loc_shift[ans]; //index in the branch
			loc1 = free_target.range(LAYERS - 1, LAYERS - 6);//index in the buddy
			branch_tree[loc1].set(loc2, 1);
			buddy_tree[ans].set(loc1, 1);
			new_loc_f = (loc2 - 1) >> 1;
			new_loc_s1 = (loc2 << 1) + 1;
			cnt = 1;
			for(ans_f = ans - 1, ans_s = ans + 1; (ans_f >= 7) || (ans_s <= LAYERS); ans_f -= (ans_f != 6), ans_s += (ans_s != LAYERS + 1)){
				#pragma HLS dependence variable=buddy_tree intra false
				#pragma HLS pipeline
				if(ans_f >= 7){
					sets = branch_tree[loc1]((new_loc_f << 1) + 2, (new_loc_f << 1) + 2) && branch_tree[loc1]((new_loc_f << 1) + 1, (new_loc_f << 1) + 1);
					branch_tree[loc1].set(new_loc_f, sets);
					buddy_tree[ans_f].set(loc1, sets);
					new_loc_f = (new_loc_f - 1) >> 1;
				}
				if(ans_s <= LAYERS){
					branch_tree[loc1] = branch_tree[loc1] | (mask_mask[cnt] << new_loc_s1);
					new_loc_s1 = new_loc_s1 << 1;
				}				
			}
			if(branch_tree[loc1](2, 2) && branch_tree[loc1](1, 1)){
				branch_tree[loc1].set(0, 1);
				buddy_tree[6].set(loc1, 1);
				new_loc_f = loc1 >> 1;
				for(ans_g = 5; ans_g > 0; ans_g -= (ans_g != 0)){
					#pragma HLS dependence variable=buddy_tree intra false
					#pragma HLS pipeline
					buddy_tree[ans_g].set(new_loc_f, buddy_tree[ans_g + 1]((new_loc_f << 1) + 1, (new_loc_f << 1) + 1) && buddy_tree[ans_g + 1]((new_loc_f << 1), (new_loc_f << 1)));
					new_loc_f = new_loc_f >> 1;
				}
			}
			buddy_tree[0] = (buddy_tree[1]==3);
		}
		
	}
}
}
