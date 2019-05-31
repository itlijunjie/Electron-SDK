/*
* Copyright (c) 2019 Agora.io
* All rights reserved.
* Proprietary and Confidential -- Agora.io
*/

#include "node_video_frame.h"
#include <stdlib.h>
#include <iostream>
#include "funama.h"
#include "FUConfig.h"
#include "Utils.h"
#include <vector>

#if defined(_WIN32)
#pragma comment(lib, "nama.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#endif

#if defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#endif

namespace agora {
    namespace rtc {
        static bool	m_namaInited = false;
        static int mFrameID = 0;
        static int mBeautyHandles = 0;
        #if defined(_WIN32)
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1u,
            PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW,
            PFD_TYPE_RGBA,
            32u,
            0u, 0u, 0u, 0u, 0u, 0u,
            8u,
            0u,
            0u,
            0u, 0u, 0u, 0u,
            24u,
            8u,
            0u,
            PFD_MAIN_PLANE,
            0u,
            0u, 0u };
        #endif

        void InitOpenGL() {
            #if defined(_WIN32)
            HWND hw = CreateWindowExA(
                0, "EDIT", "", ES_READONLY,
                0, 0, 1, 1,
                NULL, NULL,
                GetModuleHandleA(NULL), NULL);
            HDC hgldc = GetDC(hw);
            int spf = ChoosePixelFormat(hgldc, &pfd);
            int ret = SetPixelFormat(hgldc, spf, &pfd);
            HGLRC hglrc = wglCreateContext(hgldc);
            wglMakeCurrent(hgldc, hglrc);

            //hglrc就是创建出的OpenGL context
            printf("hw=%08x hgldc=%08x spf=%d ret=%d hglrc=%08x\n",
                hw, hgldc, spf, ret, hglrc);
            #endif

            #if defined(__APPLE__)
            CGLPixelFormatAttribute attrib[] = {kCGLPFADoubleBuffer};
            CGLPixelFormatObj pixelFormat = NULL;
            GLint numPixelFormats = 0;
            CGLContextObj cglContext1 = NULL;
            CGLChoosePixelFormat (attrib, &pixelFormat, &numPixelFormats);
            CGLCreateContext(pixelFormat, NULL, &cglContext1);
            CGLSetCurrentContext(cglContext1);
            std::cout << "mac InitOpenGL" << std::endl;
            #endif
        }

        NodeVideoFrameObserver::NodeVideoFrameObserver(char* authdata, int authsize) {
			do {
                auth_package_size = authsize;
                auth_package = new char[authsize];
                memcpy(auth_package, authdata, authsize);
            } while (false);
        }

        NodeVideoFrameObserver::~NodeVideoFrameObserver() {
            fuDestroyAllItems();
			delete auth_package;
        }

		int NodeVideoFrameObserver::setFaceUnityOptions(FaceUnityOptions options) {
			int result = -1;
			do {
				mOptions = options;
				mNeedUpdateFUOptions = true;
				result = 0;
			} while (false);
			return result;
		}

        unsigned char *NodeVideoFrameObserver::yuvData(VideoFrame& videoFrame)
        {
            int ysize = videoFrame.yStride * videoFrame.height;
            int usize = videoFrame.uStride * videoFrame.height / 2;
            int vsize = videoFrame.vStride * videoFrame.height / 2;
            unsigned char *temp = (unsigned char *)malloc(ysize + usize + vsize);
            
            memcpy(temp, videoFrame.yBuffer, ysize);
            memcpy(temp + ysize, videoFrame.uBuffer, usize);
            memcpy(temp + ysize + usize, videoFrame.vBuffer, vsize);
            return (unsigned char *)temp;
        }

        int NodeVideoFrameObserver::yuvSize(VideoFrame& videoFrame)
        {
          int ysize = videoFrame.yStride * videoFrame.height;
          int usize = videoFrame.uStride * videoFrame.height / 2;
          int vsize = videoFrame.vStride * videoFrame.height / 2;
          return ysize + usize + vsize;
        }

        void NodeVideoFrameObserver::videoFrameData(VideoFrame& videoFrame, unsigned char *yuvData)
        {
            int ysize = videoFrame.yStride * videoFrame.height;
            int usize = videoFrame.uStride * videoFrame.height / 2;
            int vsize = videoFrame.vStride * videoFrame.height / 2;
            
            memcpy(videoFrame.yBuffer, yuvData,  ysize);
            memcpy(videoFrame.uBuffer, yuvData + ysize, usize);
            memcpy(videoFrame.vBuffer, yuvData + ysize + usize, vsize);
        }
        
