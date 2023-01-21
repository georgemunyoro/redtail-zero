#include "Board.h"
#include <vector>
#include <string>
#include <unordered_map>
#pragma once

static int N = -16;
static int E = 1;
static int S = 16;
static int W = -1;

static int KnightDirections[8] = {N + N + E, E + E + N, S + S + E, W + W + S, S + E + E, S + S + W, N + N + W, N + W + W};
static int KingDirections[8] = {N, W, N + W, S + W, S, E, E + S, N + E};
static int BishopDirections[4] = {N + E, E + S, S + W, W + N};
static int RookDirections[4] = {N, E, S, W};

static std::unordered_map<int, std::vector<int>> PieceMovements = {
	{WhiteKnight, {N + N + E, E + E + N, S + S + E, W + W + S, S + E + E, S + S + W, N + N + W, N + W + W}},
	{WhiteQueen, {N, W, N + W, S + W, S, E, E + S, N + E}},
	{WhiteRook, {N, E, S, W}},
	{WhiteKing, {N, W, N + W, S + W, S, E, E + S, N + E}},
	{WhiteBishop, {N + E, E + S, S + W, W + N}},
	{BlackKnight, {N + N + E, E + E + N, S + S + E, W + W + S, S + E + E, S + S + W, N + N + W, N + W + W}},
	{BlackQueen, {N, W, N + W, S + W, S, E, E + S, N + E}},
	{BlackRook, {N, E, S, W}},
	{BlackKing, {N, W, N + W, S + W, S, E, E + S, N + E}},
	{BlackBishop, {N + E, E + S, S + W, W + N}} };
