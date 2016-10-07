#include "Model.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <vector>
#include <iomanip>

#include "stb_image.h"
namespace stbi {


#include "crn_decomp.h"
}

#define FOURK
//#define TWOK
//#define CRN
//#define GTC
#define DXT1
//#define JPG
//#define BMP
#define PBO
#define MAX_TEXTURES 580

#ifdef FOURK
#if (defined GTC) || (defined JPG) || (defined BMP) || (defined CRN)
static const size_t kImageWidth = 3584; 
static const size_t kImageHeight = 1792;
#else
static const size_t kImageWidth = 3840;
static const size_t kImageHeight = 1920;
#endif
#else
static const size_t kImageWidth = 2560; 
static const size_t kImageHeight = 1280;
#endif

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
		printf("GL-ERROR-%s\n", errString); \
	    assert(false); \
      } \
   } \
   while (0)
#else
#define CHECK_GL(fn, ...)  fn(__VA_ARGS__)
#endif


GLuint loadShaders(const char * vertex_file_path, const char * fragment_file_path)
{

	// Create the shaders
	GLuint VertexShaderID = CHECK_GL(glCreateShader,GL_VERTEX_SHADER);
	GLuint FragmentShaderID = CHECK_GL(glCreateShader,GL_FRAGMENT_SHADER);

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
	CHECK_GL(glShaderSource,VertexShaderID, 1, &VertexSourcePointer, NULL);
	CHECK_GL(glCompileShader,VertexShaderID);

	// Check Vertex Shader
	CHECK_GL(glGetShaderiv,VertexShaderID, GL_COMPILE_STATUS, &Result);
	CHECK_GL(glGetShaderiv, VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		CHECK_GL(glGetShaderInfoLog, VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	CHECK_GL(glShaderSource, FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	CHECK_GL(glCompileShader, FragmentShaderID);

	// Check Fragment Shader
	CHECK_GL(glGetShaderiv, FragmentShaderID, GL_COMPILE_STATUS, &Result);
	CHECK_GL(glGetShaderiv, FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		CHECK_GL(glGetShaderInfoLog, FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = CHECK_GL(glCreateProgram);
	CHECK_GL(glAttachShader,ProgramID, VertexShaderID);
	CHECK_GL(glAttachShader, ProgramID, FragmentShaderID);
	CHECK_GL(glLinkProgram, ProgramID);

	// Check the program
	CHECK_GL(glGetProgramiv,ProgramID, GL_LINK_STATUS, &Result);
	CHECK_GL(glGetProgramiv, ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		CHECK_GL(glGetProgramInfoLog, ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	CHECK_GL(glDeleteShader, VertexShaderID);
	CHECK_GL(glDeleteShader,FragmentShaderID);

	return ProgramID;
}

 void Model::loadBMP_custom(const char * imagepath, GLuint texID, GLuint pbo) {

	// Data read from the header of the BMP file
	unsigned char header[54];
	unsigned int dataPos;
	unsigned int imageSize;
	unsigned int width, height;
	// Actual RGB data
	unsigned char * data;

	// Open the file
	FILE * file = fopen(imagepath, "rb");
	assert(file);

	// Read the header, i.e. the 54 first bytes



	// If less than 54 bytes are read, problem
	if (fread(header, 1, 54, file) != 54){
		assert(!"Not a correct BMP file\n");
		return;
	}
	// A BMP files always begins with "BM"
	if (header[0] != 'B' || header[1] != 'M'){
		assert(!"Not a correct BMP file\n");
		return;
	}
	// Make sure this is a 24bpp file
	if (*(int*)&(header[0x1E]) != 0) { assert(!"Not a correct BMP file\n");    return; }
	if (*(int*)&(header[0x1C]) != 24)         { assert(!"Not a correct BMP file\n");    return; }

	// Read the information about the image
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	width = *(int*)&(header[0x12]);
	height = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)    imageSize = width*height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)      dataPos = 54; // The BMP header is done that way
	


	// Create a buffer


	GLuint64 gpu_load_time1;
	glBeginQuery(GL_TIME_ELAPSED, GPULoadQuery[0]);

	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, pbo);
	GLubyte *textureData = (GLubyte*)CHECK_GL(glMapBuffer, GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

	glEndQuery(GL_TIME_ELAPSED);
	GLint available = GL_FALSE;
	
	assert(textureData);
	
	// Read the actual data from the file into the buffer

	std::chrono::high_resolution_clock::time_point CPUload_start =
		std::chrono::high_resolution_clock::now();
	fread(textureData, 1, imageSize, file);
	std::chrono::high_resolution_clock::time_point CPUload_end =
		std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds load_time_cpu =
		std::chrono::duration_cast<std::chrono::nanoseconds>(CPUload_end - CPUload_start);
	//std::cout << "CPU Load time: " << load_time_cpu.count() << "(s)" << std::endl;
	m_CPULoad.push_back(load_time_cpu.count());

	assert(width == kImageWidth);
	assert(height == kImageHeight);
	fclose(file);

	GLuint64 gpu_load_time2;
	glBeginQuery(GL_TIME_ELAPSED, GPULoadQuery[1]);

	CHECK_GL(glUnmapBuffer, GL_PIXEL_UNPACK_BUFFER);

	// Everything is in memory now, the file wan be closed
	
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, texID);
	CHECK_GL(glTexSubImage2D, GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_BGR,          // Data format
		GL_UNSIGNED_BYTE, // Type of texture data
		0);     // The image data to use for this texture

	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);

	glEndQuery(GL_TIME_ELAPSED);

	available = GL_FALSE;
	while (!available)
		glGetQueryObjectiv(GPULoadQuery[1], GL_QUERY_RESULT_AVAILABLE, &available);

	glGetQueryObjectui64v(GPULoadQuery[0], GL_QUERY_RESULT, &gpu_load_time1);
	glGetQueryObjectui64v(GPULoadQuery[1], GL_QUERY_RESULT, &gpu_load_time2);
	m_GPULoad.push_back(gpu_load_time1 + gpu_load_time2);
}


////------------------------------End of Helper Functions-------------------------//

//-----------------Loading Texture functions------------------//
bool Model::LoadCompressedTextureDXT(const string imagepath){

	unsigned char *Pixel = NULL;
	char *blocks;
	int Idx = 0, NextIdx = 0;

	std::ifstream is(imagepath, std::ios::in | std::ifstream::binary);
	assert(is);

	is.seekg(0, is.end);
	size_t length = static_cast<size_t>(is.tellg());
	assert(length == kImageHeight*kImageWidth / 2);
	is.seekg(0, is.beg);

	//std::ifstream input(imagepath, std::ios::binary);
#ifdef PBO	

	GLuint64 gpu_load_time1;
	glBeginQuery(GL_TIME_ELAPSED, GPULoadQuery[0]);


	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, PboID);

	blocks = (char*)CHECK_GL(glMapBuffer, GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

	glEndQuery(GL_TIME_ELAPSED);
	GLint available = GL_FALSE;
	
	assert(blocks);

	std::chrono::high_resolution_clock::time_point CPUload_start =
		std::chrono::high_resolution_clock::now();

	is.read(blocks, length);
	CHECK_GL(glUnmapBuffer, GL_PIXEL_UNPACK_BUFFER);

	std::chrono::high_resolution_clock::time_point CPUload_end =
		std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds load_time_cpu =
		std::chrono::duration_cast<std::chrono::nanoseconds>(CPUload_end - CPUload_start);
	//std::cout << "CPU Load time: " << load_time_cpu.count() << "(s)" << std::endl;
	m_CPULoad.push_back(load_time_cpu.count());


	GLuint64 gpu_load_time2;
	glBeginQuery(GL_TIME_ELAPSED, GPULoadQuery[1]);
	
	//std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
	
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);

	CHECK_GL(glCompressedTexSubImage2D,GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT,          // Data format
		kImageWidth*kImageHeight / 2, // Type of texture data
		0);     // 
	//std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	//std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	//m_GPULoad.push_back(GPULoad_Time.count());

	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);


	glEndQuery(GL_TIME_ELAPSED);

	available = GL_FALSE;
	while (!available)
		glGetQueryObjectiv(GPULoadQuery[1], GL_QUERY_RESULT_AVAILABLE, &available);
	glGetQueryObjectui64v(GPULoadQuery[0], GL_QUERY_RESULT, &gpu_load_time1);
	glGetQueryObjectui64v(GPULoadQuery[1], GL_QUERY_RESULT, &gpu_load_time2);
	m_GPULoad.push_back(gpu_load_time1 + gpu_load_time2);
#else

	blocks = (uint8_t*)malloc((kImageHeight*kImageWidth / 2));

	std::chrono::high_resolution_clock::time_point CPUload_start =
		std::chrono::high_resolution_clock::now();


	input.read(blocks, (kImageHeight*kImageWidth / 2));


	std::chrono::high_resolution_clock::time_point CPUload_end =
		std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> load_time_cpu =
	std::chrono::duration_cast<std::chrono::duration<double>>(CPUload_end - CPUload_start);
		//std::cout << "CPU Load time: " << load_time_cpu.count() << "(s)" << std::endl;
	m_CPULoad.push_back(load_time_cpu.count());


	std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);

	CHECK_GL(glCompressedTexSubImage2D,GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT,          // Data format
		kImageWidth*kImageHeight / 2, // Type of texture data
		blocks);     // 
	std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	m_GPULoad.push_back(GPULoad_Time.count());

	free(blocks);
#endif

	

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
	//std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
	GLuint64 gpu_load_time;
	glBeginQuery(GL_TIME_ELAPSED, GPULoadQuery[0]);
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

	glEndQuery(GL_TIME_ELAPSED);

	GLint available = GL_FALSE;
	while (!available)
		glGetQueryObjectiv(GPULoadQuery[0], GL_QUERY_RESULT_AVAILABLE, &available);

	glGetQueryObjectui64v(GPULoadQuery[0], GL_QUERY_RESULT, &gpu_load_time);
	//std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	//std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	m_GPULoad.push_back(gpu_load_time);
	// Specify our minification and magnification filters
	
	stbi_image_free(textureData);

		// Finally, return the texture ID
	return true;
}