        bool NodeVideoFrameObserver::onCaptureVideoFrame(VideoFrame& videoFrame)
        {
			do {
				// 1. initialize if not yet done
				if (!m_namaInited) {
					InitOpenGL();
					//load nama and initialize
					std::vector<char> v3data;
					if (false == Utils::LoadBundle(g_fuDataDir + g_v3Data, v3data)) {
						break;
					}
					//CheckGLContext();
					fuSetup(reinterpret_cast<float*>(&v3data[0]), v3data.size(), NULL, auth_package, auth_package_size);
                    
					std::vector<char> propData;
					if (false == Utils::LoadBundle(g_fuDataDir + g_faceBeautification, propData)) {
						std::cout << "load face beautification data failed." << std::endl;
						break;
					}
					std::cout << "load face beautification data." << std::endl;

					mBeautyHandles = fuCreateItemFromPackage(&propData[0], propData.size());
					m_namaInited = true;
				}
                // 2. beauty params
				// check if options needs to be updated
				if (mNeedUpdateFUOptions) {
					fuItemSetParams(mBeautyHandles, "filter_name", const_cast<char*>(mOptions.filter_name.c_str()));
					fuItemSetParamd(mBeautyHandles, "filter_level", mOptions.filter_level);
					fuItemSetParamd(mBeautyHandles, "color_level", mOptions.color_level);
					fuItemSetParamd(mBeautyHandles, "red_level", mOptions.red_level);
					fuItemSetParamd(mBeautyHandles, "blur_level", mOptions.blur_level);
					fuItemSetParamd(mBeautyHandles, "skin_detect", mOptions.skin_detect);
					fuItemSetParamd(mBeautyHandles, "nonshin_blur_scale", mOptions.nonshin_blur_scale);
					fuItemSetParamd(mBeautyHandles, "heavy_blur", mOptions.heavy_blur);
					fuItemSetParamd(mBeautyHandles, "face_shape", mOptions.face_shape);
					fuItemSetParamd(mBeautyHandles, "face_shape_level", mOptions.face_shape_level);
					fuItemSetParamd(mBeautyHandles, "eye_enlarging", mOptions.eye_enlarging);
					fuItemSetParamd(mBeautyHandles, "cheek_thinning", mOptions.cheek_thinning);
					fuItemSetParamd(mBeautyHandles, "intensity_nose", mOptions.intensity_nose);
					fuItemSetParamd(mBeautyHandles, "intensity_forehead", mOptions.intensity_forehead);
					fuItemSetParamd(mBeautyHandles, "intensity_mouth", mOptions.intensity_mouth);
					fuItemSetParamd(mBeautyHandles, "intensity_chin", mOptions.intensity_chin);
					fuItemSetParamd(mBeautyHandles, "change_frames", mOptions.change_frames);
					fuItemSetParamd(mBeautyHandles, "eye_bright", mOptions.eye_bright);
					fuItemSetParamd(mBeautyHandles, "tooth_whiten", mOptions.tooth_whiten);
					fuItemSetParamd(mBeautyHandles, "is_beauty_on", mOptions.is_beauty_on);
					mNeedUpdateFUOptions = false;
				}

				// 3. make it beautiful
				unsigned char *in_ptr = yuvData(videoFrame);
				int handle[] = { mBeautyHandles };
				int handleSize = sizeof(handle) / sizeof(handle[0]);
				fuRenderItemsEx2(
					FU_FORMAT_I420_BUFFER, reinterpret_cast<int*>(in_ptr),
					FU_FORMAT_I420_BUFFER, reinterpret_cast<int*>(in_ptr),
					videoFrame.width, videoFrame.height,
					mFrameID, handle, handleSize,
					NAMA_RENDER_FEATURE_FULL, NULL);
				videoFrameData(videoFrame, in_ptr);
				delete in_ptr;
			} while (false);
            
            return true;
        }
        
        bool NodeVideoFrameObserver::onRenderVideoFrame(unsigned int uid, VideoFrame& videoFrame) {
            return true;
        }
    }
}
