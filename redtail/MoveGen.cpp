#include <vector>
#include <iostream>
#include "Board.h"
#include "MoveGen.h"
#pragma	once

#define createMove(f, t, pi, pt, tp) ((f << 20) | (t << 12) | (pi << 8) | (pt << 4) | tp)

std::vector<S_MOVE> Board::generate_moves()
{
	std::vector<S_MOVE> legal_moves;
	std::vector<S_MOVE> pseudo_moves = generate_pseudo_moves();

	for (S_MOVE move : pseudo_moves)
	{
		make_move(move.move);
		switch_turn();

		if (!in_check()) legal_moves.push_back(move);

		switch_turn();
		undo_last_move();
	}

	return legal_moves;
}

std::vector<S_MOVE> Board::generate_pseudo_moves()
{
	std::vector<S_MOVE> moves;
	for (int pos = 0; pos < squares.size(); ++pos) {
		int piece = squares[pos];

		if ((pos & 0x88) != 0 || get_color(piece) != turn) continue;

		// Pawn Move Generation
		if (piece == WhitePawn)
		{
			int dest = pos + N;
			if ((dest & 0x88) == 0) {
				if (squares[dest] == Empty) {
					moves.push_back({ (pos << 20) | ((pos + N) << 12) | (squares[pos] << 8) | (squares[pos + N] << 4) | 0, 0 });

					dest = pos + N + N;
					if ((dest & 0x88) == 0 && floor(pos / 16) == 6) {
						if (squares[dest] == Empty) {
							moves.push_back({ (pos << 20) | ((pos + N + N) << 12) | (squares[pos] << 8) | (squares[pos + N + N] << 4) | 0, 0 });
						}
					}
				}
			}

			dest = pos + N + E;
			if ((dest & 0x88) == 0) {
				if (get_color(squares[pos + N + E]) == Black && ((pos + N + E) & 0x88) == 0) {
					moves.push_back({ (pos << 20) | ((pos + N + E)) << 12 | (squares[pos] << 8) | (squares[pos + N + E] << 4) | 1, mvv_lva_scores[squares[pos + N + E]][WhitePawn]});
				}
			}

			dest = pos + N + W;
			if ((dest & 0x88) == 0)
			{
				if (get_color(squares[pos + N + W]) == Black && ((pos + N + W) & 0x88) == 0) {
					moves.push_back({(pos << 20) | ((pos + N + W)) << 12 | (squares[pos] << 8) | (squares[pos + N + W] << 4) | 1, mvv_lva_scores[squares[pos + N + W]][WhitePawn]});
				}
			}
		}

		else if (piece == BlackPawn)
		{
			int dest = pos + S;
			if ((dest & 0x88) == 0) {
				if (squares[dest] == Empty) {
					moves.push_back({(pos << 20) | (dest << 12) | (squares[pos] << 8) | (squares[dest] << 4) | 0, 0 });
					dest = pos + S + S;
					if ((dest & 0x88) == 0 && floor(pos / 16) == 1) {
						if (squares[dest] == Empty) {
							moves.push_back({ (pos << 20) | ((dest) << 12) | (squares[pos] << 8) | (squares[dest] << 4) | 0, 0 });
						}
					}
				}
			}

			dest = pos + S + E;
			if ((dest & 0x88) == 0) {
				if (get_color(squares[pos + S + E]) == White) {
					moves.push_back({(pos << 20) | ((pos + S + E)) << 12 | (squares[pos] << 8) | (squares[pos + S + E] << 4) | 1, mvv_lva_scores[squares[pos + S + E]][BlackPawn]});
				}
			}

			dest = pos + S + W;
			if ((dest & 0x88) == 0)
			{
				if (get_color(squares[pos + S + W]) == White) {
					moves.push_back({(pos << 20) | ((pos + S + W)) << 12 | (squares[pos] << 8) | (squares[pos + S + W] << 4) | 1, mvv_lva_scores[squares[pos + S + W]][BlackPawn]});
				}
			}
		}

		// King Move Generation
		if (piece == WhiteKing || piece == BlackKing) {
			for (int direction : KingDirections)
			{
				int dest = pos + direction;
				if ((dest & 0x88) == 0 && get_color(squares[dest]) != turn && !is_square_attacked(dest, ~turn)) {
					moves.push_back({ (pos << 20) | ((dest)) << 12 | (squares[pos] << 8) | (squares[dest] << 4) | 0, mvv_lva_scores[squares[dest]][piece] });
				}
			}
		}

		// Knight Move Generation
		if (piece == WhiteKnight || piece == BlackKnight) {
			for (int direction : KnightDirections)
			{
				int dest = pos + direction;
				if ((dest & 0x88) == 0 && get_color(squares[dest]) != turn) {
					moves.push_back({(pos << 20) | ((dest)) << 12 | (squares[pos] << 8) | (squares[dest] << 4) | 0, mvv_lva_scores[squares[dest]][piece]});
				}
			}
		}

		// Rook + Partial Queen Move Generation
		if (piece == WhiteRook || piece == WhiteQueen || piece == BlackRook || piece == BlackQueen) {
			for (int direction : RookDirections) {
				int dest = pos + direction;
				while ((dest & 0x88) == 0 && get_color(squares[dest]) != turn) {
					moves.push_back({(pos << 20) | ((dest) << 12) | (squares[pos] << 8) | (squares[dest] << 4) | 0, mvv_lva_scores[squares[dest]][piece]});
					if (squares[dest] != Empty) break;
					dest += direction;
				}
			}
		}

		// Bishop + Partial Queen Move Generation
		if (piece == WhiteBishop || piece == WhiteQueen || piece == BlackBishop || piece == BlackQueen) {
			for (int direction : BishopDirections) {
				int dest = pos + direction;
				while ((dest & 0x88) == 0 && get_color(squares[dest]) != turn) {
					moves.push_back({(pos << 20) | ((dest) << 12) | (squares[pos] << 8) | (squares[dest] << 4) | 0, mvv_lva_scores[squares[dest]][piece]});
					if (squares[dest] != Empty) break;
					dest += direction;
				}
			}
		}
	}
	return moves;
}

