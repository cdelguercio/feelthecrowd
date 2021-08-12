#include <stdio.h>
#include <string.h>
#include <time.h>

#include <chrono>
#include <string>
#include <iostream>

#include "nvCVOpenCV.h"
#include "nvVideoEffects.h"
#include "opencv2/opencv.hpp"

#include "AigsEffectApp.h"

#define BAIL_IF_ERR(err) \
  do {                   \
    if (0 != (err)) {    \
      goto bail;         \
    }                    \
  } while (0)
#define BAIL_IF_NULL(x, err, code) \
  do {                             \
    if ((void *)(x) == NULL) {     \
      err = code;                  \
      goto bail;                   \
    }                              \
  } while (0)
#define NVCV_ERR_HELP 411

#define DEFAULT_CODEC "H264"

bool        FLAG_progress = false;
bool        FLAG_show     = false;
bool        FLAG_useOTAU  = false;
bool        FLAG_verbose  = false;
bool        FLAG_webcam   = false;
int         FLAG_compMode = 3 /*compWhite*/;
int         FLAG_mode     = 0;
float       FLAG_blurStrength = 0.5;
std::string FLAG_camRes;
std::string FLAG_codec    = DEFAULT_CODEC;
std::string FLAG_inFile;
std::string FLAG_modelDir;
std::string FLAG_outDir;
std::string FLAG_outFile;

FXApp::FXApp() {
  _eff = nullptr;
  _bgblurEff = nullptr;
  _effectName = nullptr;
  _inited = false;
  _total = 0.0;
  _count = 0;
  _compMode = compLight;
  _showFPS = false;
  _stream = nullptr;
  _progress = false;
  _show = false;
  _framePeriod = 0.f;
  _lastTime = std::chrono::high_resolution_clock::time_point::min();
  aigsOTAUpdatesPath = NULL;
  _blurStrength = 0.5f;
}

FXApp::~FXApp() {
  NvVFX_DestroyEffect(_eff);
  NvVFX_DestroyEffect(_bgblurEff);

  if (_stream) {
    NvVFX_CudaStreamDestroy(_stream);
  }
}

const char* FXApp::errorStringFromCode(Err code) {
  struct LutEntry { Err code; const char *str; };
  static const LutEntry lut[] = {
    { errRead,    "There was a problem reading a file"                    },
    { errWrite,   "There was a problem writing a file"                    },
    { errQuit,    "The user chose to quit the application"                },
    { errFlag,    "There was a problem with the command-line arguments"   },
  };
  if ((int)code <= 0) return NvCV_GetErrorStringFromCode((NvCV_Status)code);
  for (const LutEntry *p = lut; p != &lut[sizeof(lut) / sizeof(lut[0])]; ++p)
    if (p->code == code) return p->str;
  return "UNKNOWN ERROR";
}

NvCV_Status FXApp::createAigsEffect(const char *modelDir) {
  NvCV_Status vfxErr;

  vfxErr = NvVFX_CreateEffect(NVVFX_FX_GREEN_SCREEN, &_eff);
  if (NVCV_SUCCESS != vfxErr) {
    std::cerr << "Error creating effect \"" << NVVFX_FX_GREEN_SCREEN << "\"\n";
    return vfxErr;
  }
  _effectName = NVVFX_FX_GREEN_SCREEN;

  vfxErr = NvVFX_SetString(_eff, NVVFX_MODEL_DIRECTORY, modelDir);
//  if (!FLAG_modelDir.empty()) {
//    vfxErr = NvVFX_SetString(_eff, NVVFX_MODEL_DIRECTORY, FLAG_modelDir.c_str());
//  } else if (FLAG_useOTAU && aigsOTAUpdatesPath && aigsOTAUpdatesPath[0]) {
//    vfxErr = NvVFX_SetString(_eff, NVVFX_MODEL_DIRECTORY, aigsOTAUpdatesPath);
//  }
  if (vfxErr != NVCV_SUCCESS) {
    std::cerr << "Error setting the model path to \"" << FLAG_modelDir << "\"\n";
    return vfxErr;
  }

  const char *cstr;  // TODO: This is not necessary
  vfxErr = NvVFX_GetString(_eff, NVVFX_INFO, &cstr);
  if (vfxErr != NVCV_SUCCESS) {
    std::cerr << "AIGS modes not found \n" << std::endl;
    return vfxErr;
  }

  // Choose one mode -> set() -> Load() -> Run()
  vfxErr = NvVFX_SetU32(_eff, NVVFX_MODE, FLAG_mode);
  if (vfxErr != NVCV_SUCCESS) {
    std::cerr << "Error setting the mode \n";
    return vfxErr;
  }

  vfxErr = NvVFX_CudaStreamCreate(&_stream);
  if (vfxErr != NVCV_SUCCESS) {
    std::cerr << "Error creating CUDA stream " << std::endl;
    return vfxErr;
  }

  vfxErr = NvVFX_SetCudaStream(_eff, NVVFX_CUDA_STREAM, _stream);
  if (vfxErr != NVCV_SUCCESS) {
    std::cerr << "Error setting up the cuda stream \n";
    return vfxErr;
  }

  vfxErr = NvVFX_Load(_eff);
  if (vfxErr != NVCV_SUCCESS) {
    std::cerr << "Error loading the model \n";
    return vfxErr;
  }

  return vfxErr;
}

void FXApp::destroyEffect() {
  NvVFX_DestroyEffect(_eff);
  _eff = nullptr;
}

static void overlay(const cv::Mat &image, const cv::Mat &mask, float alpha, cv::Mat &result) {
  cv::Mat maskClr;
  cv::cvtColor(mask, maskClr, cv::COLOR_GRAY2BGR);
  result = image * (1.f - alpha) + maskClr * alpha;
}

FXApp::Err FXApp::processImage(const cv::Mat &input, cv::Mat &output) {
  NvCV_Status vfxErr;
  bool ok;

  if (!_eff) return errEffect;

  _dstImg = cv::Mat::zeros(input.size(), CV_8UC1);
  if (!_dstImg.data) {
    return errMemory;
  }

  (void)NVWrapperForCVMat(&input, &_srcVFX);
  (void)NVWrapperForCVMat(&_dstImg, &_dstVFX);

  NvCVImage fxSrcChunkyGPU(input.cols, input.rows, NVCV_BGR, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);
  NvCVImage fxDstChunkyGPU(input.cols, input.rows, NVCV_A, NVCV_U8, NVCV_CHUNKY, NVCV_GPU, 1);

  BAIL_IF_ERR(vfxErr = NvVFX_SetImage(_eff, NVVFX_INPUT_IMAGE, &fxSrcChunkyGPU));
  BAIL_IF_ERR(vfxErr = NvVFX_SetImage(_eff, NVVFX_OUTPUT_IMAGE, &fxDstChunkyGPU));
  BAIL_IF_ERR(vfxErr = NvCVImage_Transfer(&_srcVFX, &fxSrcChunkyGPU, 1.0f, _stream, NULL));
  BAIL_IF_ERR(vfxErr = NvVFX_Run(_eff, 0));
  BAIL_IF_ERR(vfxErr = NvCVImage_Transfer(&fxDstChunkyGPU, &_dstVFX, 1.0f, _stream, NULL));

  overlay(input, _dstImg, 0.5, output);

  if (_show) {
    cv::imshow("Output", output);
    cv::waitKey(3000);
  }
bail:
  return (FXApp::Err)vfxErr;
}
