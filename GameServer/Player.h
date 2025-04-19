#pragma once
#include "GameSessionManager.h"


class Player
{
public:
	uint64					playerId = 0;
	GameSessionRef			ownerSession; // Cycle
};
