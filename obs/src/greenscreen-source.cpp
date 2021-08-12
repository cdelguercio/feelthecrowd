#include <obs-module.h>

#include "AigsEffectApp.h"

struct greenscreen {
	obs_source_t *source;
	FXApp *fx;
};

static const char *greenscreen_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("GreenScreenFilter");
}

static void greenscreen_destroy(void *data)
{
	struct greenscreen *gf = reinterpret_cast<greenscreen *>(data);

	if (gf) {
		obs_enter_graphics();

		bfree(gf);

		obs_leave_graphics();
	}
}

static void *greenscreen_create(obs_data_t *settings, obs_source_t *source)
{
	struct greenscreen *gf = reinterpret_cast<greenscreen *>(bzalloc(sizeof(struct greenscreen)));

	obs_enter_graphics();

	gf->source = source;

	gf->fx = new FXApp;

  FXApp::Err fxErr = FXApp::errNone;

  const char* modelDir = "/usr/local/VideoFX/lib/models";
  fxErr = gf->fx->appErrFromVfxStatus(gf->fx->createAigsEffect(modelDir));

  if (fxErr) std::cerr << "Error: " << gf->fx->errorStringFromCode(fxErr) << std::endl;

  if (!gf->fx) {
    greenscreen_destroy(gf);
    gf = NULL;
  }

	obs_leave_graphics();

	UNUSED_PARAMETER(settings);
	return gf;
}

// https://www.morethantechnical.com/2021/04/15/obs-plugin-for-portrait-background-removal-with-onnx-sinet-model/
// https://github.com/royshil/obs-backgroundremoval/blob/7554414dbb3c2b3c2358b7b619d4ed9a9a31ccca/src/background-filter.cpp
enum YUV_TYPE {
	YUV_TYPE_UYVY = 0,
	YUV_TYPE_YVYU = 1,
	YUV_TYPE_YUYV = 2,
};

void rgb_to_yuv(const cv::Mat& rgb, cv::Mat& yuv, YUV_TYPE yuvType = YUV_TYPE_UYVY) {
	assert(rgb.depth() == CV_8U &&
		   rgb.channels() == 3 &&
		   yuv.depth() == CV_8U &&
		   yuv.channels() == 2 &&
		   rgb.size() == yuv.size());

	for (int ih = 0; ih < rgb.rows; ih++) {
		const uint8_t* rgbRowPtr = rgb.ptr<uint8_t>(ih);
		uint8_t* yuvRowPtr = yuv.ptr<uint8_t>(ih);

		for (int iw = 0; iw < rgb.cols; iw = iw + 2) {
			const int rgbColIdxBytes = iw * rgb.elemSize();
			const int yuvColIdxBytes = iw * yuv.elemSize();

			const uint8_t R1 = rgbRowPtr[rgbColIdxBytes + 0];
			const uint8_t G1 = rgbRowPtr[rgbColIdxBytes + 1];
			const uint8_t B1 = rgbRowPtr[rgbColIdxBytes + 2];
			const uint8_t R2 = rgbRowPtr[rgbColIdxBytes + 3];
			const uint8_t G2 = rgbRowPtr[rgbColIdxBytes + 4];
			const uint8_t B2 = rgbRowPtr[rgbColIdxBytes + 5];

			const int Y  =  (0.257f * R1) + (0.504f * G1) + (0.098f * B1) + 16.0f ;
			const int U  = -(0.148f * R1) - (0.291f * G1) + (0.439f * B1) + 128.0f;
			const int V  =  (0.439f * R1) - (0.368f * G1) - (0.071f * B1) + 128.0f;
			const int Y2 =  (0.257f * R2) + (0.504f * G2) + (0.098f * B2) + 16.0f ;

			if (yuvType == YUV_TYPE_UYVY) {
				yuvRowPtr[yuvColIdxBytes + 0] = cv::saturate_cast<uint8_t>(U );
				yuvRowPtr[yuvColIdxBytes + 1] = cv::saturate_cast<uint8_t>(Y );
				yuvRowPtr[yuvColIdxBytes + 2] = cv::saturate_cast<uint8_t>(V );
				yuvRowPtr[yuvColIdxBytes + 3] = cv::saturate_cast<uint8_t>(Y2);
			}
			if (yuvType == YUV_TYPE_YVYU) {
				yuvRowPtr[yuvColIdxBytes + 0] = cv::saturate_cast<uint8_t>(Y );
				yuvRowPtr[yuvColIdxBytes + 1] = cv::saturate_cast<uint8_t>(V );
				yuvRowPtr[yuvColIdxBytes + 2] = cv::saturate_cast<uint8_t>(Y2);
				yuvRowPtr[yuvColIdxBytes + 3] = cv::saturate_cast<uint8_t>(U );
			}
			if (yuvType == YUV_TYPE_YUYV) {
				yuvRowPtr[yuvColIdxBytes + 0] = cv::saturate_cast<uint8_t>(Y );
				yuvRowPtr[yuvColIdxBytes + 1] = cv::saturate_cast<uint8_t>(U );
				yuvRowPtr[yuvColIdxBytes + 2] = cv::saturate_cast<uint8_t>(Y2);
				yuvRowPtr[yuvColIdxBytes + 3] = cv::saturate_cast<uint8_t>(V );
			}
		}
	}
}

