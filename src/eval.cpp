
// eval.cpp

// includes

#include <cstdlib> // for abs()

#include "attack.h"
#include "board.h"
#include "colour.h"
#include "eval.h"
#include "material.h"
#include "move.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "see.h"
#include "util.h"
#include "value.h"
#include "vector.h"
//#include "probe.h"
#include "search.h"
#include "nnue_eval.h"
#include <stdio.h>

// macros

#define THROUGH(piece) ((piece)==Empty)
#define ABS(x) ((x)<0?-(x):(x))

// constants and variables

const int KnightOutpostMatrix[2][64] = {
     0,	 0,   0,   0,   0,   0,   0,   0,
     0,	 0,   0,   0,   0,   0,   0,   0,
     0,	 0,   0,   0,   0,   0,   0,   0,
     0,	 2,   5,  10,  10,   5 ,  2,   0,
     0,	 2,   5,  10,  10,   5,   2,   0,
     0,	 0,   4,   5,   5,   4,   0,   0,
     0,	 0,   0,   0,   0,   0,   0,   0,
     0,	 0,   0,   0,   0,   0,   0,   0,

     0,	 0,   0,   0,   0,   0,   0,   0,
     0,	 0,   0,   0,   0,   0,   0,   0,
     0,	 0,   4,   5,   5,   4,   0,   0,
     0,	 2,   5,  10,  10,   5,   2,   0,
     0,	 2,   5,  10,  10,   5,   2,   0,
     0,	 0,   0,   0,   0,   0,   0,   0,
     0,	 0,   0,   0,   0,   0,   0,   0,
     0,	 0,   0,   0,   0,   0,   0,   0,
};

static int lazy_eval_cutoff = 200; /* Thomas */
static int second_lazy_eval_cutoff = 125; /* JD */
static bool LazyEval = false; // true
//static bool KingSafety = false; // true
//static int KingSafetyMargin = 1600;
//static bool king_is_safe[ColourNb];

static /* const */ int PieceActivityWeight = 256; // 100%
static /* const */ int KingSafetyWeight = 256; // 100%
static /* const */ int PassedPawnWeight = 256; // 100%

static const int KnightUnit = 4;
static const int BishopUnit = 4; // only forwards
static const int RookUnit = 7;
//static const int QueenUnit = 13;

static const int PawnAttack = 25;

static const int MobMove = 1;
static const int MobAttack = 1;
static const int MobDefense = 0;
static const int MobPieceDefense = 1;

static const bool UseMobAttack = true; // JD; from Fruit 3.2.1
static const int MobPawnAttack = 1; 
static const int MobMinorAttack = 6;
static const int MobRookAttack = 10;
static const int MobQueenAttack = 15;
static const int MobKingAttack = 30;

// Ryan's speedup of eval_piece().  Posted open source in GambitFruit.  
// Bishop mobility modified by Jerry Donald
static const int knight_mob_opening[ 8 + 1] = {-16, -12,  -8,  -4,   0,   4,   8,  12,  16};                         // WHM; slope == KinghtMobOpening == 4
static const int knight_mob_endgame[ 8 + 1] = {-16, -12,  -8,  -4,   0,   4,   8,  12,  16};                         // WHM; slope == KinghtMobEndgame == 4
static const int bishop_mob_opening[13 + 1] = {-30, -25, -20, -15, -10,  -5,   0,   5,  10, 15, 20, 25, 30, 35};     // WHM; slope == BishopMobOpening == 5
static const int bishop_mob_endgame[13 + 1] = {-30, -25, -20, -15, -10,  -5,   0,   5,  10, 15, 20, 25, 30, 35};     // WHM; slope == BishopMobEndgame == 5
static const int   rook_mob_opening[14 + 1] = {-14, -12, -10,  -8,  -6,  -4,  -2,   0,   2,  4,  6,  8, 10, 12, 14}; // WHM; slope == RookMobOpening == 2
static const int   rook_mob_endgame[14 + 1] = {-28, -24, -20, -16, -12,  -8,  -4,   0,   4,  8, 12, 16, 20, 24, 28}; // WHM; slope == RookMobEndgame == 4
static const int   queen_mob_opening[27 + 1] = {-5,  -4,  -3,  -2,  -1,  +0,  +1,  +2,  +3,  +4,  +5,  +6,  +7,  +8,  +9, +10, +11, +12, +13, +14, +15, +16, +17, +18, +19, +20, +21, +22}; // WHM; slope == RookMobOpening == 1
static const int   queen_mob_endgame[27 + 1] = {-24, -17, -11,  -7,  -3,  +0,  +2,  +4,  +6,  +8, +10, +12, +14, +16, +18, +20, +22, +24, +26, +28, +30, +32, +34, +36, +38, +40, +42, +44,}; // WHM; slope == RookMobEndgame == 4

static const bool UseOpenFile = true;
static const int RookSemiOpenFileOpening = 10;
static const int RookSemiOpenFileEndgame = 10;
static const int RookOpenFileOpening = 20;
static const int RookOpenFileEndgame = 20;
static const int RookSemiKingFileOpening = 10;
static const int RookKingFileOpening = 20;

static const bool UseKingAttack = true;
static const int KingAttackOpening = 20; 

static const bool UseShelter = true;
static const int ShelterOpening = 256; // 100%
static const bool UseStorm = true;
static const int StormOpening = 10;

static const int Rook7thOpening = 20;
static const int Rook7thEndgame = 40;
//static const int Queen7thOpening = 10;
static const int Queen7thEndgame = 20;

static const int TrappedBishop = 100;

static const int BlockedBishop = 50;
static const int BlockedRook = 50;

static const int ProtectedPassedOpening = 10;
static const int ProtectedPassedEndgame = 20;

static const int PassedOpeningMin = 10;
static const int PassedOpeningMax = 70;
static const int PassedEndgameMin = 20;
static const int PassedEndgameMax = 140;

static const int UnstoppablePasser = 800;
static const int FreePasser = 60;

static const int AttackerDistance = 5;
static const int DefenderDistance = 20;

// "constants"

// pawn shelter penalties

static const int PawnShelterFront[7] = {   
   72, 70, 64, 54, 40, 22, 0, // rank 8 (or no pawn) to rank 2 (0 penalty)
};

static const int PawnShelterOpenSide[7] = {
   36, 35, 32, 26, 18, 14, 0, 
};

static const int PawnShelterClosedSide[7] = {
   36, 35, 32, 27, 19, 9, 0, 
};

// weights

static const int KingAttackWeight[16] = {
   0, 0, 128, 192, 224, 240, 248, 252, 254, 255, 256, 256 ,256, 256, 256, 256,
};

static const int SSpaceWeight[16] = {
   0, 2, 5, 8, 12, 15, 20, 20, 20, 20, 20, 20 ,20, 20, 20, 20,
};

// types

static const int TypeShelterFront = 1;
static const int TypeShelterOpen = 0;
static const int TypeShelterClosed = -1;

// variables

static int MobUnit[ColourNb][PieceNb];
static int AttackUnit[ColourNb][PieceNb][PieceNb];
static int KingAttackUnit[PieceNb];

// prototypes

static void eval_draw          (const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int mul[2]);

static void eval_piece         (const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int * opening, int * endgame);
static void eval_king          (const board_t * board, const material_info_t * mat_info, int * opening, int * endgame);
static void eval_passer        (const board_t * board, const pawn_info_t * pawn_info, int * opening, int * endgame);
static void eval_pattern       (const board_t * board, int * opening, int * endgame);

static bool unstoppable_passer (const board_t * board, int pawn, int colour);
static bool king_passer        (const board_t * board, int pawn, int colour);
static bool free_passer        (const board_t * board, int pawn, int colour);

static int  pawn_att_dist      (int pawn, int king, int colour);
static int  pawn_def_dist      (int pawn, int king, int colour);

static void draw_init_list     (int list[], const board_t * board, int pawn_colour);

static bool draw_kpkq          (const int list[], int turn);
static bool draw_kpkr          (const int list[], int turn);
static bool draw_kpkb          (const int list[], int turn);
static bool draw_kpkn          (const int list[], int turn);

