include_directories("${OculusRenderer_SOURCE_DIR}/libs/glfw-3.1/include/")
include_directories("${OculusRenderer_SOURCE_DIR}/libs/glm/")
include_directories("${OculusRenderer_SOURCE_DIR}/libs/glew-1.13.0/include/")

include_directories("${OculusRenderer_SOURCE_DIR}/libs/")
include_directories("${OculusRenderer_SOURCE_DIR}/VideoDecoding/")

include_directories("${OculusRenderer_SOURCE_DIR}/OculusSDK/LibOVRKernel/src")

include_directories("${OculusRenderer_SOURCE_DIR}/OculusSDK/LibOVR/Include/")

include_directories("${OculusRenderer_SOURCE_DIR}/OculusSDK/LibOVR/Include/")

#GENTC Include files....
include_directories("${OculusRenderer_SOURCE_DIR}/GenTC/demo")
include_directories("${OculusRenderer_BINARY_DIR}/GenTC/demo")

include_directories("${OculusRenderer_SOURCE_DIR}/GenTC/codec")
include_directories("${OculusRenderer_SOURCE_DIR}/GenTC/gpu")
include_directories(${OPENCL_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(${OculusRenderer_SOURCE_DIR}/googletest/include)
INCLUDE_DIRECTORIES("Include/")

SET( HEADERS
	"Include/Model.h"
	"Include/ModelData.h"
	"Include/ObjLoader.h"
	"Include/OculusSystem.h"
	"Include/OVRDepthBuffer.h"
	"Include/OVRTextureBuffer.h"
	"Include/Scene.h"
	""
)

SET( SOURCES
	"Src/Main.cpp"
	"Src/Model.cpp"
	"Src/ObjLoader.cpp"
	"Src/OculusSystem.cpp"
	"Src/OVRDepthBuffer.cpp"
	"Src/OVRTextureBuffer.cpp"
	"Src/Scene.cpp"	
)

ADD_EXECUTABLE( Renderer ${HEADERS} ${SOURCES})
TARGET_LINK_LIBRARIES( Renderer glfw )
TARGET_LINK_LIBRARIES(Renderer ovr)
TARGET_LINK_LIBRARIES(Renderer ans)
TARGET_LINK_LIBRARIES(Renderer gentc_gpu)
TARGET_LINK_LIBRARIES(Renderer gentc_codec)
TARGET_LINK_LIBRARIES(Renderer ${GLFW_LIBRARIES})
TARGET_LINK_LIBRARIES(Renderer GLEW_1130)
TARGET_LINK_LIBRARIES(Renderer ${OPENCL_LIBRARIES})
TARGET_LINK_LIBRARIES(Renderer ${OPENGL_LIBRARY})
TARGET_LINK_LIBRARIES(Renderer mptc_decoder)
