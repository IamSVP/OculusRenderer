#include "Model.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "crn_decomp.h"
static const size_t kImageWidth = 512; 
static const size_t kImageHeight = 512;



GLuint loadShaders(const char * vertex_file_path, const char * fragment_file_path)
{

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}



	GLint Result = GL_FALSE;
	int InfoLogLength;



	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}



	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}
GLuint loadBMP_custom(const char * imagepath){

	printf("Reading image %s\n", imagepath);

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	// Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = fopen(imagepath, "rb");
	if (!file)							    { printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath); getchar(); return 0; }

	// Read the header, i.e. the 54 first bytes

	// If less than 54 bytes are read, problem
	if (fread(header, 1, 54, file) != 54){
		printf("Not a correct BMP file\n");
		return 0;
	}
	// A BMP files always begins with "BM"
	if (header[0] != 'B' || header[1] != 'M'){
		printf("Not a correct BMP file\n");
		return 0;
	}
	// Make sure this is a 24bpp file
	if (*(int*)&(header[0x1E]) != 0)         { printf("Not a correct BMP file\n");    return 0; }
	if (*(int*)&(header[0x1C]) != 24)         { printf("Not a correct BMP file\n");    return 0; }

	// Read the information about the image
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width*height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way

	// Create a buffer
	data = new unsigned char[imageSize];

	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);

	// Everything is in memory now, the file wan be closed
	fclose(file);

	// Create one OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

	// OpenGL has now copied the data. Free our own version
	delete[] data;

	// Poor filtering, or ...
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

	// ... nice trilinear filtering.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Return the ID of the texture we just created
	return textureID;
}


////------------------------------End of Helper Functions-------------------------//


#ifndef NDEBUG

static const char *GetGLErrString(GLenum err) {
	const char *errString = "Unknown error";
	switch (err) {
	case GL_INVALID_ENUM: errString = "Invalid enum"; break;
	case GL_INVALID_VALUE: errString = "Invalid value"; break;
	case GL_INVALID_OPERATION: errString = "Invalid operation"; break;
	}
	return errString;
}

#define CHECK_GL(fn, ...)  \
	fn(__VA_ARGS__);               \
    do { \
	  GLenum glError = glGetError(); \
      if (glError != GL_NO_ERROR)	{ \
	    const char *errString = GetGLErrString(glError); \
	    assert(false); \
      } \
   } \
   while (0)
#else
#define CHECK_GL(fn, ...) do { fn(__VA_ARGS__); } while (0)
#endif


//-----------------Loading Texture functions------------------//

bool Model::LoadCompressedTexture(const char *imagepath){

	unsigned char *Pixel = NULL;
	unsigned char *blocks;
	int Idx = 0, NextIdx = 0;
	std::basic_ifstream<unsigned char> input(imagepath, std::ios::in | std::ios::binary);
	//std::ifstream input(imagepath, std::ios::binary);
	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PboID[NextIdx]);

	blocks = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if (blocks)
	{
		std::chrono::high_resolution_clock::time_point load_start =
			std::chrono::high_resolution_clock::now();


		input.read(blocks, (kImageHeight*kImageWidth / 2));


		std::chrono::high_resolution_clock::time_point cpu_load_end =
			std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> load_time_cpu =
			std::chrono::duration_cast<std::chrono::duration<double>>(cpu_load_end - load_start);
		std::cout << "CPU Load time: " << load_time_cpu.count() << "(s)" << std::endl;
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}

	glBindTexture(GL_TEXTURE_2D, TextureID);

	glCompressedTexSubImage2D(GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,          // Data format
		kImageWidth*kImageHeight / 2, // Type of texture data
		0);     // 

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//stbi_image_free(Pixel);
	return true;
}