static bool draw_knpk          (const int list[], int turn);

static bool draw_krpkr         (const int list[], int turn);
static bool draw_kbpkb         (const int list[], int turn);

static int  shelter_square     (const board_t * board, const material_info_t * mat_info, int square, int colour);
static int  shelter_file       (const board_t * board, int file, int rank, int colour, int type);

static int  storm_file         (const board_t * board, int file, int colour);

static bool bishop_can_attack  (const board_t * board, int to, int colour);

// functions

void eval_parameter() {

    // UCI options

   PieceActivityWeight = (option_get_int("Piece Activity") * 256 + 50) / 100;
   KingSafetyWeight    = (option_get_int("King Safety")    * 256 + 50) / 100;
   PassedPawnWeight    = (option_get_int("Passed Pawns")   * 256 + 50) / 100;

   LazyEval = option_get_bool("Toga Lazy Eval"); /* Thomas */
   lazy_eval_cutoff = option_get_int("Toga Lazy Eval Margin");
   second_lazy_eval_cutoff = option_get_int("Toga Lazy Eval Mobility Margin");
}

// eval_init()

void eval_init() {

   int colour;
   int piece, piece_attack;

   // UCI options

   eval_parameter();

    // mobility table

   for (colour = 0; colour < ColourNb; colour++) {
      for (piece = 0; piece < PieceNb; piece++) {
      	    MobUnit[colour][piece] = 0;
      		for (piece_attack = 0; piece_attack < PieceNb; piece_attack++) {
         		AttackUnit[colour][piece][piece_attack] = 0;
     		}
      }
   }
   
   MobUnit[White][Empty] = MobMove;

   MobUnit[White][BP] = MobAttack;
   MobUnit[White][BN] = MobAttack;
   MobUnit[White][BB] = MobAttack;
   MobUnit[White][BR] = MobAttack;
   MobUnit[White][BQ] = MobAttack;
   MobUnit[White][BK] = MobAttack;

   MobUnit[White][WP] = MobDefense;
   MobUnit[White][WN] = MobPieceDefense;
   MobUnit[White][WB] = MobPieceDefense;
   MobUnit[White][WR] = MobPieceDefense;
   MobUnit[White][WQ] = MobPieceDefense;
   MobUnit[White][WK] = MobDefense;

   MobUnit[Black][Empty] = MobMove;

   MobUnit[Black][WP] = MobAttack;
   MobUnit[Black][WN] = MobAttack;
   MobUnit[Black][WB] = MobAttack;
   MobUnit[Black][WR] = MobAttack;
   MobUnit[Black][WQ] = MobAttack;
   MobUnit[Black][WK] = MobAttack;

   MobUnit[Black][BP] = MobDefense;
   MobUnit[Black][BN] = MobPieceDefense;
   MobUnit[Black][BB] = MobPieceDefense;
   MobUnit[Black][BR] = MobPieceDefense;
   MobUnit[Black][BQ] = MobPieceDefense;
   MobUnit[Black][BK] = MobDefense;
   
   // Attack Units
   // Knights

   AttackUnit[White][WN][BP] = MobPawnAttack;
   AttackUnit[White][WN][BN] = MobMinorAttack;
   AttackUnit[White][WN][BB] = MobMinorAttack;
   AttackUnit[White][WN][BR] = MobRookAttack;
   AttackUnit[White][WN][BQ] = MobQueenAttack;
   AttackUnit[White][WN][BK] = MobKingAttack;

   AttackUnit[Black][BN][WP] = MobPawnAttack;
   AttackUnit[Black][BN][WN] = MobMinorAttack;
   AttackUnit[Black][BN][WB] = MobMinorAttack;
   AttackUnit[Black][BN][WR] = MobRookAttack;
   AttackUnit[Black][BN][WQ] = MobQueenAttack;
   AttackUnit[Black][BN][WK] = MobKingAttack;

   // Bishops

   AttackUnit[White][WB][BP] = MobPawnAttack;
   AttackUnit[White][WB][BN] = MobMinorAttack;
   AttackUnit[White][WB][BB] = MobMinorAttack;
   AttackUnit[White][WB][BR] = MobRookAttack;
   AttackUnit[White][WB][BQ] = MobQueenAttack;
   AttackUnit[White][WB][BK] = MobKingAttack;

   AttackUnit[Black][BB][WP] = MobPawnAttack;
   AttackUnit[Black][BB][WN] = MobMinorAttack;
   AttackUnit[Black][BB][WB] = MobMinorAttack;
   AttackUnit[Black][BB][WR] = MobRookAttack;
   AttackUnit[Black][BB][WQ] = MobQueenAttack;
   AttackUnit[Black][BB][WK] = MobKingAttack;
   
   // Rooks

   AttackUnit[White][WR][BP] = MobPawnAttack;
   AttackUnit[White][WR][BN] = MobMinorAttack;
   AttackUnit[White][WR][BB] = MobMinorAttack;
   AttackUnit[White][WR][BR] = MobRookAttack;
   AttackUnit[White][WR][BQ] = MobQueenAttack;
   AttackUnit[White][WR][BK] = MobKingAttack;

   AttackUnit[Black][BR][WP] = MobPawnAttack;
   AttackUnit[Black][BR][WN] = MobMinorAttack;
   AttackUnit[Black][BR][WB] = MobMinorAttack;
   AttackUnit[Black][BR][WR] = MobRookAttack;
   AttackUnit[Black][BR][WQ] = MobQueenAttack;
   AttackUnit[Black][BR][WK] = MobKingAttack;
   
   // Queens

   AttackUnit[White][WQ][BP] = MobPawnAttack;
   AttackUnit[White][WQ][BN] = MobMinorAttack;
   AttackUnit[White][WQ][BB] = MobMinorAttack/2;
   AttackUnit[White][WQ][BR] = MobRookAttack/2;
   AttackUnit[White][WQ][BQ] = MobQueenAttack;
   AttackUnit[White][WQ][BK] = MobKingAttack;

   AttackUnit[Black][BQ][WP] = MobPawnAttack;
   AttackUnit[Black][BQ][WN] = MobMinorAttack;
   AttackUnit[Black][BQ][WB] = MobMinorAttack/2;
   AttackUnit[Black][BQ][WR] = MobRookAttack/2;
   AttackUnit[Black][BQ][WQ] = MobQueenAttack;
   AttackUnit[Black][BQ][WK] = MobKingAttack;

   // KingAttackUnit[]

   for (piece = 0; piece < PieceNb; piece++) {
      KingAttackUnit[piece] = 0;
   }

   KingAttackUnit[WN] = 1;
   KingAttackUnit[WB] = 1;
   KingAttackUnit[WR] = 2;
   KingAttackUnit[WQ] = 4;

   KingAttackUnit[BN] = 1;
   KingAttackUnit[BB] = 1;
   KingAttackUnit[BR] = 2;
   KingAttackUnit[BQ] = 4;
}

// eval_nnue()

// Stockfish NNUE piece encoding
int nnue_pieces[13] = {6, 12, 5, 11, 4, 10, 3, 9, 2, 8, 1, 7, 0};