bool Model::LoadTextureDataPBO(const string imagepath){
	GLubyte *textureData;
	int Idx = 0, NextIdx = 0;
	
	FILE *fp = NULL;
	fopen_s(&fp, imagepath.c_str(), "rb");
	assert(fp);

	fseek(fp, 0, SEEK_END);
	size_t size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	unsigned char *ImageDataPtr = new unsigned char[size];

	//CPU LOADING.....
	std::chrono::high_resolution_clock::time_point CPULoad_Start = std::chrono::high_resolution_clock::now();
	fread(ImageDataPtr, 1, size, fp);
	std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
	m_CPULoad.push_back(CPULoad_Time.count());
	fclose(fp);

	GLuint64 gpu_load_time1;
	glBeginQuery(GL_TIME_ELAPSED, GPULoadQuery[0]);

	CHECK_GL(glBindBuffer,GL_PIXEL_UNPACK_BUFFER, PboID);
	textureData = (GLubyte*)CHECK_GL(glMapBuffer,GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);


	glEndQuery(GL_TIME_ELAPSED);

	GLint available = GL_FALSE;
	
	assert(textureData);
	
	std::chrono::high_resolution_clock::time_point CPUDecode_Start = std::chrono::high_resolution_clock::now();
	int x, y, n;

	stbi_load_from_memory_into_dst(textureData, ImageDataPtr, size, &x, &y, &n, 4);
	std::chrono::high_resolution_clock::time_point CPUDecode_End = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds CPUDecode_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPUDecode_End - CPUDecode_Start);
	m_CPUDecode.push_back(CPUDecode_Time.count());
		
	assert(x == kImageWidth);
	assert(y == kImageHeight);


	GLuint64 gpu_load_time2;
	glBeginQuery(GL_TIME_ELAPSED, GPULoadQuery[1]);
	CHECK_GL(glUnmapBuffer,GL_PIXEL_UNPACK_BUFFER);
	

	
	//std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
	
	
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);
	CHECK_GL(glTexSubImage2D, GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_RGBA,          // Data format
		GL_UNSIGNED_BYTE, // Type of texture data
		0);     // The image data to use for this texture

	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);
	
	glEndQuery(GL_TIME_ELAPSED);

	available = GL_FALSE;
	while (!available)
		glGetQueryObjectiv(GPULoadQuery[1], GL_QUERY_RESULT_AVAILABLE, &available);

	glGetQueryObjectui64v(GPULoadQuery[0], GL_QUERY_RESULT, &gpu_load_time1);
	glGetQueryObjectui64v(GPULoadQuery[1], GL_QUERY_RESULT, &gpu_load_time2);
	m_GPULoad.push_back(gpu_load_time1 + gpu_load_time2);

	//std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	//std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	//m_GPULoad.push_back(GPULoad_Time.count());
	// stbi_image_free(textureData);
	delete[] ImageDataPtr;
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

  //CPU file load times
  

  is.read(reinterpret_cast<char *>(&hdr), kHeaderSz);

 


  std::vector<uint8_t> cmp_data(mem_sz + 512);

  std::chrono::high_resolution_clock::time_point CPULoad_Start =
	  std::chrono::high_resolution_clock::now();
  is.read(reinterpret_cast<char *>(cmp_data.data()) + 512, mem_sz);

  std::chrono::high_resolution_clock::time_point CPULoad_End = std::chrono::high_resolution_clock::now();

  std::chrono::nanoseconds CPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPULoad_End - CPULoad_Start);
  m_CPULoad.push_back(CPULoad_Time.count());

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
  cmp_event = GenTC::LoadCompressedDXT(ctx, hdr, queue, cmp_buf, output, 1, &acquire_event);

  // Release the PBO
  cl_event release_event;
  CHECK_CL(clEnqueueReleaseGLObjects, queue, 1, &output, 1, &cmp_event, &release_event);

  CHECK_CL(clFlush, ctx->GetDefaultCommandQueue());

  // Wait on the release
  CHECK_CL(clWaitForEvents, 1, &release_event);

  cl_ulong gtc_start;
  cl_ulong gtc_end;
  CHECK_CL(clGetEventProfilingInfo, acquire_event, CL_PROFILING_COMMAND_SUBMIT,
	                                sizeof(cl_ulong), &gtc_start, NULL);
  CHECK_CL(clGetEventProfilingInfo, release_event, CL_PROFILING_COMMAND_END,
                                    sizeof(cl_ulong), &gtc_end, NULL);
  cl_ulong gpu_decode_nanosecond = gtc_end - gtc_start;
  m_GPUDecode.push_back(gpu_decode_nanosecond);
  m_CPUDecode.push_back(0);

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

  GLuint64 gpu_load_time1;
  glBeginQuery(GL_TIME_ELAPSED, GPULoadQuery[0]);
  CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, PboID);
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);
  CHECK_GL(glCompressedTexSubImage2D, GL_TEXTURE_2D, 0, 0, 0, hdr.width, hdr.height,
                                      GL_COMPRESSED_RGB_S3TC_DXT1_EXT, dxt_size, 0);
  CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);
 

  glEndQuery(GL_TIME_ELAPSED);

  GLint  available = GL_FALSE;
  while (!available)
	  glGetQueryObjectiv(GPULoadQuery[0], GL_QUERY_RESULT_AVAILABLE, &available);

  glGetQueryObjectui64v(GPULoadQuery[0], GL_QUERY_RESULT, &gpu_load_time1);
  m_GPULoad.push_back(gpu_load_time1);

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
#ifdef PBO

	GLuint64 gpu_load_time1;
	// CHECK_GL(glBeginQuery, GL_TIME_ELAPSED, GPULoadQuery[0]);
    CHECK_GL(glBindBuffer,GL_PIXEL_UNPACK_BUFFER, PboID);
	char *TextureData;

	TextureData = (char *)CHECK_GL(glMapBuffer,GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	// CHECK_GL(glEndQuery, GL_TIME_ELAPSED);

	assert(TextureData);
	std::chrono::high_resolution_clock::time_point CPUDecode_start =
		std::chrono::high_resolution_clock::now();

	void *pTextureData = TextureData;
	stbi::crnd::crnd_unpack_level(pContext, &pTextureData, total_face_size, row_pitch,0);
	
	std::chrono::high_resolution_clock::time_point CPUDecode_end =
		std::chrono::high_resolution_clock::now();

	CHECK_GL(glUnmapBuffer, GL_PIXEL_UNPACK_BUFFER);

	std::chrono::nanoseconds decode_time_cpu =
		std::chrono::duration_cast<std::chrono::nanoseconds>(CPUDecode_end - CPUDecode_start);
	//std::cout << "CPU Load time: " << load_time_cpu.count() << "(s)" << std::endl;
	m_CPUDecode.push_back(decode_time_cpu.count());

	GLuint64 gpu_load_time2;
	CHECK_GL(glQueryCounter, GPULoadQuery[0], GL_TIMESTAMP);

	//std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);

	CHECK_GL(glCompressedTexSubImage2D,GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT,          // Data format
		kImageWidth*kImageHeight / 2, // Type of texture data
		0);     // 
	//std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	//std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	//m_GPULoad.push_back(GPULoad_Time.count());

	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);

	CHECK_GL(glQueryCounter, GPULoadQuery[1], GL_TIMESTAMP);

	GLint available = GL_FALSE;
	while (!available)
		CHECK_GL(glGetQueryObjectiv, GPULoadQuery[1], GL_QUERY_RESULT_AVAILABLE, &available);
	CHECK_GL(glGetQueryObjectui64v, GPULoadQuery[0], GL_QUERY_RESULT, &gpu_load_time1);
	CHECK_GL(glGetQueryObjectui64v, GPULoadQuery[1], GL_QUERY_RESULT, &gpu_load_time2);
	m_GPULoad.push_back(gpu_load_time2 - gpu_load_time1);
