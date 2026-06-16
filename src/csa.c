#include "csa.h"
#include "mydefs.h"


/*
   count how many bits in mask was set to 1
   return = 0 ( have no bit was set to 1)
*/
int count1(mask_t msk){
   return __builtin_popcountll(msk);
}

/*
  To find the index of the block corresponding to offset
  return -1 == nothing find
*/
int block_index(csa* c, unsigned int offset){
   if(c == NULL){
      return -1;
   }
   for(int i = 0; i < c->n; i++){
      if(c->b[i].offset == offset){
         return i;
      }
   }
   return -1;
}

/*
   Given the position of a bit
   calculate its position in the vals[] array.
*/
int value_index(mask_t msk, int bit){
   mask_t lower = 0;
   if(bit > 0){
      lower = msk & (((mask_t)1 << bit) - 1);
   }
   return count1(lower);
}

/*
   Insert a new block at the correct position, 
   the block array is sorted in ascending order by offset.
   Returns a pointer to the new block.
   Step:
      a. Find the position where the new block should be 
      inserted (in ascending order of offset)
      b. Expanding array c->b
      c. Shift the entire subsequent block to the right.
      d. Fill in the corresponding position
      e. returns pointer
*/
block* insert_block(csa* c, unsigned int offset){
   if(c==NULL){
      return NULL;
   }
   
   int pos = 0;
   while(pos < c->n && c->b[pos].offset < offset){
      pos++;
   }

   block* nb = realloc(c->b, sizeof(block) * (c->n + 1));
   if(nb==NULL){
      return NULL;
   }
   
   c->b = nb;
   memmove(&c->b[pos + 1], &c->b[pos], 
            sizeof(block) * (c->n - pos));
   c->b[pos].offset = offset;
   c->b[pos].msk = 0;
   c->b[pos].vals = NULL;
   c->n++;
   return &c->b[pos];
}


/*
   helper: cover existing value at bit in blk 
   step:
      a. Call value_index to find the bit location of vals[]
      b. Write the new value cover the old value
*/
bool block_cover_value(block* blk, int bit, int val){
   if(blk == NULL) return false;
   if(!(blk->msk & ((mask_t)1<<bit))) return false;
   int pos = value_index(blk->msk, bit);
   blk->vals[pos] = val;
   return true;
}

/*
   When a bit does not exist in the block, 
   insert the new value into the correct position of vals[] 
   and update the mask.
   step:
      a. Calculate the position pos that the bit in vals[].
      b. Expand vals[],Shift one position to the right.
      c. Write new value, updata to the right.
*/
bool block_insert_value(block* blk, int bit, int val){
   if(blk == NULL){
      return false;
   }
   int pos = value_index(blk->msk, bit);
   int cnt = count1(blk->msk);
   int* nv = realloc(blk->vals, sizeof(int) * (cnt + 1));

   if(nv == NULL){
      return false;
   }
   blk->vals = nv;
   memmove(&blk->vals[pos + 1], &blk->vals[pos],
      sizeof(int) * (cnt - pos));
   blk->vals[pos] = val;
   blk->msk |= ((mask_t)1 << bit);
   return true;
}


csa* csa_init(void){
   csa* c = malloc(sizeof(csa));
   if(c==NULL){
      return NULL;
   }
   c->b = NULL;
   c->n = 0;
   return c;
}

/*
a.Calculate offset = idx / sixty - four
b.Calculate bit = idx % sixty - four
c.Call block_index
d.If the block cannot be found(return false)
e.Check if the corresponding bit of the mask is 1.
f.Call value_index to find the position of vals[]
g.Write vals[pos] to *val
h.return true  
*/
bool csa_get(csa* c, int idx, int* val){
   if(c==NULL || val==NULL || idx<0){
      return false;
   }
   unsigned int offset = (unsigned int)(idx/MSKLEN)*MSKLEN;
   int bit = idx % MSKLEN;
   int bi = block_index(c, offset);
   if(bi<0){
      return false;
   }
   block* blk = &c->b[bi];
   if(!(blk->msk & ((mask_t)1<<bit))){
      return false;
   }
   int pos = value_index(blk->msk, bit);
   *val = blk->vals[pos];
   return true;
}

/*
a.Calculate offset, bit and find block
b.If it does not exist (insert_block)
c.If it exists and the bit already exists(block_cover_value)
d.If it does not exist (block_insert_value)
e.return true
*/
bool csa_set(csa* c, int idx, int val){

   if(c==NULL || idx<0){
      return false;
   }
   unsigned int offset = (unsigned int)(idx/MSKLEN)*MSKLEN;
   int bit = idx % MSKLEN;
   int bi = block_index(c, offset);
   block* blk = NULL;
   
   if(bi<0){
      blk = insert_block(c, offset);
      if(blk==NULL){
         return false;
      }
   } else {
      blk = &c->b[bi];
   }

   if(blk->msk & ((mask_t)1<<bit)){
      return block_cover_value(blk, bit, val);
   }
   return block_insert_value(blk, bit, val);
}


