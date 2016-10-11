#include "ObjLoader.h"

int add(int x, int y){
	return x + y;
}
bool ObjLoader::loadOBJ(
	const char * path,
	std::vector<OVR::Vector3f> & out_vertices,
	std::vector<OVR::Vector2f> & out_uvs,
	std::vector<OVR::Vector3f> & out_normals
	){
	printf("Loading OBJ file %s...\n", path);
	
	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<OVR::Vector3f> temp_vertices;
	std::vector<OVR::Vector2f> temp_uvs;
	std::vector<OVR::Vector3f> temp_normals;


	FILE * file = fopen(path, "r");
	if (file == NULL){
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while (1){

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

		// else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0){
			OVR::Vector3f vertex;

			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0){
			OVR::Vector2f uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y;
			uv.x = -uv.x;// Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0){
			OVR::Vector3f normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0){
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9){
				printf("File can't be read by our simple parser :-( Try exporting with other options\n");
				return false;
			}
			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);
		}
		else{
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for (unsigned int i = 0; i<vertexIndices.size(); i++){

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		OVR::Vector3f vertex = temp_vertices[vertexIndex - 1];
		OVR::Vector2f uv = temp_uvs[uvIndex - 1];
		OVR::Vector3f normal = temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);

	}

	return true;
}



//----------------------------Indexing code -----------------------------------//


//----------------------------Helper functions for Indexing-----------------------//
// Returns true iif v1 can be considered equal to v2
bool is_near(float v1, float v2){
	return fabs(v1 - v2) < 0.01f;
}

// Searches through all already-exported vertices
// for a similar one.
// Similar = same position + same UVs + same normal
bool getSimilarVertexIndex(
	OVR::Vector3f & in_vertex,
	OVR::Vector2f & in_uv,
	OVR::Vector3f & in_normal,
	std::vector<OVR::Vector3f> & out_vertices,
	std::vector<OVR::Vector2f> & out_uvs,
	std::vector<OVR::Vector3f> & out_normals,
	unsigned short & result
	){
	// Lame linear search
	for (unsigned int i = 0; i<out_vertices.size(); i++){
		if (
			is_near(in_vertex.x, out_vertices[i].x) &&
			is_near(in_vertex.y, out_vertices[i].y) &&
			is_near(in_vertex.z, out_vertices[i].z) &&
			is_near(in_uv.x, out_uvs[i].x) &&
			is_near(in_uv.y, out_uvs[i].y) &&
			is_near(in_normal.x, out_normals[i].x) &&
			is_near(in_normal.y, out_normals[i].y) &&
			is_near(in_normal.z, out_normals[i].z)
			){
			result = i;
			return true;
		}
	}
	// No other vertex could be used instead.
	// Looks like we'll have to add it to the VBO.
	return false;
}

void indexVBO_slow(
	std::vector<OVR::Vector3f> & in_vertices,
	std::vector<OVR::Vector2f> & in_uvs,
	std::vector<OVR::Vector3f> & in_normals,

	std::vector<unsigned short> & out_indices,
	std::vector<OVR::Vector3f> & out_vertices,
	std::vector<OVR::Vector2f> & out_uvs,
	std::vector<OVR::Vector3f> & out_normals
	){
	// For each input vertex
	for (unsigned int i = 0; i<in_vertices.size(); i++){

		// Try to find a similar vertex in out_XXXX
		unsigned short index;
		bool found = getSimilarVertexIndex(in_vertices[i], in_uvs[i], in_normals[i], out_vertices, out_uvs, out_normals, index);

		if (found){ // A similar vertex is already in the VBO, use it instead !
			out_indices.push_back(index);
		}
		else{ // If not, it needs to be added in the output data.
			out_vertices.push_back(in_vertices[i]);
			out_uvs.push_back(in_uvs[i]);
			out_normals.push_back(in_normals[i]);
			out_indices.push_back((unsigned short)out_vertices.size() - 1);
		}
	}
}

struct PackedVertex{
	OVR::Vector3f position;
	OVR::Vector2f uv;
	OVR::Vector3f normal;
	bool operator<(const PackedVertex that) const{
		return memcmp((void*)this, (void*)&that, sizeof(PackedVertex))>0;
	};
};

bool getSimilarVertexIndex_fast(
	PackedVertex & packed,
	std::map<PackedVertex, unsigned short> & VertexToOutIndex,
	unsigned short & result
	){
	std::map<PackedVertex, unsigned short>::iterator it = VertexToOutIndex.find(packed);
	if (it == VertexToOutIndex.end()){
		return false;
	}
	else{
		result = it->second;
		return true;
	}
}