#else

	void *TextureData;
	TextureData = malloc(kImageWidth*kImageHeight / 2);
	void *CrunchPtr = TextureData;
	stbi::crnd::crnd_unpack_level(pContext, &TextureData, total_face_size, row_pitch,0);
	std::chrono::high_resolution_clock::time_point cpu_load_end =
		std::chrono::high_resolution_clock::now();
	std::chrono::high_resolution_clock::time_point CPUDecode_End = std::chrono::high_resolution_clock::now();

	std::chrono::nanoseconds CPUDecode_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(CPUDecode_End - CPUDecode_Start);
	m_CPUDecode.push_back(CPUDecode_Time.count());
    
	
	std::chrono::high_resolution_clock::time_point GPULoad_Start = std::chrono::high_resolution_clock::now();

	CHECK_GL(glBindTexture,GL_TEXTURE_2D, TextureID);
    //glDrawPixels
	CHECK_GL(glCompressedTexSubImage2D,GL_TEXTURE_2D,    // Type of texture
		0,                // level (0 being the top level i.e. full size)
		0, 0,             // Offset
		kImageWidth,       // Width of the texture
		kImageHeight,      // Height of the texture,
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT,          // Data format
		kImageWidth*kImageHeight / 2, // Type of texture data
		TextureData);
	std::chrono::high_resolution_clock::time_point GPULoad_End = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds GPULoad_Time = std::chrono::duration_cast<std::chrono::nanoseconds>(GPULoad_End - GPULoad_Start);
	m_GPULoad.push_back(GPULoad_Time.count());
	free(TextureData);
