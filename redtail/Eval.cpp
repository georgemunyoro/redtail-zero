#include "Board.h"
#include "Eval.h"
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <algorithm>

#define INFINITY 30000
#define MATE 29900

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

std::vector<S_MOVE> Board::ordered_moves() {
	std::vector<S_MOVE> unordered_moves = generate_moves();
	std::sort(unordered_moves.begin(), unordered_moves.end(), [](const S_MOVE& x, const S_MOVE& y) {
		return x.score > y.score;
	});
	return unordered_moves;
}

int Board::quiesce(int alpha, int beta) {

	if (search_info.nodes & 2047) {
		if ((get_time_ms() - search_info.starttime) > search_info.timeset) search_info.stopped = true;
	}

	search_info.nodes++;

	int stand_pat = get_score();
	if (stand_pat >= beta) return beta;
	if (alpha < stand_pat) alpha = stand_pat;

	std::vector<S_MOVE> moves = ordered_moves();

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

#define R 2

int Board::alpha_beta(int alpha, int beta, int depth, bool do_null) {
	if (search_info.nodes & 2047) {
		if ((get_time_ms() - search_info.starttime) > search_info.timeset) search_info.stopped = true;
	}

	if (search_info.stopped) return 0;

	int best_score = -INFINITY;
	int best_move = 0;
	int old_alpha = alpha;

	u64 pos_key = position_key();

	search_info.nodes++;

	if (transposition_table.find(pos_key) != transposition_table.end()) {
		TT_ENTRY stored = transposition_table[pos_key];
		if (stored.depth >= depth) {
			if (stored.flag == TT_EXACT) return stored.score;
			if (stored.flag == TT_ALPHA && stored.score <= alpha) return alpha;
			if (stored.flag == TT_BETA && stored.score >= beta) return beta;
		}
	}

	if (depth == 0) {
		int val = quiesce(alpha, beta);
		transposition_table.insert({ pos_key, {TT_EXACT, val, depth, 0} });
		return val;
	}

	if (do_null && ply > 0 && !in_check() && depth >= 4) {
		make_null_move();
		int null_move_val = -alpha_beta(-beta, -beta + 1, depth - 4, false);
		undo_null_move();

		if (search_info.stopped) return 0;
		if (null_move_val >= beta) {
			transposition_table.insert({ pos_key, {TT_BETA, beta, depth, 0} });
			return beta;
		}
			
	}

	std::vector<S_MOVE> moves = ordered_moves();

	for (S_MOVE move : moves) {
		make_move(move.move);
		int score = -alpha_beta(-beta, -alpha, depth - 1, !do_null);
		undo_last_move();

		if (search_info.stopped) return 0;

		if (score > alpha) {
			alpha = score;

			store_pv_move(move.move);

			if (score >= beta) {
				transposition_table.insert({ pos_key, {TT_BETA, beta, depth, move.move} });
				return beta;
			}
		}
	}

	transposition_table.insert({ pos_key, {TT_ALPHA, alpha, depth, best_move} });
	return alpha;
}

void Board::clear_for_search() {
	search_info.stopped = false;
	search_info.nodes = 0;
	search_info.depth = 1;
	search_info.timeset = 5000;
	search_info.starttime = get_time_ms();

	clear_pv_table();
}

int Board::search() {
	int best_score = -INFINITY;
	int score = -INFINITY;

	int alpha = -INFINITY;
	int beta = INFINITY;

	clear_for_search();

	int last_pv_array[100];

	int best_move;

	while (true) {
		int curr_time = get_time_ms();
		if (search_info.stopped || search_info.depth > MAX_DEPTH) break;

		score = alpha_beta(alpha, beta, search_info.depth, true);

		if (score > best_score) best_score = score;
		else {
			if (search_info.stopped) break;
		}

		// if (best_score <= alpha || best_score >= beta) {
		// 	alpha = -INFINITY;
		// 	beta = INFINITY;
		// 	continue;
		// }

		// alpha = best_score - 50;
		// beta = best_score + 50;

		int pv_moves = get_pv_line(search_info.depth);
		best_move = pv_array[0];

		std::cout << "info depth " << search_info.depth << " time " << curr_time - search_info.starttime << " nodes " << search_info.nodes << " score cp " << score << " currmove " << get_move_ref(pv_array[0]);

		printf(" pv");
		for (int pv_num = 0; pv_num < pv_moves; ++pv_num)
		{
			printf(" %s", get_move_ref(pv_array[pv_num]).c_str());
		}
		printf("\n");

		search_info.depth++;
	}

	return best_move;
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
			score += 100  +PawnTable[get64pos(i)];
			break;
		case WhiteRook:
			score += 500  +RookTable[get64pos(i)];
			break;
		case WhiteBishop:
			score += 330  +BishopTable[get64pos(i)];
			break;
		case WhiteQueen:
			score += 900  +BishopTable[get64pos(i)] + RookTable[get64pos(i)];
			break;
		case WhiteKnight:
			score += 320  +KnightTable[get64pos(i)];
			break;

		case BlackKing:
			score -= 50000;
			break;
		case BlackPawn:
			score -= 100  +PawnTable[Mirror64[get64pos(i)]];
			break;
		case BlackRook:
			score -= 500  +RookTable[Mirror64[get64pos(i)]];
			break;
		case BlackBishop:
			score -= 330  +BishopTable[Mirror64[get64pos(i)]];
			break;
		case BlackQueen:
			score -= 900  +BishopTable[Mirror64[get64pos(i)]] + RookTable[Mirror64[get64pos(i)]];
			break;
		case BlackKnight:
			score -= 320  +KnightTable[Mirror64[get64pos(i)]];
			break;
		}
	}

	if (turn == White) return score;
	else return -score;
}

