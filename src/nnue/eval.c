#include <stdint.h>
#include "./nnue.h"
#include "./eval.h"
int init_nnue(char * filename) {
  nnue_init(filename);
}
int evaluate_fen(char * fen) {
  nnue_evaluate_fen(fen);
}