#endif
	
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

	CHECK_GL(glGenVertexArrays,1, &vertexArrayId);
	CHECK_GL(glBindVertexArray,vertexArrayId);


	CHECK_GL(glGenBuffers,1, &VertexBuffer);
	CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER, VertexBuffer);
	CHECK_GL(glBufferData,GL_ARRAY_BUFFER, indexed_vertices.size()*sizeof(Vector3f), &indexed_vertices[0], GL_STATIC_DRAW);

	CHECK_GL(glGenBuffers,1, &UVBuffer);
	CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, UVBuffer);
	CHECK_GL(glBufferData,GL_ARRAY_BUFFER, indexed_uvs.size()*sizeof(Vector2f), &indexed_uvs[0], GL_STATIC_DRAW);

	glGenBuffers(1, &NormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, NormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size()*sizeof(Vector3f), &indexed_normals[0], GL_STATIC_DRAW);

	CHECK_GL(glGenBuffers, 1, &IndexBuffer);
	CHECK_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
	CHECK_GL(glBufferData, GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
	numIndices = indices.size();
	DynamicModel = false;
}

void Model::InitializeTextures() {
#if (defined DXT1) || (defined GTC) || (defined CRN)
	InitializeCompressedTexture();
#elif (defined BMP)
	InitializeTextureRGB();
#else
	InitializeTexture();
#endif
}

