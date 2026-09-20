/*
 ************************************************************
 lzhuf.c
 written by Haruyasu Yoshizaki 11/20/1988
 some minor changes 4/6/1989
 comments translated by Haruhiko Okumura 4/7/1989
 Adapted to Jnos 1.10h by Jack Snodgrass, KF5MG, 12/19/94
 ************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef MSDOS
#include <alloc.h>
#endif

#include <time.h>
#include "global.h"

#if defined(LZHUF) || defined(FBBCMP)
#include "proc.h"
#include "socket.h"
#include "timer.h"
#include "usock.h"
#include "netuser.h"
#include "session.h"

#include "lzhuf.h"

#ifdef XMS
#include "xms.h"
#endif
#ifdef UNIX
#define printf tcmdprintf
#endif

#undef  DEBUG 1

/* see error codes defined in fbbfwd.c */
#define FBBerror(x,y) { tprintf("*** Protocole Error (%d)\n", x); log(y,"lzhuf: detected FBB protocol error %d", x); }

static char EarlyDisconnect[] = "lzhuf: unexpected disconnect";

/* return 1 if allocations succeeded, else
   return 0 after freeing partial allocations
*/
int AllocDataBuffers(struct fwd *f) {

   if ((f->lzhuf = calloc(sizeof(struct lzhufstruct),1)) == NULL) return 0;
   if ((f->tmpBuffer = malloc(260)) == NULL) {
       free (f->lzhuf);
       f->lzhuf = NULL;
       return 0;
   }

#ifdef LZHDEBUG
   if(Current->output == Command->output)
      printf("Getting %d bytes of storage.\n", sizeof(struct lzhufdata));
#endif

   f->lzhuf->data_type = 0;        // 0 means one big buffer (UMB or not)
#ifdef XMS
   f->lzhuf->data = (struct lzhufdata *)mallocUMB(sizeof(struct lzhufdata));
   if (f->lzhuf->data == NULL)  /* err, or not enough paras avail, so use malloc */
#endif
   f->lzhuf->data = (struct lzhufdata *)malloc(sizeof(struct lzhufdata));
   if(f->lzhuf->data == (struct lzhufdata *)NULL) {  /* can't get one big buffer, try in pieces */
      f->lzhuf->data_type = 1;        // 1 means small buffers + lower memory.
      f->lzhuf->dad       = malloc((N + 1) * sizeof(int));
      f->lzhuf->lson      = malloc((N + 1) * sizeof(int));
      f->lzhuf->rson      = malloc((N + 257) * sizeof(int));
      f->lzhuf->text_buf  = malloc((N + F - 1) * sizeof(unsigned char));
      f->lzhuf->freq      = malloc((T + 1) * sizeof(unsigned));
      f->lzhuf->prnt      = malloc((T + N_CHAR) * sizeof(int));
      f->lzhuf->son       = malloc((T) * sizeof(int));

      if (!f->lzhuf->dad || !f->lzhuf->lson || !f->lzhuf->rson ||
          !f->lzhuf->text_buf || !f->lzhuf->freq || !f->lzhuf->prnt ||
          !f->lzhuf->son) {  /* not enough mem for all allocations */
              FreeDataBuffers(f);
              return 0;
          }

   } else {
      // point pointers to correct spot in large buffer.
      f->lzhuf->dad       = f->lzhuf->data->dad;
      f->lzhuf->rson      = f->lzhuf->data->rson;
      f->lzhuf->lson      = f->lzhuf->data->lson;
      f->lzhuf->text_buf  = f->lzhuf->data->text_buf;
      f->lzhuf->freq      = f->lzhuf->data->freq;
      f->lzhuf->prnt      = f->lzhuf->data->prnt;
      f->lzhuf->son       = f->lzhuf->data->son;
   }
   return 1;    /* all OK */
}

void FreeDataBuffers(struct fwd *f) {

   free(f->tmpBuffer);

   if(f->lzhuf->data_type == 1) {
      // Free lower memory blocks.
      free(f->lzhuf->dad);
      free(f->lzhuf->lson);
      free(f->lzhuf->rson);
      free(f->lzhuf->text_buf);
      free(f->lzhuf->freq);
      free(f->lzhuf->prnt);
      free(f->lzhuf->son);
   } else
      free(f->lzhuf->data);
   free(f->lzhuf);
}

