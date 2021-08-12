#ifndef AIGSEFFECTAPP_H
#define AIGSEFFECTAPP_H

#include "nvVideoEffects.h"
#include "opencv2/opencv.hpp"

struct FXApp {
  enum Err {
    errQuit = +1,                              // Application errors
    errFlag = +2,
    errRead = +3,
    errWrite = +4,
    errNone = NVCV_SUCCESS,                     // Video Effects SDK errors
    errGeneral = NVCV_ERR_GENERAL,
    errUnimplemented = NVCV_ERR_UNIMPLEMENTED,
    errMemory = NVCV_ERR_MEMORY,
    errEffect = NVCV_ERR_EFFECT,
    errSelector = NVCV_ERR_SELECTOR,
    errBuffer = NVCV_ERR_BUFFER,
    errParameter = NVCV_ERR_PARAMETER,
    errMismatch = NVCV_ERR_MISMATCH,
    errPixelFormat = NVCV_ERR_PIXELFORMAT,
    errModel = NVCV_ERR_MODEL,
    errLibrary = NVCV_ERR_LIBRARY,
    errInitialization = NVCV_ERR_INITIALIZATION,
    errFileNotFound = NVCV_ERR_FILE,
    errFeatureNotFound = NVCV_ERR_FEATURENOTFOUND,
    errMissingInput = NVCV_ERR_MISSINGINPUT,
    errResolution = NVCV_ERR_RESOLUTION,
    errUnsupportedGPU = NVCV_ERR_UNSUPPORTEDGPU,
    errWrongGPU = NVCV_ERR_WRONGGPU,
    errCudaMemory = NVCV_ERR_CUDA_MEMORY,       // CUDA errors
    errCudaValue = NVCV_ERR_CUDA_VALUE,
    errCudaPitch = NVCV_ERR_CUDA_PITCH,
    errCudaInit = NVCV_ERR_CUDA_INIT,
    errCudaLaunch = NVCV_ERR_CUDA_LAUNCH,
    errCudaKernel = NVCV_ERR_CUDA_KERNEL,
    errCudaDriver = NVCV_ERR_CUDA_DRIVER,
    errCudaUnsupported = NVCV_ERR_CUDA_UNSUPPORTED,
    errCudaIllegalAddress = NVCV_ERR_CUDA_ILLEGAL_ADDRESS,
    errCuda = NVCV_ERR_CUDA,
  };
  enum CompMode { compMatte, compLight, compGreen, compWhite, compNone, compBG, compBlur };

  FXApp();
  ~FXApp();

  void setShow(bool show) { _show = show; }
  NvCV_Status createAigsEffect(const char *modelDir);
  void destroyEffect();
  NvCV_Status allocBuffers(unsigned width, unsigned height);
  NvCV_Status allocTempBuffers();
  //Err processImage(const char *inFile, const char *outFile);
  Err processImage(const cv::Mat &input, cv::Mat &output);
  Err processMovie(const char *inFile, const char *outFile);
  Err processKey(int key);
  void nextCompMode();
  void drawFrameRate(cv::Mat &img);
  Err appErrFromVfxStatus(NvCV_Status status) { return (Err)status; }
  const char *errorStringFromCode(Err code);

  NvVFX_Handle _eff, _bgblurEff;
  cv::Mat _srcImg;
  cv::Mat _dstImg;
  NvCVImage _srcVFX;
  NvCVImage _dstVFX;
  bool _show;
  bool _inited;
  bool _showFPS;
  bool _progress;
  const char *_effectName;
  float _total;
  int _count;
  CompMode _compMode;
  float _framePeriod;
  CUstream _stream;
  std::chrono::high_resolution_clock::time_point _lastTime;
  NvCVImage _srcNvVFXImage;
  NvCVImage _dstNvVFXImage;
  NvCVImage _blurNvVFXImage;
  char *aigsOTAUpdatesPath;
  float _blurStrength;
};

#endif
