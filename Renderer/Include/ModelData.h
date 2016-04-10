#ifndef MODEL_DATA
#define MODEL_DATA

#include <Extras\OVR_Math.h>
#include <vector>
using namespace OVR;




struct ModelData{

	std::vector<Vector3f> Vertices;
	std::vector<unsigned short> Indices;

};

#endif