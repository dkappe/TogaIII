
// main.cpp

// includes

#include <cstdio>
#include <cstdlib>

#include "attack.h"
#include "book.h"
#include "hash.h"
#include "move_do.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "protocol.h"
#include "random.h"
#include "square.h"
#include "trans.h"
#include "util.h"
#include "value.h"
#include "vector.h"
#include "nnue_eval.h"

// functions

// main()

int main(int argc, char * argv[]) {

   // init

   util_init();
   my_random_init(); // for opening book

   printf("Toga III 0.1 UCI based on Fruit 2.1 by Jerry Donald Watson, Thomas Gaksch and Fabien Letouzey. \n");
   //printf("Code was based on Toga II 1.4.1SE by Thomas Gaksch (modified by Chris Formula), with bugfixes by Michel van den Bergh\n");

   // early initialisation (the rest is done after UCI options are parsed in protocol.cpp)

   option_init();

   square_init();
   piece_init();
   pawn_init_bit();
   value_init();
   vector_init();
   attack_init();
   move_do_init();

   random_init();
   
   trans_init(Trans);
   hash_init();
   
   book_init();

   init_nnue_embedded();

   // loop

   loop();

   return EXIT_SUCCESS;
}

// end of main.cpp

