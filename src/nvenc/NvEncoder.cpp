////////////////////////////////////////////////////////////////////////////
//
// Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
//
// Please refer to the NVIDIA end user license agreement (EULA) associated
// with this source code for terms and conditions that govern your use of
// this software. Any use, reproduction, disclosure, or distribution of
// this software and related documentation outside the terms of the EULA
// is strictly prohibited.
//
////////////////////////////////////////////////////////////////////////////

#include "nvCPUOPSys.h"
#include "nvEncodeAPI.h"
#include "nvUtils.h"
#include "NvEncoder.h"
#include "nvFileIO.h"
#include <new>
#include <vector>

#if HAVE_UNISTD_H
    #include <unistd.h> //sleep
#else
    #include <stdint.h>
#endif
#include "../settings/Settings.h"
#include "../Buffer/ImageBuffer.h"
#include "../VideoWriter.h"

#if HALIDE
    #include "halideYuv420Conv.h"
    #include "Halide.h"
#endif
#include <memory>
#include <opencv2/opencv.hpp>

#define BITSTREAM_BUFFER_SIZE 2 * 1024 * 1024

void convertYUVpitchtoNV12(unsigned char* yuv_luma,
                           unsigned char* yuv_cb,
                           unsigned char* yuv_cr,
                           unsigned char* nv12_luma,
                           unsigned char* nv12_chroma,
                           int            width,
                           int            height,
                           int            srcStride,
                           int            dstStride)
{
    int y;
    int x;
    if (srcStride == 0)
        srcStride = width;
    if (dstStride == 0)
        dstStride = width;

    for (y = 0; y < height; y++)
    {
        memcpy(nv12_luma + (dstStride * y), yuv_luma + (srcStride * y), width);
    }

    for (y = 0; y < height / 2; y++)
    {
        for (x = 0; x < width; x = x + 2)
        {
            nv12_chroma[(y * dstStride) + x]       = yuv_cb[((srcStride / 2) * y) + (x >> 1)];
            nv12_chroma[(y * dstStride) + (x + 1)] = yuv_cr[((srcStride / 2) * y) + (x >> 1)];
        }
    }
}

void convertYUVpitchtoYUV444(unsigned char* yuv_luma,
                             unsigned char* yuv_cb,
                             unsigned char* yuv_cr,
                             unsigned char* surf_luma,
                             unsigned char* surf_cb,
                             unsigned char* surf_cr,
                             int            width,
                             int            height,
                             int            srcStride,
                             int            dstStride)
{
    int h;

    for (h = 0; h < height; h++)
    {
        memcpy(surf_luma + dstStride * h, yuv_luma + srcStride * h, width);
        memcpy(surf_cb + dstStride * h, yuv_cb + srcStride * h, width);
        memcpy(surf_cr + dstStride * h, yuv_cr + srcStride * h, width);
    }
}

CNvEncoder::CNvEncoder()
{
    m_pNvHWEncoder = new CNvHWEncoder;
    m_pDevice      = NULL;
#if defined(NV_WINDOWS)
    m_pD3D = NULL;
#endif
    m_cuContext = NULL;

    m_uEncodeBufferCount = 0;
    memset(&m_stEncoderInput, 0, sizeof(m_stEncoderInput));
    memset(&m_stEOSOutputBfr, 0, sizeof(m_stEOSOutputBfr));

    memset(&m_stEncodeBuffer, 0, sizeof(m_stEncodeBuffer));
}

CNvEncoder::~CNvEncoder()
{
    if (m_pNvHWEncoder)
    {
        delete m_pNvHWEncoder;
        m_pNvHWEncoder = NULL;
    }
}

NVENCSTATUS CNvEncoder::InitCuda(uint32_t deviceID)
{
    CUresult  cuResult;
    CUdevice  device;
    CUcontext cuContextCurr;
    int       deviceCount = 0;
    int       SMminor = 0, SMmajor = 0;

#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    typedef HMODULE CUDADRIVER;
#else
    typedef void* CUDADRIVER;
#endif
    CUDADRIVER hHandleDriver = 0;
    cuResult                 = cuInit(0, __CUDA_API_VERSION, hHandleDriver);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuInit error:0x%x\n", cuResult);
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuDeviceGetCount(&deviceCount);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuDeviceGetCount error:0x%x\n", cuResult);
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    // If dev is negative value, we clamp to 0
    if ((int) deviceID < 0)
        deviceID = 0;

    if (deviceID > (unsigned int) deviceCount - 1)
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    cuResult = cuDeviceGet(&device, deviceID);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuDeviceGet error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuDeviceComputeCapability(&SMmajor, &SMminor, deviceID);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuDeviceComputeCapability error:0x%x\n", cuResult);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    if (((SMmajor << 4) + SMminor) < 0x30)
    {
        PRINTERR("GPU %d does not have NVENC capabilities exiting\n", deviceID);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuCtxCreate((CUcontext*) (&m_pDevice), 0, device);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuCtxCreate error:0x%x\n", cuResult);
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }

    cuResult = cuCtxPopCurrent(&cuContextCurr);
    if (cuResult != CUDA_SUCCESS)
    {
        PRINTERR("cuCtxPopCurrent error:0x%x\n", cuResult);
        assert(0);
        return NV_ENC_ERR_NO_ENCODE_DEVICE;
    }
    return NV_ENC_SUCCESS;
}

