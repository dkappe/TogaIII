/* NNUE wrapper function headers */
void init_nnue(char *filename);
void init_nnue_embedded();
int evaluate_nnue(int player, int *pieces, int *squares);
int evaluate_fen_nnue(char *fen);
