#include "Model.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <vector>
#include "stb_image.h"
namespace stbi {


#include "crn_decomp.h"
}

static const size_t kImageWidth = 2560; 
static const size_t kImageHeight = 1280;




//#define FOURK
#define TWOK
#define CRN
//#define GTC
//#define DXT1
//#define JPG
#define PBO
#define MAX_TEXTURES 588
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
bool Model::LoadCompressedTextureDXT(const string imagepath){

	unsigned char *Pixel = NULL;
	unsigned char *blocks;
	int Idx = 0, NextIdx = 0;
	std::basic_ifstream<unsigned char> input(imagepath, std::ios::in | std::ios::binary);
	//std::ifstream input(imagepath, std::ios::binary);
	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PboID);

	blocks = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if (blocks)
	{
		std::chrono::high_resolution_clock::time_point CPUload_start =
			std::chrono::high_resolution_clock::now();


		input.read(blocks, (kImageHeight*kImageWidth / 2));


		std::chrono::high_resolution_clock::time_point CPUload_end =
			std::chrono::high_resolution_clock::now();

		std::chrono::duration<double> load_time_cpu =
			std::chrono::duration_cast<std::chrono::duration<double>>(CPUload_end - CPUload_start);
		//std::cout << "CPU Load time: " << load_time_cpu.count() << "(s)" << std::endl;
		m_CPULoad.push_back(load_time_cpu.count());
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}
	std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
	glBindTexture(GL_TEXTURE_2D, TextureID);

	glCompressedTexSubImage2D(GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,          // Data format
		kImageWidth*kImageHeight / 2, // Type of texture data
		0);     // 
	std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	m_GPULoad.push_back(GPULoad_Time.count());

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//stbi_image_free(Pixel);
	return true;
}

bool Model::LoadTextureDataJPG(const string fileName){

	unsigned char *ImageDataPtr = (unsigned char *)malloc((kImageHeight * kImageWidth * 4));
	FILE *fp = fopen(fileName.c_str() , "rb");


	//CPU LOADING.....
	std::chrono::high_resolution_clock::time_point CPULoad_Start = std::chrono::high_resolution_clock::now();
	fread(ImageDataPtr, 1, kImageHeight * kImageWidth * 4, fp);
	std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
	m_CPULoad.push_back(CPULoad_Time.count());


	std::chrono::high_resolution_clock::time_point CPUDecode_Start = std::chrono::high_resolution_clock::now();

	int x, y, n;
	unsigned char *textureData = stbi_load_from_memory(ImageDataPtr, (kImageHeight * kImageWidth * 4), &x, &y, &n, 4);
	assert(x == kImageWidth);
	assert(y == kImageHeight);
	std::chrono::high_resolution_clock::time_point CPUDecode_End = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds CPUDecode_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPUDecode_End - CPUDecode_Start);
	m_CPUDecode.push_back(CPUDecode_Time.count());

	// Generate a texture ID and bind to it
	//		cout << "There was an error loading the texture: " << fileName << endl;
	std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
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
	std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	m_GPULoad.push_back(GPULoad_Time.count());
	// Specify our minification and magnification filters
	
	stbi_image_free(textureData);

		// Finally, return the texture ID
	return true;

}

bool Model::LoadTextureDataPBO(const string imagepath){
	GLubyte *textureData;
	int Idx = 0, NextIdx = 0;
	
	unsigned char *ImageDataPtr = (unsigned char *)malloc((kImageHeight * kImageWidth * 4));
	FILE *fp = fopen(imagepath.c_str() , "rb");


	//CPU LOADING.....
	std::chrono::high_resolution_clock::time_point CPULoad_Start = std::chrono::high_resolution_clock::now();
	fread(ImageDataPtr, 1, kImageHeight * kImageWidth * 4, fp);
	std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
	m_CPULoad.push_back(CPULoad_Time.count());


	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PboID);
	textureData = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if (textureData){

		
		std::chrono::high_resolution_clock::time_point CPUDecode_Start = std::chrono::high_resolution_clock::now();
		int x, y, n;
		stbi_load_from_memory_into_dst(textureData, ImageDataPtr,(kImageWidth * kImageHeight * 4) , &x, &y, &n, 4);
		std::chrono::high_resolution_clock::time_point CPUDecode_End = std::chrono::high_resolution_clock::now();

		std::chrono::nanoseconds CPUDecode_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPUDecode_End - CPUDecode_Start);
		m_CPUDecode.push_back(CPUDecode_Time.count());
		
		assert(x == kImageWidth);
		assert(y == kImageHeight);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}

	
	std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
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
	
	std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	m_GPULoad.push_back(GPULoad_Time.count());
	//stbi_image_free(textureData);
	return true;
}