void bit_tostring(block* blk, char** p, 
                  size_t* rem, int* seen){
   if (blk == NULL || p == NULL || *p == NULL || 
       rem == NULL || *rem == 0 || seen == NULL) {
      return;
   }
    
   for (int bit = 0; bit < MSKLEN && *rem > 0; bit++) {
      mask_t m = (mask_t)1 << bit;
      if (blk->msk & m) {
         int written;
         if (*seen) {
            written = snprintf(*p, *rem, ":");
            if (written < 0) {
               return;
            }
            *p += written; *rem -= (size_t)written;
         }
         written = snprintf(*p, *rem, "[%u]=%d", 
                    (unsigned)(blk->offset + bit),
                    blk->vals[value_index(blk->msk, bit)]);
         if (written < 0) {
            return;
         }
         *p += written; *rem -= (size_t)written;
         (*seen)++;
      }
   }
}


void block_tostring(block* blk, char** p, size_t* rem)
{
   if (blk == NULL || p == NULL || *p == NULL || 
       rem == NULL || *rem == 0) return;
   int written = snprintf(*p, *rem,"{%d|",count1(blk->msk));
   if (written < 0) return;
   *p += written; *rem -= (size_t)written;

   int seen = 0;
   bit_tostring(blk, p, rem, &seen);

   written = snprintf(*p, *rem, "}");
   if (written < 0) {
      return;
   }
   *p += written; *rem -= (size_t)written;
}


void csa_tostring(csa* c, char* s){
   if (s == NULL) {
      return;
   }
   char *p = s; size_t rem = TBUFSIZE; int written;

   if (c == NULL) {
      *p = '\0';
      return;
   }

   if (c->n == 0) {
      snprintf(p, rem, "0 blocks");
      return;
   }

   written = snprintf(p, rem, "%d block%s ", 
                      c->n, c->n == 1 ? "" : "s");
   if (written < 0) {
      return;
   }
   p += written; rem -= (size_t)written;

   for (int i = 0; i < c->n; i++) {
      block *blk = &c->b[i];
      block_tostring(blk, &p, &rem);
   }

   *p = '\0';
}


void csa_free(csa** l)
{
   if(l==NULL || *l==NULL){
      return;
   }
   csa* c = *l;
   for(int i=0; i<c->n; i++){
      free(c->b[i].vals);
   }
   free(c->b);
   free(c);
   *l = NULL;
}


void test(void)
{
   csa* c = NULL;

//conut1
   assert(count1(0ULL) == 0);
   assert(count1(1ULL) == 1);
   assert(count1(3ULL) == 2);

//value_index (mask with bits 2,5,6 set)
   mask_t m = 0;
   m |= (1ULL << 2);
   m |= (1ULL << 5);
   m |= (1ULL << 6);
   assert(value_index(m, 2) == 0);
   assert(value_index(m, 5) == 1);
   assert(value_index(m, 6) == 2);

//insert_block / block_index / basic structure checks 
   c = csa_init();
   assert(c != NULL);

   block* b0 = insert_block(c, 0);
   assert(b0 != NULL);
   assert(c->n == 1);
   assert(b0->offset == 0);

   block* b1 = insert_block(c, 64);
   block* b2 = insert_block(c, 128);
   assert(b1 != NULL && b2 != NULL);
   assert(c->n == 3);

   assert(block_index(c, 0) == 0);
   assert(block_index(c, 64) == 1);
   assert(block_index(c, 128) == 2);
   assert(block_index(c, 192) == -1);

   int tmp = 0;
   if (csa_set(c, 2, 25)) {
      assert(csa_get(c, 2, &tmp));
      assert(tmp == 25);
   }


//invalid args 
   assert(!csa_set(NULL, 1, 1));
   assert(!csa_set(c, -1, 5));
   assert(!csa_get(c, 0, NULL));

 //insert out-of-order and ensure blocks remain sorted
   block* bmid = insert_block(c, 32);
   assert(bmid != NULL);
   assert(block_index(c, 0) == 0);
   assert(block_index(c, 32) == 1);
   assert(block_index(c, 64) == 2);
   assert(block_index(c, 128) == 3);

//test block-level helpers at edge bits (0 and MSKLEN-1)
   block* blk0 = &c->b[block_index(c, 0)];
   assert(blk0 != NULL);
   assert(block_insert_value(blk0, 0, 100));//low bit 
   assert(block_insert_value(blk0, MSKLEN - 1, 200));//high
   assert(block_insert_value(blk0, MSKLEN/2, 150));//middle

//retrieval via csa_get for edge bits 
   assert(csa_get(c, 0, &tmp) && tmp == 100);
   assert(csa_get(c, (int)(blk0->offset + (MSKLEN - 1)), 
                           &tmp) && tmp == 200);
   assert(csa_get(c, (int)(blk0->offset + (MSKLEN / 2)), 
                           &tmp) && tmp == 150);
 
   assert(!block_cover_value(blk0, 1, 5));//bit 1 not set 
   assert(block_cover_value(blk0, 0, 111));//overwrite low b
   assert(csa_get(c, 0, &tmp) && tmp == 111);

//csa_set should call cover when updating existing value 
   assert(csa_set(c, 0, 222));
   assert(csa_get(c, 0, &tmp) && tmp == 222);

//tostring covers block_tostring and bit_tostring paths
   char tbuf[TBUFSIZE];
   csa_tostring(c, tbuf);
//must contain block count and some inserted entries
   assert(strstr(tbuf, "block") != NULL);
   assert(strstr(tbuf, "[0]=222") != NULL);
//ensure ordering inside block: low -> middle -> high 
   char midpat[64], highpat[64];
   snprintf(midpat, sizeof(midpat), 
   "[%u]=%d", (unsigned)(blk0->offset + (MSKLEN/2)), 150);
   snprintf(highpat, sizeof(highpat), 
   "[%u]=%d", (unsigned)(blk0->offset + (MSKLEN-1)), 200);
   char *p0 = strstr(tbuf, "[0]=222");
   char *pm = strstr(tbuf, midpat);
   char *ph = strstr(tbuf, highpat);
   assert(p0 && pm && ph && p0 < pm && pm < ph);

//test csa_free robustness: free NULL and double free path
   csa_free(&c);
   assert(c == NULL);
   csa_free(&c); //hould be no-op and not crash 

//tostring for empty csa 
   c = csa_init();
   assert(c != NULL);
   csa_tostring(c, tbuf);
   assert(strcmp(tbuf, "0 blocks") == 0);

//clean up
   csa_free(&c);
   assert(c == NULL);

}