bool Board::is_square_attacked(int pos, int attacker) {
	if (attacker == Black)
	{
		if (((pos + N + E) & 0x88) == 0) {
			if (squares[pos + N + E] == BlackPawn) return true;
		}
		if (((pos + N + W) & 0x88) == 0) {
			if (squares[pos + N + W] == BlackPawn) return true;
		}
	}

	else if (attacker == White)
	{
		if (((pos + S + E) & 0x88) == 0) {
			if (squares[pos + S + E] == WhitePawn) return true;
		}
		if (((pos + S + W) & 0x88) == 0) {
			if (squares[pos + S + W] == WhitePawn) return true;
		}
	}

	for (int direction : KnightDirections) {
		if (((pos + direction) & 0x88) != 0) continue;
		int piece = squares[pos + direction];
		if ((piece == WhiteKnight || piece == BlackKnight) && get_color(piece) == attacker) return true;
	}

	for (int direction : KingDirections) {
		if (((pos + direction) & 0x88) != 0) continue;
		int piece = squares[pos + direction];
		if ((piece == WhiteKing || piece == BlackKing) && get_color(piece) == attacker) return true;
	}

	for (int direction : RookDirections) {
		int index = pos;
		while (((index + direction) & 0x88) == 0) {
			index += direction;
			int piece = squares[index];

			if (piece != Empty) {
				if ((piece == WhiteRook || piece == BlackRook || piece == WhiteQueen || piece == BlackQueen) && get_color(piece) == attacker) return true;
				break;
			}
		}
	}

	for (int direction : BishopDirections) {
		int index = pos;
		while (((index + direction) & 0x88) == 0) {
			index += direction;
			int piece = squares[index];

			if (piece != Empty) {
				if ((piece == WhiteBishop || piece == BlackBishop || piece == WhiteQueen || piece == BlackQueen) && get_color(piece) == attacker) return true;
				break;
			}
		}
	}

	return false;
}
