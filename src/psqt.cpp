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
  { },
  { // Knight
   { S(-175, -96), S(-92,-65), S(-74,-49), S(-73,-21) },
   { S( -77, -67), S(-41,-54), S(-27,-18), S(-15,  8) },
   { S( -61, -40), S(-17,-27), S(  6, -8), S( 12, 29) },
   { S( -35, -35), S(  8, -2), S( 40, 13), S( 49, 28) },
   { S( -34, -45), S( 13,-16), S( 44,  9), S( 51, 39) },
   { S(  -9, -51), S( 22,-44), S( 58,-16), S( 53, 17) },
   { S( -67, -69), S(-27,-50), S(  4,-51), S( 37, 12) },
   { S(-201,-100), S(-83,-88), S(-56,-56), S(-26,-17) }
  },
  { // Bishop
   { S(-53,-57), S( -5,-30), S( -8,-37), S(-23,-12) },
   { S(-15,-37), S(  8,-13), S( 19,-17), S(  4,  1) },
   { S( -7,-16), S( 21, -1), S( -5, -2), S( 17, 10) },
   { S( -5,-20), S( 11, -6), S( 25,  0), S( 39, 17) },
   { S(-12,-17), S( 29, -1), S( 22,-14), S( 31, 15) },
   { S(-16,-30), S(  6,  6), S(  1,  4), S( 11,  6) },
   { S(-17,-31), S(-14,-20), S(  5, -1), S(  0,  1) },
   { S(-48,-46), S(  1,-42), S(-14,-37), S(-23,-24) }
  },
  { // Rook
   { S(-26, -7), S(-19,-15), S( -8,-10), S(-1,-10) },
   { S(-21,-19), S(-24, -7), S( -6,  0), S( 5, -1) },
   { S(-29, -1), S(-20, -4), S(  6,  8), S(12,-19) },
   { S(-14, -1), S( -1, -3), S(-12, -8), S(-7, 10) },
   { S(-37,  1), S( -6, 13), S(  0, 14), S(10,  4) },
   { S(-16,  9), S( -1, 15), S(  4, -8), S( 1, 16) },
   { S( -1,  2), S( 10,  4), S( 13, 23), S(23, -4) },
   { S( -2, 21), S(-21, -3), S(  4, 26), S(-1, 20) }
  },
  { // Queen
   { S( -2,-72), S( -6,-44), S(-18,-53), S(26,-58) },
   { S(  9,-89), S( 14,-39), S( 10,-13), S(29,-19) },
   { S(-29,-15), S(  7,-17), S( 32,-16), S(23,-25) },
   { S(  5,-25), S( 10,  1), S( 33, 26), S(35,  6) },
   { S( -2,-11), S( 15,  9), S( 23, 13), S(35, 16) },
   { S( 27,-43), S(  1, -8), S( 40,-13), S(-9, -6) },
   { S(-16,-47), S(-16, -5), S( 10,-40), S(16,-26) },
   { S(-33,-85), S( 12,-75), S( 32,-64), S(10,-13) }
  },
  { // King
   { S(271,  1), S(327, 45), S(270, 85), S(192, 76) },
   { S(278, 53), S(303,100), S(230,133), S(174,135) },
   { S(195, 88), S(258,130), S(169,169), S(120,175) },
   { S(164,103), S(190,156), S(138,172), S( 98,172) },
   { S(154, 96), S(179,166), S(105,199), S( 70,199) },
   { S(123, 92), S(145,172), S( 81,184), S( 31,191) },
   { S( 88, 47), S(120,121), S( 65,116), S( 33,131) },
   { S( 59, 11), S( 89, 59), S( 45, 73), S( -1, 78) }
  }
};

constexpr Score PBonus[RANK_NB][FILE_NB] =
  { // Pawn (asymmetric distribution)
   { },
   { S(  3,-10), S(  3, -6), S( 10, 10), S( 19,  0), S( 16, 14), S( 19,  7), S(  7, -5), S( -5,-19) },
   { S( -9,-10), S(-15,-10), S( 11,-10), S( 15,  4), S( 32,  4), S( 22,  3), S(  5, -6), S(-22, -4) },
   { S( -8,  6), S(-23, -2), S(  6, -8), S( 20, -4), S( 40,-13), S( 17,-12), S(  4,-10), S(-12, -9) },
   { S( 13,  9), S(  0,  4), S(-13,  3), S(  1,-12), S( 11,-12), S( -2, -6), S(-13, 13), S(  5,  8) },
   { S( -5, 28), S(-12, 20), S( -7, 21), S( 22, 28), S( -8, 30), S( -5,  7), S(-15,  6), S(-18, 13) },
   { S( -7,  0), S(  7,-11), S( -3, 12), S(-13, 21), S(  5, 25), S(-16, 19), S( 10,  4), S( -8,  7) }
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
          File f = map_to_queenside(file_of(s));
          psq[ pc][ s] = score + (type_of(pc) == PAWN ? PBonus[rank_of(s)][file_of(s)]
                                                      : Bonus[pc][rank_of(s)][f]);
          psq[~pc][~s] = -psq[pc][s];
      }
  }
}

} // namespace PSQT