//---------------------------------END of helper functions----------------------//



void ObjLoader:: indexVBO(
	std::vector<OVR::Vector3f> & in_vertices,
	std::vector<OVR::Vector2f> & in_uvs,
	std::vector<OVR::Vector3f> & in_normals,

	std::vector<unsigned short> & out_indices,
	std::vector<OVR::Vector3f> & out_vertices,
	std::vector<OVR::Vector2f> & out_uvs,
	std::vector<OVR::Vector3f> & out_normals
	){
	std::map<PackedVertex, unsigned short> VertexToOutIndex;

	// For each input vertex
	for (unsigned int i = 0; i<in_vertices.size(); i++){

		PackedVertex packed = { in_vertices[i], in_uvs[i], in_normals[i] };


		// Try to find a similar vertex in out_XXXX
		unsigned short index;
		bool found = getSimilarVertexIndex_fast(packed, VertexToOutIndex, index);

		if (found){ // A similar vertex is already in the VBO, use it instead !
			out_indices.push_back(index);
		}
		else{ // If not, it needs to be added in the output data.
			out_vertices.push_back(in_vertices[i]);
			out_uvs.push_back(in_uvs[i]);
			out_normals.push_back(in_normals[i]);
			unsigned short newindex = (unsigned short)out_vertices.size() - 1;
			out_indices.push_back(newindex);
			VertexToOutIndex[packed] = newindex;
		}
	}
}



void ObjLoader:: indexVBO_TBN(
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
	){
	// For each input vertex
	for (unsigned int i = 0; i<in_vertices.size(); i++){

		// Try to find a similar vertex in out_XXXX
		unsigned short index;
		bool found = getSimilarVertexIndex(in_vertices[i], in_uvs[i], in_normals[i], out_vertices, out_uvs, out_normals, index);

		if (found){ // A similar vertex is already in the VBO, use it instead !
			out_indices.push_back(index);

			// Average the tangents and the bitangents
			out_tangents[index] += in_tangents[i];
			out_bitangents[index] += in_bitangents[i];
		}
		else{ // If not, it needs to be added in the output data.
			out_vertices.push_back(in_vertices[i]);
			out_uvs.push_back(in_uvs[i]);
			out_normals.push_back(in_normals[i]);
			out_tangents.push_back(in_tangents[i]);
			out_bitangents.push_back(in_bitangents[i]);
			out_indices.push_back((unsigned short)out_vertices.size() - 1);
		}
	}
}

//-----End of indexing functions--------------//



#ifdef USE_ASSIMP
//-------------------------AssIMP functions-------------------//

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


////------------------------------Helper Functions-------------------------//
bool ObjLoader
::loadAssImp(
	const char * path,
	std::vector<unsigned short> & indices,
	std::vector<OVR::Vector3f> & vertices,
	std::vector<OVR::Vector2f> & uvs,
	std::vector<OVR::Vector3f> & normals
	){

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path, 0/*aiProcess_JoinIdenticalVertices | aiProcess_SortByPType*/);
	if (!scene) {
		fprintf(stderr, importer.GetErrorString());
		getchar();
		return false;
	}
	const aiMesh* mesh = scene->mMeshes[0]; // In this simple example code we always use the 1rst mesh (in OBJ files there is often only one anyway)

	// Fill vertices positions
	vertices.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i<mesh->mNumVertices; i++){
		aiVector3D pos = mesh->mVertices[i];
		vertices.push_back(OVR::Vector3f(pos.x, pos.y, pos.z));
	}

	// Fill vertices texture coordinates
	uvs.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i<mesh->mNumVertices; i++){
		aiVector3D UVW = mesh->mTextureCoords[0][i]; // Assume only 1 set of UV coords; AssImp supports 8 UV sets.
		uvs.push_back(OVR::Vector2f(UVW.x, UVW.y));
	}

	// Fill vertices normals
	if (mesh->mNormals != NULL){


		normals.reserve(mesh->mNumVertices);

		for (unsigned int i = 0; i < mesh->mNumVertices; i++){
			aiVector3D n = mesh->mNormals[i];
			normals.push_back(OVR::Vector3f(n.x, n.y, n.z));
		}

	}

	// Fill face indices
	indices.reserve(3 * mesh->mNumFaces);
	for (unsigned int i = 0; i<mesh->mNumFaces; i++){
		// Assume the model has only triangles.
		indices.push_back(mesh->mFaces[i].mIndices[0]);
		indices.push_back(mesh->mFaces[i].mIndices[1]);
		indices.push_back(mesh->mFaces[i].mIndices[2]);
	}

	// The "scene" pointer will be deleted automatically by "importer"

}

#endif