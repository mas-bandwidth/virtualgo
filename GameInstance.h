#ifndef GAME_INSTANCE_H
#define GAME_INSTANCE_H

#include "Physics.h"
#include "Board.h"
#include "StoneData.h"
#include "StoneInstance.h"

class GameInstance
{
public:

	GameInstance()
	{
        #if LOCKED
		locked = true;
        #else
        locked = false;
        #endif
		paused = false;
		zoomed = false;
		smoothZoom = 10;
	}

    Board board;

    StoneData stoneData;

    std::vector<StoneInstance> stones;

    vec3f lightPosition;

    bool locked;
    bool paused;
    bool zoomed;
    float smoothZoom;
};

#endif