void Model::InitializeTextureRGB() {
	CHECK_GL(glGenTextures, 1, &TextureID);
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);
	CHECK_GL(glTexStorage2D, GL_TEXTURE_2D, 1, GL_RGB8, kImageWidth, kImageHeight);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);

	CHECK_GL(glGenBuffers, 1, &PboID);
	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, PboID);
	CHECK_GL(glBufferData, GL_PIXEL_UNPACK_BUFFER, kImageHeight * kImageWidth * 3, 0, GL_STREAM_DRAW);
	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);
}

void Model::InitializeTexture(){
	CHECK_GL(glGenTextures,1, &TextureID);
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);
	CHECK_GL(glTexStorage2D,GL_TEXTURE_2D, 1, GL_RGBA8, kImageWidth, kImageHeight);	
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);

	CHECK_GL(glGenBuffers, 1, &PboID);
	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER,PboID);
	CHECK_GL(glBufferData, GL_PIXEL_UNPACK_BUFFER, kImageHeight*kImageWidth * 4, 0, GL_STREAM_DRAW);
	CHECK_GL(glBindBuffer,GL_PIXEL_UNPACK_BUFFER, 0);
}


//-------Initialize compressed texture buffers----------------//
void Model::InitializeCompressedTexture(){

	CHECK_GL(glGenTextures, 1, &TextureID);
	CHECK_GL(glBindTexture,GL_TEXTURE_2D, TextureID);
	CHECK_GL(glTexStorage2D, GL_TEXTURE_2D, 1, GL_COMPRESSED_RGB_S3TC_DXT1_EXT, kImageWidth, kImageHeight);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	CHECK_GL(glTexParameteri,GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	CHECK_GL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	CHECK_GL(glBindTexture, GL_TEXTURE_2D, 0);

	CHECK_GL(glGenBuffers, 1, &PboID);
	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, PboID);
	CHECK_GL(glBufferData, GL_PIXEL_UNPACK_BUFFER, (kImageHeight*kImageWidth)/2, 0, GL_STREAM_DRAW);
	CHECK_GL(glBindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);


}


