#include "stdafx.h"
#include "tutorials.h"


int main() {
	
	_MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
	_MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );

	return tutorial_3("../../../data/room/room.obj");
	//return tutorial_3("../../../data/living_room/living_room.obj");
}