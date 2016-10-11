#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "TextureLoader.h"

using namespace std;

GLuint TextureLoader::loadTexture(const char* fileName)
{
	// Determine the format of the image.
	// Note: The second paramter ('size') is currently unused, and we should use 0 for it.
	FREE_IMAGE_FORMAT format = FreeImage_GetFileType(fileName, 0);

	// Image not found? Abort! Without this section we get a 0 by 0 image with 0 bits-per-pixel but we don't abort, which
	// you might find preferable to dumping the user back to the desktop.
	if (format == -1)
	{
		return -1;
	}

	// Found image, but couldn't determine the file format? Try again...
	if (format == FIF_UNKNOWN)
	{
		printf("Couldn't determine file format - attempting to get from file extension...");

		// ...by getting the filetype from the filename extension (i.e. .PNG, .GIF etc.)
		// Note: This is slower and more error-prone that getting it from the file itself,
		// also, we can't use the 'U' (unicode) variant of this method as that's Windows only.
		format = FreeImage_GetFIFFromFilename(fileName);

		// Check that the plugin has reading capabilities for this format (if it's FIF_UNKNOWN,
		// for example, then it won't have) - if we can't read the file, then we bail out =(
		if (!FreeImage_FIFSupportsReading(format))
		{
			printf("Detected image format cannot be read!");
			return -1;
		}
	}

	// If we're here we have a known image format, so load the image into a bitap
	FIBITMAP* bitmap = FreeImage_Load(format, fileName);

	// How many bits-per-pixel is the source image?
	int bitsPerPixel = FreeImage_GetBPP(bitmap);

	// Convert our image up to 32 bits (8 bits per channel, Red/Green/Blue/Alpha) -
	// but only if the image is not already 32 bits (i.e. 8 bits per channel).
	// Note: ConvertTo32Bits returns a CLONE of the image data - so if we
	// allocate this back to itself without using our bitmap32 intermediate
	// we will LEAK the original bitmap data, and valgrind will show things like this:
	//
	// LEAK SUMMARY:
	//  definitely lost: 24 bytes in 2 blocks
	//  indirectly lost: 1,024,874 bytes in 14 blocks    <--- Ouch.
	//
	// Using our intermediate and cleaning up the initial bitmap data we get:
	//
	// LEAK SUMMARY:
	//  definitely lost: 16 bytes in 1 blocks
	//  indirectly lost: 176 bytes in 4 blocks
	//
	// All above leaks (192 bytes) are caused by XGetDefault (in /usr/lib/libX11.so.6.3.0) - we have no control over this.
	//
	FIBITMAP* bitmap32;
	if (bitsPerPixel == 32)
	{
		cout << "Source image has " << bitsPerPixel << " bits per pixel. Skipping conversion." << endl;
		bitmap32 = bitmap;
	}
	else
	{
		cout << "Source image has " << bitsPerPixel << " bits per pixel. Converting to 32-bit colour." << endl;
		bitmap32 = FreeImage_ConvertTo32Bits(bitmap);
	}

	// Some basic image info - strip it out if you don't care
	int imageWidth = FreeImage_GetWidth(bitmap32);
	int imageHeight = FreeImage_GetHeight(bitmap32);
	cout << "Image: " << fileName << " is size: " << imageWidth << "x" << imageHeight << "." << endl;

	// Get a pointer to the texture data as an array of unsigned bytes.
	// Note: At this point bitmap32 ALWAYS holds a 32-bit colour version of our image - so we get our data from that.
	// Also, we don't need to delete or delete[] this textureData because it's not on the heap (so attempting to do
	// so will cause a crash) - just let it go out of scope and the memory will be returned to the stack.
	GLubyte* textureData = FreeImage_GetBits(bitmap32);

	// Generate a texture ID and bind to it
	GLuint tempTextureID;
	glGenTextures(1, &tempTextureID);
	glBindTexture(GL_TEXTURE_2D, tempTextureID);

	// Construct the texture.
	// Note: The 'Data format' is the format of the image data as provided by the image library. FreeImage decodes images into
	// BGR/BGRA format, but we want to work with it in the more common RGBA format, so we specify the 'Internal format' as such.
	glTexImage2D(GL_TEXTURE_2D,    // Type of texture
		0,                // Mipmap level (0 being the top level i.e. full size)
		GL_SRGB8_ALPHA8,          // Internal format
		imageWidth,       // Width of the texture
		imageHeight,      // Height of the texture,
		0,                // Border in pixels
		GL_BGRA,          // Data format
		GL_UNSIGNED_BYTE, // Type of texture data
		textureData);     // The image data to use for this texture

	// Specify our minification and magnification filters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);



	// Check for OpenGL texture creation errors
	GLenum glError = glGetError();
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
	}

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

	// Finally, return the texture ID
	return tempTextureID;
}