bool Model::LoadTextureData_JPG(const char * fileName){


	std::chrono::high_resolution_clock::time_point load_start = 
		std::chrono::high_resolution_clock::now();

	int x, y, n;
	unsigned char *textureData = stbi_load(fileName, &x, &y, &n, 4);
	assert(x == kImageWidth);
	assert(y == kImageHeight);


	std::chrono::high_resolution_clock::time_point cpu_load_end =
		std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> load_time_cpu =
		std::chrono::duration_cast<std::chrono::duration<double>>(cpu_load_end - load_start);
	std::cout << "CPU Load time: " << load_time_cpu.count() << "(s)" << std::endl;

	// Generate a texture ID and bind to it
	//		cout << "There was an error loading the texture: " << fileName << endl;

	cout << "See"<<fileName<< endl;
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);

	// Construct the texture.
	// Note: The 'Data format' is the format of the image data as provided by the image library. FreeImage decodes images into
	// BGR/BGRA format, but we want to work with it in the more common RGBA format, so we specify the 'Internal format' as such.
	CHECK_GL(glTexSubImage2D, GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_RGBA,          // Data format
		GL_UNSIGNED_BYTE, // Type of texture data
		textureData);     // The image data to use for this texture

	glBindTexture(GL_TEXTURE_2D, 0);
	// Specify our minification and magnification filters
	


	// Check for OpenGL texture creation errors
	/* glError = glGetError();
	if (glError)
	{
		cout << "There was an error loading the texture: " << fileName << endl;

		switch (glError)
		{
		case GL_INVALID_ENUM:
			cout << "Invalid enum." << endl;
			break;

		case GL_INVALID_VALUE:
			cout << "Invalid value." << endl;
			break;

		case GL_INVALID_OPERATION:
			cout << "Invalid operation." << endl;

		default:
			cout << "Unrecognised GLenum." << endl;
			break;
		}

		cout << "See https://www.opengl.org/sdk/docs/man/html/glTexImage2D.xhtml for further details." << endl;
	}*/

#if 0
	// Unload the 32-bit colour bitmap
	FreeImage_Unload(bitmap32);

	// If we had to do a conversion to 32-bit colour, then unload the original
	// non-32-bit-colour version of the image data too. Otherwise, bitmap32 and
	// bitmap point at the same data, and that data's already been free'd, so
	// don't attempt to free it again! (or we'll crash).
	if (bitsPerPixel != 32)
	{
		FreeImage_Unload(bitmap);
	}
#else
	stbi_image_free(textureData);
#endif

	std::chrono::high_resolution_clock::time_point load_end =
		std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> load_time = 
		std::chrono::duration_cast<std::chrono::duration<double>>(load_end - load_start);
	std::cout << "Load time: " << load_time.count() << "(s)" << std::endl;

	// Finally, return the texture ID
	return true;

}

bool Model::LoadTextureDataPBO(const char *imagepath){
	GLubyte *textureData;
	int Idx = 0, NextIdx = 0;
	
	// Generate a texture ID and bind to it
	//		cout << "There was an error loading the texture: " << fileName << endl;
	std::chrono::high_resolution_clock::time_point decode_start;
	std::chrono::high_resolution_clock::time_point decode_end;


	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PboID[Idx]);
	textureData = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if (textureData){

		 decode_start =
			std::chrono::high_resolution_clock::now();

		int x, y, n;
		stbi_load_into_dst(textureData, imagepath, &x, &y, &n, 4);

		 decode_end =
			std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> decode_time_cpu =
			std::chrono::duration_cast<std::chrono::duration<double>>(decode_end - decode_start);
		std::cout << "CPU  time: " << decode_time_cpu.count() << "(s)" << std::endl;
		assert(x == kImageWidth);
		assert(y == kImageHeight);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}

	

	glBindTexture(GL_TEXTURE_2D, TextureID);
	glTexSubImage2D(GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_RGBA,          // Data format
		GL_UNSIGNED_BYTE, // Type of texture data
		0);     // The image data to use for this texture

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	std::chrono::high_resolution_clock::time_point gpu_load_end =
		std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> cpu_to_gpu =
		std::chrono::duration_cast<std::chrono::duration<double>>(gpu_load_end - decode_end);
	std::cout << "CPU to GPU: " << cpu_to_gpu.count() << "(s)" << std::endl;
	//stbi_image_free(textureData);
	return true;
}



static crn_uint8 *read_file_into_buffer(const char *pFilename, crn_uint32 &size)
{
	size = 0;

	FILE* pFile = NULL;
	fopen_s(&pFile, pFilename, "rb");
	if (!pFile)
		return NULL;

	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	crn_uint8 *pSrc_file_data = static_cast<crn_uint8*>(malloc(std::max(1U, size)));
	if ((!pSrc_file_data) || (fread(pSrc_file_data, size, 1, pFile) != 1))
	{
		fclose(pFile);
		free(pSrc_file_data);
		size = 0;
		return NULL;
	}

	fclose(pFile);
	return pSrc_file_data;
}