cv::Mat convertFrameToRGB(struct obs_source_frame *frame) {
	cv::Mat imageRGB;
	if (frame->format == VIDEO_FORMAT_UYVY) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_UYVY);
	}
	if (frame->format == VIDEO_FORMAT_YVYU) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_YVYU);
	}
	if (frame->format == VIDEO_FORMAT_YUY2) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_YUY2);
	}
	if (frame->format == VIDEO_FORMAT_NV12) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_NV12);
	}
	if (frame->format == VIDEO_FORMAT_I420) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB_I420);
	}
	if (frame->format == VIDEO_FORMAT_I444) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC3, frame->data[0]);
		cv::cvtColor(imageYUV, imageRGB, cv::ColorConversionCodes::COLOR_YUV2RGB);
	}
	if (frame->format == VIDEO_FORMAT_RGBA) {
		cv::Mat imageRGBA(frame->height, frame->width, CV_8UC4, frame->data[0]);
		cv::cvtColor(imageRGBA, imageRGB, cv::ColorConversionCodes::COLOR_RGBA2RGB);
	}
	if (frame->format == VIDEO_FORMAT_BGRA) {
		cv::Mat imageBGRA(frame->height, frame->width, CV_8UC4, frame->data[0]);
		cv::cvtColor(imageBGRA, imageRGB, cv::ColorConversionCodes::COLOR_BGRA2RGB);
	}
	if (frame->format == VIDEO_FORMAT_Y800) {
		cv::Mat imageGray(frame->height, frame->width, CV_8UC1, frame->data[0]);
		cv::cvtColor(imageGray, imageRGB, cv::ColorConversionCodes::COLOR_GRAY2RGB);
	}
	return imageRGB;
}

void convertRGBToFrame(cv::Mat imageRGB, struct obs_source_frame *frame) {
	if (frame->format == VIDEO_FORMAT_UYVY) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		rgb_to_yuv(imageRGB, imageYUV, YUV_TYPE_UYVY);
	}
	if (frame->format == VIDEO_FORMAT_YVYU) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		rgb_to_yuv(imageRGB, imageYUV, YUV_TYPE_YVYU);
	}
	if (frame->format == VIDEO_FORMAT_YUY2) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		rgb_to_yuv(imageRGB, imageYUV, YUV_TYPE_YUYV);
	}
	if (frame->format == VIDEO_FORMAT_NV12) {
		cv::Mat imageYUV(frame->height, frame->width * 3 / 2, CV_8UC1, frame->data[0]);
		cv::cvtColor(imageRGB, imageYUV, cv::ColorConversionCodes::COLOR_RGB2YUV_YV12);
	}
	if (frame->format == VIDEO_FORMAT_I420) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC2, frame->data[0]);
		cv::cvtColor(imageRGB, imageYUV, cv::ColorConversionCodes::COLOR_RGB2YUV_I420);
	}
	if (frame->format == VIDEO_FORMAT_I444) {
		cv::Mat imageYUV(frame->height, frame->width, CV_8UC3, frame->data[0]);
		cv::cvtColor(imageRGB, imageYUV, cv::ColorConversionCodes::COLOR_RGB2YUV);
	}
	if (frame->format == VIDEO_FORMAT_RGBA) {
		cv::Mat imageRGBA(frame->height, frame->width, CV_8UC4, frame->data[0]);
		cv::cvtColor(imageRGB, imageRGBA, cv::ColorConversionCodes::COLOR_RGB2RGBA);
	}
	if (frame->format == VIDEO_FORMAT_BGRA) {
		cv::Mat imageBGRA(frame->height, frame->width, CV_8UC4, frame->data[0]);
		cv::cvtColor(imageRGB, imageBGRA, cv::ColorConversionCodes::COLOR_RGB2BGRA);
	}
	if (frame->format == VIDEO_FORMAT_Y800) {
		cv::Mat imageGray(frame->height, frame->width, CV_8UC1, frame->data[0]);
		cv::cvtColor(imageRGB, imageGray, cv::ColorConversionCodes::COLOR_RGB2GRAY);
	}
}

static struct obs_source_frame *
greenscreen_video(void *data, struct obs_source_frame *frame)
{
  struct greenscreen *gf = reinterpret_cast<greenscreen *>(data);

  FXApp::Err fxErr = FXApp::errNone;

  cv::Mat inputRGB = convertFrameToRGB(frame);
  cv::Mat outputRGB;
  fxErr = gf->fx->processImage(inputRGB, outputRGB);
  if (fxErr) std::cerr << "Error: " << gf->fx->errorStringFromCode(fxErr) << std::endl;

  convertRGBToFrame(outputRGB, frame);

  return frame;
}

static void greenscreen_render(void *data, gs_effect_t *effect)
{
	struct greenscreen *gf = reinterpret_cast<greenscreen *>(data);

	UNUSED_PARAMETER(effect);
}

struct obs_source_info greenscreen = {
	.id = "greenscreen",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO,
	.get_name = greenscreen_name,
	.create = greenscreen_create,
	.destroy = greenscreen_destroy,
//	.update = greenscreen_update,
//	.get_properties = greenscreen_properties,
	.filter_video = greenscreen_video,
//	.filter_remove = greenscreen_remove,
	//.video_render = greenscreen_render,
};
