#pragma once
#include "csa.h"

#define TBUFSIZE 100000

int popcount_mask(mask_t msk);
//count how many bits are set to 1 in the mask
//m : a 64-bits mask_t where each bit repersent an idx

int block_index (csa* c, unsigned int offset);
//Searches the CSA for a block whose offset equals offset.
//return -1 if no block with the given offset exists

int value_index (mask_t msk , int bit);
//given a block's mask m and bit postion bit
//return how many bot are set after this bit
//this gives the index in the vals[] arrays where the value
//corresponding to bit and should be found or inserted

block* insert_block(csa* c, unsigned int offset);
//to insert a new block by the offset given to csa
//block sorted by offset
//to initialized new block: msk = 0; vals = null;

// helpers for csa_set
bool block_cover_value(block* blk, int bit, int val);
bool block_insert_value(block* blk, int bit, int val);

//helpers for tostring
void block_tostring(block* blk, char** p, size_t* rem);
void bit_tostring(block* blk, char** p, size_t* rem, int* seen);


