#ifndef OCULUS_SYSTEM
#define OCULUS_SYSTEM
// Include GLEW
#include "GL/CAPI_GLE.h"

// Include GLM
#include <glm\\glm.hpp>
#include "glm\\gtc\\matrix_transform.hpp"

#include "Kernel/OVR_System.h"

// Include the Oculus SDK
#include "OVR_CAPI_GL.h"

// Include GLFW
#include "GLFW\glfw3.h"

#include "OVRTextureBuffer.h"


#include <vector>
#include "gpu.h"

using namespace OVR;


class OculusSystem{
public:
	OculusSystem(){}
	~OculusSystem();
	void initialize();

	void render();
	void moveCamera(Vector3f &up, Vector3f &forward, Vector3f & pos);
	void shutdown();
	void destroy();
	// intializes both openCL and GL
	bool initializeGL(const int width, const int height);
	bool LoadBuffers();


	int winWidth;
	int winHeight;

	Matrix4f PerspectiveMatrix;
	Matrix4f ViewMatrix;

	GLFWwindow* window;

	GLuint fboID;
	GLEContext gleContext;


	//OpenCL context should be passed to till render function
	std::unique_ptr<gpu::GPUContext> ctx;

	ovrHmd HMD;
	ovrHmdDesc hmdDesc;

	OVRTextureBuffer* eyeRenderTexture[2];
	OVRDepthBuffer* eyeDepthBuffer[2];
	ovrEyeRenderDesc EyeRenderDesc[2];

	ovrGLTexture* mirrorTexture;
	GLuint        mirrorFBO;
	

};

#endif