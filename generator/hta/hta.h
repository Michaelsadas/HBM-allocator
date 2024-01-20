#include<ap_int.h>
#include "ap_utils.h"

ap_uint<8> log_2_8bit(ap_uint<8> tmp)
{
	#pragma HLS INLINE
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
	ap_uint<8> rc =0;
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

ap_uint<16> log_2_128bit(ap_uint<128> tmp)
{
	#pragma HLS INLINE off
	ap_uint<64> AA,BB;
	ap_uint<16> loc1;
	AA=tmp(63,0);BB=tmp(127,64);
	if (AA)loc1 = log_2_64bit(AA);
	if (BB)loc1 = log_2_64bit(BB)+64;
	return loc1;
}

ap_uint<16> log_2_256bit(ap_uint<256> tmp)
{
	#pragma HLS INLINE off
	ap_uint<64> AA,BB,CC,DD;
	ap_uint<16> loc1;
	AA=tmp(63,0);BB=tmp(127,64);CC=tmp(191,128);DD=tmp(255,192);
	if (AA)loc1 = log_2_64bit(AA);
	if (BB)loc1 = log_2_64bit(BB)+64;
	if (CC)loc1 = log_2_64bit(CC)+128;
	if (DD)loc1 = log_2_64bit(DD)+192;
	return loc1;
}

ap_uint<16> log_2_512bit(ap_uint<512> tmp)
{
	#pragma HLS INLINE off
	ap_uint<128> AA,BB,CC,DD;
	ap_uint<16> loc1;
	AA=tmp(127,0);BB=tmp(255,128);CC=tmp(383,256);DD=tmp(511,384);
	if (AA)loc1 = log_2_128bit(AA);
	if (BB)loc1 = log_2_128bit(BB)+128;
	if (CC)loc1 = log_2_128bit(CC)+256;
	if (DD)loc1 = log_2_128bit(DD)+384;
	return loc1;
}

ap_uint<16> log_2_1024bit(ap_uint<1024> tmp)
{
	#pragma HLS INLINE off
	ap_uint<256> AA,BB,CC,DD;
	ap_uint<16> loc1;
	AA=tmp(255,0);BB=tmp(511,256);CC=tmp(767,512);DD=tmp(1023,768);
	if (AA)loc1 = log_2_256bit(AA);
	if (BB)loc1 = log_2_256bit(BB)+256;
	if (CC)loc1 = log_2_256bit(CC)+512;
	if (DD)loc1 = log_2_256bit(DD)+768;
	return loc1;
}

ap_uint<16> log_2_2048bit(ap_uint<2048> tmp)
{
	#pragma HLS INLINE off
	ap_uint<512> AA,BB,CC,DD;
	ap_uint<16> loc1;
	AA=tmp(511,0);BB=tmp(1023,512);CC=tmp(1535,1024);DD=tmp(2047,1536);
	if (AA)loc1 = log_2_512bit(AA);
	if (BB)loc1 = log_2_512bit(BB)+512;
	if (CC)loc1 = log_2_512bit(CC)+1024;
	if (DD)loc1 = log_2_512bit(DD)+1536;
	return loc1;
	return loc1;
}