int eval_nnue(board_t * board) {
  // NNUE probe arrays
  int pieces[33];
  int squares[33];

  // NNUE probe arrays index
  int index = 2;

  // fill nnue probe arrays
  int sq, piece, colour;
  int size, pos;

  int eval =0;
  bool is_white;
  
  for (colour = 0; colour < ColourNb; colour++) {
    is_white = COLOUR_IS_WHITE(colour);
    
    // piece list

    size = board->piece_size[colour];
    for (pos = 0; pos < size; pos++) {
      sq = board->piece[colour][pos];
      piece = PIECE_TO_12(board->square[sq]);
      if (piece == WhiteKing12) {
	pieces[0] = nnue_pieces[piece];
	squares[0] = SQUARE_TO_64(sq);
	//printf("*white king = %d => %d\n", piece, nnue_pieces[piece]);
      } else if (piece == BlackKing12) {
	pieces[1] = nnue_pieces[piece];
	squares[1] = SQUARE_TO_64(sq);
	//printf("*black king = %d => %d\n", piece, nnue_pieces[piece]);
      } else {
	pieces[index] = nnue_pieces[piece];
	squares[index] = SQUARE_TO_64(sq);
	index++;
      }
    }
    // now deal with the pawns
    size = board->pawn_size[colour];
    for (pos = 0; pos < size; pos++) {
      sq = board->pawn[colour][pos];
      piece = PIECE_TO_12(board->square[sq]);
      pieces[index] = nnue_pieces[piece];
      squares[index] = SQUARE_TO_64(sq);
      //      if (piece == BlackPawn12) {
      //printf("*%s piece = %d => %d\n", "BP", piece, nnue_pieces[piece]);
      //}
      index++;
    }
  }
  // end piece and square arrays with zero terminating characters
  pieces[index] = 0;
  squares[index] = 0;

  int retval = evaluate_nnue(board->turn, pieces, squares);
  return retval;
  
}

// eval()

int eval(board_t * board, int alpha, int beta, int ThreadId) {

   int opening, endgame;
   material_info_t mat_info[1];
   pawn_info_t pawn_info[1];
   int mul[ColourNb];
   int phase;
   int eval;
   int wb, bb;
   int lazy_eval; // Thomas

   ASSERT(board!=NULL);

   ASSERT(board_is_legal(board));
   ASSERT(!board_is_check(board)); // fixed by adding in_check condition to razoring (JD)

   // init

   opening = 0;
   endgame = 0;

   // material

   material_get_info(mat_info,board,ThreadId);
   
   phase = mat_info->phase;

   opening += mat_info->opening;
   endgame += mat_info->endgame;

   mul[White] = mat_info->mul[White];
   mul[Black] = mat_info->mul[Black];

   // PST

   opening += board->opening;
   endgame += board->endgame;

   // draw

   eval_draw(board,mat_info,pawn_info,mul);

   if (mat_info->mul[White] < mul[White]) mul[White] = mat_info->mul[White];
   if (mat_info->mul[Black] < mul[Black]) mul[Black] = mat_info->mul[Black];

   if (mul[White] == 0 && mul[Black] == 0) return ValueDraw;

   // tempo

   if (COLOUR_IS_WHITE(board->turn)){
		opening += 20;
		endgame += 10;
   } else {
		opening -= 20;
		endgame -= 10;
   } 

   eval_pattern(board,&opening,&endgame);

   // Lazy Eval (Thomas) 
   // returns material+pst+pattern+pawn structure
	
   if (LazyEval && board->piece_size[White] > 3 && board->piece_size[Black] > 3){

     lazy_eval = ((opening * (256 - phase)) + (endgame * phase)) / 256;

     ASSERT(eval>=-ValueEvalInf&&eval<=+ValueEvalInf);

	 if (COLOUR_IS_BLACK(board->turn)) lazy_eval = -lazy_eval;
	 if (lazy_eval - lazy_eval_cutoff >= beta)
		return (lazy_eval);
	 if (lazy_eval + lazy_eval_cutoff <= alpha)
		return (lazy_eval);  
 
   } 


   // nnue post lazy
   if ((board->piece_size[White] + board->piece_size[Black]) > 6) {
     return eval_nnue(board);
   }
   
   // pawns (moved JD: very small gain)

   pawn_get_info(pawn_info,board,ThreadId);

   opening += pawn_info->opening;
   endgame += pawn_info->endgame;

   // eval

   eval_king(board,mat_info,&opening,&endgame);
   eval_passer(board,pawn_info,&opening,&endgame);
   
   // 2nd Lazy Eval Cutoff JD 
   // returns without computing expensive mobility
   
   if (LazyEval && board->piece_size[White] > 3 && board->piece_size[Black] > 3){ // TODO try without 2nd & 3rd conditions

     lazy_eval = ((opening * (256 - phase)) + (endgame * phase)) / 256;

     ASSERT(eval>=-ValueEvalInf&&eval<=+ValueEvalInf);

	 if (COLOUR_IS_BLACK(board->turn)) lazy_eval = -lazy_eval;
	 if (lazy_eval - second_lazy_eval_cutoff >= beta)
		return (lazy_eval);
	 if (lazy_eval + second_lazy_eval_cutoff <= alpha)
		return (lazy_eval);  
   }
   
   eval_piece(board,mat_info,pawn_info,&opening,&endgame); 
   
   // phase mix

   eval = ((opening * (256 - phase)) + (endgame * phase)) / 256;

   // drawish bishop endgames

   if ((mat_info->flags & DrawBishopFlag) != 0) {

      wb = board->piece[White][1];
      ASSERT(PIECE_IS_BISHOP(board->square[wb]));

      bb = board->piece[Black][1];
      ASSERT(PIECE_IS_BISHOP(board->square[bb]));

      if (SQUARE_COLOUR(wb) != SQUARE_COLOUR(bb)) {
         if (mul[White] == 16) mul[White] = 8; // 1/2
         if (mul[Black] == 16) mul[Black] = 8; // 1/2
      }
   }

   // draw bound

   if (eval > ValueDraw) {
      eval = (eval * mul[White]) / 16;
   } else if (eval < ValueDraw) {
      eval = (eval * mul[Black]) / 16;
   }

   // value range

   if (eval < -ValueEvalInf) eval = -ValueEvalInf;
   if (eval > +ValueEvalInf) eval = +ValueEvalInf;

   ASSERT(eval>=-ValueEvalInf&&eval<=+ValueEvalInf);

   // turn

   if (COLOUR_IS_BLACK(board->turn)) eval = -eval;

   ASSERT(!value_is_mate(eval));

   return eval;
}

// eval_draw()