bool Model::LoadCRNCompressedTexture(const char *imagepath){

	crn_uint32 src_file_size;
	std::chrono::high_resolution_clock::time_point disk_load_start =
		std::chrono::high_resolution_clock::now();
	crn_uint8 *pSrc_file_data = read_file_into_buffer(imagepath, src_file_size);
	
	if (!pSrc_file_data)
		std::cout << "error in file reading\n";


    std::chrono::high_resolution_clock::time_point disk_load_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> disk_load_duration = std::chrono::duration_cast<std::chrono::duration<double>>(disk_load_start-disk_load_end);     

    std::cout << "Disk to CPU load time::" << disk_load_duration.count() << std::endl;


	crnd::crn_texture_info tex_info;
	if (!crnd::crnd_get_texture_info(pSrc_file_data, src_file_size, &tex_info))
	{
		free(pSrc_file_data);
		std::cout << "crnd_get_texture_info() failed!\n";
	}
	const crn_uint32 width = std::max(1U, tex_info.m_width >> 0);
	const crn_uint32 height = std::max(1U, tex_info.m_height >> 0);
	const crn_uint32 blocks_x = std::max(1U, (width + 3) >> 2);
	const crn_uint32 blocks_y = std::max(1U, (height + 3) >> 2);
	const crn_uint32 row_pitch = blocks_x * crnd::crnd_get_bytes_per_dxt_block(tex_info.m_format);
	const crn_uint32 total_face_size = row_pitch * blocks_y;


	crnd::crnd_unpack_context pContext = crnd::crnd_unpack_begin(pSrc_file_data, src_file_size);
	
	void *TextureData;
	TextureData = malloc(kImageWidth*kImageHeight / 2);
	crnd::crnd_unpack_level(pContext, &TextureData, total_face_size, row_pitch,0);
	std::chrono::high_resolution_clock::time_point cpu_load_end =
		std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> load_time_cpu =
		std::chrono::duration_cast<std::chrono::duration<double>>(cpu_load_end - disk_load_end);
	std::cout << "CPU Decode time: " << load_time_cpu.count() << "(s)" << std::endl;
    
	glBindTexture(GL_TEXTURE_2D, TextureID);
    //glDrawPixels
	glCompressedTexSubImage2D(GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,          // Data format
		kImageWidth*kImageHeight / 2, // Type of texture data
		TextureData);
	glBindTexture(GL_TEXTURE_2D, 0);
	std::chrono::high_resolution_clock::time_point gpu_load_end =
		std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> cpu_to_gpu =
		std::chrono::duration_cast<std::chrono::duration<double>>(gpu_load_end - cpu_load_end);
	std::cout << "CPU to GPU: " << cpu_to_gpu.count() << "(s)" << std::endl;
	free(TextureData);
	return true;
}

//-------------------End of loading Texture functions--------------//

Model::Model(const char * imagepath){


	std::vector<Vector3f> vertices;
	std::vector<Vector2f> uvs;
	std::vector<Vector3f> normals;
	bool res = ObjLoader::loadOBJ(imagepath, vertices, uvs, normals);
	std::vector<unsigned short> indices;
	std::vector<Vector3f> indexed_vertices;
	std::vector<Vector2f> indexed_uvs;
	std::vector<Vector3f> indexed_normals;
	ObjLoader::indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	glGenVertexArrays(1, &vertexArrayId);
	glBindVertexArray(vertexArrayId);


	glGenBuffers(1, &VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size()*sizeof(Vector3f), &indexed_vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &UVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, UVBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size()*sizeof(Vector2f), &indexed_uvs[0], GL_STATIC_DRAW);

	glGenBuffers(1, &NormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, NormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size()*sizeof(Vector3f), &indexed_normals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &IndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
	numIndices = indices.size();
	DynamicModel = false;
}

void Model::InitializeTexture(){
	glGenTextures(1, &TextureID);
	glBindTexture(GL_TEXTURE_2D, TextureID);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, kImageWidth, kImageHeight);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenBuffers(2, PboID);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER,PboID[0]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, kImageHeight*kImageWidth * 4, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PboID[1]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, kImageHeight*kImageWidth * 4, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}


//-------Initialize compressed texture buffers----------------//
void Model::InitializeCompressedTexture(){

	glGenTextures(1, &TextureID);
	glBindTexture(GL_TEXTURE_2D, TextureID);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, kImageWidth, kImageHeight);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenBuffers(2, PboID);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PboID[0]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, (kImageHeight*kImageWidth)/2, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PboID[1]);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, (kImageHeight*kImageWidth) / 2, 0, GL_STREAM_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);


}

Model::Model(const char * imagepath, bool dynamic){

	std::vector<Vector3f> vertices;
	std::vector<Vector2f> uvs;
	std::vector<Vector3f> normals;
	//bool res = ObjLoader::loadAssImp(imagepath, indices, indexed_vertices, indexed_uvs, indexed_normals);
	
	//ObjLoader::indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	
	
	DynamicModel = dynamic;
	TextureNumber = 0;

}

Model::Model(bool dynamic){

	DynamicModel = dynamic;
	TextureNumber = 0;

}

