#include "csa.h"
#include "mydefs.h"


int popcount_mask(mask_t msk)
{
   return __builtin_popcountll(msk);
}

int block_index(csa* c, unsigned int offset)
{
   if(c==NULL){
      return -1;
   }
   for(int i=0; i<c->n; i++){
      if(c->b[i].offset==offset){
         return i;
      }
      if(c->b[i].offset>offset){
         return -1;
      }
   }
   return -1;
}

int value_index(mask_t msk, int bit)
{
   mask_t lower = 0;
   if(bit > 0){
      lower = msk & (((mask_t)1 << bit) - 1);
   }
   return popcount_mask(lower);
}

block* insert_block(csa* c, unsigned int offset)
{
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
   memmove(&c->b[pos + 1], &c->b[pos], sizeof(block) * (c->n - pos));
   c->b[pos].offset = offset;
   c->b[pos].msk = 0;
   c->b[pos].vals = NULL;
   c->n++;
   return &c->b[pos];
}

csa* csa_init(void)
{
   csa* c = malloc(sizeof(csa));
   if(c==NULL){
      return NULL;
   }
   c->b = NULL;
   c->n = 0;
   return c;
}

bool csa_get(csa* c, int idx, int* val)
{
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

bool csa_set(csa* c, int idx, int val)
{
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
   int pos = value_index(blk->msk, bit);
   int cnt = popcount_mask(blk->msk);
   if(blk->msk & ((mask_t)1<<bit)){
      blk->vals[pos] = val;
      return true;
   }

   int* nv = realloc(blk->vals, sizeof(int)*(cnt+1));
   if(nv==NULL){
      return false;
   }
   blk->vals = nv;
   memmove(&blk->vals[pos+1], &blk->vals[pos], sizeof(int)*(cnt - pos));
   blk->vals[pos] = val;
   blk->msk |= ((mask_t)1<<bit);
   return true;
}

void csa_tostring(csa* c, char* s)
{
    if (s == NULL) return;
    char *p = s;
    size_t rem = 100000;
    int written;

    if (c == NULL) {
        *p = '\0';
        return;
    }

    if (c->n == 0) {
        snprintf(p, rem, "0 blocks");
        return;
    }

    written = snprintf(p, rem, "%d block%s ", c->n, c->n == 1 ? "" : "s");
    if (written < 0) return;
    p += written; rem -= (size_t)written;

    for (int i = 0; i < c->n; i++) {
        block *blk = &c->b[i];
        int cnt = popcount_mask(blk->msk);
        written = snprintf(p, rem, "{%d|", cnt);
        if (written < 0) return;
        p += written; rem -= (size_t)written;

        int seen = 0;
        for (int bit = 0; bit < MSKLEN && rem > 0; bit++) {
            mask_t m = (mask_t)1 << bit;
            if (blk->msk & m) {
                if (seen) {
                    written = snprintf(p, rem, ":");
                    if (written < 0) return;
                    p += written; rem -= (size_t)written;
                }
                written = snprintf(p, rem, "[%u]=%d", (unsigned)(blk->offset + bit),
                                   blk->vals[value_index(blk->msk, bit)]);
                if (written < 0) return;
                p += written; rem -= (size_t)written;
                seen++;
            }
        }

        written = snprintf(p, rem, "}");
        if (written < 0) return;
        p += written; rem -= (size_t)written;
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

   /* popcount */
   assert(popcount_mask(0ULL) == 0);
   assert(popcount_mask(1ULL) == 1);
   assert(popcount_mask(3ULL) == 2);

   /* value_index (mask with bits 2,5,6 set) */
   mask_t m = 0;
   m |= (1ULL << 2);
   m |= (1ULL << 5);
   m |= (1ULL << 6);
   assert(value_index(m, 2) == 0);
   assert(value_index(m, 5) == 1);
   assert(value_index(m, 6) == 2);

   /* insert_block / block_index / basic structure checks */
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

   /* try a basic set/get if implemented */
   int tmp = 0;
   if (csa_set(c, 2, 25)) {
      assert(csa_get(c, 2, &tmp));
      assert(tmp == 25);
   }

   /* cleanup */
   csa_free(&c);
   assert(c == NULL);

}

#ifdef EXT
void csa_foreach(void (*func)(int* p, int* ac), csa* c, int* ac)
{
}

bool csa_delete(csa* c, int indx)
{
}
#endif
