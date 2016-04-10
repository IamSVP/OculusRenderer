
#include "GL/CAPI_GLE.h"

#include "OVR_CAPI_GL.h"

#include <Extras\OVR_Math.h>

#include "ObjLoader.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include "GL/CAPI_GLE.h"
#include "Extras/OVR_Math.h"
#include "Kernel/OVR_Log.h"
#include "OVR_CAPI_GL.h"
#include "TextureLoader.h"
#include <vector>
using namespace OVR;

struct Color
{
	unsigned char R, G, B, A;

	Color(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0, unsigned char a = 0xff)
		: R(r), G(g), B(b), A(a)
	{ }
};

struct Vertex
{
	Vector3f  Pos;
	Vector3f Normal;
	Color     C;
	float     U, V;
};

class Model{
public:
	Model(const char * imagepath);
	Model(const char * imagepath, bool dynamic);
	Model();
	Model(bool dynamic);
	~Model();
	void AllocateVertexBuffers();
	void RenderModel(Matrix4f view, Matrix4f proj);
	void LoadTexture();
	void LoadShaders(const char * vertex_file_path, const char * fragment_file_path);




	void InitializeTexture();
	void InitializeCompressedTexture();

	bool LoadTextureData(const char * imagepath);
	bool LoadTextureData_JPG(const char * imagepath);
	bool LoadTextureDataPBO(const char *imagepath);
	bool CompressImage(const char *imagepath);
	bool LoadCompressedTexture(const char *imagepath);
	bool LoadCRNCompressedTexture(const char *imagepath);
	void RenderDynamicModel(Matrix4f view, Matrix4f proj);


	Vector3f Postion;
	Quatf Rotation;
	Matrix4f ModelMatrix;
	
	

	std::vector<unsigned short> indices;
	std::vector<Vector3f> indexed_vertices;
	std::vector<Vector2f> indexed_uvs;
	std::vector<Vector3f> indexed_normals;

	GLuint ProgramID;
	GLuint VertexBuffer;
	GLuint UVBuffer;
	GLuint NormalBuffer;
	GLuint IndexBuffer;
	GLuint TextureID;
	GLuint vertexArrayId;
	GLbyte* TextureData;
	GLuint PboID[2];
	bool DynamicModel;
	int numIndices;
	int TextureNumber;
};


