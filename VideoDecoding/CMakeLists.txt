INCLUDE_DIRECTORIES("${OculusRenderer_SOURCE_DIR}/VideoDecoding")


SET( HEADERS 
  "arithmetic_codec.h"
  )
SET( SOURCES
  "arithmetic_codec.cpp"
  )

set(D_HEADERS
    "wavelet.h"
    "decoder.h"
    )
set(D_SOURCES
    "wavelet.cpp"
    "decoder.cpp"
    )
    
add_library(mptc_decoder ${D_HEADERS} ${D_SOURCES})
find_package(Threads REQUIRED)
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(mptc_decoder "${CMAKE_THREAD_LIBS_INIT}")
endif()
target_link_libraries(mptc_decoder arith_codec)

add_executable(mpeg_codec mpeg_codec.c)
target_link_libraries(mpeg_codec avutil avformat avcodec z avutil m swscale)

ADD_LIBRARY(arith_codec ${HEADERS} ${SOURCES})
