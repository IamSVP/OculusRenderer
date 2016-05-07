
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



#include "gpu.h"
#include "codec.h"
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

typedef unsigned long long ull;

class Model{
public:
	Model(const char * imagepath);
	Model(const char * imagepath, bool dynamic);
	Model();
	Model(bool dynamic);
	~Model();
	void AllocateVertexBuffers();
	void RenderModel(Matrix4f view, Matrix4f proj, std::unique_ptr<gpu::GPUContext> &ctx);
	void LoadTexture();
	void LoadShaders(const char * vertex_file_path, const char * fragment_file_path);




	void InitializeTexture();
	void InitializeCompressedTexture();

	bool LoadTextureData(const string imagepath);
	bool LoadTextureDataJPG(const string imagepath);
	bool LoadTextureDataPBO(const string imagepath);
	bool CompressImage(const string imagepath);
	bool LoadCompressedTextureDXT(const string imagepath);
	bool LoadCompressedTextureCRN(const string imagepath);
	bool LoadCompressedTextureGTC(const string imagepath, std::unique_ptr<gpu::GPUContext> &ctx);
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
	GLuint PboID;


	uint32_t m_numframes;
	std::vector<ull> m_CPULoad;
	std::vector<ull> m_CPUDecode;
	std::vector<ull> m_GPULoad;
	std::vector<ull> m_GPUDecode;
	std::vector<ull> m_TotalFps;
	std::string m_TexturePath;
	std::string m_TexturePathDXT;
	std::string m_TexturePathJPG;
	std::string m_TexturePathGTC;
	std::string m_TexturePathCRN;

	//OpenCL context;

	bool DynamicModel;
	int numIndices;
	int TextureNumber;
};