#if defined(NV_WINDOWS)
NVENCSTATUS CNvEncoder::InitD3D9(uint32_t deviceID)
{
    D3DPRESENT_PARAMETERS  d3dpp;
    D3DADAPTER_IDENTIFIER9 adapterId;
    unsigned int           iAdapter = NULL; // Our adapter
    HRESULT                hr       = S_OK;

    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (m_pD3D == NULL)
    {
        assert(m_pD3D);
        return NV_ENC_ERR_OUT_OF_MEMORY;
        ;
    }

    if (deviceID >= m_pD3D->GetAdapterCount())
    {
        PRINTERR(
            "Invalid Device Id = %d\n. Please use DX10/DX11 to detect headless video "
            "devices.\n",
            deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    hr = m_pD3D->GetAdapterIdentifier(deviceID, 0, &adapterId);
    if (hr != S_OK)
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed             = TRUE;
    d3dpp.BackBufferFormat     = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth      = 640;
    d3dpp.BackBufferHeight     = 480;
    d3dpp.BackBufferCount      = 1;
    d3dpp.SwapEffect           = D3DSWAPEFFECT_COPY;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    d3dpp.Flags                = D3DPRESENTFLAG_VIDEO; // D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    DWORD dwBehaviorFlags      = D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED |
                            D3DCREATE_HARDWARE_VERTEXPROCESSING;

    hr = m_pD3D->CreateDevice(deviceID,
                              D3DDEVTYPE_HAL,
                              GetDesktopWindow(),
                              dwBehaviorFlags,
                              &d3dpp,
                              (IDirect3DDevice9**) (&m_pDevice));

    if (FAILED(hr))
        return NV_ENC_ERR_OUT_OF_MEMORY;

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::InitD3D10(uint32_t deviceID)
{
    HRESULT       hr;
    IDXGIFactory* pFactory = NULL;
    IDXGIAdapter* pAdapter;

    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**) &pFactory) != S_OK)
    {
        return NV_ENC_ERR_GENERIC;
    }

    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        hr = D3D10CreateDevice(pAdapter,
                               D3D10_DRIVER_TYPE_HARDWARE,
                               NULL,
                               0,
                               D3D10_SDK_VERSION,
                               (ID3D10Device**) (&m_pDevice));
        if (FAILED(hr))
        {
            PRINTERR("Problem while creating %d D3d10 device \n", deviceID);
            return NV_ENC_ERR_OUT_OF_MEMORY;
        }
    }
    else
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::InitD3D11(uint32_t deviceID)
{
    HRESULT       hr;
    IDXGIFactory* pFactory = NULL;
    IDXGIAdapter* pAdapter;

    if (CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**) &pFactory) != S_OK)
    {
        return NV_ENC_ERR_GENERIC;
    }

    if (pFactory->EnumAdapters(deviceID, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        hr = D3D11CreateDevice(pAdapter,
                               D3D_DRIVER_TYPE_UNKNOWN,
                               NULL,
                               0,
                               NULL,
                               0,
                               D3D11_SDK_VERSION,
                               (ID3D11Device**) (&m_pDevice),
                               NULL,
                               NULL);
        if (FAILED(hr))
        {
            PRINTERR("Problem while creating %d D3d11 device \n", deviceID);
            return NV_ENC_ERR_OUT_OF_MEMORY;
        }
    }
    else
    {
        PRINTERR("Invalid Device Id = %d\n", deviceID);
        return NV_ENC_ERR_INVALID_ENCODERDEVICE;
    }

    return NV_ENC_SUCCESS;
}
#endif

