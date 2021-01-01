/* NNUE wrapping functions */

// include headers
#include <stdint.h>
#include <stdio.h>
#include "./nnue/nnue.h"
#include "nnue_eval.h"

extern const unsigned char _binary_toganet_bin_start[];

// init NNUE
void init_nnue(char *filename)
{
  // call NNUE probe lib function
  nnue_init(filename);
}

// init embedded NNUE
void init_nnue_embedded()
{
  init_weights(_binary_toganet_bin_start);
  
  printf("Embedded NNUE loaded !\n");
  fflush(stdout);
}

// get NNUE score directly
int evaluate_nnue(int player, int *pieces, int *squares)
{
  // call NNUE probe lib function
  nnue_evaluate(player, pieces, squares);
}

// det NNUE score from FEN input
int evaluate_fen_nnue(char *fen)
{
  // call NNUE probe lib function
  nnue_evaluate_fen(fen);
}
