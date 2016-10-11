#include "OculusSystem.h"
#include "Scene.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
//GLE_WGL_ENABLED
#include "GLFW\glfw3.h"
#include "GLFW\glfw3native.h"
using namespace OVR;

void OculusSystem::initialize(){

	OVR::System::Init();

	// Initializes LibOVR, and the Rift
	ovrResult result = ovr_Initialize(nullptr);
	if (!OVR_SUCCESS(result))
		printf("Sorry Failed to initialize libOVR.\n");
	else
		printf("OVR initialized\n");



	ovrGraphicsLuid luid;
	result = ovr_Create(&HMD, &luid);
	if (!OVR_SUCCESS(result))
	{
		printf("Sorry Device creation failed\n");
	}
	else
		printf("OVR Device created\n");

	hmdDesc = ovr_GetHmdDesc(HMD);

	// Setup Window and Graphics
	// Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
	ovrSizei windowSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };

	if (initializeGL(hmdDesc.Resolution.w, hmdDesc.Resolution.h))
		printf("Oculus System Initialized\n");
	else
		printf("Sorry Oculus System Initialization Failed\n");
}

bool OculusSystem::initializeGL(const int width, const int height)
{
	winWidth = width;
	winHeight = height;

	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return false;
	}

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(width, height, "Oculus Renderer", NULL, NULL);

	if (window == NULL)
	{
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return false;
	}
	
	// Set mouse position to the center
	glfwSetCursorPos(window, winWidth / 2, winHeight / 2);

	glfwMakeContextCurrent(window);
	// Putting VSync OFF , Oculus SDK controls the frame rate
	glfwSwapInterval(0);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	

	// Initilizing GLE

	OVR::GLEContext::SetCurrentContext(&gleContext);
	gleContext.PlatformInit();
    int attribList[] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 2, WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB, 0 };
    HGLRC h = wglCreateContextAttribsARB(wglGetCurrentDC(), wglGetCurrentContext(), attribList);
	gleContext.Init();


	// Generating Main Frame buffer
	glGenFramebuffers(1, &fboID);

	glEnable(GL_DEPTH_TEST);

	// Our Indices are stored in CCW fashion 
	// Switching Back Culling off specific to our application

	//glFrontFace(GL_CCW);
	//glEnable(GL_CULL_FACE);

	//Initialize OpenCL 
	ctx = gpu::GPUContext::InitializeOpenCL(true);

	return true;
}

bool OculusSystem::LoadBuffers(){

	for (int eye = 0; eye < 2; ++eye)
	{
		eyeRenderTexture[eye] = nullptr;
		eyeDepthBuffer[eye] = nullptr;
	}
	mirrorTexture = nullptr;

	// Make eye render buffers
	for (int eye = 0; eye < 2; ++eye)
	{
		ovrSizei idealTextureSize = ovr_GetFovTextureSize(HMD, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
		eyeRenderTexture[eye] = new OVRTextureBuffer(HMD, true, true, idealTextureSize, 1, NULL, 1);
		eyeDepthBuffer[eye] = new OVRDepthBuffer(eyeRenderTexture[eye]->GetSize(), 0);

		if (!eyeRenderTexture[eye]->getTextureSet())
		{
			printf("Failed to create texture.\n");
			return false;
		}
	}

	// Create mirror texture and an FBO used to copy mirror texture to back buffer
	ovrResult result = ovr_CreateMirrorTextureGL(HMD, GL_SRGB8_ALPHA8, winWidth, winHeight, reinterpret_cast<ovrTexture**>(&mirrorTexture));
	if (!OVR_SUCCESS(result))
	{
		printf("Failed to create mirror texture\n");
		return false;
	}

	// Configure the mirror read buffer
	glGenFramebuffers(1, &mirrorFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTexture->OGL.TexId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);


	EyeRenderDesc[0] = ovr_GetRenderDesc(HMD, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
	EyeRenderDesc[1] = ovr_GetRenderDesc(HMD, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

	return true;

}

void OculusSystem::moveCamera(Vector3f & up, Vector3f & forward, Vector3f & pos)
{
	forward.Normalize();
	up.Normalize();
	Vector3f right = forward.Cross(up);
	right.Normalize();

	float walkThroughSpeed = 0.05f;

	// Move forward
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		pos += forward * walkThroughSpeed;
	}
	// Move backward
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		pos -= forward * walkThroughSpeed;
	}
	// Strafe right
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		pos += right *  walkThroughSpeed;
	}
	// Strafe left
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		pos -= right *  walkThroughSpeed;
	}

	if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
		pos += up * walkThroughSpeed;
	}

	if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
		pos -= up *  walkThroughSpeed;
	}

}