bool loadOBJ(
	const char * path,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
);

bool loadOBJ(
	const char * path,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec2> & out_uvs,
	std::vector<glm::vec3> & out_normals
) {
	printf("Loading OBJ file %s...\n", path);

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;


	FILE * file = fopen(path, "r");
	if (file == NULL) {
		printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
		getchar();
		return false;
	}

	while (1) {

		char lineHeader[128];
		// read the first word of the line
		int res = fscanf(file, "%s", lineHeader);
		if (res == EOF)
			break; // EOF = End Of File. Quit the loop.

				   // else : parse lineHeader

		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			std::string vertex1, vertex2, vertex3;
			unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9) {
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
		else {
			// Probably a comment, eat up the rest of the line
			char stupidBuffer[1000];
			fgets(stupidBuffer, 1000, file);
		}

	}

	// For each vertex of each triangle
	for (unsigned int i = 0; i<vertexIndices.size(); i++) {

		// Get the indices of its attributes
		unsigned int vertexIndex = vertexIndices[i];
		unsigned int uvIndex = uvIndices[i];
		unsigned int normalIndex = normalIndices[i];

		// Get the attributes thanks to the index
		glm::vec3 vertex = temp_vertices[vertexIndex - 1];
		glm::vec2 uv = temp_uvs[uvIndex - 1];
		glm::vec3 normal = temp_normals[normalIndex - 1];

		// Put the attributes in buffers
		out_vertices.push_back(vertex);
		out_uvs.push_back(uv);
		out_normals.push_back(normal);

	}

	return true;
}



Model::Model(const char * imagepath, bool dynamic){

	std::vector<Vector3f> vertices;
	std::vector<Vector2f> uvs;
	std::vector<Vector3f> normals;
	//bool res = ObjLoader::loadAssImp(imagepath, indices, indexed_vertices, indexed_uvs, indexed_normals);
	ObjLoader::loadOBJ(imagepath, vertices, uvs, normals);
	ObjLoader::indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

#ifdef TWOK
	m_TexturePath = "C:\\Users\\psrihariv\\Google Drive\\Video Datasets\\360MegaCoaster2K\\";
	m_TexturePathCRN = m_TexturePath + "CRN\\360MegaC2K";
	m_TexturePathDXT = m_TexturePath + "DXT1\\360MegaC2K";
	m_TexturePathJPG = m_TexturePath + "JPG\\360MegaC2k";
	m_TexturePathBMP = m_TexturePath + "BMP\\360MegaC2K";
	m_TexturePathGTC = m_TexturePath + "GTC\\360MegaC2K";
#endif	

#ifdef FOURK
	m_TexturePath = "C:\\Users\\psrihariv\\Google Drive\\Video Datasets\\360MegaCoaster4K\\";
	m_TexturePathCRN = m_TexturePath + "CRN-cropped\\360MegaC4K";
	m_TexturePathDXT = m_TexturePath + "DXT1\\360MegaC4K";
	m_TexturePathJPG = m_TexturePath + "JPG\\360MegaC4K";
	m_TexturePathBMP = m_TexturePath + "BMP\\360MegaC4K";
	m_TexturePathGTC = m_TexturePath + "GTC-16\\360MegaC4K";
#endif	


	// Initialize timer for GPU load timmmings 

	glGenQueries(2, GPULoadQuery);

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
	m_TexturePathJPG = m_TexturePath + "JPG\\360MegaC2k";
	m_TexturePathBMP = m_TexturePath + "BMP\\360MegaC2K";
	m_TexturePathGTC = m_TexturePath + "GTC\\360MegaC2K";
#endif	

#ifdef FOURK
	m_TexturePath = "C:\\Users\\psrihariv\\Google Drive\\Video Datasets\\360MegaCoaster4K\\";
	m_TexturePathCRN = m_TexturePath + "CRN\\360MegaC4K";
	m_TexturePathDXT = m_TexturePath + "DXT1\\360MegaC4K";
	m_TexturePathJPG = m_TexturePath + "JPG\\360MegaC4K";
	m_TexturePathBMP = m_TexturePath + "BMP\\360MegaC4K";
	m_TexturePathGTC = m_TexturePath + "GTC-256\\360MegaC4K";
#endif	


	// Initialize timer for GPU load timmmings 

	glGenQueries(2, GPULoadQuery);
}