NVENCSTATUS CNvEncoder::AllocateIOBuffers(uint32_t uInputWidth,
                                          uint32_t uInputHeight,
                                          uint32_t isYuv444)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    m_EncodeBufferQueue.Initialize(m_stEncodeBuffer, m_uEncodeBufferCount);
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        nvStatus = m_pNvHWEncoder->NvEncCreateInputBuffer(
            uInputWidth,
            uInputHeight,
            &m_stEncodeBuffer[i].stInputBfr.hInputSurface,
            isYuv444);
        if (nvStatus != NV_ENC_SUCCESS)
            return nvStatus;

        m_stEncodeBuffer[i].stInputBfr.bufferFmt = isYuv444 ? NV_ENC_BUFFER_FORMAT_YUV444_PL
                                                            : NV_ENC_BUFFER_FORMAT_NV12_PL;
        m_stEncodeBuffer[i].stInputBfr.dwWidth  = uInputWidth;
        m_stEncodeBuffer[i].stInputBfr.dwHeight = uInputHeight;

        // Allocate output surface
        if (m_stEncoderInput.enableMEOnly)
        {
            uint32_t encodeWidthInMbs  = (uInputWidth + 15) >> 4;
            uint32_t encodeHeightInMbs = (uInputHeight + 15) >> 4;
            uint32_t dwSize            = encodeWidthInMbs * encodeHeightInMbs * 64;
            nvStatus                   = m_pNvHWEncoder->NvEncCreateMVBuffer(
                dwSize,
                &m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
            if (nvStatus != NV_ENC_SUCCESS)
            {
                PRINTERR("nvEncCreateMVBuffer error:0x%x\n", nvStatus);
                return nvStatus;
            }
            m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = dwSize;
        }
        else
        {
            nvStatus = m_pNvHWEncoder->NvEncCreateBitstreamBuffer(
                BITSTREAM_BUFFER_SIZE,
                &m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
            if (nvStatus != NV_ENC_SUCCESS)
                return nvStatus;
            m_stEncodeBuffer[i].stOutputBfr.dwBitstreamBufferSize = BITSTREAM_BUFFER_SIZE;
        }

#if defined(NV_WINDOWS)
        nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(
            &m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
        if (nvStatus != NV_ENC_SUCCESS)
            return nvStatus;
        if (m_stEncoderInput.enableMEOnly)
        {
            m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = false;
        }
        else
            m_stEncodeBuffer[i].stOutputBfr.bWaitOnEvent = true;
#else
        m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
#endif
    }

    m_stEOSOutputBfr.bEOSFlag = TRUE;

#if defined(NV_WINDOWS)
    nvStatus = m_pNvHWEncoder->NvEncRegisterAsyncEvent(&m_stEOSOutputBfr.hOutputEvent);
    if (nvStatus != NV_ENC_SUCCESS)
        return nvStatus;
#else
    m_stEOSOutputBfr.hOutputEvent = NULL;
#endif

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::ReleaseIOBuffers()
{
    for (uint32_t i = 0; i < m_uEncodeBufferCount; i++)
    {
        m_pNvHWEncoder->NvEncDestroyInputBuffer(m_stEncodeBuffer[i].stInputBfr.hInputSurface);
        m_stEncodeBuffer[i].stInputBfr.hInputSurface = NULL;

        if (m_stEncoderInput.enableMEOnly)
        {
            m_pNvHWEncoder->NvEncDestroyMVBuffer(m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
            m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
        }
        else
        {
            m_pNvHWEncoder->NvEncDestroyBitstreamBuffer(
                m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer);
            m_stEncodeBuffer[i].stOutputBfr.hBitstreamBuffer = NULL;
        }

#if defined(NV_WINDOWS)
        m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
        nvCloseFile(m_stEncodeBuffer[i].stOutputBfr.hOutputEvent);
        m_stEncodeBuffer[i].stOutputBfr.hOutputEvent = NULL;
#endif
    }

    if (m_stEOSOutputBfr.hOutputEvent)
    {
#if defined(NV_WINDOWS)
        m_pNvHWEncoder->NvEncUnregisterAsyncEvent(m_stEOSOutputBfr.hOutputEvent);
        nvCloseFile(m_stEOSOutputBfr.hOutputEvent);
        m_stEOSOutputBfr.hOutputEvent = NULL;
#endif
    }

    return NV_ENC_SUCCESS;
}

NVENCSTATUS CNvEncoder::FlushEncoder()
{
    NVENCSTATUS nvStatus = m_pNvHWEncoder->NvEncFlushEncoderQueue(m_stEOSOutputBfr.hOutputEvent);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        assert(0);
        return nvStatus;
    }

    EncodeBuffer* pEncodeBufer = m_EncodeBufferQueue.GetPending();
    while (pEncodeBufer)
    {
        m_pNvHWEncoder->ProcessOutput(pEncodeBufer);
        pEncodeBufer = m_EncodeBufferQueue.GetPending();
    }

#if defined(NV_WINDOWS)
    if (WaitForSingleObject(m_stEOSOutputBfr.hOutputEvent, 500) != WAIT_OBJECT_0)
    {
        assert(0);
        nvStatus = NV_ENC_ERR_GENERIC;
    }
#endif

    return nvStatus;
}

NVENCSTATUS CNvEncoder::Deinitialize(uint32_t devicetype)
{
    NVENCSTATUS nvStatus = NV_ENC_SUCCESS;

    ReleaseIOBuffers();

    nvStatus = m_pNvHWEncoder->NvEncDestroyEncoder();

    if (m_pDevice)
    {
        switch (devicetype)
        {
#if defined(NV_WINDOWS)
        case NV_ENC_DX9:
            ((IDirect3DDevice9*) (m_pDevice))->Release();
            break;

        case NV_ENC_DX10:
            ((ID3D10Device*) (m_pDevice))->Release();
            break;

        case NV_ENC_DX11:
            ((ID3D11Device*) (m_pDevice))->Release();
            break;
#endif

        case NV_ENC_CUDA:
            CUresult cuResult = CUDA_SUCCESS;
            cuResult          = cuCtxDestroy((CUcontext) m_pDevice);
            if (cuResult != CUDA_SUCCESS)
                PRINTERR("cuCtxDestroy error:0x%x\n", cuResult);
        }

        m_pDevice = NULL;
    }

#if defined(NV_WINDOWS)
    if (m_pD3D)
    {
        m_pD3D->Release();
        m_pD3D = NULL;
    }
#endif

    return nvStatus;
}

NVENCSTATUS loadframe(uint8_t*  yuvInput[3],
                      HANDLE    hInputYUVFile,
                      uint32_t  frmIdx,
                      uint32_t  width,
                      uint32_t  height,
                      uint32_t& numBytesRead,
                      uint32_t  isYuv444)
{
    uint64_t fileOffset;
    uint32_t result;
    // Set size depending on whether it is YUV 444 or YUV 420
    uint32_t dwInFrameSize = isYuv444 ? width * height * 3 : width * height + (width * height) / 2;
    fileOffset             = (uint64_t) dwInFrameSize * frmIdx;
    result                 = nvSetFilePointer64(hInputYUVFile, fileOffset, NULL, FILE_BEGIN);
    if (result == INVALID_SET_FILE_POINTER)
    {
        return NV_ENC_ERR_INVALID_PARAM;
    }
    if (isYuv444)
    {
        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[1], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[2], width * height, &numBytesRead, NULL);
    }
    else
    {
        nvReadFile(hInputYUVFile, yuvInput[0], width * height, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[1], width * height / 4, &numBytesRead, NULL);
        nvReadFile(hInputYUVFile, yuvInput[2], width * height / 4, &numBytesRead, NULL);
    }
    return NV_ENC_SUCCESS;
}

void PrintHelp()
{
    printf(
        "Usage : NvEncoder \n"
        "-i <string>                  Specify input yuv420 file\n"
        "-o <string>                  Specify output bitstream file\n"
        "-size <int int>              Specify input resolution <width height>\n"
        "\n### Optional parameters ###\n"
        "-codec <integer>             Specify the codec \n"
        "                                 0: H264\n"
        "                                 1: HEVC\n"
        "-preset <string>             Specify the preset for encoder settings\n"
        "                                 hq : nvenc HQ \n"
        "                                 hp : nvenc HP \n"
        "                                 lowLatencyHP : nvenc low latency HP \n"
        "                                 lowLatencyHQ : nvenc low latency HQ \n"
        "                                 lossless : nvenc Lossless HP \n"
        "-startf <integer>            Specify start index for encoding. Default is 0\n"
        "-endf <integer>              Specify end index for encoding. Default is end of file\n"
        "-fps <integer>               Specify encoding frame rate\n"
        "-goplength <integer>         Specify gop length\n"
        "-numB <integer>              Specify number of B frames\n"
        "-bitrate <integer>           Specify the encoding average bitrate\n"
        "-vbvMaxBitrate <integer>     Specify the vbv max bitrate\n"
        "-vbvSize <integer>           Specify the encoding vbv/hrd buffer size\n"
        "-rcmode <integer>            Specify the rate control mode\n"
        "                                 0:  Constant QP\n"
        "                                 1:  Single pass VBR\n"
        "                                 2:  Single pass CBR\n"
        "                                 4:  Single pass VBR minQP\n"
        "                                 8:  Two pass frame quality\n"
        "                                 16: Two pass frame size cap\n"
        "                                 32: Two pass VBR\n"
        "-qp <integer>                Specify qp for Constant QP mode\n"
        "-i_qfactor <float>           Specify qscale difference between I-frames and P-frames\n"
        "-b_qfactor <float>           Specify qscale difference between P-frames and B-frames\n"
        "-i_qoffset <float>           Specify qscale offset between I-frames and P-frames\n"
        "-b_qoffset <float>           Specify qscale offset between P-frames and B-frames\n"
        "-picStruct <integer>         Specify the picture structure\n"
        "                                 1:  Progressive frame\n"
        "                                 2:  Field encoding top field first\n"
        "                                 3:  Field encoding bottom field first\n"
        "-devicetype <integer>        Specify devicetype used for encoding\n"
        "                                 0:  DX9\n"
        "                                 1:  DX11\n"
        "                                 2:  Cuda\n"
        "                                 3:  DX10\n"
        "-yuv444 <integer>             Specify the input YUV format\n"
        "                                 0: YUV 420\n"
        "                                 1: YUV 444\n"
        "-deviceID <integer>           Specify the GPU device on which encoding will take place\n"
        "-meonly <integer>             Specify Motion estimation only(permissive value 1 and 2) "
        "to generates motion vectors and Mode information\n"
        "                                 1: Motion estimation between startf and endf\n"
        "                                 2: Motion estimation for all consecutive frames from "
        "startf to endf\n"
        "-preloadedFrameCount <integer> Specify number of frame to load in memory(default "
        "value=240) with min value 2(1 frame for ref, 1 frame for input)\n"
        "-help                         Prints Help Information\n\n");
}

_NV_ENC_PARAMS_RC_MODE getRcmode(int rcmode)
{
    switch (rcmode)
    {
    case 0:
        return NV_ENC_PARAMS_RC_CONSTQP;
    case 1:
        return NV_ENC_PARAMS_RC_VBR;
    case 2:
        return NV_ENC_PARAMS_RC_CBR;
    case 3:
        return NV_ENC_PARAMS_RC_VBR_MINQP;
    case 4:
        return NV_ENC_PARAMS_RC_2_PASS_QUALITY;
    case 5:
        return NV_ENC_PARAMS_RC_2_PASS_FRAMESIZE_CAP;
    case 6:
        return NV_ENC_PARAMS_RC_2_PASS_VBR;
    }
    return NV_ENC_PARAMS_RC_CONSTQP;
}
GUID getGUID(int id)
{
    switch (id)
    {
    case 0:
        return NV_ENC_PRESET_DEFAULT_GUID;
    case 1:
        return NV_ENC_PRESET_HP_GUID;
    case 2:
        return NV_ENC_PRESET_HQ_GUID;
    case 3:
        return NV_ENC_PRESET_BD_GUID;
    case 4:
        return NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
    case 5:
        return NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
    case 6:
        return NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
    case 7:
        return NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID;
    case 8:
        return NV_ENC_PRESET_LOSSLESS_HP_GUID;
    }
    return NV_ENC_PRESET_DEFAULT_GUID;
}

std::shared_ptr<ImageBuffer> scaleImage(ImageBuffer*         img,
                                        EncodeConfig         encodeConfig,
                                        EncoderQualityConfig encPrevCfg)
{
    // ImageBuffer *newImage = new
    // ImageBuffer(encodeConfig.width/2,encodeConfig.height/2,img->camid,img->timestamp);
    std::shared_ptr<ImageBuffer> newImage = std::shared_ptr<ImageBuffer>(
        new ImageBuffer(encPrevCfg.width, encPrevCfg.height, img->camid, img->timestamp));

    /*std::shared_ptr<ImageBuffer> newImage =
     std::shared_ptr<ImageBuffer>(new
     ImageBuffer(encodeConfig.width/2,encodeConfig.height/2,img->camid,img->timestamp));
     //TODO: Put this in a function and do smart scaling
     */
    if (encPrevCfg.width == encPrevCfg.width / 2 && encPrevCfg.height == encPrevCfg.height / 2)
    {
        unsigned int x = 0, y = 0;
        for (y = 0; y < encodeConfig.height; y += 2)
        {
            for (x = 0; x < encodeConfig.width; x += 2)
            {
                newImage->data[y / 2 * encodeConfig.width / 2 + x / 2] =
                    img->data[y * encodeConfig.width + x];
            }
        }
    }
    else if (encPrevCfg.width == encPrevCfg.width / 4 &&
             encPrevCfg.height == encPrevCfg.height / 4)
    {
        unsigned int x = 0, y = 0;
        for (y = 0; y < encodeConfig.height; y += 4)
        {
            for (x = 0; x < encodeConfig.width; x += 4)
            {
                newImage->data[y / 4 * encodeConfig.width / 4 + x / 4] =
                    img->data[y * encodeConfig.width + x];
            }
        }
    }
    else
    {

        cv::Mat imageWithData =
            cv::Mat(encodeConfig.width * encodeConfig.height, 1, 0 /*CV_8U*/, &img->data[0])
                .clone();
        cv::Mat  reshapedImage = imageWithData.reshape(1, encodeConfig.height);
        cv::Size size(encPrevCfg.width, encPrevCfg.height); // the dst image size,e.g.100x100
        cv::Mat  dst;                                       // dst image
        cv::resize(reshapedImage, dst, size);               // resize image

        int x = 0, y = 0;
        for (y = 0; y < encPrevCfg.height; y += 1)
        {
            for (x = 0; x < encPrevCfg.width; x += 1)
            {
                newImage->data[y * encPrevCfg.width + x] = dst.at<uint8_t>(y, x);
            }
        }
    }

    return newImage;
}

int rawTo420NoHalide(uint8_t* outputImage, uint8_t* inputImage, int rows, int cols)
{
    /*  See:
     * http://stackoverflow.com/questions/8349352/how-to-encode-grayscale-video-streams-with-ffmpeg
     */

    int            bytesRead = 0;
    unsigned char* prtM      = inputImage;
    unsigned int   stride    = 1;

    // Being lazy pays: conversion in 58.22ms (don't set all channels - Cb and Cr are a waste of
    // time)
    unsigned int x = 0, y = 0;
    for (y = 0; y < rows; y++)
    {
        for (x = 0; x < cols; x++)
        {
            outputImage[y * cols + x] = (uint8_t)(0.895 * (*prtM) + 16);
            prtM++;
            bytesRead++;
        }
    }

    return bytesRead;
}

int rawTo420(uint8_t* outputImage, uint8_t* inputImage, int rows, int cols)
{
    /*  See:
     * http://stackoverflow.com/questions/8349352/how-to-encode-grayscale-video-streams-with-ffmpeg
     */

    int bytesRead;
#if HALIDE
    unsigned long long time, time2;
    unsigned char*     prtM   = inputImage;
    unsigned int       stride = 1;

    // Precompiled code gets run in 17.88ms. BAM!
    buffer_t input_buf = {0}, output_buf = {0};

    // The host pointers point to the start of the image data:
    input_buf.host  = prtM;
    output_buf.host = outputImage;
    // See the halide tutorial how to set these.
    input_buf.stride[0] = output_buf.stride[0] = 1;
    input_buf.stride[1] = output_buf.stride[1] = stride;
    input_buf.extent[0] = output_buf.extent[0] = cols;
    input_buf.extent[1] = output_buf.extent[1] = rows;
    input_buf.elem_size = output_buf.elem_size = 1;

    // do it
    int error = halideYuv420Conv(&input_buf, &output_buf);
    bytesRead = rows * cols * 3; // fool the system. We never read/written or set Cb and Cr
#else
    return rawTo420NoHalide(outputImage, inputImage, rows, cols);
#endif

    return bytesRead;
}

int CNvEncoder::EncodeMain(double*              elapsedTimeP,
                           double*              avgtimeP,
                           MutexBuffer*         buffer,
                           MutexBuffer*         bufferPrev,
                           writeHandler*        wh,
                           EncoderQualityConfig encCfg,
                           EncoderQualityConfig encPrevCfg)
{
    HANDLE             hInput;
    uint8_t*           yuv[3];
    unsigned long long lStart, lEnd, lFreq;
    int                numFramesEncoded = 0;
    NVENCSTATUS        nvStatus         = NV_ENC_SUCCESS;
    bool               bError           = false;
    EncodeConfig       encodeConfig;
    unsigned int       fstart = 0;
    unsigned int       fsize  = 0;

    memset(&encodeConfig, 0, sizeof(EncodeConfig));

    // This is important. If these are higher than
    // 4096 it may crash the entire system.
    if (encCfg.width > 4096 || encCfg.height > 4096)
    {
        return -1;
    }

    encodeConfig.endFrameIdx = INT_MAX;
    encodeConfig.bitrate     = encCfg.bitrate;
    encodeConfig.rcMode      = getRcmode(encCfg.rcmode);
    encodeConfig.gopLength   = NVENC_INFINITE_GOPLENGTH;
    encodeConfig.deviceType  = NV_ENC_CUDA;
#if defined(NV_WINDOWS)
    encodeConfig.deviceType = NV_ENC_HEVC;
#endif
    encodeConfig.codec         = NV_ENC_HEVC;
    encodeConfig.fps           = encCfg.fps;
    encodeConfig.qp            = encCfg.qp;
    encodeConfig.presetGUID    = getGUID(encCfg.preset);
    encodeConfig.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
    encodeConfig.isYuv444      = 0;

    encodeConfig.width  = encCfg.width;
    encodeConfig.height = encCfg.height;

    encodeConfig.fOutput = wh->_video;

    hInput = 0; /*nvOpenFile(encodeConfig.inputFileName);*/

    switch (encodeConfig.deviceType)
    {
#if defined(NV_WINDOWS)
    case NV_ENC_DX9:
        InitD3D9(encodeConfig.deviceID);
        break;

    case NV_ENC_DX10:
        InitD3D10(encodeConfig.deviceID);
        break;

    case NV_ENC_DX11:
        InitD3D11(encodeConfig.deviceID);
        break;
#endif
    case NV_ENC_CUDA:
        InitCuda(encodeConfig.deviceID);
        break;
    }

    if (encodeConfig.deviceType != NV_ENC_CUDA)
        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_DIRECTX);
    else
        nvStatus = m_pNvHWEncoder->Initialize(m_pDevice, NV_ENC_DEVICE_TYPE_CUDA);

    if (nvStatus != NV_ENC_SUCCESS)
        return -1;

    nvStatus = m_pNvHWEncoder->CreateEncoder(&encodeConfig);
    if (nvStatus != NV_ENC_SUCCESS)
        return 1;

    m_uEncodeBufferCount = encodeConfig.numB + 4; // min buffers is numb + 1 + 3 pipelining

    m_uPicStruct = encodeConfig.pictureStruct;

    nvStatus = AllocateIOBuffers(encodeConfig.width, encodeConfig.height, encodeConfig.isYuv444);
    if (nvStatus != NV_ENC_SUCCESS)
        return 1;

    if (encodeConfig.isYuv444)
    {
        yuv[0] = new uint8_t[encodeConfig.width * encodeConfig.height];
        yuv[1] = new uint8_t[encodeConfig.width * encodeConfig.height];
        yuv[2] = new uint8_t[encodeConfig.width * encodeConfig.height];
    }
    else
    {
        yuv[0] = new uint8_t[encodeConfig.width * encodeConfig.height];
        yuv[1] = new uint8_t[encodeConfig.width * encodeConfig.height / 4];
        yuv[2] = new uint8_t[encodeConfig.width * encodeConfig.height / 4];
        // memset(yuv[0],128, encodeConfig.width*encodeConfig.height);
        memset(yuv[1], 128, encodeConfig.width * encodeConfig.height / 4);
        memset(yuv[2], 128, encodeConfig.width * encodeConfig.height / 4);
    }
    // To never ever write in used memory. Maybe unnecessary
    std::vector<uint8_t> temporaryBuffer;
    temporaryBuffer.resize(static_cast<size_t>(encodeConfig.width * encodeConfig.height));

    NvQueryPerformanceCounter(&lStart);

    VideoWriter videoWriter(std::string(wh->_videofile.c_str(), wh->_videofile.size() - 4) +
                                ".mp4",
                            {encCfg.width,
                             encCfg.height,
                             {encCfg.fps, 1},
                             {"hevc_nvenc", {{"preset", "default"}, {"rc", "vbr_hq"}, {"cq", "25"}}}});

    for (int frm = 0; frm < encCfg.totalFrames; frm++)
    {
        uint32_t numBytesRead = 0;

        // Wait until there is a new image available (done by pop)
        std::shared_ptr<ImageBuffer> imgptr = buffer->pop();
        ImageBuffer*                 img    = imgptr.get();
        numBytesRead = static_cast<decltype(numBytesRead)>(img->width * img->height);

        // Debug output TODO: remove?
        if (frm % 100 == 0)
            printf("Loaded frame %d \n", frm);

        // This should not happen
        if (numBytesRead == 0)
            break;

        videoWriter.write(*img);

        EncodeFrameConfig stEncodeFrame;
        memset(&stEncodeFrame, 0, sizeof(stEncodeFrame));

        // Fill data structure for the encoder
        rawTo420NoHalide(temporaryBuffer.data(),
                         &img->data[0],
                         encodeConfig.height,
                         encodeConfig.width);
        stEncodeFrame.yuv[0] = temporaryBuffer.data();
        // memcpy(stEncodeFrame.yuv[0], &img->data[0], encodeConfig.height*encodeConfig.width);
        // stEncodeFrame.yuv[0] = yuv[0];
        stEncodeFrame.yuv[1] = yuv[1];
        stEncodeFrame.yuv[2] = yuv[2];

        stEncodeFrame.stride[0] = encodeConfig.width;
        stEncodeFrame.stride[1] = (encodeConfig.isYuv444) ? encodeConfig.width
                                                          : encodeConfig.width / 2;
        stEncodeFrame.stride[2] = (encodeConfig.isYuv444) ? encodeConfig.width
                                                          : encodeConfig.width / 2;
        stEncodeFrame.width  = encodeConfig.width;
        stEncodeFrame.height = encodeConfig.height;

        // Invoke the encoder
        EncodeFrame(&stEncodeFrame, false, encodeConfig.width, encodeConfig.height);
        numFramesEncoded++;

        // Log the progress to the writeHandler
        wh->log(img->timestamp);

        if (bufferPrev != NULL)
        {

            std::shared_ptr<ImageBuffer> newImage = scaleImage(img, encodeConfig, encPrevCfg);
            bufferPrev->push(newImage);
        }
    }
    videoWriter.close();

    nvStatus = EncodeFrame(NULL, true, encodeConfig.width, encodeConfig.height);
    if (nvStatus != NV_ENC_SUCCESS)
    {
        std::cerr << "ENCODER ERROR: " << nvStatus << std::endl;
        bError = true;
        goto exit;
    }

    if (numFramesEncoded > 0)
    {
        NvQueryPerformanceCounter(&lEnd);
        NvQueryPerformanceFrequency(&lFreq);
        *elapsedTimeP = (double) (lEnd - lStart);
        printf("Encoded %d frames in %6.2fms\n",
               numFramesEncoded,
               ((*elapsedTimeP) * 1000.0) / lFreq);
        printf("Average Encode Time : %6.2fms\n",
               (((*elapsedTimeP) * 1000.0) / numFramesEncoded) / lFreq);
        *avgtimeP = (((*elapsedTimeP) * 1000.0) / numFramesEncoded) / lFreq;
    }

exit:
    fsize = ftell(encodeConfig.fOutput) - fstart;

    if (hInput)
    {
        nvCloseFile(hInput);
    }

    Deinitialize(encodeConfig.deviceType);

    for (int i = 0; i < 3; i++)
    {
        if (yuv[i])
        {
            delete[] yuv[i];
        }
    }

    return bError ? -1 : fsize;
}