void Model::AllocateVertexBuffers(){

	glGenVertexArrays(1, &vertexArrayId);
	glBindVertexArray(vertexArrayId);


	glGenBuffers(1, &VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size()*sizeof(Vector3f), &indexed_vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &UVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, UVBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size()*sizeof(Vector2f), &indexed_uvs[0], GL_STATIC_DRAW);

	/*glGenBuffers(1, &NormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, NormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size()*sizeof(Vector3f), &indexed_normals[0], GL_STATIC_DRAW);
	*/

	glGenBuffers(1, &IndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
	
	
}

void Model::LoadTexture(){

	//TextureID = loadBMP_custom("RenderStuff//Textures//grass.bmp");
	//TextureID = TextureLoader::loadTexture("RenderStuff//Textures//rock162.jpg");
}

Model::~Model(){

	glDeleteBuffers(1, &VertexBuffer);
	glDeleteBuffers(1, &UVBuffer);
	glDeleteBuffers(1, &NormalBuffer);
	glDeleteBuffers(1, &IndexBuffer);
	glDeleteProgram(ProgramID);
	glDeleteTextures(1, &TextureID);
	glDeleteBuffers(2, PboID);
	glDeleteVertexArrays(1, &vertexArrayId);

}

void Model::LoadShaders(const char * vertex_file_path, const char * fragment_file_path){

	ProgramID = loadShaders(vertex_file_path,fragment_file_path);

}


//---------------------Render function call glbind calls, glDraw calss-----------------//
void Model::RenderModel(Matrix4f view, Matrix4f proj){


	Matrix4f Model = Matrix4f::Scaling(Vector3f(8.0f,6.0f,5.0f));
	Matrix4f MVP = proj * view*Model;

	glUseProgram(ProgramID);
	glUniform1i(glGetUniformLocation(ProgramID, "myTextureSampler"), 0);
	glUniformMatrix4fv(glGetUniformLocation(ProgramID, "MVP"), 1, GL_TRUE, (FLOAT*)&MVP);
	glUniformMatrix4fv(glGetUniformLocation(ProgramID, "V"), 1, GL_TRUE, (FLOAT*)&view);
	glUniformMatrix4fv(glGetUniformLocation(ProgramID, "M"), 1, GL_TRUE, (FLOAT*)&Model);
	Vector3f lightPos(4, 4, 4);
	glUniform3f(glGetUniformLocation(ProgramID, "LightPosition_worldspace"), lightPos.x, lightPos.y, lightPos.z);

	glActiveTexture(GL_TEXTURE0);
	
	if (DynamicModel){
		char fileName[256];
		
		//"RenderStuff//Textures//sunset//frame%04d.jpg" 
		//..//Evaluation//ImageDatasets//Pixar128//DXT1//Pixar%05d.DXT1
		//..//Evaluation//ImageDatasets//ReTragetME//DXT1//ReTargetME%05d.DXT1
		// ..//Evaluation//ImageDatasets\\Pixar128\\CRN\\Pixar%05d.CRN
		TextureNumber = 1;
		sprintf(fileName, "..//Evaluation//ImageDatasets//Pixar_normal//CRN//compressed//Pixar_normal%05d.CRN", TextureNumber);
		LoadCRNCompressedTexture(fileName);
		//LoadCompressedTexture(fileName);
		//LoadTextureDataPBO(fileName);

		if (TextureNumber > 157)
			TextureNumber = 0;
	}

	glBindTexture(GL_TEXTURE_2D, TextureID);
	glBindVertexArray(vertexArrayId);
	



	GLuint vertexPosition_modelspace = glGetAttribLocation(ProgramID, "vertexPosition_modelspace");
	GLuint vertexNormal_modelspace = glGetAttribLocation(ProgramID, "vertexNormal_modelspace");
	GLuint vertexUV = glGetAttribLocation(ProgramID, "vertexUV");

	glEnableVertexAttribArray(vertexPosition_modelspace);
	glEnableVertexAttribArray(vertexNormal_modelspace);
	glEnableVertexAttribArray(vertexUV);


	glBindBuffer(GL_ARRAY_BUFFER,VertexBuffer);
	glVertexAttribPointer(vertexPosition_modelspace, 3, GL_FLOAT, GL_FALSE,0, (void*)0);

	/*glBindBuffer(GL_ARRAY_BUFFER, NormalBuffer);
	glVertexAttribPointer(vertexNormal_modelspace, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);*/

	glBindBuffer(GL_ARRAY_BUFFER,UVBuffer);
	glVertexAttribPointer(vertexUV, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, NULL);

	glDisableVertexAttribArray(vertexPosition_modelspace);
	glDisableVertexAttribArray(vertexNormal_modelspace);
	glDisableVertexAttribArray(vertexUV);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glUseProgram(0);

}