void TextureLoader::saveBitmap(const char* filename, BitmapData bitmapDesc)
{

	FIBITMAP* bitmap = NULL;

	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(filename);
	if (fif == FIF_UNKNOWN)
	{
		fprintf(stderr, "[SNAPSHOT] Unrecognized image extension in %s\n", filename);
		return;
	}


	if (fif == FIF_EXR || fif == FIF_JXR || fif == FIF_TIFF)
	{
		// Use the raw float data
		bitmap = FreeImage_ConvertFromRawBitsEx(false, (BYTE*)&bitmapDesc.pixelData[0], FIT_RGBAF, bitmapDesc.width, bitmapDesc.height, bitmapDesc.width*sizeof(glm::vec4), 128, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);
	}
	else
	{
		// convert to 16bit unsigned
		std::vector<glm::uint16> rawData;
		rawData.reserve(bitmapDesc.width*bitmapDesc.height * 4);

		// Copy the data
		for (int j = 0; j < bitmapDesc.height; j++)
		for (int k = 0; k < bitmapDesc.width; k++)
		{
			// Use the tone-mapped data
			glm::vec4 sample = vec4((float)bitmapDesc.pixelDataToneMapped[j*bitmapDesc.width * 4 + k * 4], (float)bitmapDesc.pixelDataToneMapped[j*bitmapDesc.width * 4 + k * 4 + 1], (float)bitmapDesc.pixelDataToneMapped[j*bitmapDesc.width * 4 + k * 4 + 2], (float)bitmapDesc.pixelDataToneMapped[j*bitmapDesc.width * 4 + k * 4 + 3]);
			sample.a = 1.f;

			rawData.push_back(glm::clamp(sample.r, 0.f, 1.f) * 65535);
			rawData.push_back(glm::clamp(sample.g, 0.f, 1.f) * 65535);
			rawData.push_back(glm::clamp(sample.b, 0.f, 1.f) * 65535);
			rawData.push_back(glm::clamp(sample.a, 0.f, 1.f) * 65535);
		}

		// Make a copy of the data when allocating the image since the array will be out of scope and destroyed when we save
		bitmap = FreeImage_ConvertFromRawBitsEx(true, (BYTE*)&rawData[0], FIT_RGBA16, bitmapDesc.width, bitmapDesc.height, bitmapDesc.width*sizeof(glm::uint16) * 4, 64, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK);

		// Check if the file format supports 16bit, convert to 8bit otherwise
		if (bitmap && !(fif == FIF_J2K || fif == FIF_JP2 || fif == FIF_JXR || fif == FIF_PNG || fif == FIF_TIFF))
		{
			FIBITMAP* bitmap32 = FreeImage_ConvertTo32Bits(bitmap);
			if (bitmap32)
			{
				FreeImage_Unload(bitmap);
				bitmap = bitmap32;
			}
		}
	}

	// Set image compression parameters
	int saveFlags = 0;
	switch (fif) {
	case FIF_EXR:
		saveFlags |= EXR_FLOAT | EXR_ZIP;
		break;
	case FIF_PNG:
		saveFlags |= PNG_Z_NO_COMPRESSION;
		break;
	case FIF_JPEG:
		saveFlags |= JPEG_QUALITYSUPERB;
		break;
	case FIF_TIFF:
		saveFlags |= TIFF_DEFLATE;
		break;
	case FIF_JXR:
		saveFlags |= JXR_LOSSLESS;
	}

	// Save the file
	if (bitmap)
	{
		FreeImage_Save(fif, bitmap, filename, saveFlags);
		FreeImage_Unload(bitmap);
	}
}