bool Model::LoadCompressedTextureGTC(const string imagepath, std::unique_ptr<gpu::GPUContext> &ctx) {
 GenTC::GenTCHeader hdr;
  // Load in compressed data.
  double start_time = glfwGetTime();
  std::ifstream is (imagepath.c_str(), std::ifstream::binary);
  if (!is) {
    assert(!"Error opening GenTC texture!");
	return false;
  }

  is.seekg(0, is.end);
  size_t length = static_cast<size_t>(is.tellg());
  is.seekg(0, is.beg);

  static const size_t kHeaderSz = sizeof(hdr);
  const size_t mem_sz = length - kHeaderSz;

  is.read(reinterpret_cast<char *>(&hdr), kHeaderSz);

  std::vector<uint8_t> cmp_data(mem_sz + 512);
  is.read(reinterpret_cast<char *>(cmp_data.data()) + 512, mem_sz);
  assert(is);
  assert(is.tellg() == static_cast<std::streamoff>(length));
  is.close();
  
  const cl_uint num_blocks = hdr.height * hdr.width / 16;
  cl_uint *offsets = reinterpret_cast<cl_uint *>(cmp_data.data());
  cl_uint output_offset = 0;
  offsets[0] = output_offset; output_offset += 2 * num_blocks; // Y planes
  offsets[1] = output_offset; output_offset += 4 * num_blocks; // Chroma planes
  offsets[2] = output_offset; output_offset += static_cast<cl_uint>(hdr.palette_bytes); // Palette
  offsets[3] = output_offset; output_offset += num_blocks; // Indices

  cl_uint input_offset = 0;
  offsets[4] = input_offset; input_offset += hdr.y_cmp_sz;
  offsets[5] = input_offset; input_offset += hdr.chroma_cmp_sz;
  offsets[6] = input_offset; input_offset += hdr.palette_sz;
  offsets[7] = input_offset; input_offset += hdr.indices_sz;

  // Create the data for OpenCL
  cl_int errCreateBuffer;
  cl_mem_flags flags = CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS;
  cl_mem cmp_buf = clCreateBuffer(ctx->GetOpenCLContext(), flags, cmp_data.size(),
                                  const_cast<uint8_t *>(cmp_data.data()), &errCreateBuffer);
  CHECK_CL((cl_int), errCreateBuffer);

  // Create an OpenGL handle to our pbo
  // !SPEED! We don't need to recreate this every time....
  cl_mem output = clCreateFromGLBuffer(ctx->GetOpenCLContext(), CL_MEM_READ_WRITE, PboID,
                                       &errCreateBuffer);
  CHECK_CL((cl_int), errCreateBuffer);

  cl_command_queue queue = ctx->GetNextQueue();

  // Acquire the PBO
  cl_event acquire_event;
  CHECK_CL(clEnqueueAcquireGLObjects, queue, 1, &output, 0, NULL, &acquire_event);

  // Load it
  cl_event cmp_event;

    cmp_event = GenTC::LoadCompressedDXT(ctx, hdr, queue, cmp_buf, output, acquire_event);
 

  // Release the PBO
  cl_event release_event;
  CHECK_CL(clEnqueueReleaseGLObjects, queue, 1, &output, 1, &cmp_event, &release_event);

  CHECK_CL(clFlush, ctx->GetDefaultCommandQueue());

  // Wait on the release
  CHECK_CL(clWaitForEvents, 1, &release_event);

  // Cleanup CL
  CHECK_CL(clReleaseMemObject, cmp_buf);
  CHECK_CL(clReleaseMemObject, output);
  CHECK_CL(clReleaseEvent, acquire_event);
  CHECK_CL(clReleaseEvent, release_event);
  CHECK_CL(clReleaseEvent, cmp_event);

  // Copy the texture over
  GLsizei width = static_cast<GLsizei>(hdr.width);
  GLsizei height = static_cast<GLsizei>(hdr.height);
  GLsizei dxt_size = (width * height) / 2;
  CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, PboID);
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);
  CHECK_GL(glCompressedTexSubImage2D, GL_TEXTURE_2D, 0, 0, 0, hdr.width, hdr.height,
                                        GL_COMPRESSED_RGB_S3TC_DXT1_EXT, dxt_size, 0);
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);


	return false;
}