void Model::AllocateVertexBuffers(){

	CHECK_GL(glGenVertexArrays,1, &vertexArrayId);
	CHECK_GL(glBindVertexArray, vertexArrayId);


	CHECK_GL(glGenBuffers, 1, &VertexBuffer);
	CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER, VertexBuffer);
	CHECK_GL(glBufferData, GL_ARRAY_BUFFER, indexed_vertices.size()*sizeof(Vector3f), &indexed_vertices[0], GL_STATIC_DRAW);

	CHECK_GL(glGenBuffers, 1, &UVBuffer);
	CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER, UVBuffer);
	CHECK_GL(glBufferData, GL_ARRAY_BUFFER, indexed_uvs.size()*sizeof(Vector2f), &indexed_uvs[0], GL_STATIC_DRAW);

	/*glGenBuffers(1, &NormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, NormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size()*sizeof(Vector3f), &indexed_normals[0], GL_STATIC_DRAW);
	*/

	CHECK_GL(glGenBuffers, 1, &IndexBuffer);
	CHECK_GL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
	CHECK_GL(glBufferData,GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
	
	
}

void Model::LoadTexture(){

	//TextureID = loadBMP_custom("RenderStuff//Textures//grass.bmp");
	//TextureID = TextureLoader::loadTexture("RenderStuff//Textures//rock162.jpg");
}

Model::~Model(){

	CHECK_GL(glDeleteBuffers,1, &VertexBuffer);
	CHECK_GL(glDeleteBuffers,1, &UVBuffer);
	CHECK_GL(glDeleteBuffers, 1, &NormalBuffer);
	CHECK_GL(glDeleteBuffers,1, &IndexBuffer);
	CHECK_GL(glDeleteProgram, ProgramID);
	CHECK_GL(glDeleteTextures, 1, &TextureID);
	CHECK_GL(glDeleteBuffers, 1, &PboID);
	CHECK_GL(glDeleteVertexArrays, 1, &vertexArrayId);

}

void Model::LoadShaders(const char * vertex_file_path, const char * fragment_file_path){

	ProgramID = loadShaders(vertex_file_path,fragment_file_path);
	texID = CHECK_GL(glGetUniformLocation, ProgramID, "myTextureSampler");
	MVPID = CHECK_GL(glGetUniformLocation, ProgramID, "MVP");
}

static double GetAverageTimeMS(const std::vector<ull> &times) {
	double sum = 0.0;
	for (const auto &t : times) {
		sum += static_cast<double>(t) / 1e6;
	}
	return sum / static_cast<double>(times.size());
}

