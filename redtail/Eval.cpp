#include "Board.h"
#include "Eval.h"
#include <iostream>
#include <chrono>
#include <unordered_map>

#define INFINITY 30000
#define MATE 29000
#define MAX_DEPTH 100

int get64pos(int pos)
{
	return (pos % 8) + (floor(pos / 16) * 8);
}

int get_time_ms()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

bool Board::is_capture(int move) {
	int piece_at_dest = (move >> 4) & 0xf;
	return piece_at_dest != Empty && piece_at_dest != OffBoard;
}

std::vector<S_MOVE> order_moves(std::vector<S_MOVE> moves) {
	return moves;
}


int Board::quiesce(int alpha, int beta) {
	int stand_pat = get_score();
	if (stand_pat >= beta) return beta;
	if (alpha < stand_pat) alpha = stand_pat;

	std::vector<S_MOVE> moves = order_moves(generate_moves());

	for (S_MOVE move : moves) {
		if (is_capture(move.move)) {
			make_move(move.move);
			int score = -quiesce(-beta, -alpha);
			undo_last_move();

			if (score >= beta) return beta;
			if (score > alpha) alpha = score;
		}
	}

	return alpha;
}

int Board::alpha_beta(int alpha, int beta, int depth) {
	int best_score = -INFINITY;

	u64 pos_key = position_key();

	if (transposition_table.find(pos_key) != transposition_table.end()) {
		TT_ENTRY stored = transposition_table[pos_key];
		if (stored.depth >= depth) {
			if (stored.flag == TT_EXACT) return stored.score;
			if (stored.flag == TT_ALPHA && stored.score <= alpha) return alpha;
			if (stored.flag == TT_BETA && stored.score >= beta) return beta;
		}
	}

	if (depth == 0) {
		nodes++;
		int val = quiesce(alpha, beta);
		transposition_table.insert({ pos_key, {TT_EXACT, val, depth} });
		return val;
	}

	std::vector<S_MOVE> moves = generate_moves();

	for (S_MOVE move : moves) {
		make_move(move.move);
		int score = -alpha_beta(-beta, -alpha, depth - 1);
		undo_last_move();

		if (score >= beta) {
			transposition_table.insert({ pos_key, {TT_BETA, beta, depth} });
			return beta;
		}

		if (score >= best_score) {
			best_score = score;
		}

		if (score > alpha) {
			alpha = score;
		}
	}

	transposition_table.insert({ pos_key, {TT_ALPHA, alpha, depth} });
	return alpha;
}

int Board::search() {

	int start_time = get_time_ms();

	S_MOVE best_move;
	int best_score = -INFINITY;

	int alpha = -INFINITY;
	int beta = INFINITY;
	int depth = 1;

	std::vector<S_MOVE> moves = generate_moves();

	nodes = 0;

	while (true) {
		int curr_time = get_time_ms();
		if (curr_time - start_time > 20000 || depth > MAX_DEPTH) break;

		for (S_MOVE move : moves)
		{
			make_move(move.move);
			int score = -alpha_beta(-beta, -alpha, depth);
			undo_last_move();

			// std::cout << get_move_ref(move.move) << " " << score << " " << turn << std::endl;

			if (score > best_score) {
				best_move = move;
				best_move.score = score;
				best_score = score;
			}

			if (score > alpha) {
				alpha = score;
			}
		}

		std::cout << "info depth " << depth << " time " << curr_time - start_time << " nodes " << nodes << " score cp " << best_score << " currmove " << get_move_ref(best_move.move) << std::endl;

		// std::cout << "info bestmove " << get_move_ref(best_move.move) << " score " << best_move.score << " depth " << depth << " nodes 0 time " << curr_time - start_time << std::endl;

		depth++;
	}

	return best_move.move;
}

int Board::get_score()
{
	int score = 0;
	for (int i = 0; i < squares.size(); ++i)
	{
		int piece = squares[i];
		if (piece == Empty || (i & 0x88) != 0)
			continue;

		switch (piece)
		{
		case WhiteKing:
			score += 50000;
			break;
		case WhitePawn:
			score += 100; // +PawnTable[get64pos(i)];
			break;
		case WhiteRook:
			score += 500; // +RookTable[get64pos(i)];
			break;
		case WhiteBishop:
			score += 330; // +BishopTable[get64pos(i)];
			break;
		case WhiteQueen:
			score += 900; // +BishopTable[get64pos(i)] + RookTable[get64pos(i)];
			break;
		case WhiteKnight:
			score += 320; // +KnightTable[get64pos(i)];
			break;

		case BlackKing:
			score -= 50000;
			break;
		case BlackPawn:
			score -= 100; // +PawnTable[Mirror64[get64pos(i)]]);
			break;
		case BlackRook:
			score -= 500; // +RookTable[Mirror64[get64pos(i)]]);
			break;
		case BlackBishop:
			score -= 330; // +BishopTable[Mirror64[get64pos(i)]]);
			break;
		case BlackQueen:
			score -= 900; // +BishopTable[Mirror64[get64pos(i)]] + RookTable[Mirror64[get64pos(i)]]);
			break;
		case BlackKnight:
			score -= 320; // +KnightTable[Mirror64[get64pos(i)]]);
			break;
		}
	}

	if (turn == White) return score;
	else return -score;
}