#ifdef EXT
void csa_foreach(void (*func)
(int* p, int* ac), csa* c, int* ac){
   if (func == NULL || c == NULL) {
      return;
   }
   if (c->n == 0) {
      return;
   }

   for (int bi = 0; bi < c->n; bi++) {
      block *blk = &c->b[bi];
      for (int bit = 0; bit < MSKLEN; bit++) {
         mask_t m = (mask_t)1 << bit;
         if (blk->msk & m) {
            int pos = value_index(blk->msk, bit);
            func(&blk->vals[pos], ac);
         }
      }
   }
}

/*
   Remove the value corresponding to a bit from the block, 
   but keep the block.
*/
bool remove_value(block* blk, int bit) {
   if (blk == NULL) {
      return false;
   }

   mask_t bitmask = (mask_t)1 << bit;
   if (!(blk->msk & bitmask)) {
      return false;
   }

   int cnt = count1(blk->msk);
   int pos = value_index(blk->msk, bit);

   if (pos < cnt - 1) {
      memmove(&blk->vals[pos], &blk->vals[pos + 1],
              sizeof(int) * (size_t)(cnt - pos - 1));
   }

   if (cnt - 1 == 0) {
      free(blk->vals);
      blk->vals = NULL;
   } else {
      int *nv = 
        realloc(blk->vals, sizeof(int) * (size_t)(cnt - 1));
      if (nv != NULL) {
         blk->vals = nv;
      }
   }

   blk->msk &= ~bitmask;
   return true;
}

bool remove_block(csa* c, int bi) {
   if (c == NULL || bi < 0 || bi >= c->n) {
      return false;
   }

   free(c->b[bi].vals);
   c->b[bi].vals = NULL;

   if (bi < c->n - 1) {
      memmove(&c->b[bi], &c->b[bi + 1],
              sizeof(block) * (size_t)(c->n - bi - 1));
   }

   if (c->n - 1 == 0) {
      free(c->b);
      c->b = NULL; c->n = 0;
   } else {
      block *nb = 
         realloc(c->b, sizeof(block) * (size_t)(c->n - 1));
      if (nb != NULL) {
         c->b = nb;
      }
      c->n--;
   }
   return true;
}


bool csa_delete(csa* c, int indx){
   if (c == NULL || indx < 0) {
      return false;
   }

   unsigned int offset = (unsigned int)(indx/MSKLEN)*MSKLEN;
   int bit = indx % MSKLEN;

   int bi = block_index(c, offset);
   if (bi < 0) {
      return false;           
   }

   block *blk = &c->b[bi];
   mask_t bitmask = (mask_t)1 << bit;

   if (!(blk->msk & bitmask)) {
      return false;          
   }

   int cnt = count1(blk->msk);
   if (cnt == 1) {
     return remove_block(c, bi);
   }

   return remove_value(blk , bit);
}
#endif