static void eval_draw(const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int mul[2]) {

   int colour;
   int me, opp;
   int pawn, king;
   int pawn_file;
   int prom;
   int list[7+1];

   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(mul!=NULL);

   // draw patterns

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      // KB*P+K* draw

      if ((mat_info->cflags[me] & MatRookPawnFlag) != 0) {

         pawn = pawn_info->single_file[me];

         if (pawn != SquareNone) { // all pawns on one file

            pawn_file = SQUARE_FILE(pawn);

            if (pawn_file == FileA || pawn_file == FileH) {

               king = KING_POS(board,opp);
               prom = PAWN_PROMOTE(pawn,me);

               if (DISTANCE(king,prom) <= 1 && !bishop_can_attack(board,prom,me)) {
                  mul[me] = 0;
               }
            }
         }
      }

      // K(B)P+K+ draw

      if ((mat_info->cflags[me] & MatBishopFlag) != 0) {

         pawn = pawn_info->single_file[me];

         if (pawn != SquareNone) { // all pawns on one file

            king = KING_POS(board,opp);

            if (SQUARE_FILE(king)  == SQUARE_FILE(pawn)
             && PAWN_RANK(king,me) >  PAWN_RANK(pawn,me)
             && !bishop_can_attack(board,king,me)) {
               mul[me] = 1; // 1/16
            }
         }
      }

      // KNPK* draw

      if ((mat_info->cflags[me] & MatKnightFlag) != 0) {

         pawn = board->pawn[me][0];
         king = KING_POS(board,opp);

         if (SQUARE_FILE(king)  == SQUARE_FILE(pawn)
          && PAWN_RANK(king,me) >  PAWN_RANK(pawn,me)
          && PAWN_RANK(pawn,me) <= Rank6) {
            mul[me] = 1; // 1/16
         }
      }
   }

   // recognisers, only heuristic draws here!

   if (false) {

   } else if (mat_info->recog == MAT_KPKQ) {

      // KPKQ (white)

      draw_init_list(list,board,White);

      if (draw_kpkq(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KQKP) {

      // KPKQ (black)

      draw_init_list(list,board,Black);

      if (draw_kpkq(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KPKR) {

      // KPKR (white)

      draw_init_list(list,board,White);

      if (draw_kpkr(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KRKP) {

      // KPKR (black)

      draw_init_list(list,board,Black);

      if (draw_kpkr(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KPKB) {

      // KPKB (white)

      draw_init_list(list,board,White);

      if (draw_kpkb(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KBKP) {

      // KPKB (black)

      draw_init_list(list,board,Black);

      if (draw_kpkb(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KPKN) {

      // KPKN (white)

      draw_init_list(list,board,White);

      if (draw_kpkn(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KNKP) {

      // KPKN (black)

      draw_init_list(list,board,Black);

      if (draw_kpkn(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KNPK) {

      // KNPK (white)

      draw_init_list(list,board,White);

      if (draw_knpk(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KKNP) {

      // KNPK (black)

      draw_init_list(list,board,Black);

      if (draw_knpk(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KRPKR) {

      // KRPKR (white)

      draw_init_list(list,board,White);

      if (draw_krpkr(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KRKRP) {

      // KRPKR (black)

      draw_init_list(list,board,Black);

      if (draw_krpkr(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KBPKB) {

      // KBPKB (white)

      draw_init_list(list,board,White);

      if (draw_kbpkb(list,board->turn)) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }

   } else if (mat_info->recog == MAT_KBKBP) {

      // KBPKB (black)

      draw_init_list(list,board,Black);

      if (draw_kbpkb(list,COLOUR_OPP(board->turn))) {
         mul[White] = 1; // 1/16;
         mul[Black] = 1; // 1/16;
      }
   }
}

// eval_piece()

static void eval_piece(const board_t * board, const material_info_t * mat_info, const pawn_info_t * pawn_info, int * opening, int * endgame) {

   int colour;
   int op[ColourNb], eg[ColourNb];
   int me, opp;
   int opp_flag;
   const sq_t * ptr;
   int from, to;
   int piece;
   int mob;
   int capture;
   const int * unit;
   const int * attack_unit;
   int rook_file, king_file;
   int king;
   int delta;
   int piece_nb, attackvalue;
   int king_rank,piece_rank,new_mob, piece_file;
   int att_value;

   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   // eval

   for (colour = 0; colour < ColourNb; colour++) {

      me = colour;
      opp = COLOUR_OPP(me);

      opp_flag = COLOUR_FLAG(opp);

      unit = MobUnit[me];
	  piece_nb = 0;
	  attackvalue = 0;

      // piece loop

      for (ptr = &board->piece[me][1]; (from=*ptr) != SquareNone; ptr++) { // HACK: no king

         piece = board->square[from];
         
         // threat initialization
         
         att_value = 0;
         attack_unit = AttackUnit[me][piece];
         
         // unsafe square penalty
         
         if (me == White && (board->square[from+17] == BP || board->square[from+15] == BP)){ 
         	op[me] -= PawnAttack;
            eg[me] -= PawnAttack;
         } else if  (me == Black && (board->square[from-17] == WP || board->square[from-15] == WP)){
         	op[me] -= PawnAttack;
            eg[me] -= PawnAttack;
         }

         switch (PIECE_TYPE(piece)) {

         case Knight64:

            // mobility

            mob = 0;

            if (me == White){

               // Safe Mobility - from Toga Returns 1.1 by Ben Tennison
               // tested JD ~5 elo self play 4000 games
               if ((board->square[from-18] != BP && board->square[from-16] != BP)) mob += unit[board->square[from-33]];
               if ((board->square[from-16] != BP && board->square[from-14] != BP)) mob += unit[board->square[from-31]];
               if ((board->square[from- 3] != BP && board->square[from- 1] != BP)) mob += unit[board->square[from-18]];
               if ((board->square[from+ 1] != BP && board->square[from+ 3] != BP)) mob += unit[board->square[from-14]];
               if ((board->square[from+48] != BP && board->square[from+50] != BP)) mob += unit[board->square[from+33]];
               if ((board->square[from+46] != BP && board->square[from+48] != BP)) mob += unit[board->square[from+31]];
               if ((board->square[from+33] != BP && board->square[from+35] != BP)) mob += unit[board->square[from+18]];
			   if ((board->square[from+29] != BP && board->square[from+31] != BP)) mob += unit[board->square[from+14]];

            } else {

               if ((board->square[from-48] != WP && board->square[from-50] != WP)) mob += unit[board->square[from-33]];
               if ((board->square[from-46] != WP && board->square[from-48] != WP)) mob += unit[board->square[from-31]];
               if ((board->square[from-33] != WP && board->square[from-35] != WP)) mob += unit[board->square[from-18]];
               if ((board->square[from-29] != WP && board->square[from-31] != WP)) mob += unit[board->square[from-14]];
               if ((board->square[from+18] != WP && board->square[from+16] != WP)) mob += unit[board->square[from+33]];
               if ((board->square[from+16] != WP && board->square[from+14] != WP)) mob += unit[board->square[from+31]];
               if ((board->square[from+ 3] != WP && board->square[from+ 1] != WP)) mob += unit[board->square[from+18]];
               if ((board->square[from- 1] != WP && board->square[from- 3] != WP)) mob += unit[board->square[from+14]];
            }
            
            op[me] += knight_mob_opening[mob];
            eg[me] += knight_mob_endgame[mob];

            // outpost
            mob = 0;
            if (me == White && (board->square[from+17] != BP && board->square[from+15] != BP)){// not attacked: idea from DLT
                if (board->square[from-17] == WP)
                    mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
                if (board->square[from-15] == WP)
                    mob += mob == 0 ? KnightOutpostMatrix[me][SquareTo64[from]] : KnightOutpostMatrix[me][SquareTo64[from]]/2; 
                    
                // adjust based on no. of opponent pawns
				if (mob > 0) mob = mob*(6 + board->number[BlackPawn12]) / 10;
                
            }
            else if (me == Black && (board->square[from-17] != WP && board->square[from-15] != WP)){
                if (board->square[from+17] == BP)
                    mob += KnightOutpostMatrix[me][SquareTo64[from]]; 
                if (board->square[from+15] == BP)
                    mob += mob == 0 ? KnightOutpostMatrix[me][SquareTo64[from]] : KnightOutpostMatrix[me][SquareTo64[from]]/2; 
                
                // adjust based on no. of opponent pawns
				if (mob > 0) mob = mob*(6 + board->number[WhitePawn12]) / 10;
            } 

            op[me] += mob;
            
            // space / piece invasion

            if (PAWN_RANK(from,me) >= Rank5){
                piece_nb++;
                attackvalue += 1;
            }
            
            // threats
            
            att_value += attack_unit[board->square[from-33]];
            att_value += attack_unit[board->square[from-31]];
            att_value += attack_unit[board->square[from-18]];
            att_value += attack_unit[board->square[from-14]];
            att_value += attack_unit[board->square[from+33]];
            att_value += attack_unit[board->square[from+31]];
            att_value += attack_unit[board->square[from+18]];
            att_value += attack_unit[board->square[from+14]];
            
            if (UseMobAttack){
            	op[me] += att_value;
            	eg[me] += att_value * 2;
            }
            
            break;

         case Bishop64:

            // mobility

            mob = 0;
            
            for (to = from-17; capture=board->square[to], THROUGH(capture); to -= 17) mob += MobMove;
           	mob += unit[capture];
           	att_value += attack_unit[capture];

            for (to = from-15; capture=board->square[to], THROUGH(capture); to -= 15) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from+15; capture=board->square[to], THROUGH(capture); to += 15) mob += MobMove;
           	mob += unit[capture];
           	att_value += attack_unit[capture];

            for (to = from+17; capture=board->square[to], THROUGH(capture); to += 17) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            op[me] += bishop_mob_opening[mob];
            eg[me] += bishop_mob_endgame[mob];
            
            // space

            if (PAWN_RANK(from,me) >= Rank5){
                piece_nb++;
                attackvalue += 1;
            }
            
            // threats
            
            if (UseMobAttack){
            	op[me] += att_value;
            	eg[me] += att_value * 2;
            }


            break;

         case Rook64:

            // mobility

            mob = 0;

            for (to = from-16; capture=board->square[to], THROUGH(capture); to -= 16) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from- 1; capture=board->square[to], THROUGH(capture); to -=  1) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from+ 1; capture=board->square[to], THROUGH(capture); to +=  1) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from+16; capture=board->square[to], THROUGH(capture); to += 16) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            op[me] += rook_mob_opening[mob];
            eg[me] += rook_mob_endgame[mob];

            // open file

            if (UseOpenFile) {

               op[me] -= RookOpenFileOpening / 2;
               eg[me] -= RookOpenFileEndgame / 2;

               rook_file = SQUARE_FILE(from);

               if (board->pawn_file[me][rook_file] == 0) { // no friendly pawn

                  op[me] += RookSemiOpenFileOpening;
                  eg[me] += RookSemiOpenFileEndgame;

                  if (board->pawn_file[opp][rook_file] == 0) { // no enemy pawn
                     op[me] += RookOpenFileOpening - RookSemiOpenFileOpening;
                     eg[me] += RookOpenFileEndgame - RookSemiOpenFileEndgame;
                  }

                  if ((mat_info->cflags[opp] & MatKingFlag) != 0) {

                     king = KING_POS(board,opp);
                     king_file = SQUARE_FILE(king);

                     delta = abs(rook_file-king_file); // file distance

                     if (delta <= 1) {
                        op[me] += RookSemiKingFileOpening;
                        if (delta == 0) op[me] += RookKingFileOpening - RookSemiKingFileOpening;
                     }
                  }
               }
            }

            // 7th rank

            if (PAWN_RANK(from,me) == Rank7) {
               if ((pawn_info->flags[opp] & BackRankFlag) != 0 // opponent pawn on 7th rank
                || PAWN_RANK(KING_POS(board,opp),me) == Rank8) {
                  op[me] += Rook7thOpening;
                  eg[me] += Rook7thEndgame;
               }
            }
            
            // space 

            if (PAWN_RANK(from,me) >= Rank5){
                piece_nb++;
                attackvalue += 2;
            }
            
            // threats
            
            if (UseMobAttack){
            	op[me] += att_value;
            	eg[me] += att_value * 2;
            }

            break;

         case Queen64:
         	
         	mob = 0;
         	
         	for (to = from-17; capture=board->square[to], THROUGH(capture); to -= 17) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from-16; capture=board->square[to], THROUGH(capture); to -= 16) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from-15; capture=board->square[to], THROUGH(capture); to -= 15) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from- 1; capture=board->square[to], THROUGH(capture); to -=  1) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from+ 1; capture=board->square[to], THROUGH(capture); to +=  1) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from+15; capture=board->square[to], THROUGH(capture); to += 15) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from+16; capture=board->square[to], THROUGH(capture); to += 16) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];

            for (to = from+17; capture=board->square[to], THROUGH(capture); to += 17) mob += MobMove;
            mob += unit[capture];
            att_value += attack_unit[capture];
            
            op[me] += queen_mob_opening[mob];
            eg[me] += queen_mob_endgame[mob];

            // 7th rank

            if (PAWN_RANK(from,me) == Rank7) {
               if ((pawn_info->flags[opp] & BackRankFlag) != 0 // opponent pawn on 7th rank
                || PAWN_RANK(KING_POS(board,opp),me) == Rank8) {
                  //op[me] += Queen7thOpening;
                  eg[me] += Queen7thEndgame; 
               }
            } 
            
            // space

            if (PAWN_RANK(from,me) >= Rank5){
                piece_nb++;
                attackvalue += 4;
            }
            
            // threats
            
            if (UseMobAttack){
            	op[me] += att_value/3;
            	eg[me] += att_value;
			}
            
            break;
         }
      }
      
      // space / piece incasion
      
      op[me] += SSpaceWeight[piece_nb] * attackvalue;
   }

   // update

   *opening += ((op[White] - op[Black]) * PieceActivityWeight) / 256;
   *endgame += ((eg[White] - eg[Black]) * PieceActivityWeight) / 256;
}

// eval_king()

static void eval_king(const board_t * board, const material_info_t * mat_info, int * opening, int * endgame) {

   int colour;
   int op[ColourNb], eg[ColourNb];
   int me, opp;
   int from;
   int penalty_1, penalty_2;
   int tmp;
   int penalty;
   int king;
   const sq_t * ptr;
   int piece;
   int attack_tot;
   int piece_nb;
   int king_file, king_rank, file;

   ASSERT(board!=NULL);
   ASSERT(mat_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   // white pawn shelter
   
   if (UseShelter && (mat_info->cflags[White] & MatKingFlag) != 0) { 

      // init 
      
	  penalty = 0;
	  me = White;

	  if (board->number[BlackQueen12] > 0){ 
	  
		  // king

		  penalty_1 = shelter_square(board,mat_info,KING_POS(board,me),me);

		  // castling

          penalty_2 = penalty_1;

          if ((board->flags & FlagsWhiteKingCastle) != 0) {
             tmp = shelter_square(board,mat_info,G1,me);
             if (tmp < penalty_2) penalty_2 = tmp;
		  }

          if ((board->flags & FlagsWhiteQueenCastle) != 0) {
             tmp = shelter_square(board,mat_info,B1,me);
             if (tmp < penalty_2) penalty_2 = tmp;
		  }

          ASSERT(penalty_2>=0&&penalty_2<=penalty_1);

          // penalty

          penalty = (penalty_1 + penalty_2) / 2;
          ASSERT(penalty>=0);
	  }
	  
	  // storm
	  
      if (UseStorm) {
		file = SQUARE_FILE(KING_POS(board,White));
		penalty += storm_file(board,file,White);
		if (file != FileA) penalty += storm_file(board,file-1,White);
		if (file != FileH) penalty += storm_file(board,file+1,White);
	  }
	  
	  // add white king safety score to evaluation
	  op[me] -= (penalty * ShelterOpening) / 256;
	  
   }

   // black pawn shelter

   if (UseShelter && (mat_info->cflags[Black] & MatKingFlag) != 0) {

	  penalty = 0;
	  me = Black;

	  if (board->number[WhiteQueen12] > 0){ 

	      // king

          penalty_1 = shelter_square(board,mat_info,KING_POS(board,me),me);

		  // castling

          penalty_2 = penalty_1;

          if ((board->flags & FlagsBlackKingCastle) != 0) {
             tmp = shelter_square(board,mat_info,G8,me);
             if (tmp < penalty_2) penalty_2 = tmp;
		  }

          if ((board->flags & FlagsBlackQueenCastle) != 0) {
             tmp = shelter_square(board,mat_info,B8,me);
             if (tmp < penalty_2) penalty_2 = tmp;
		  }

          ASSERT(penalty_2>=0&&penalty_2<=penalty_1);

          // penalty

          penalty = (penalty_1 + penalty_2) / 2;
          ASSERT(penalty>=0);

	  }
	  
	  // storm
	  
	  if (UseStorm) {
		file = SQUARE_FILE(KING_POS(board,Black));
		penalty += storm_file(board,file,Black);
		if (file != FileA) penalty += storm_file(board,file-1,Black);
		if (file != FileH) penalty += storm_file(board,file+1,Black);
	  }
	  
	  op[me] -= (penalty * ShelterOpening) / 256;
   }

   // king attacks

   if (UseKingAttack) {

      for (colour = 0; colour < ColourNb; colour++) {

         if ((mat_info->cflags[colour] & MatKingFlag) != 0) {

            me = colour;
            opp = COLOUR_OPP(me);

            king = KING_POS(board,me);
			king_file = SQUARE_FILE(king);
			king_rank = SQUARE_RANK(king);
            
            // piece attacks

            attack_tot = 0;
            piece_nb = 0;
			
            for (ptr = &board->piece[opp][1]; (from=*ptr) != SquareNone; ptr++) { // HACK: no king

               piece = board->square[from];

               if (piece_attack_king(board,piece,from,king)) {
                  piece_nb++;
                  attack_tot += KingAttackUnit[piece];
               }
/*		       else{
			       if ((abs(king_file-SQUARE_FILE(from)) + abs(king_rank-SQUARE_RANK(from))) <= 4){
					   piece_nb++;
                       attack_tot += KingAttackUnit[piece];
				   }
			   } */
            }

            // scoring

            ASSERT(piece_nb>=0&&piece_nb<16);

			op[colour] -= (attack_tot * KingAttackOpening * KingAttackWeight[piece_nb]) / 256;
         }
      }
   }

   // update

   *opening += ((op[White] - op[Black]) * KingSafetyWeight) / 256;
   *endgame += ((eg[White] - eg[Black]) * KingSafetyWeight) / 256;
}

// eval_passer()

static void eval_passer(const board_t * board, const pawn_info_t * pawn_info, int * opening, int * endgame) {

   int colour;
   int op[ColourNb], eg[ColourNb];
   int att, def;
   int bits;
   int file, rank;
   int sq;
   int min, max;
   int delta;
   int white_passed_nb, black_passed_nb;
   
   ASSERT(board!=NULL);
   ASSERT(pawn_info!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // init

   for (colour = 0; colour < ColourNb; colour++) {
      op[colour] = 0;
      eg[colour] = 0;
   }

   white_passed_nb = 0;
   black_passed_nb = 0;

   // passed pawns

   for (colour = 0; colour < ColourNb; colour++) {

      att = colour;
      def = COLOUR_OPP(att);

      for (bits = pawn_info->passed_bits[att]; bits != 0; bits &= bits-1) {

         file = BIT_FIRST(bits);
         ASSERT(file>=FileA&&file<=FileH);

         rank = BIT_LAST(board->pawn_file[att][file]);
         ASSERT(rank>=Rank2&&rank<=Rank7);

         sq = SQUARE_MAKE(file,rank);
         if (COLOUR_IS_BLACK(att)) sq = SQUARE_RANK_MIRROR(sq);

         ASSERT(PIECE_IS_PAWN(board->square[sq]));
         ASSERT(COLOUR_IS(board->square[sq],att));

		 // Thomas

	/*	 if (att == White){
             if (board->piece_size[Black]-1 == board->number[BlackKnight12] && (file == FileA || file == FileH)){
				eg[att] += 30;
			 }
		
		 }
		 else{
			 if (board->piece_size[White]-1 == board->number[WhiteKnight12] && (file == FileA || file == FileH)){
				eg[att] += 30;
			 }
			 
		 }  */

         // opening scoring

         op[att] += quad(PassedOpeningMin,PassedOpeningMax,rank);

         // endgame scoring init

         min = PassedEndgameMin;
         max = PassedEndgameMax;

         delta = max - min;
         ASSERT(delta>0);

         // "dangerous" bonus

         if (board->piece_size[def] <= 1 // defender has no piece
          && (unstoppable_passer(board,sq,att) || king_passer(board,sq,att))) {
            delta += UnstoppablePasser;
         } else if (free_passer(board,sq,att)) {
            delta += FreePasser;
         }

         // king-distance bonus

         delta -= pawn_att_dist(sq,KING_POS(board,att),att) * AttackerDistance;
         delta += pawn_def_dist(sq,KING_POS(board,def),att) * DefenderDistance;

         // endgame scoring

         eg[att] += min;
         if (delta > 0) eg[att] += quad(0,delta,rank);
      }
   }

   // update

/*   if (white_passed_nb > 100){
		op[White] = ((white_passed_nb / 10) * op[White]) / 100 + op[White];
        eg[White] = ((white_passed_nb / 10) * eg[White]) / 100 + eg[White]; 
   }
   if (black_passed_nb > 100){
		op[Black] = ((black_passed_nb / 10) * op[Black]) / 100 + op[Black];
        eg[Black] = ((black_passed_nb / 10) * eg[Black]) / 100 + eg[Black]; 
   } */
   
   *opening += ((op[White] - op[Black]) * PassedPawnWeight) / 256;
   *endgame += ((eg[White] - eg[Black]) * PassedPawnWeight) / 256;
}

// eval_pattern()

static void eval_pattern(const board_t * board, int * opening, int * endgame) {

   ASSERT(board!=NULL);
   ASSERT(opening!=NULL);
   ASSERT(endgame!=NULL);

   // trapped bishop (7th rank)

   if ((board->square[A7] == WB && board->square[B6] == BP)
    || (board->square[B8] == WB && board->square[C7] == BP)) {
      *opening -= TrappedBishop;
      *endgame -= TrappedBishop;
   }

   if ((board->square[H7] == WB && board->square[G6] == BP)
    || (board->square[G8] == WB && board->square[F7] == BP)) {
      *opening -= TrappedBishop;
      *endgame -= TrappedBishop;
   }

   if ((board->square[A2] == BB && board->square[B3] == WP)
    || (board->square[B1] == BB && board->square[C2] == WP)) {
      *opening += TrappedBishop;
      *endgame += TrappedBishop;
   }

   if ((board->square[H2] == BB && board->square[G3] == WP)
    || (board->square[G1] == BB && board->square[F2] == WP)) {
      *opening += TrappedBishop;
      *endgame += TrappedBishop;
   }

   // trapped bishop (6th rank)

   if (board->square[A6] == WB && board->square[B5] == BP) {
      *opening -= TrappedBishop / 2;
      *endgame -= TrappedBishop / 2;
   }

   if (board->square[H6] == WB && board->square[G5] == BP) {
      *opening -= TrappedBishop / 2;
      *endgame -= TrappedBishop / 2;
   }

   if (board->square[A3] == BB && board->square[B4] == WP) {
      *opening += TrappedBishop / 2;
      *endgame += TrappedBishop / 2;
   }

   if (board->square[H3] == BB && board->square[G4] == WP) {
      *opening += TrappedBishop / 2;
      *endgame += TrappedBishop / 2;
   }

   // blocked bishop

   if (board->square[D2] == WP && board->square[D3] != Empty && board->square[C1] == WB) {
      *opening -= BlockedBishop;
   }

   if (board->square[E2] == WP && board->square[E3] != Empty && board->square[F1] == WB) {
      *opening -= BlockedBishop;
   }

   if (board->square[D7] == BP && board->square[D6] != Empty && board->square[C8] == BB) {
      *opening += BlockedBishop;
   }

   if (board->square[E7] == BP && board->square[E6] != Empty && board->square[F8] == BB) {
      *opening += BlockedBishop;
   }

   // blocked rook

   if ((board->square[C1] == WK || board->square[B1] == WK)
    && (board->square[A1] == WR || board->square[A2] == WR || board->square[B1] == WR)) {
      *opening -= BlockedRook;
   }

   if ((board->square[F1] == WK || board->square[G1] == WK)
    && (board->square[H1] == WR || board->square[H2] == WR || board->square[G1] == WR)) {
      *opening -= BlockedRook;
   }

   if ((board->square[C8] == BK || board->square[B8] == BK)
    && (board->square[A8] == BR || board->square[A7] == BR || board->square[B8] == BR)) {
      *opening += BlockedRook;
   }

   if ((board->square[F8] == BK || board->square[G8] == BK)
    && (board->square[H8] == BR || board->square[H7] == BR || board->square[G8] == BR)) {
      *opening += BlockedRook;
   }
}

// unstoppable_passer()

static bool unstoppable_passer(const board_t * board, int pawn, int colour) {

   int me, opp;
   int file, rank;
   int king;
   int prom;
   const sq_t * ptr;
   int sq;
   int dist;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   opp = COLOUR_OPP(me);

   file = SQUARE_FILE(pawn);
   rank = PAWN_RANK(pawn,me);

   king = KING_POS(board,opp);

   // clear promotion path?

   for (ptr = &board->piece[me][0]; (sq=*ptr) != SquareNone; ptr++) {
      if (SQUARE_FILE(sq) == file && PAWN_RANK(sq,me) > rank) {
         return false; // "friendly" blocker
      }
   }

   // init

   if (rank == Rank2) {
      pawn += PAWN_MOVE_INC(me);
      rank++;
      ASSERT(rank==PAWN_RANK(pawn,me));
   }

   ASSERT(rank>=Rank3&&rank<=Rank7);

   prom = PAWN_PROMOTE(pawn,me);

   dist = DISTANCE(pawn,prom);
   ASSERT(dist==Rank8-rank);
   if (board->turn == opp) dist++;

   if (DISTANCE(king,prom) > dist) return true; // not in the square

   return false;
}

// king_passer()

static bool king_passer(const board_t * board, int pawn, int colour) {

   int me;
   int king;
   int file;
   int prom;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;

   king = KING_POS(board,me);
   file = SQUARE_FILE(pawn);
   prom = PAWN_PROMOTE(pawn,me);

   if (DISTANCE(king,prom) <= 1
    && DISTANCE(king,pawn) <= 1
    && (SQUARE_FILE(king) != file
     || (file != FileA && file != FileH))) {
      return true;
   }

   return false;
}

// free_passer()

static bool free_passer(const board_t * board, int pawn, int colour) {

   int me, opp;
   int inc;
   int sq;
   int move;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   opp = COLOUR_OPP(me);

   inc = PAWN_MOVE_INC(me);
   sq = pawn + inc;
   ASSERT(SQUARE_IS_OK(sq));

   if (board->square[sq] != Empty) return false;

   move = MOVE_MAKE(pawn,sq);
   if (see_move(move,board) < 0) return false;

   return true;
}

// pawn_att_dist()

static int pawn_att_dist(int pawn, int king, int colour) {

   int me;
   int inc;
   int target;

   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(SQUARE_IS_OK(king));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   inc = PAWN_MOVE_INC(me);

   target = pawn + inc;

   return DISTANCE(king,target);
}

// pawn_def_dist()

static int pawn_def_dist(int pawn, int king, int colour) {

   int me;
   int inc;
   int target;

   ASSERT(SQUARE_IS_OK(pawn));
   ASSERT(SQUARE_IS_OK(king));
   ASSERT(COLOUR_IS_OK(colour));

   me = colour;
   inc = PAWN_MOVE_INC(me);

   target = pawn + inc;

   return DISTANCE(king,target);
}

// draw_init_list()

static void draw_init_list(int list[], const board_t * board, int pawn_colour) {

   int pos;
   int att, def;
   const sq_t * ptr;
   int sq;
   int pawn;
   int i;

   ASSERT(list!=NULL);
   ASSERT(board!=NULL);
   ASSERT(COLOUR_IS_OK(pawn_colour));

   // init

   pos = 0;

   att = pawn_colour;
   def = COLOUR_OPP(att);

   ASSERT(board->pawn_size[att]==1);
   ASSERT(board->pawn_size[def]==0);

   // att

   for (ptr = &board->piece[att][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   for (ptr = &board->pawn[att][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   // def

   for (ptr = &board->piece[def][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   for (ptr = &board->pawn[def][0]; (sq=*ptr) != SquareNone; ptr++) {
      list[pos++] = sq;
   }

   // end marker

   ASSERT(pos==board->piece_nb);

   list[pos] = SquareNone;

   // file flip?

   pawn = board->pawn[att][0];

   if (SQUARE_FILE(pawn) >= FileE) {
      for (i = 0; i < pos; i++) {
         list[i] = SQUARE_FILE_MIRROR(list[i]);
      }
   }

   // rank flip?

   if (COLOUR_IS_BLACK(pawn_colour)) {
      for (i = 0; i < pos; i++) {
         list[i] = SQUARE_RANK_MIRROR(list[i]);
      }
   }
}

// draw_kpkq()

static bool draw_kpkq(const int list[], int turn) {

   int wk, wp, bk, bq;
   int prom;
   int dist;
   // int wp_file, wp_rank;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bq = *list++;
   ASSERT(SQUARE_IS_OK(bq));

   ASSERT(*list==SquareNone);

   // test

   if (false) {

   } else if (wp == A7) {

      prom = A8;
      dist = 4;

      if (wk == B7 || wk == B8) { // best case
         if (COLOUR_IS_WHITE(turn)) dist--;
      } else if (wk == A8 || ((wk == C7 || wk == C8) && bq != A8)) { // white loses a tempo
         if (COLOUR_IS_BLACK(turn) && SQUARE_FILE(bq) != FileB) return false;
      } else {
         return false;
      }

      ASSERT(bq!=prom);
      if (DISTANCE(bk,prom) > dist) return true;

   } else if (wp == C7) {

      prom = C8;
      dist = 4;

      if (false) {

      } else if (wk == C8) { // dist = 0

         dist++; // self-blocking penalty
         if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

      } else if (wk == B7 || wk == B8) { // dist = 1, right side

         dist--; // right-side bonus
         if (DELTA_INC_LINE(wp-bq) == wk-wp) dist++; // pinned-pawn penalty
         if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

      } else if (wk == D7 || wk == D8) { // dist = 1, wrong side

         if (DELTA_INC_LINE(wp-bq) == wk-wp) dist++; // pinned-pawn penalty
         if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

      } else if ((wk == A7 || wk == A8) && bq != C8) { // dist = 2, right side

         if (COLOUR_IS_BLACK(turn) && SQUARE_FILE(bq) != FileB) return false;

         dist--; // right-side bonus

      } else if ((wk == E7 || wk == E8) && bq != C8) { // dist = 2, wrong side

         if (COLOUR_IS_BLACK(turn) && SQUARE_FILE(bq) != FileD) return false;

      } else {

         return false;
      }

      ASSERT(bq!=prom);
      if (DISTANCE(bk,prom) > dist) return true;
   }

   return false;
}

// draw_kpkr()

static bool draw_kpkr(const int list[], int turn) {

   int wk, wp, bk, br;
   int wk_file, wk_rank;
   int wp_file, wp_rank;
   int br_file, br_rank;
   int inc;
   int prom;
   int dist;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   br = *list++;
   ASSERT(SQUARE_IS_OK(br));

   ASSERT(*list==SquareNone);

   // init

   wk_file = SQUARE_FILE(wk);
   wk_rank = SQUARE_RANK(wk);

   wp_file = SQUARE_FILE(wp);
   wp_rank = SQUARE_RANK(wp);

   br_file = SQUARE_FILE(br);
   br_rank = SQUARE_RANK(br);

   inc = PAWN_MOVE_INC(White);
   prom = PAWN_PROMOTE(wp,White);

   // conditions

   if (false) {

   } else if (DISTANCE(wk,wp) == 1) {

      ASSERT(abs(wk_file-wp_file)<=1);
      ASSERT(abs(wk_rank-wp_rank)<=1);

      // no-op

   } else if (DISTANCE(wk,wp) == 2 && abs(wk_rank-wp_rank) <= 1) {

      ASSERT(abs(wk_file-wp_file)==2);
      ASSERT(abs(wk_rank-wp_rank)<=1);

      if (COLOUR_IS_BLACK(turn) && br_file != (wk_file + wp_file) / 2) return false;

   } else {

      return false;
   }

   // white features

   dist = DISTANCE(wk,prom) + DISTANCE(wp,prom);
   if (wk == prom) dist++;

   if (wk == wp+inc) { // king on pawn's "front square"
      if (wp_file == FileA) return false;
      dist++; // self-blocking penalty
   }

   // black features

   if (br_file != wp_file && br_rank != Rank8) {
      dist--; // misplaced-rook bonus
   }

   // test

   if (COLOUR_IS_WHITE(turn)) dist--; // right-to-move bonus

   if (DISTANCE(bk,prom) > dist) return true;

   return false;
}

// draw_kpkb()

static bool draw_kpkb(const int list[], int turn) {

   int wk, wp, bk, bb;
   int inc;
   int end, to;
   int delta, inc_2;
   int sq;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bb = *list++;
   ASSERT(SQUARE_IS_OK(bb));

   ASSERT(*list==SquareNone);

   // blocked pawn?

   inc = PAWN_MOVE_INC(White);
   end = PAWN_PROMOTE(wp,White) + inc;

   for (to = wp+inc; to != end; to += inc) {

      ASSERT(SQUARE_IS_OK(to));

      if (to == bb) return true; // direct blockade

      delta = to - bb;
      ASSERT(delta_is_ok(delta));

      if (PSEUDO_ATTACK(BB,delta)) {

         inc_2 = DELTA_INC_ALL(delta);
         ASSERT(inc_2!=IncNone);

         sq = bb;
         do {
            sq += inc_2;
            ASSERT(SQUARE_IS_OK(sq));
            ASSERT(sq!=wk);
            ASSERT(sq!=wp);
            ASSERT(sq!=bb);
            if (sq == to) return true; // indirect blockade
         } while (sq != bk);
      }
   }

   return false;
}

// draw_kpkn()

static bool draw_kpkn(const int list[], int turn) {

   int wk, wp, bk, bn;
   int inc;
   int end;
   int file;
   int sq;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bn = *list++;
   ASSERT(SQUARE_IS_OK(bn));

   ASSERT(*list==SquareNone);

   // blocked pawn?

   inc = PAWN_MOVE_INC(White);
   end = PAWN_PROMOTE(wp,White) + inc;

   file = SQUARE_FILE(wp);
   if (file == FileA || file == FileH) end -= inc;

   for (sq = wp+inc; sq != end; sq += inc) {

      ASSERT(SQUARE_IS_OK(sq));

      if (sq == bn || PSEUDO_ATTACK(BN,sq-bn)) return true; // blockade
   }

   return false;
}

// draw_knpk()

static bool draw_knpk(const int list[], int turn) {

   int wk, wn, wp, bk;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wn = *list++;
   ASSERT(SQUARE_IS_OK(wn));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   ASSERT(*list==SquareNone);

   // test

   if (wp == A7 && DISTANCE(bk,A8) <= 1) return true;

   return false;
}

// draw_krpkr()

static bool draw_krpkr(const int list[], int turn) {

   int wk, wr, wp, bk, br;
   int wp_file, wp_rank;
   int bk_file, bk_rank;
   int br_file, br_rank;
   int prom;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wr = *list++;
   ASSERT(SQUARE_IS_OK(wr));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   br = *list++;
   ASSERT(SQUARE_IS_OK(br));

   ASSERT(*list==SquareNone);

   // test

   wp_file = SQUARE_FILE(wp);
   wp_rank = SQUARE_RANK(wp);

   bk_file = SQUARE_FILE(bk);
   bk_rank = SQUARE_RANK(bk);

   br_file = SQUARE_FILE(br);
   br_rank = SQUARE_RANK(br);

   prom = PAWN_PROMOTE(wp,White);

   if (false) {

   } else if (bk == prom) {

      // TODO: rook near Rank1 if wp_rank == Rank6?

      if (br_file > wp_file) return true;

   } else if (bk_file == wp_file && bk_rank > wp_rank) {

      return true;

   } else if (wr == prom && wp_rank == Rank7 && (bk == G7 || bk == H7) && br_file == wp_file) {

      if (br_rank <= Rank3) {
         if (DISTANCE(wk,wp) > 1) return true;
      } else { // br_rank >= Rank4
         if (DISTANCE(wk,wp) > 2) return true;
      }
   }

   return false;
}

// draw_kbpkb()

static bool draw_kbpkb(const int list[], int turn) {

   int wk, wb, wp, bk, bb;
   int inc;
   int end, to;
   int delta, inc_2;
   int sq;

   ASSERT(list!=NULL);
   ASSERT(COLOUR_IS_OK(turn));

   // load

   wk = *list++;
   ASSERT(SQUARE_IS_OK(wk));

   wb = *list++;
   ASSERT(SQUARE_IS_OK(wb));

   wp = *list++;
   ASSERT(SQUARE_IS_OK(wp));
   ASSERT(SQUARE_FILE(wp)<=FileD);

   bk = *list++;
   ASSERT(SQUARE_IS_OK(bk));

   bb = *list++;
   ASSERT(SQUARE_IS_OK(bb));

   ASSERT(*list==SquareNone);

   // opposit colour?

   if (SQUARE_COLOUR(wb) == SQUARE_COLOUR(bb)) return false; // TODO

   // blocked pawn?

   inc = PAWN_MOVE_INC(White);
   end = PAWN_PROMOTE(wp,White) + inc;

   for (to = wp+inc; to != end; to += inc) {

      ASSERT(SQUARE_IS_OK(to));

      if (to == bb) return true; // direct blockade

      delta = to - bb;
      ASSERT(delta_is_ok(delta));

      if (PSEUDO_ATTACK(BB,delta)) {

         inc_2 = DELTA_INC_ALL(delta);
         ASSERT(inc_2!=IncNone);

         sq = bb;
         do {
            sq += inc_2;
            ASSERT(SQUARE_IS_OK(sq));
            ASSERT(sq!=wk);
            ASSERT(sq!=wb);
            ASSERT(sq!=wp);
            ASSERT(sq!=bb);
            if (sq == to) return true; // indirect blockade
         } while (sq != bk);
      }
   }

   return false;
}

// shelter_square()

static int shelter_square(const board_t * board, const material_info_t * mat_info, int square, int colour) {

   int penalty;
   int file, rank;
   int type;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(square));
   ASSERT(COLOUR_IS_OK(colour));

   penalty = 0;

   file = SQUARE_FILE(square);
   rank = PAWN_RANK(square,colour);

   penalty += shelter_file(board,file,rank,colour,TypeShelterFront);
   
   if (file != FileA){
      if (file > FileE)
          penalty += shelter_file(board,file-1,rank,colour,TypeShelterOpen);
      else 
          penalty += shelter_file(board,file-1,rank,colour,TypeShelterClosed);
   }
          
   if (file != FileH){ 
       if (file < FileD)
          penalty += shelter_file(board,file+1,rank,colour,TypeShelterOpen);
       else
          penalty += shelter_file(board,file+1,rank,colour,TypeShelterClosed);
   }
  
   if (penalty == 0) penalty = 11; // weak back rank

   if (UseStorm) {
      penalty += storm_file(board,file,colour);
      if (file != FileA) penalty += storm_file(board,file-1,colour);
      if (file != FileH) penalty += storm_file(board,file+1,colour);
   }

   return penalty;
}

// shelter_file()

static int shelter_file(const board_t * board, int file, int rank, int colour, int type) {

   int dist;

   ASSERT(board!=NULL);
   ASSERT(file>=FileA&&file<=FileH);
   ASSERT(rank>=Rank1&&rank<=Rank8);
   ASSERT(COLOUR_IS_OK(colour));

   dist = BIT_FIRST(board->pawn_file[colour][file]&BitGE[rank]);
   ASSERT(dist>=Rank2&&dist<=Rank8);

   dist = Rank8 - dist;
   ASSERT(dist>=0&&dist<=6);

   if (type == TypeShelterFront)
       return PawnShelterFront[dist];
   else if (type == TypeShelterOpen)
       return PawnShelterOpenSide[dist];
   else
       return PawnShelterClosedSide[dist];

}

// storm_file()

static int storm_file(const board_t * board, int file, int colour) {

   int dist;
   int penalty;

   ASSERT(board!=NULL);
   ASSERT(file>=FileA&&file<=FileH);
   ASSERT(COLOUR_IS_OK(colour));

   dist = BIT_LAST(board->pawn_file[COLOUR_OPP(colour)][file]);
   ASSERT(dist>=Rank1&&dist<=Rank7);

   penalty = 0;

   switch (dist) {
   case Rank4:
      penalty = StormOpening * 1;
      break;
   case Rank5:
      penalty = StormOpening * 2;
      break;
   case Rank6:
      penalty = StormOpening * 4; // JD
      break;
   }

   return penalty;
}

// bishop_can_attack()

static bool bishop_can_attack(const board_t * board, int to, int colour) {

   const sq_t * ptr;
   int from;
   int piece;

   ASSERT(board!=NULL);
   ASSERT(SQUARE_IS_OK(to));
   ASSERT(COLOUR_IS_OK(colour));

   for (ptr = &board->piece[colour][1]; (from=*ptr) != SquareNone; ptr++) { // HACK: no king

      piece = board->square[from];

      if (PIECE_IS_BISHOP(piece) && SQUARE_COLOUR(from) == SQUARE_COLOUR(to)) {
         return true;
      }
   }

   return false;
}

// end of eval.cpp

