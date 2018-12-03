/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2019 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>

#include "types.h"

Value PieceValue[PHASE_NB][PIECE_NB] = {
  { VALUE_ZERO, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg },
  { VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg }
};

namespace PSQT {

#define S(mg, eg) make_score(mg, eg)

// Bonus[PieceType][Square / 2] contains Piece-Square scores. For each piece
// type on a given square a (middlegame, endgame) score pair is assigned. Table
// is defined for files A..D and white side: it is symmetric for black side and
// second half of the files.
constexpr Score Bonus[][RANK_NB][int(FILE_NB) / 2] = {
  { },
  { // Pawn
   { S(  0, 0), S(  0,  0), S(  0, 0), S( 0, 0) },
   { S(-11,-3), S(  7, -1), S(  7, 7), S(17, 2) },
   { S(-16,-2), S( -3,  2), S( 23, 6), S(23,-1) },
   { S(-14, 7), S( -7, -4), S( 20,-8), S(24, 2) },
   { S( -5,13), S( -2, 10), S( -1,-1), S(12,-8) },
   { S(-11,16), S(-12,  6), S( -2, 1), S( 4,16) },
   { S( -2, 1), S( 20,-12), S(-10, 6), S(-2,25) }
  },
  { // Knight
   { S(-169,-102), S(-96,-74), S(-81,-45), S(-79,-18) },
   { S( -80, -69), S(-42,-55), S(-24,-15), S(-11,  7) },
   { S( -65, -40), S(-19,-34), S(  4, -5), S( 18, 26) },
   { S( -27, -38), S(  5,  0), S( 40, 13), S( 47, 33) },
   { S( -30, -40), S( 14,-19), S( 42,  6), S( 51, 36) },
   { S( -11, -51), S( 28,-39), S( 65,-18), S( 55, 18) },
   { S( -67, -67), S(-20,-44), S(  6,-37), S( 38, 17) },
   { S(-200, -98), S(-79,-88), S(-54,-55), S(-33,-16) }
  },
  { // Bishop
   { S(-49,-60), S( -5,-31), S(-10,-39), S(-32,-20) },
   { S(-23,-36), S(  7, -9), S( 16,-14), S(  1,  2) },
   { S( -8,-23), S( 23, -2), S( -4, -4), S( 12, 17) },
   { S(  5,-28), S(  9, -2), S( 20, -5), S( 40, 16) },
   { S( -7,-27), S( 26, -5), S( 13, -9), S( 29, 13) },
   { S(-18,-24), S( 14, -3), S( -7,  1), S(  8, 12) },
   { S(-18,-34), S(-12, -9), S(  7,-12), S(-13,  6) },
   { S(-45,-52), S( -5,-32), S(-18,-35), S(-28,-16) }
  },
  { // Rook
   { S(-24,-1), S(-16, 4), S(-10, 0), S(-1, 5) },
   { S(-19,-7), S( -4,-5), S( -2,-5), S( 0, 1) },
   { S(-21, 7), S(-11,-8), S(  2, 2), S( 1, 5) },
   { S(-23, 0), S( -9, 3), S( -3, 0), S(-2, 4) },
   { S(-21,-7), S(-14, 5), S( -1,-6), S( 3,-7) },
   { S(-21, 4), S(-11, 3), S(  2,-1), S( 7, 4) },
   { S(-11, 0), S( 11, 6), S(  9,12), S(12, 1) },
   { S(-26, 8), S(-19, 0), S(-13, 9), S( 2, 3) }
  },
  { // Queen
   { S(  4,-70), S(-6,-57), S(-4,-48), S( 5,-28) },
   { S( -2,-54), S( 6,-32), S( 7,-23), S(11, -2) },
   { S( -3,-38), S( 7,-16), S(13, -8), S( 7,  3) },
   { S(  3,-22), S( 4, -4), S( 8, 14), S( 7, 23) },
   { S( -1,-26), S(15, -7), S(12,  8), S( 3, 21) },
   { S( -4,-37), S(10,-17), S( 6,-11), S( 6,  1) },
   { S( -7,-50), S( 3,-28), S(10,-23), S( 6, -9) },
   { S( -3,-74), S(-3,-53), S( 0,-44), S(-2,-36) }
  },
  { // King
   { S(273, -1), S(326, 39), S(272, 78), S(190, 93) },
   { S(277, 59), S(305, 96), S(241,136), S(186,130) },
   { S(197, 87), S(253,137), S(169,165), S(121,174) },
   { S(171,103), S(191,151), S(136,168), S(108,169) },
   { S(145,100), S(175,165), S(113,195), S( 70,192) },
   { S(123, 87), S(159,164), S( 85,177), S( 36,190) },
   { S( 88, 40), S(120, 95), S( 63,131), S( 25,142) },
   { S( 64,  6), S( 85, 60), S( 49, 73), S(  1, 75) }
  }
};

#undef S

Score psq[PIECE_NB][SQUARE_NB];

// init() initializes piece-square tables: the white halves of the tables are
// copied from Bonus[] adding the piece value, then the black halves of the
// tables are initialized by flipping and changing the sign of the white scores.
void init() {

  for (Piece pc = W_PAWN; pc <= W_KING; ++pc)
  {
      PieceValue[MG][~pc] = PieceValue[MG][pc];
      PieceValue[EG][~pc] = PieceValue[EG][pc];

      Score score = make_score(PieceValue[MG][pc], PieceValue[EG][pc]);

      for (Square s = SQ_A1; s <= SQ_H8; ++s)
      {
          File f = std::min(file_of(s), ~file_of(s));
          psq[ pc][ s] = score + Bonus[pc][rank_of(s)][f];
          psq[~pc][~s] = -psq[pc][s];
      }
  }
}

} // namespace PSQT
