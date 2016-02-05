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

#if defined(NV_WINDOWS)
    #include <d3d9.h>
    #include <d3d10_1.h>
    #include <d3d11.h>
#pragma warning(disable : 4996)
#endif

#include "NvHWEncoder.h"
#include "../Buffer/MutexBuffer.h"
#include "../writeHandler.h"

#define MAX_ENCODE_QUEUE 32

#define SET_VER(configStruct, type) {configStruct.version = type##_VER;}

template<class T>
class CNvQueue {
    T** m_pBuffer;
    unsigned int m_uSize;
    unsigned int m_uPendingCount;
    unsigned int m_uAvailableIdx;
    unsigned int m_uPendingndex;
public:
    CNvQueue(): m_pBuffer(NULL), m_uSize(0), m_uPendingCount(0), m_uAvailableIdx(0),
                m_uPendingndex(0)
    {
    }

    ~CNvQueue()
    {
        delete[] m_pBuffer;
    }

    bool Initialize(T *pItems, unsigned int uSize)
    {
        m_uSize = uSize;
        m_uPendingCount = 0;
        m_uAvailableIdx = 0;
        m_uPendingndex = 0;
        m_pBuffer = new T *[m_uSize];
        for (unsigned int i = 0; i < m_uSize; i++)
        {
            m_pBuffer[i] = &pItems[i];
        }
        return true;
    }


    T * GetAvailable()
    {
        T *pItem = NULL;
        if (m_uPendingCount == m_uSize)
        {
            return NULL;
        }
        pItem = m_pBuffer[m_uAvailableIdx];
        m_uAvailableIdx = (m_uAvailableIdx+1)%m_uSize;
        m_uPendingCount += 1;
        return pItem;
    }

    T* GetPending()
    {
        if (m_uPendingCount == 0) 
        {
            return NULL;
        }

        T *pItem = m_pBuffer[m_uPendingndex];
        m_uPendingndex = (m_uPendingndex+1)%m_uSize;
        m_uPendingCount -= 1;
        return pItem;
    }
};

typedef struct _EncodeFrameConfig
{
    uint8_t  *yuv[3];
    uint32_t stride[3];
    uint32_t width;
    uint32_t height;
}EncodeFrameConfig;

typedef struct _EncoderQualityConfig
{
	int rcmode;
	int preset;
	int qp;
	int bitrate;
	int totalFrames;
	int width;
	int height;
	int fps;
}EncoderQualityConfig;

typedef enum 
{
    NV_ENC_DX9 = 0,
    NV_ENC_DX11 = 1,
    NV_ENC_CUDA = 2,
    NV_ENC_DX10 = 3,
} NvEncodeDeviceType;

class CNvEncoder
{
public:
    CNvEncoder();
    virtual ~CNvEncoder();

    /**
     * @brief Creates a video in HEVC format for passed configuration
     *
     * This function wraps up the entire encoding process on GPU.
     * Encoding details are passed as parameters.
     * The hardware slot is selected automatically. You may be able
     * to encode two videos simultaneously, but no more.
     * Warning: The encoder will wait (sleep!) on buffer underrun.
     *
     * @param rcmode		The rc (rate control) mode to use. <br>
     * 						0 = Constant quantizer parameter (default)<br>
     * 						1 = VBR mode<br>
     * 						2 = CBR mode<br>
     * 						3 = VBR mode using minimum quantizer parameter<br>
     * 						4 = 2-pass quality<br>
     * 						5 = 2-pass frame size cap<br>
     * 						6 = 2-pass VBR
     * @param preset		NvEncoder preset. <br>
     * 						0 = Default<br>
     * 						1 = High performance<br>
     * 						2 = High quality<br>
     * 						3 = BD<br>
     * 						4 = Low latency default<br>
     * 						5 = Low latency high quality<br>
     * 						6 = Low latency high performance<br>
     * 						7 = Lossless default<br>
     * 						8 = Lossless high performance.<br>
     * 						Please see NvEncoder documentation for what
     * 						hardware is supporting lossless.
     * @param qp			The quantizer parameter
     * @param bitrate		Desired bitrate. Irrelevant for constant qp.
     * @param elapsedTimeP	Output parameter. Returns used encoding time.
     * @param avgtimeP		Output parameter. Returns used time per frame.
     * @param buffer		Pointer to the ringbuffer which is used.
     * @param f				Filehandle to write the raw encoded frames to.
     * @param totalFrames	Total number of frames to encode.
     * @param fps			Framerate of target video.
     * @param width			Width of the input and output. No scaling is done.<br>
     * 						Please note that NvEnc only supports up to 4096x4096.
     * @param height		Height of the input and output. No scaling is done.<br>
     * 						Please note that NvEnc only supports up to 4096x4096.
     * @return 				The encoded file size or -1 in case of an error.
     */
    int                                                  EncodeMain(double *elapsedTimeP, double *avgtimeP,beeCompress::MutexBuffer *buffer, beeCompress::MutexBuffer *bufferPrev,
    																beeCompress::writeHandler *wh, EncoderQualityConfig encCfg);

protected:
    CNvHWEncoder                                        *m_pNvHWEncoder;
    uint32_t                                             m_uEncodeBufferCount;
    uint32_t                                             m_uPicStruct;
    void*                                                m_pDevice;
#if defined(NV_WINDOWS)
    IDirect3D9                                          *m_pD3D;
#endif

    CUcontext                                            m_cuContext;
    EncodeConfig                                         m_stEncoderInput;
    EncodeBuffer                                         m_stEncodeBuffer[MAX_ENCODE_QUEUE];
    CNvQueue<EncodeBuffer>                               m_EncodeBufferQueue;
    EncodeOutputBuffer                                   m_stEOSOutputBfr; 

protected:
    NVENCSTATUS                                          Deinitialize(uint32_t devicetype);
    NVENCSTATUS                                          EncodeFrame(EncodeFrameConfig *pEncodeFrame, bool bFlush=false, uint32_t width=0, uint32_t height=0);
    NVENCSTATUS                                          InitD3D9(uint32_t deviceID = 0);
    NVENCSTATUS                                          InitD3D11(uint32_t deviceID = 0);
    NVENCSTATUS                                          InitD3D10(uint32_t deviceID = 0);
    NVENCSTATUS                                          InitCuda(uint32_t deviceID = 0);
    NVENCSTATUS                                          AllocateIOBuffers(uint32_t uInputWidth, uint32_t uInputHeight, uint32_t isYuv444);
    NVENCSTATUS                                          ReleaseIOBuffers();
    unsigned char*                                       LockInputBuffer(void * hInputSurface, uint32_t *pLockedPitch);
    NVENCSTATUS                                          FlushEncoder();
};

// NVEncodeAPI entry point
typedef NVENCSTATUS (NVENCAPI *MYPROC)(NV_ENCODE_API_FUNCTION_LIST*); 