NVENCSTATUS CNvEncoder::EncodeFrame(EncodeFrameConfig* pEncodeFrame,
                                    bool               bFlush,
                                    uint32_t           width,
                                    uint32_t           height)
{
    NVENCSTATUS   nvStatus      = NV_ENC_SUCCESS;
    uint32_t      lockedPitch   = 0;
    EncodeBuffer* pEncodeBuffer = NULL;

    if (bFlush)
    {
        FlushEncoder();
        return NV_ENC_SUCCESS;
    }

    if (!pEncodeFrame)
    {
        return NV_ENC_ERR_INVALID_PARAM;
    }

    pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
    if (!pEncodeBuffer)
    {
        m_pNvHWEncoder->ProcessOutput(m_EncodeBufferQueue.GetPending());
        pEncodeBuffer = m_EncodeBufferQueue.GetAvailable();
    }

    unsigned char* pInputSurface;

    nvStatus = m_pNvHWEncoder->NvEncLockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface,
                                                    (void**) &pInputSurface,
                                                    &lockedPitch);
    if (nvStatus != NV_ENC_SUCCESS)
        return nvStatus;

    if (pEncodeBuffer->stInputBfr.bufferFmt == NV_ENC_BUFFER_FORMAT_NV12_PL)
    {
        unsigned char* pInputSurfaceCh = pInputSurface +
                                         (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
        convertYUVpitchtoNV12(pEncodeFrame->yuv[0],
                              pEncodeFrame->yuv[1],
                              pEncodeFrame->yuv[2],
                              pInputSurface,
                              pInputSurfaceCh,
                              width,
                              height,
                              width,
                              lockedPitch);
    }
    else
    {
        unsigned char* pInputSurfaceCb = pInputSurface +
                                         (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
        unsigned char* pInputSurfaceCr = pInputSurfaceCb +
                                         (pEncodeBuffer->stInputBfr.dwHeight * lockedPitch);
        convertYUVpitchtoYUV444(pEncodeFrame->yuv[0],
                                pEncodeFrame->yuv[1],
                                pEncodeFrame->yuv[2],
                                pInputSurface,
                                pInputSurfaceCb,
                                pInputSurfaceCr,
                                width,
                                height,
                                width,
                                lockedPitch);
    }
    nvStatus = m_pNvHWEncoder->NvEncUnlockInputBuffer(pEncodeBuffer->stInputBfr.hInputSurface);
    if (nvStatus != NV_ENC_SUCCESS)
        return nvStatus;

    nvStatus = m_pNvHWEncoder->NvEncEncodeFrame(pEncodeBuffer,
                                                NULL,
                                                width,
                                                height,
                                                (NV_ENC_PIC_STRUCT) m_uPicStruct);
    return nvStatus;
}
