// tutorial01.c
// Code based on a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
// Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1
// With updates from https://github.com/chelyaev/ffmpeg-tutorial
// Updates tested on:
// LAVC 54.59.100, LAVF 54.29.104, LSWS 2.1.101
// on GCC 4.7.2 in Debian February 2015

// A small sample program that shows how to use libavformat and libavcodec to
// read video from a file.
//
// Use
//
// gcc -o tutorial01 tutorial01.c -lavformat -lavcodec -lswscale -lz
//
// to build (assuming libavformat and libavcodec are correctly installed
// your system).
//
// Run using
//
// tutorial01 myvideofile.mpg
//
// to write the first five frames from "myvideofile.mpg" to disk in PPM
// format.
// gcc -o mpeg_codec mpeg_codec.c -lavutil -lavformat -lavcodec -lz -lavutil -lm -lswscale

#include <libavcodec\avcodec.h>
#include <libavformat\avformat.h>
#include <libswscale\swscale.h>

#include <assert.h>
#include <stdio.h>

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

// Initalizing these to NULL prevents segfaults!
AVFormatContext   *pFormatCtx = NULL;
int               i, videoStream;
AVCodecContext    *pCodecCtxOrig = NULL;
AVCodecContext    *pCodecCtx = NULL;
AVCodec           *pCodec = NULL;
AVFrame           *pFrame = NULL;
AVFrame           *pFrameRGB = NULL;
AVPacket          packet;
int               frameFinished;
int               numBytes;
uint8_t           *buffer = NULL;
struct SwsContext *sws_ctx = NULL;

AVFrame *data = NULL;

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;

  // Open file
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;

  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

  // Close file
  fclose(pFile);
}

int allFramesRead = 0;

AVFrame *getFrame() {
  int readSuccess = av_read_frame(pFormatCtx, &packet);
  if (readSuccess < 0) {
    allFramesRead = 1;
    return NULL;
  }

  // Is this a packet from the video stream?
  if(packet.stream_index==videoStream) {
    // Decode video frame
    avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

    // Did we get a video frame?
    if(frameFinished) {
      // Convert the image from its native format to RGB
      sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
        pFrame->linesize, 0, pCodecCtx->height,
        pFrameRGB->data, pFrameRGB->linesize);

      // Save the frame to disk
      i++;
      data = pFrameRGB;
      printf("frame %d pFrameRGB %p\n", i, pFrameRGB);
      return pFrameRGB;
      // if (i < 5)
        // SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
    }
  }

  // Free the packet that was allocated by av_read_frame
  av_free_packet(&packet);
}

void initEverything(const char *videoFile) {
  // Register all formats and codecs
  av_register_all();

  // Open video file
  if(avformat_open_input(&pFormatCtx, videoFile, NULL, NULL)!=0)
    exit(1); // Couldn't open file

  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    exit(1); // Couldn't find stream information

  // Dump information about file onto standard error
  av_dump_format(pFormatCtx, 0, videoFile, 0);

  // Find the first video stream
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    exit(1); // Didn't find a video stream

  // Get a pointer to the codec context for the video stream
  pCodecCtxOrig=pFormatCtx->streams[videoStream]->codec;
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    exit(1); // Codec not found
  }
  // Copy context
  pCodecCtx = avcodec_alloc_context3(pCodec);
  if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
    fprintf(stderr, "Couldn't copy codec context");
    exit(1); // Error copying codec context
  }

  // Open codec
  if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
    exit(1); // Could not open codec

  // Allocate video frame
  pFrame=av_frame_alloc();

  // Allocate an AVFrame structure
  pFrameRGB=av_frame_alloc();
  if(pFrameRGB==NULL)
    exit(1);

  // Determine required buffer size and allocate buffer
  numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
            pCodecCtx->height);
  buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
  // of AVPicture
  avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
     pCodecCtx->width, pCodecCtx->height);

  // initialize SWS context for software scaling
  sws_ctx = sws_getContext(pCodecCtx->width,
         pCodecCtx->height,
         pCodecCtx->pix_fmt,
         pCodecCtx->width,
         pCodecCtx->height,
         PIX_FMT_RGB24,
         SWS_BILINEAR,
         NULL,
         NULL,
         NULL
         );
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "usage: mpeg_codec <video_file>\n");
    return -1;
  }

  const char *videoFile = argv[1];

  if(argc < 2) {
    printf("Please provide a movie file\n");
    return -1;
  }

  initEverything(videoFile);

  // Read frames and save first five frames to disk
  i=0;
  while(1) {
    data = getFrame();
    printf("data pointer %p\n", data);
    if (allFramesRead) {
      printf("all frames read\n");
      break;
    } else {
      // assert(data != NULL);
    }
  }

  // Free the RGB image
  av_free(buffer);
  av_frame_free(&pFrameRGB);

  // Free the YUV frame
  av_frame_free(&pFrame);

  // Close the codecs
  avcodec_close(pCodecCtx);
  avcodec_close(pCodecCtxOrig);

  // Close the video file
  avformat_close_input(&pFormatCtx);

  return 0;
}
