//


#include "OculusSystem.h"
int main(int argc, const char *argv[]){


	OculusSystem* OVRSystem = new OculusSystem();
	OVRSystem->initialize();

	if (!OVRSystem->LoadBuffers())
		printf("Sorry loading of the required buffers failed \n");
	else
		printf("All Buffers loaded \n");
	OVRSystem->render();

	delete OVRSystem;

	/*int TextureNumber = 0;
	unsigned char *Pixel = NULL;
	unsigned char *blocks;
	blocks = (unsigned char*)malloc((kImageHeight*kImageWidth / 2)*sizeof(unsigned char));
	std::ofstream outfile;
	while (TextureNumber < 720){
		char fileNameIn[256];
		char fileNameOut[256];
		TextureNumber++;

		sprintf(fileNameIn, "RenderStuff//Textures//sunset//frame%04d.jpg", TextureNumber);

		sprintf(fileNameOut, "RenderStuff//Textures//sunsetCompressedDXT1//frame%04d.DXT1", TextureNumber);
		outfile.open(fileNameOut, std::ios::out | std::ios::binary);
		int x, y, n;

		Pixel = stbi_load(fileNameIn, &x, &y, &n, 4);
		squish::CompressImage(Pixel, kImageWidth, kImageHeight, blocks, squish::kDxt1);
		outfile.write(reinterpret_cast<char*>(blocks), (kImageHeight*kImageWidth / 2)*sizeof(unsigned char));
		outfile.close();


	}*/

	return 0;
}