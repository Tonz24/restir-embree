#include "stdafx.h"
#include "tutorials.h"


int main() {
	
	_MM_SET_FLUSH_ZERO_MODE( _MM_FLUSH_ZERO_ON );
	_MM_SET_DENORMALS_ZERO_MODE( _MM_DENORMALS_ZERO_ON );

	//return tutorial_1();
	//return tutorial_2();
	//return tutorial_3( "../../../data/avenger/6887_allied_avenger.obj" );
	//return tutorial_3("../../../data/cornell_box/cornell_box2.obj");
	//return tutorial_3("../../../data/ris/street_lamps/street.mtl.obj");
	//return tutorial_3("../../../data/ris/mis_test/mis.obj");

	//return tutorial_3("../../../data/room/room.obj");
	return tutorial_3("../../../data/living_room/living_room.obj");


	//return tutorial_3("../../../data/ris/mis_test/lightSampleDebug.obj");
	//return tutorial_3( "../../../data/geosphere.obj" );
	//return tutorial_3( "../../../data/cube.obj" );
	//return tutorial_3( "../../../data/chair/chair.obj" );
	//return tutorial_3( "../../../data/triangle/triangle.obj" );
	//return tutorial_3( "../../../data/quad/quad.obj" );
}