//---------------------Render function call glbind calls, glDraw calss-----------------//
static std::chrono::high_resolution_clock::time_point last_frame;
void Model::RenderModel(Matrix4f view, Matrix4f proj, std::unique_ptr<gpu::GPUContext> &ctx){

	//Start of a timer
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

	Matrix4f Model = Matrix4f::Scaling(Vector3f(8.0f,6.0f,5.0f));
	Matrix4f MVP = proj * view*Model;

	CHECK_GL(glUseProgram, ProgramID);
//	glUniform1i(, 0);
	CHECK_GL(glUniformMatrix4fv, MVPID, 1, GL_TRUE, (FLOAT*)&MVP);

	CHECK_GL(glActiveTexture, GL_TEXTURE0);

	static bool first_frame = true;
	if (first_frame) {
		last_frame = std::chrono::high_resolution_clock::now();
		first_frame = false;
	}

	bool load_tex = false;
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
	if (elapsed > 70.0) {
		TextureNumber = (TextureNumber + 1) % MAX_TEXTURES;
		load_tex = true;
		last_frame = now;
	}

	if (load_tex) {
		char cNumber[10];
		sprintf(cNumber, "%03d", TextureNumber + 1);
		std::string number(cNumber);
		if (DynamicModel) {
			std::string imagepath;
#ifdef JPG
			imagepath = m_TexturePathJPG + number + ".jpg";
#ifdef PBO
			LoadTextureDataPBO(imagepath);
#else
			LoadTextureDataJPG(imagepath);
#endif
#endif	

#ifdef BMP
			imagepath = m_TexturePathBMP + number + ".bmp";
#ifdef PBO
			loadBMP_custom(imagepath.c_str(), TextureID, PboID);
#else
#error "Not implemented"
#endif
#endif	

#ifdef CRN
			imagepath = m_TexturePathCRN + number + ".crn";
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
	}

	CHECK_GL(glBindTexture, GL_TEXTURE_2D, TextureID);
	CHECK_GL(glBindVertexArray,vertexArrayId);
	



	GLint vertexPosition_modelspace = CHECK_GL(glGetAttribLocation, ProgramID, "vertexPosition_modelspace");
	GLint vertexUV = CHECK_GL(glGetAttribLocation, ProgramID, "vertexUV");

	CHECK_GL(glEnableVertexAttribArray, vertexPosition_modelspace);
	CHECK_GL(glEnableVertexAttribArray, vertexUV);


	CHECK_GL(glBindBuffer, GL_ARRAY_BUFFER,VertexBuffer);
	CHECK_GL(glVertexAttribPointer,vertexPosition_modelspace, 3, GL_FLOAT, GL_FALSE,0, (void*)0);

	/*glBindBuffer(GL_ARRAY_BUFFER, NormalBuffer);
	glVertexAttribPointer(vertexNormal_modelspace, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);*/

	CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER,UVBuffer);
	CHECK_GL(glVertexAttribPointer,vertexUV, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	CHECK_GL(glBindBuffer,GL_ELEMENT_ARRAY_BUFFER, IndexBuffer);
	CHECK_GL(glDrawElements,GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, NULL);
	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	std::chrono::nanoseconds frame_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	m_TotalFps.push_back(frame_time.count());
	CHECK_GL(glDisableVertexAttribArray,vertexPosition_modelspace);
	CHECK_GL(glDisableVertexAttribArray,vertexUV);

	CHECK_GL(glBindBuffer,GL_ARRAY_BUFFER, 0);
	CHECK_GL(glBindBuffer,GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_GL(glUseProgram,0);

	m_numframes = m_numframes + 1;

	if (m_numframes >= 2 * MAX_TEXTURES && m_numframes % MAX_TEXTURES == 0) {
		double CPU_load, CPU_decode, GPU_load, GPU_decode, FPS;

		if (!m_CPULoad.empty()) {
			CPU_load = GetAverageTimeMS(m_CPULoad);
		}

		if (!m_CPUDecode.empty()) {
			CPU_decode = GetAverageTimeMS(m_CPUDecode);
		}

		if (!m_GPUDecode.empty()) {
			GPU_decode = GetAverageTimeMS(m_GPUDecode);
		}

		if (!m_GPULoad.empty()) {
			GPU_load = GetAverageTimeMS(m_GPULoad);
		}

		if (!m_TotalFps.empty()) {

			std::ofstream file("FrameNum.csv");
			int i = 0;
			for (auto &val : m_TotalFps) {
				file  << i++ << ", ";
			}
			FPS = GetAverageTimeMS(m_TotalFps);
		}
		
		printf("CPU Load Time:   %.4f\n", CPU_load);
		printf("CPU Decode Time: %.4f\n", CPU_decode);
		printf("GPU Decode Time: %.4f\n", GPU_decode);
		printf("GPU Load Time:   %.4f\n", GPU_load);
		printf("FPS:             %.4f\n", FPS);
		
	}

	if (m_numframes % MAX_TEXTURES == 0) {
		m_CPULoad.clear();
		m_CPUDecode.clear();
		m_GPULoad.clear();
		m_GPUDecode.clear();
		m_TotalFps.clear();
	}
}