#include "OVRDepthBuffer.h"

OVRDepthBuffer::OVRDepthBuffer(Sizei size, int sampleCount)
{
	OVR_ASSERT(sampleCount <= 1); // The code doesn't currently handle MSAA textures.

	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLenum internalFormat = GL_DEPTH_COMPONENT24;
	GLenum type = GL_UNSIGNED_INT;
	if (GLE_ARB_depth_buffer_float)
	{
		internalFormat = GL_DEPTH_COMPONENT32F;
		type = GL_FLOAT;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.w, size.h, 0, GL_DEPTH_COMPONENT, type, NULL);
}

OVRDepthBuffer::~OVRDepthBuffer()
{
	if (texId)
	{
		glDeleteTextures(1, &texId);
		texId = 0;
	}
}