int  Encode    (int, char *, char *, struct lzhufstruct *);
int  Decode    (int, char *, char *, struct lzhufstruct *);
static int  GetBit    (struct lzhufstruct *);
#ifdef MSDOS
static int  GetByte   (struct lzhufstruct *);
#else
static unsigned short  GetByte   (struct lzhufstruct *);
#endif
static void Putcode   (struct lzhufstruct *, int, unsigned);
static void EncodeEnd (struct lzhufstruct *);
static int  DecodeChar(struct lzhufstruct *);
static int  recvbuf   (int,char *,int,int32);

/********** LZSS compression **********/

static void InitTree(struct lzhufstruct *lzhuf)  /* initialize trees */
{
   int  i;

   for (i = N + 1; i <= N + 256; i++)
       lzhuf->rson[i] = NIL;                  /* root */
   for (i = 0; i < N; i++)
       lzhuf->dad[i] = NIL;                   /* node */
}

static void InsertNode(struct lzhufstruct *lzhuf, int r)  /* insert to tree */
{
   int  i, p, cmp;
   unsigned char  *key;
   unsigned c;

   cmp = 1;
   key = &lzhuf->text_buf[r];
   p = N + 1 + key[0];
   lzhuf->rson[r] = lzhuf->lson[r] = NIL;
   lzhuf->match_length = 0;
   for(;;) {
      if(cmp >= 0) {
         if(lzhuf->rson[p] != NIL)
            p = lzhuf->rson[p];
         else {
            lzhuf->rson[p] = r;
            lzhuf->dad[r] = p;
            return;
         }
      } else {
         if(lzhuf->lson[p] != NIL)
            p = lzhuf->lson[p];
         else {
            lzhuf->lson[p] = r;
            lzhuf->dad[r] = p;
            return;
         }
      }
      for(i = 1; i < F; i++)
         if((cmp = key[i] - lzhuf->text_buf[p + i]) != 0)
            break;
      if(i > THRESHOLD) {
         if(i > lzhuf->match_length) {
            lzhuf->match_position = ((r - p) & (N - 1)) - 1;
            if((lzhuf->match_length = i) >= F)
               break;
         }
         if(i == lzhuf->match_length) {
            if((c = ((r - p) & (N - 1)) - 1) < lzhuf->match_position) {
               lzhuf->match_position = c;
            }
         }
      }
   }
   lzhuf->dad[r] = lzhuf->dad[p];
   lzhuf->lson[r] = lzhuf->lson[p];
   lzhuf->rson[r] = lzhuf->rson[p];
   lzhuf->dad[lzhuf->lson[p]] = r;
   lzhuf->dad[lzhuf->rson[p]] = r;
   if(lzhuf->rson[lzhuf->dad[p]] == p)
      lzhuf->rson[lzhuf->dad[p]] = r;
   else
      lzhuf->lson[lzhuf->dad[p]] = r;
   lzhuf->dad[p] = NIL;  /* remove p */
}

static void DeleteNode(struct lzhufstruct *lzhuf, int p)  /* remove from tree */
{
   int  q;

   if(lzhuf->dad[p] == NIL)
      return;                 /* not registered */
   if(lzhuf->rson[p] == NIL)
      q = lzhuf->lson[p];
   else
   if(lzhuf->lson[p] == NIL)
      q = lzhuf->rson[p];
   else {
      q = lzhuf->lson[p];
      if(lzhuf->rson[q] != NIL) {
         do {
            q = lzhuf->rson[q];
         } while (lzhuf->rson[q] != NIL);
         lzhuf->rson[lzhuf->dad[q]] = lzhuf->lson[q];
         lzhuf->dad[lzhuf->lson[q]] = lzhuf->dad[q];
         lzhuf->lson[q] = lzhuf->lson[p];
         lzhuf->dad[lzhuf->lson[p]] = q;
      }
      lzhuf->rson[q] = lzhuf->rson[p];
      lzhuf->dad[lzhuf->rson[p]] = q;
   }
   lzhuf->dad[q] = lzhuf->dad[p];
   if(lzhuf->rson[lzhuf->dad[p]] == p)
      lzhuf->rson[lzhuf->dad[p]] = q;
   else
      lzhuf->lson[lzhuf->dad[p]] = q;
   lzhuf->dad[p] = NIL;
}