static stbi::crn_uint8 *read_file_into_buffer(const char *pFilename, stbi::crn_uint32 &size)
{
	size = 0;

	FILE* pFile = NULL;
	fopen_s(&pFile, pFilename, "rb");
	if (!pFile)
		return NULL;

	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	stbi::crn_uint8 *pSrc_file_data = static_cast<stbi::crn_uint8*>(malloc(std::max(1U, size)));
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

bool Model::LoadCompressedTextureCRN(const string imagepath){

	stbi::crn_uint32 src_file_size;
	std::chrono::high_resolution_clock::time_point CPULoad_Start = std::chrono::high_resolution_clock::now();
	stbi::crn_uint8 *pSrc_file_data = read_file_into_buffer(imagepath.c_str(), src_file_size);
	
	if (!pSrc_file_data)
		std::cout << "error in file reading\n";


	std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
	m_CPULoad.push_back(CPULoad_Time.count());

	std::chrono::high_resolution_clock::time_point CPUDecode_Start = std::chrono::high_resolution_clock::now();
	stbi::crnd::crn_texture_info tex_info;
	if (!stbi::crnd::crnd_get_texture_info(pSrc_file_data, src_file_size, &tex_info))
	{
		free(pSrc_file_data);
		std::cout << "crnd_get_texture_info() failed!\n";
	}
	const stbi::crn_uint32 width = std::max(1U, tex_info.m_width >> 0);
	const stbi::crn_uint32 height = std::max(1U, tex_info.m_height >> 0);
	const stbi::crn_uint32 blocks_x = std::max(1U, (width + 3) >> 2);
	const stbi::crn_uint32 blocks_y = std::max(1U, (height + 3) >> 2);
	const stbi::crn_uint32 row_pitch = blocks_x * stbi::crnd::crnd_get_bytes_per_dxt_block(tex_info.m_format);
	const stbi::crn_uint32 total_face_size = row_pitch * blocks_y;


	stbi::crnd::crnd_unpack_context pContext = stbi::crnd::crnd_unpack_begin(pSrc_file_data, src_file_size);
	
	void *TextureData;
	TextureData = malloc(kImageWidth*kImageHeight / 2);
	stbi::crnd::crnd_unpack_level(pContext, &TextureData, total_face_size, row_pitch,0);
	std::chrono::high_resolution_clock::time_point cpu_load_end =
		std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point CPUDecode_End = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds CPUDecode_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPUDecode_End - CPUDecode_Start);
	m_CPUDecode.push_back(CPUDecode_Time.count());
    
	
	std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();

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
	std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	m_GPULoad.push_back(GPULoad_Time.count());
	glBindTexture(GL_TEXTURE_2D, 0);
	
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

	glGenBuffers(1, &PboID);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER,PboID);
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

	glGenBuffers(1, &PboID);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, PboID);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, (kImageHeight*kImageWidth)/2, 0, GL_STREAM_DRAW);
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
#ifdef TWOK
	m_TexturePath = "C:\\Users\\psrihariv\\Google Drive\\Video Datasets\\360MegaCoaster2K\\";
	m_TexturePathCRN = m_TexturePath + "CRN\\360MegaC2K";
	m_TexturePathDXT = m_TexturePath + "DXT1\\360MegaC2K";
	m_TexturePathJPG = m_TexturePath + "JPG\\360MegaC2K";
	m_TexturePathGTC = m_TexturePath + "GTC\\360MegaC2K";