void OculusSystem::render(){

	Scene *scene = new Scene();
	scene->CreateScene();

	glEnable(GL_DEPTH_TEST);
	//glCullFace(GL_CW);
	Vector3f Pos2(0.0f, 0.0f, 0.0f);

	bool isVisible = true;

	
	double           ftiming = ovr_GetPredictedDisplayTime(HMD, 0);
	ovrTrackingState hmdState = ovr_GetTrackingState(HMD, ftiming, ovrTrue);
	ovrPosef cameraPose = hmdState.CameraPose;
	ovrPosef leveledCameraPose = hmdState.LeveledCameraPose;

	Matrix4f cameraPoseMat = Matrix4f(cameraPose);

	do{

		//Pos2.y = ovr_GetFloat(HMD, OVR_KEY_EYE_HEIGHT, Pos2.y);

		// Get eye poses, feeding in correct IPD offset
		ovrVector3f               ViewOffset[2] = { EyeRenderDesc[0].HmdToEyeViewOffset,
			EyeRenderDesc[1].HmdToEyeViewOffset };
		ovrPosef                  EyeRenderPose[2];

		double           ftiming = ovr_GetPredictedDisplayTime(HMD, 0);
		// Keeping sensorSampleTime as close to ovr_GetTrackingState as possible - fed into the layer
		double           sensorSampleTime = ovr_GetTimeInSeconds();
		ovrTrackingState hmdState = ovr_GetTrackingState(HMD, ftiming, ovrTrue);
		ovr_CalcEyePoses(hmdState.HeadPose.ThePose, ViewOffset, EyeRenderPose);

		if (isVisible)
		{
			for (int eye = 0; eye < 2; eye++)
			{
				// Increment to use next texture, just before writing
				eyeRenderTexture[eye]->getTextureSet()->CurrentIndex = (eyeRenderTexture[eye]->getTextureSet()->CurrentIndex + 1) % eyeRenderTexture[eye]->getTextureSet()->TextureCount;

				// Switch to eye render target
				eyeRenderTexture[eye]->SetAndClearRenderSurface(eyeDepthBuffer[eye]);

				// Get view and projection matrices
				Matrix4f rollPitchYaw = Matrix4f::RotationY(0.0f);
				Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(EyeRenderPose[eye].Orientation);
				Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
				Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));

				// Moving camera using Keyboard
				moveCamera(finalUp, finalForward, Pos2);

				Vector3f shiftedEyePos = Pos2 + rollPitchYaw.Transform(EyeRenderPose[eye].Position);

				Matrix4f view = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
				Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_RightHanded);

				scene->Render(view, proj, ctx);
				eyeRenderTexture[eye]->UnsetRenderSurface();
			}
		}

		// Do distortion rendering, Present and flush/sync

		// Set up positional data.
		ovrViewScaleDesc viewScaleDesc;
		viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
		viewScaleDesc.HmdToEyeViewOffset[0] = ViewOffset[0];
		viewScaleDesc.HmdToEyeViewOffset[1] = ViewOffset[1];

		ovrLayerEyeFov ld;
		ld.Header.Type = ovrLayerType_EyeFov;
		ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

		for (int eye = 0; eye < 2; ++eye)
		{
			ld.ColorTexture[eye] = eyeRenderTexture[eye]->getTextureSet();
			ld.Viewport[eye] = Recti(eyeRenderTexture[eye]->GetSize());
			ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
			ld.RenderPose[eye] = EyeRenderPose[eye];
			ld.SensorSampleTime = sensorSampleTime;
		}

		ovrLayerHeader* layers = &ld.Header;
		ovrResult result = ovr_SubmitFrame(HMD, 0, &viewScaleDesc, &layers, 1);
		// exit the rendering loop if submit returns an error
		if (!OVR_SUCCESS(result))
		{
			printf("Display Lost\n");
			break;
		}

		isVisible = (result == ovrSuccess);

		// Blit mirror texture to back buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		GLint w = mirrorTexture->OGL.Header.TextureSize.w;
		GLint h = mirrorTexture->OGL.Header.TextureSize.h;
		glBlitFramebuffer(0, h, w, 0,
			0, 0, w, h,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
		//Sleep(50);

	
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	glDisable(GL_DEPTH_TEST);
	delete scene;

}

OculusSystem::~OculusSystem()
{
	// Deleteing Mirror FBO and Texture
	if (mirrorFBO)
		glDeleteFramebuffers(1, &mirrorFBO);
	if (mirrorTexture)
		ovr_DestroyMirrorTexture(HMD, reinterpret_cast<ovrTexture*>(mirrorTexture));

	// Releasing Render Buffers
	for (int eye = 0; eye < 2; ++eye)
	{
		delete eyeRenderTexture[eye];
		delete eyeDepthBuffer[eye];
	}

	// Releasing Device
	if (fboID)
	{
		glDeleteFramebuffers(1, &fboID);
		fboID = 0;
	}
	gleContext.Shutdown();


	glfwTerminate();
	ovr_Destroy(HMD);

	shutdown();
	destroy();
}

void OculusSystem::shutdown()
{
	ovr_Shutdown();
}

void OculusSystem::destroy()
{
	OVR::System::Destroy();
}