/* Huffman coding */

/* table for encoding and decoding the upper 6 bits of position */

/* for encoding */
static uchar p_len[64] = {
        0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

static uchar p_code[64] = {
        0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
        0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
        0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
        0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
        0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
        0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
        0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
        0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

/* for decoding */
static uchar d_code[256] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
        0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
        0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
        0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D,
        0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
        0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11,
        0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
        0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15,
        0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
        0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B,
        0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
        0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23,
        0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
        0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B,
        0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};

static uchar d_len[256] = {
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
};

#ifdef MSDOS
static int GetBit(struct lzhufstruct *lzhuf)        /* get one bit */
{
   int i;

   while(lzhuf->getlen <= 8) {
      if((i = getc(lzhuf->iFile)) < 0) i = 0;
      lzhuf->getbuf |= i << (8 - lzhuf->getlen);
      lzhuf->getlen += 8;
   }
   i = lzhuf->getbuf;
   lzhuf->getbuf <<= 1;
   lzhuf->getlen--;
   return (i < 0);
}

static int GetByte(struct lzhufstruct *lzhuf)       /* get one byte */
{
   unsigned i;

   while(lzhuf->getlen <= 8) {
      if((i = getc(lzhuf->iFile)) == (unsigned)-1) i = 0;
      lzhuf->getbuf |= i << (8 - lzhuf->getlen);
      lzhuf->getlen += 8;
   }
   i = lzhuf->getbuf;
   lzhuf->getbuf <<= 8;
   lzhuf->getlen -= 8;
   return i >> 8;
}
#else	/* alternate from KO4KS, better for non-Borland compilers */
static int GetBit(struct lzhufstruct *lzhuf)        /* get one bit */
{
   register unsigned i;
   register unsigned dx = lzhuf->getbuf;
   register unsigned char glen = lzhuf->getlen;
   
   while(glen <= 8) {
      i = getc(lzhuf->iFile);
      if ((int)i < 0)
	   i = 0;
      dx |= i << (8 - glen);
      glen += 8;
   }
   lzhuf->getbuf = dx << 1;
   lzhuf->getlen = glen - 1;
   return (dx & 0x8000) ? 1 : 0;
}


static unsigned short GetByte(struct lzhufstruct *lzhuf)       /* get one byte */
{
   register unsigned i;
   register unsigned dx = lzhuf->getbuf;
   register unsigned char glen = lzhuf->getlen;

   while(glen <= 8) {
      i = getc(lzhuf->iFile);
      if ((int)i < 0)
	   i = 0;
      dx |= i << (8 - glen);
      glen += 8;
   }
   lzhuf->getbuf = dx << 8;
   lzhuf->getlen = glen - 8;
   return (dx >> 8) & 0xff;
}
#endif

static void Putcode(struct lzhufstruct *lzhuf, int l, unsigned c)         /* output c bits of code */
{
   lzhuf->putbuf |= c >> lzhuf->putlen;
   if((lzhuf->putlen += l) >= 8) {
      if(putc(lzhuf->putbuf >> 8, lzhuf->oFile) == EOF) {
         return;
      }
      if((lzhuf->putlen -= 8) >= 8) {
         if(putc(lzhuf->putbuf, lzhuf->oFile) == EOF) {
            return;
         }
         lzhuf->codesize += 2;
         lzhuf->putlen -= 8;
         lzhuf->putbuf = c << (l - lzhuf->putlen);
      } else {
         lzhuf->putbuf <<= 8;
         lzhuf->codesize++;
      }
   }
}


/* initialization of tree */

static void StartHuff(struct lzhufstruct *lzhuf)
{
   int i, j;

   for(i = 0; i < N_CHAR; i++) {
       lzhuf->freq[i] = 1;
       lzhuf->son[i] = i + T;
       lzhuf->prnt[i + T] = i;
   }
   i = 0; j = N_CHAR;
   while(j <= R) {
       lzhuf->freq[j] = lzhuf->freq[i] + lzhuf->freq[i + 1];
       lzhuf->son[j] = i;
       lzhuf->prnt[i] = lzhuf->prnt[i + 1] = j;
       i += 2; j++;
   }
   lzhuf->freq[T] = 0xffff;
   lzhuf->prnt[R] = 0;
}


/* reconstruction of tree */

static void reconst(struct lzhufstruct *lzhuf)
{
   int i, j, k;
   unsigned first, last;

   /* collect leaf nodes in the first half of the table */
   /* and replace the freq by (freq + 1) / 2. */
   j = 0;
   for(i = 0; i < T; i++) {
       if(lzhuf->son[i] >= T) {
          lzhuf->freq[j] = (lzhuf->freq[i] + 1) / 2;
          lzhuf->son[j] = lzhuf->son[i];
          j++;
       }
   }
   /* begin constructing tree by connecting sons */
   for(i = 0, j = N_CHAR; j < T; i += 2, j++) {
       k = i + 1;
       first = lzhuf->freq[j] = lzhuf->freq[i] + lzhuf->freq[k];
       for (k = j - 1; first < lzhuf->freq[k]; k--);
       k++;
       last = (j - k) * sizeof(unsigned int);  /* # BYTES to move right */
       memmove(&lzhuf->freq[k + 1], &lzhuf->freq[k], last);
       lzhuf->freq[k] = first;
       memmove(&lzhuf->son[k + 1], &lzhuf->son[k], last);
       lzhuf->son[k] = i;
   }
   /* connect prnt */
   for(i = 0; i < T; i++) {
       if((k = lzhuf->son[i]) >= T) {
          lzhuf->prnt[k] = i;
       } else {
          lzhuf->prnt[k] = lzhuf->prnt[k + 1] = i;
       }
   }
}


/* increment frequency of given code by one, and update tree */

static void update(struct lzhufstruct *lzhuf, int c)
{
   int i, j, k, l;

   if(lzhuf->freq[R] == MAX_FREQ) {
      reconst(lzhuf);
   }
   c = lzhuf->prnt[c + T];
   do {
      k = ++lzhuf->freq[c];

      /* if the order is disturbed, exchange nodes */
      if(k > lzhuf->freq[l = c + 1]) {
         while (k > lzhuf->freq[++l]);
         l--;
         lzhuf->freq[c] = lzhuf->freq[l];
         lzhuf->freq[l] = k;

         i = lzhuf->son[c];
         lzhuf->prnt[i] = l;
         if (i < T) lzhuf->prnt[i + 1] = l;

         j = lzhuf->son[l];
         lzhuf->son[l] = i;

         lzhuf->prnt[j] = c;
         if (j < T) lzhuf->prnt[j + 1] = c;
         lzhuf->son[c] = j;

         c = l;
      }
   } while ((c = lzhuf->prnt[c]) != 0);   /* repeat up to root */
}

static void EncodeChar(struct lzhufstruct *lzhuf, unsigned c)
{
   unsigned i;
   int j, k;

   i = 0;
   j = 0;
   k = lzhuf->prnt[c + T];

   /* travel from leaf to root */
   do {
      i >>= 1;

      /* if node's address is odd-numbered, choose bigger brother node */
      if (k & 1) i += 0x8000;

      j++;
   } while ((k = lzhuf->prnt[k]) != R);
   Putcode(lzhuf, j, i);
   lzhuf->code = i;
   lzhuf->len = j;
   update(lzhuf, c);
}

static void EncodePosition(struct lzhufstruct *lzhuf, unsigned c)
{
   unsigned i;

   /* output upper 6 bits by table lookup */
   i = c >> 6;
   Putcode(lzhuf, p_len[i], (unsigned)p_code[i] << 8);

   /* output lower 6 bits verbatim */
   Putcode(lzhuf, 6, (c & 0x3f) << 10);
}

static void EncodeEnd(struct lzhufstruct *lzhuf)
{
   if(lzhuf->putlen) {
      if(putc(lzhuf->putbuf >> 8, lzhuf->oFile) == EOF) {
         return;
      }
      lzhuf->codesize++;
   }
}

static int DecodeChar(struct lzhufstruct *lzhuf)
{
   unsigned c;

   c = lzhuf->son[R];

   /* travel from root to leaf, */
   /* choosing the smaller child node (son[]) if the read bit is 0, */
   /* the bigger (son[]+1} if 1 */
   while (c < T) {
       c += GetBit(lzhuf);
       c = lzhuf->son[c];
   }
   c -= T;
   update(lzhuf, c);
   return c;
}

static int DecodePosition(struct lzhufstruct *lzhuf)
{
   unsigned i, j, c;

   /* recover upper 6 bits from table */
   i = GetByte(lzhuf);
   c = (unsigned)d_code[i] << 6;
   j = d_len[i];

   /* read lower 6 bits verbatim */
   j -= 2;
   while (j--) {
      i = (i << 1) + GetBit(lzhuf);
   }
   return c | (i & 0x3f);
}

/* compression */

/* Now returns 0 if encodes OK, else errno or -1 for failure to encode */
int Encode(int usock, char *iFile, char *oFile, struct lzhufstruct *lzhuf)
{
   int  i, c, len, r, s, last_match_length;

   unsigned long int  filesize   = 0;

#ifdef LZHDEBUG
   printf("Encoding %s into %s\n", iFile, oFile);
#endif

   // Open input and output files.
   if ( ((lzhuf->iFile = fopen(iFile, "rb")) == NULLFILE)
   || ((lzhuf->oFile = fopen(oFile, "wb")) == NULLFILE) )
       goto errxit;

   fseek(lzhuf->iFile, 0L, 2);
   if ((filesize = ftell(lzhuf->iFile)) == 0)
      goto errxit;
   /* output size of text */
   if(fwrite(&filesize, sizeof(filesize), 1, lzhuf->oFile) < 1) {
errxit:
      i=errno;
      if (!i) i--;  /* some non-zero value */
      fclose(lzhuf->iFile);
      fclose(lzhuf->oFile);
      return i;
   }
   rewind(lzhuf->iFile);

   lzhuf->iFileSize = filesize;
   filesize = 0;                   /* rewind and re-read */
   StartHuff(lzhuf);
   InitTree(lzhuf);
   s = 0;
   r = N - F;
   for(i = s; i < r; i++)
       lzhuf->text_buf[i] = ' ';
   for(len = 0; len < F && (c = getc(lzhuf->iFile)) != EOF; len++)
       lzhuf->text_buf[r + len] = c;
   filesize = len;
   for(i = 1; i <= F; i++)
       InsertNode(lzhuf, r - i);
   InsertNode(lzhuf, r);
   do {
      pwait(NULL);
      if(lzhuf->match_length > len)
         lzhuf->match_length = len;
      if(lzhuf->match_length <= THRESHOLD) {
         lzhuf->match_length = 1;
         EncodeChar(lzhuf,lzhuf->text_buf[r]);
      } else {
         EncodeChar(lzhuf,255 - THRESHOLD + lzhuf->match_length);
         EncodePosition(lzhuf,lzhuf->match_position);
      }
      last_match_length = lzhuf->match_length;
      for(i = 0; i < last_match_length && (c = getc(lzhuf->iFile)) != EOF; i++) {
         DeleteNode(lzhuf, s);
         lzhuf->text_buf[s] = c;
         if(s < F - 1)
            lzhuf->text_buf[s + N] = c;
         s = (s + 1) & (N - 1);
         r = (r + 1) & (N - 1);
         InsertNode(lzhuf, r);
      }
      while(i++ < last_match_length) {
         DeleteNode(lzhuf, s);
         s = (s + 1) & (N - 1);
         r = (r + 1) & (N - 1);
         if (--len) InsertNode(lzhuf, r);
      }
   } while (len > 0);
   EncodeEnd(lzhuf);
   fclose(lzhuf->iFile);
   fclose(lzhuf->oFile);

#ifdef LZHSTAT
   if(lzhuf->iFileSize == 0)
      lzhuf->iFileSize  = 1;
   if(Current->output == Command->output)
      printf("lzhuf Compress: %ld/%ld = %ld%%\n",
              lzhuf->codesize, lzhuf->iFileSize,
             (lzhuf->iFileSize - lzhuf->codesize) * 100L / lzhuf->iFileSize);
#endif /* LZHSTAT */
   return 0;
}

/* Now returns 0 if decodes OK, else errno or -1 if decode fails */
int Decode(int usock, char* iFile, char* oFile, struct lzhufstruct *lzhuf)
{
   int  i = 0;
   int  j = 0;
   int  k = 0;
   int  r = 0;
   int  c = 0;

   unsigned long int  count      = 0;
            long int  filesize   = 0;

#ifdef LZHDEBUG
   printf("Decoding %s into %s\n", iFile, oFile);
#endif

   // Open input and output files.
   if ( ((lzhuf->iFile = fopen(iFile, "rb")) == NULLFILE)
   || ((lzhuf->oFile = fopen(oFile, "wb")) == NULLFILE) )
       goto errxit;

   fseek(lzhuf->iFile, 0L, 2);
   if ((filesize = ftell(lzhuf->iFile)) == 0)
      goto errxit;
   lzhuf->iFileSize = filesize;
   rewind(lzhuf->iFile);

   if((fread(&filesize, sizeof(filesize), 1, lzhuf->iFile) < 1)
   || (filesize == 0)) {
errxit:
      i=errno;
      if (!i) i--;  /* some non-zero value */
      fclose(lzhuf->iFile);
      fclose(lzhuf->oFile);
      return i;
   }

   StartHuff(lzhuf);
   for(i = 0; i < N - F; i++)
      lzhuf->text_buf[i] = ' ';

   r = N - F;
   for(count = 0; count < filesize; ) {
      pwait(NULL);
      c = DecodeChar(lzhuf);
      if(c < 256) {
         if(putc(c, lzhuf->oFile) == EOF) {
            goto errxit;
         }
         lzhuf->text_buf[r++] = c;
         r &= (N - 1);
         count++;
      } else {
         i = (r - DecodePosition(lzhuf) - 1) & (N - 1);
         j = c - 255 + THRESHOLD;
         for(k = 0; k < j; k++) {
            c = lzhuf->text_buf[(i + k) & (N - 1)];
            if(putc(c, lzhuf->oFile) == EOF) {
               goto errxit;
            }
            lzhuf->text_buf[r++] = c;
            r &= (N - 1);
            count++;
         }
      }
   }

   fclose(lzhuf->iFile);
   fclose(lzhuf->oFile);

#ifdef LZHSTAT
   if(count == 0)
      count  = 1;
   if(Current->output == Command->output)
      printf("lzhuf uncompress: %ld/%ld = %ld%%\n",
             lzhuf->iFileSize, count,
             (count - lzhuf->iFileSize) * 100L / count);
#endif
   return 0;
}

/* Compress an input file, and write it to the output socket.
  Return 0 if error (and write note to the log),
  return 1 if all OK.
*/
int send_yapp(int usock, struct fwd *f, char *subj) {
   #define SLEN 79                           // Maximum subject length
   int  oldmode;                             // Socket Mode holder.
   FILE *oFile;
#ifdef LZHDEBUG
   FILE *debug;
#endif

   char *buffer;                             // buffer data.
   int  buffer_len;                          // buffer Length.

   int  x;                                   // misc counter.
   int  cnt;                                 // misc counter.
   int  rc;

/*   short b_checksum;                         // buffer checksum.*/
   short f_checksum;                         // file checksum.


#ifdef LZHDEBUG
    if (((debug  = fopen(tmpnam(NULL),"wb")) == NULLFILE)) {
        printf("Error opening input file.\n");
        return 0;
    }
#endif

   // Encode code.
      f->lzhuf->codesize = 0;
      f->lzhuf->getbuf   = 0;
      f->lzhuf->getlen   = 0;
      f->lzhuf->putbuf   = 0;
      f->lzhuf->putlen   = 0;
      f->lzhuf->code     = 0;
      f->lzhuf->len      = 0;

      rc = Encode(usock, f->iFile, f->oFile, f->lzhuf);
      if(rc) {
         log(usock, "lzhuf: Encode() error %d",rc);
#ifdef LZHDEBUG
         fclose(debug);
#endif
         return 0;
      }

      // Open the compressed data file.
      // We're going to read from the file and close it when we exit.
      oFile = fopen(f->oFile, "rb");

#ifdef LZHDEBUG
         printf("we opended %s for input.\n",  f->oFile);
#endif

      // Grab some space. Largest YAPP packet is 250+ bytes.
      buffer    = f->tmpBuffer;

      // Set the socket to Binary mode since we'll be sending Binary data.
      oldmode = sockmode(usock,SOCK_BINARY);

   // Send the subject buffer
      // The buffer is setup as follows:
      // Pos Data
      //   1 SOH
      //   2 Length of entire buffer ( 5 bytes + strlen(subject) )
      //   3 Null terminated Subject string.
      //   x Null terminated '0'-offset string.

      // Make sure that the subject strlen() is equal to or less than SLEN
      x = strlen(subj);
      if (x > SLEN) {
         x = SLEN;
         subj[SLEN] = '\0';
      }

      // length of subject + NULL + length of "     0" + NULL
      buffer_len = x + 1 + 6 + 1;

      // Build the buffer.
      buffer[0] = SOH;                       // buffer_Type
      buffer[1] = buffer_len;                // buffer_Len
      strcpy(&buffer[2], subj);              // Subject info.
      strcpy(&buffer[x+3], "     0");        // Always 0 for FBB Messages.

      // Now we can send it.
      // buffer_len + 2 ( for the first two bytes.
      send(usock, buffer, buffer_len+2, 0);
#ifdef LZHDEBUG
          fwrite(buffer, buffer_len+2, 1, debug);
#endif

   // Send the data buffers.
      f_checksum = 0;
      // fill buffer with data. Bytes 0 and 1 are reserved.
      while ((x = fread(&buffer[2], 1, 250, oFile)) > 0) {
         // prepare the buffer.
         buffer[0]  = STX;                         // buffer_Type
         buffer[1]  = x;                           // buffer_Len
/*         b_checksum = 0;*/
         for (cnt=0;cnt<x;cnt++) {
             f_checksum += buffer[2 + cnt];        // file checksum.
         }
/*         buffer[x + 2] = ((-b_checksum) & 0xff);   // Store b_checksum.*/

         // and send it.
         send(usock, buffer, (x + 2), 0);
#ifdef LZHDEBUG
         fwrite(buffer, x + 2, 1, debug);
#endif

/*         f_checksum += b_checksum;*/
      } /* endwhile */

   // Send the EOT
      // Prepare the buffer.
      buffer[0] = EOT;                             // buffer_Type
      buffer[1] = ((-f_checksum) & 0xff);          // Checksum.

      // and send it.
      send(usock, buffer, 2, 0);
#ifdef LZHDEBUG
      fwrite(buffer, 2, 1, debug);
#endif

      // Terminate.
      fclose(oFile);
#ifdef LZHDEBUG
      fclose(debug);
#endif

      // Set the socket back to it's original mode.
      sockmode(usock,oldmode);
      return 1;
}


/* read a FBB-compressed file from a socket, and uncompress into a file.
   Return 0 if error (and write note to the log),
   Return 1 if all OK.
*/
int recv_yapp(int usock, struct fwd *f, char **pzSubject, int32 Timeoutms) {
   int  recvcnt;
   FILE *iFile=NULLFILE;

   int  packet_type;
   int  packet_size;
   char packet_data[256];
   int  GetSubject;
   int  NoteDone;
   int  NoteError;
   int  checksumctr;
   int  checksum;
   int  rc;
   int  oldmode;                             // Socket Mode holder.

   // Set the socket to Binary mode since we'll be receiving Binary data.
      oldmode = sockmode(usock,SOCK_BINARY);

      GetSubject = TRUE;
      NoteDone   = FALSE;
      NoteError  = FALSE;
      checksum   = 0;

      while(!NoteDone) {
         // Get the data packets.
         alarm(Timeoutms);
         if ((packet_type = recvchar(usock)) == -1) {
ReadTimeout:
             log(usock, EarlyDisconnect);
             NoteError = TRUE;
             break;
         }
         alarm(0);

         if (GetSubject) {
             if (packet_type != SOH) {
                 FBBerror(0,usock);
                 NoteError = TRUE;
                 break;
             }
         }
         else
         if ((packet_type != STX) && (packet_type != EOT))  {
             FBBerror(3,usock);
             NoteError = TRUE;
             break;
         }

         // Get the packet size.
         alarm(Timeoutms);
         if ((packet_size = recvchar(usock)) == -1)
             goto ReadTimeout;
         alarm(0);
         if (!packet_size) packet_size=256;   /* 0x00 always means 256 */

         if (packet_type == SOH) {
            // This is the subject. Reset the flag so we don't
            // come here again.
            GetSubject = FALSE;

            // Open the output file.
            iFile = fopen(f->iFile, "wb");

            // This is a subject packet.
            recvcnt = recvbuf(usock, (char *)&packet_data, packet_size, Timeoutms);
            if(recvcnt == -1) {
               log(usock, EarlyDisconnect);
               // We've lost the connection ... close the data file.
               fclose(iFile);
               NoteDone  = TRUE;
               NoteError = TRUE;
               *pzSubject = NULLCHAR;
            } else
               *pzSubject = strdup(packet_data);  /* note this loses the offset digits */
         } /* end if */
         else
         if (packet_type == STX) {
            // Validate the packet
            // and write it to the file.
            recvcnt = recvbuf(usock, (char *)&packet_data, packet_size, Timeoutms);
            if(recvcnt == -1) {
               log(usock, EarlyDisconnect);
               // We've lost the connection....
               NoteDone  = TRUE;
               NoteError = TRUE;
            } else {
               // Write to disk
               fwrite(packet_data, (unsigned)recvcnt, 1, iFile);

               // add the data to the checksum count.
               for(checksumctr=0;checksumctr<packet_size;checksumctr++) {
                  checksum += packet_data[checksumctr];
               }
            }
         } /* end if */
         else
         if (packet_type == EOT) {
            if (((-checksum) & 0xff) != (packet_size & 0xff)) {
                 FBBerror(1,usock);
                 NoteError = TRUE;
                 /*break;  NoteDone is set next so we'll exit loop anyhow */
            }
            // We're done with this message.  Exit while loop and process it.
            NoteDone  = TRUE;
         } /* end if */
      } // End while !NoteDone

      if(iFile)
          fclose(iFile);   // Close the data file.

      if(!NoteError) {
         f->lzhuf->codesize = 0;
         f->lzhuf->getbuf   = 0;
         f->lzhuf->getlen   = 0;
         f->lzhuf->putbuf   = 0;
         f->lzhuf->putlen   = 0;
         f->lzhuf->code     = 0;
         f->lzhuf->len      = 0;

         rc = Decode(usock, f->iFile, f->oFile, f->lzhuf);
         if(rc)
            log(usock, "lzhuf: Decode() error %d",rc), ++NoteError;
      }

      // Set the socket back to it's orginal mode.
      sockmode(usock,oldmode);

      if(!NoteError)
         return 1;
      else
         return 0;
}

/* Receive a buffer from a socket, returning # chars read (or -1 if EOF/disconnect)
 */
static int
recvbuf(int s, char *buf, int len, int32 timeoutms) {
    int c;
    int cnt = 0;

    alarm(timeoutms);   /* assume long enough to read <len> bytes */
    while(len-- > 0){
        if((c = recvchar(s)) == EOF){
            cnt = -1;
            break;
        }
        if(buf != NULLCHAR)
            *buf++ = c;
        cnt++;
    }
    alarm(0);
    return cnt;
}

#ifdef LZHTEST
main(argc, argv, envp)
   int argc;
   char *argv[];
   char *envp[];
{
   if (argv[1] == 'd') {
      Decode(argv[2], argv[3]);
   } else {
      Encode(argv[2], argv[3]);
   } /* endif */
}
#endif /* LZHTEST */
#endif /* defined(LZHUF) || defined(FBBCMP) */
