
#include <vector>
#include <stdio.h>
#include <string>
#include <cstring>
#include <Extras/OVR_Math.h>

#include <map>
class ObjLoader{

public:
	static bool loadOBJ(
	const char * path,
	std::vector<OVR::Vector3f> & out_vertices,
	std::vector<OVR::Vector2f> & out_uvs,
	std::vector<OVR::Vector3f> & out_normals
	);



	static bool loadAssImp(
		const char * path,
		std::vector<unsigned short> & indices,
		std::vector<OVR::Vector3f> & vertices,
		std::vector<OVR::Vector2f> & uvs,
		std::vector<OVR::Vector3f> & normals
		);

	static void indexVBO(
		std::vector<OVR::Vector3f> & in_vertices,
		std::vector<OVR::Vector2f> & in_uvs,
		std::vector<OVR::Vector3f> & in_normals,

		std::vector<unsigned short> & out_indices,
		std::vector<OVR::Vector3f> & out_vertices,
		std::vector<OVR::Vector2f> & out_uvs,
		std::vector<OVR::Vector3f> & out_normals
		);


	static void indexVBO_TBN(
		std::vector<OVR::Vector3f> & in_vertices,
		std::vector<OVR::Vector2f> & in_uvs,
		std::vector<OVR::Vector3f> & in_normals,
		std::vector<OVR::Vector3f> & in_tangents,
		std::vector<OVR::Vector3f> & in_bitangents,

		std::vector<unsigned short> & out_indices,
		std::vector<OVR::Vector3f> & out_vertices,
		std::vector<OVR::Vector2f> & out_uvs,
		std::vector<OVR::Vector3f> & out_normals,
		std::vector<OVR::Vector3f> & out_tangents,
		std::vector<OVR::Vector3f> & out_bitangents
		);
};