#endif	

#ifdef FOURK
	m_TexturePath = "C:\\Users\\psrihariv\\Google Drive\\Video Datasets\\360MegaCoaster4K\\";
	m_TexturePathCRN = m_TexturePath + "CRN\\360MegaC4K";
	m_TexturePathDXT = m_TexturePath + "DXT1\\360MegaC4K";
	m_TexturePathJPG = m_TexturePath + "JPG\\360MegaC4K";
	m_TexturePathGTC = m_TexturePath + "GTC\\360MegaC4K";
#endif	
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
	glDeleteBuffers(1, &PboID);
	glDeleteVertexArrays(1, &vertexArrayId);

}

void Model::LoadShaders(const char * vertex_file_path, const char * fragment_file_path){

	ProgramID = loadShaders(vertex_file_path,fragment_file_path);

}

//---------------------Render function call glbind calls, glDraw calss-----------------//
void Model::RenderModel(Matrix4f view, Matrix4f proj, std::unique_ptr<gpu::GPUContext> &ctx){

	//Start of a timer
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

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

	char cNumber[10];
	sprintf(cNumber, "%03d", TextureNumber);
	std::string number(cNumber);
	TextureNumber = (TextureNumber + 1) % MAX_TEXTURES;
	if (DynamicModel){
		std::string imagepath;
#ifdef JPG
		imagepath = m_TexturePathJPG + number + ".jpg";
		LoadTextureDataJPG(imagepath);
#endif	

#ifdef CRN
		imagepath = m_TexturePathCRN + number +".CRN";
		LoadCompressedTextureCRN(imagepath);
#endif

#ifdef DXT1
		imagepath = m_TexturePathDXT + number + ".DXT1";
		LoadCompressedTextureDXT(imagepath);
#endif

#ifdef GTC
		imagepath = m_TexturePathGTC + number + ".gtc";
		LoadCompressedTextureGTC(imagepath, ctx);
#endif
		
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
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds frame_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	m_TotalFps.push_back(frame_time.count());
	glDisableVertexAttribArray(vertexPosition_modelspace);
	glDisableVertexAttribArray(vertexNormal_modelspace);
	glDisableVertexAttribArray(vertexUV);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glUseProgram(0);

	m_numframes = m_numframes + 1;
	if (m_numframes >= 10) {

		ull CPU_load, CPU_decode, GPU_Load, FPS;
		ull GCPU_load, GCPU_decode, GGPU_Load, GFPS;

		if (!m_CPULoad.empty()) {
			CPU_load = std::accumulate(m_CPULoad.begin(), m_CPULoad.end(), 0) / m_CPULoad.size();

		}

		if (!m_CPUDecode.empty()) {
			CPU_decode = std::accumulate(m_CPUDecode.begin(), m_CPUDecode.end(), 0) / m_CPUDecode.size();

		}

		if (!m_GPULoad.empty()) {
			GPU_Load = std::accumulate(m_GPULoad.begin(), m_GPULoad.end(), 0) / m_GPULoad.size();

		}

		if (!m_TotalFps.empty()) {
			FPS = std::accumulate(m_TotalFps.begin(), m_TotalFps.end(), 0) / m_TotalFps.size();

		}

		printf("CPU Load Time : %lld--%d\n", CPU_load, m_CPULoad.size());
		printf("CPU Decode Time: %lld--%d\n", CPU_decode, m_CPUDecode.size());
		printf("GPU Load Time: %lld--%d\n", GPU_Load, m_GPULoad.size());
		printf("FPS: %lld--%d\n", FPS, m_TotalFps.size());

		m_CPULoad.clear();
		m_CPUDecode.clear();
		m_GPULoad.clear();
		m_TotalFps.clear();


		m_numframes = 0;
	}


}