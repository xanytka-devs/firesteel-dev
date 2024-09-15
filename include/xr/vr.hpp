#include "../common.hpp"
#include "OVR_CAPI_GL.h"
#include "Extras/OVR_Math.h"
#include <glm/gtc/type_ptr.hpp>

#pragma region VR_STATE
OculusTextureBuffer* eyeRenderTexture[2] = { nullptr, nullptr };
ovrSession session;
ovrGraphicsLuid luid;
ovrHmdDesc hmdDesc;
double sensorSampleTime;
ovrEyeRenderDesc eyeRenderDesc[2];
ovrPosef EyeRenderPose[2];
ovrPosef HmdToEyePose[2];
#pragma endregion VR_STATE

#pragma region VR_HELPER_FN
// OculusTextureBuffer is from the OculusSDK\Samples\OculusRoomTiny\OculusRoomTiny (GL)\main.cpp
struct OculusTextureBuffer
{
    ovrSession          Session;
    ovrTextureSwapChain ColorTextureChain;
    ovrTextureSwapChain DepthTextureChain;
    GLuint              fboId;
    OVR::Sizei               texSize;

    OculusTextureBuffer(ovrSession session, OVR::Sizei size, int sampleCount) :
        Session(session),
        ColorTextureChain(nullptr),
        DepthTextureChain(nullptr),
        fboId(0),
        texSize(0, 0)
    {
        assert(sampleCount <= 1); // The code doesn't currently handle MSAA textures.

        texSize = size;

        // This texture isn't necessarily going to be a rendertarget, but it usually is.
        assert(session); // No HMD? A little odd.

        ovrTextureSwapChainDesc desc = {};
        desc.Type = ovrTexture_2D;
        desc.ArraySize = 1;
        desc.Width = size.w;
        desc.Height = size.h;
        desc.MipLevels = 1;
        desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc.SampleCount = sampleCount;
        desc.StaticImage = ovrFalse;

        {
            ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &ColorTextureChain);

            int length = 0;
            ovr_GetTextureSwapChainLength(session, ColorTextureChain, &length);

            if (OVR_SUCCESS(result))
            {
                for (int i = 0; i < length; ++i)
                {
                    GLuint chainTexId;
                    ovr_GetTextureSwapChainBufferGL(Session, ColorTextureChain, i, &chainTexId);
                    glBindTexture(GL_TEXTURE_2D, chainTexId);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
            }
        }

        desc.Format = OVR_FORMAT_D32_FLOAT;

        {
            ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &DepthTextureChain);

            int length = 0;
            ovr_GetTextureSwapChainLength(session, DepthTextureChain, &length);

            if (OVR_SUCCESS(result))
            {
                for (int i = 0; i < length; ++i)
                {
                    GLuint chainTexId;
                    ovr_GetTextureSwapChainBufferGL(Session, DepthTextureChain, i, &chainTexId);
                    glBindTexture(GL_TEXTURE_2D, chainTexId);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                }
            }
        }

        glGenFramebuffers(1, &fboId);
    }

    ~OculusTextureBuffer()
    {
        if (ColorTextureChain)
        {
            ovr_DestroyTextureSwapChain(Session, ColorTextureChain);
            ColorTextureChain = nullptr;
        }
        if (DepthTextureChain)
        {
            ovr_DestroyTextureSwapChain(Session, DepthTextureChain);
            DepthTextureChain = nullptr;
        }
        if (fboId)
        {
            glDeleteFramebuffers(1, &fboId);
            fboId = 0;
        }
    }

    OVR::Sizei GetSize() const
    {
        return texSize;
    }

    void SetAndClearRenderSurface()
    {
        GLuint curColorTexId;
        GLuint curDepthTexId;
        {
            int curIndex;
            ovr_GetTextureSwapChainCurrentIndex(Session, ColorTextureChain, &curIndex);
            ovr_GetTextureSwapChainBufferGL(Session, ColorTextureChain, curIndex, &curColorTexId);
        }
        {
            int curIndex;
            ovr_GetTextureSwapChainCurrentIndex(Session, DepthTextureChain, &curIndex);
            ovr_GetTextureSwapChainBufferGL(Session, DepthTextureChain, curIndex, &curDepthTexId);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curColorTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, curDepthTexId, 0);

        glViewport(0, 0, texSize.w, texSize.h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_FRAMEBUFFER_SRGB);
    }

    void UnsetRenderSurface()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fboId);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    }

    void Commit()
    {
        ovr_CommitTextureSwapChain(Session, ColorTextureChain);
        ovr_CommitTextureSwapChain(Session, DepthTextureChain);
    }
};

#pragma endregion VR_HELPER_FN

#pragma region VR_RENDERING
static void setupOculus() {
    ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
    ovrResult result = ovr_Initialize(&initParams);
    assert(OVR_SUCCESS(result) && "Failed to initialize libOVR.");
    result = ovr_Create(&session, &luid);
    assert(OVR_SUCCESS(result) && "Failed to create libOVR session.");

    hmdDesc = ovr_GetHmdDesc(session);

    // Setup Window and Graphics
    // Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
    ovrSizei windowSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };

    // Make eye render buffers
    for (int eye = 0; eye < 2; eye++) {
        int pixelsPerDisplayUnits = 1;
        ovrSizei idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], pixelsPerDisplayUnits);
        eyeRenderTexture[eye] = new OculusTextureBuffer(session, idealTextureSize, 1);
    }
}

glm::mat4 vrProj;
glm::mat4 vrView;
static void renderVR(glm::mat4& viewMatWorld, glm::mat4& viewMatCam, int frameIndex) {
    for (int eye = 0; eye < 2; eye++) {
        eyeRenderTexture[eye]->SetAndClearRenderSurface();
        // Get view and projection matrices

        float Yaw = 0;
        static OVR::Vector3f Pos2(0.0f, 0.0f, -5.0f);
        OVR::Matrix4f rollPitchYaw = OVR::Matrix4f::RotationY(Yaw);
        OVR::Matrix4f finalRollPitchYaw = OVR::Matrix4f(EyeRenderPose[eye].Orientation);
        OVR::Vector3f finalUp = finalRollPitchYaw.Transform(OVR::Vector3f(0, 1, 0));
        OVR::Vector3f finalForward = finalRollPitchYaw.Transform(OVR::Vector3f(0, 0, -1));
        OVR::Vector3f shiftedEyePos = Pos2 + rollPitchYaw.Transform(EyeRenderPose[eye].Position);

        OVR::Matrix4f view = OVR::Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
        OVR::Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], camera.nearPlane, camera.farPlane, ovrProjection_None);
        viewMatCam = glm::transpose(glm::make_mat4(view.M[0])) * viewMatWorld;
        glm::mat4 projG = glm::transpose(glm::make_mat4(proj.M[0]));

        vrProj = projG;
        vrView = viewMatCam;

        // Avoids an error when calling SetAndClearRenderSurface during next iteration.
        // Without this, during the next while loop iteration SetAndClearRenderSurface
        // would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
        // associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
        eyeRenderTexture[eye]->UnsetRenderSurface();

        // Commit changes to the textures so they get picked up frame
        eyeRenderTexture[eye]->Commit();
    }

    // Do distortion rendering, Present and flush/sync

    ovrLayerEyeFovDepth ld = {};
    ld.Header.Type = ovrLayerType_EyeFovDepth;
    ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.
    ovrTimewarpProjectionDesc posTimewarpProjectionDesc = {};
    ld.ProjectionDesc = posTimewarpProjectionDesc;

    for (int eye = 0; eye < 2; ++eye)
    {
        ld.ColorTexture[eye] = eyeRenderTexture[eye]->ColorTextureChain;
        ld.DepthTexture[eye] = eyeRenderTexture[eye]->DepthTextureChain;
        ld.Viewport[eye] = OVR::Recti(eyeRenderTexture[eye]->GetSize());
        ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
        ld.RenderPose[eye] = EyeRenderPose[eye];
        ld.SensorSampleTime = sensorSampleTime;
    }

    ovrLayerHeader* layers = &ld.Header;
    ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);
}

static void updateHMDMatrixPose(int frameIndex) {
    for (int eye = 0; eye < 2; ++eye)
    {
        // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyePose) may change at runtime.
        eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye]);

        // Get eye poses, feeding in correct IPD offset
        HmdToEyePose[eye] = eyeRenderDesc[eye].HmdToEyePose;
    }

    // this data is fetched only for the debug display, no need to do this to just get the rendering work
    auto m_frameTiming = ovr_GetPredictedDisplayTime(session, frameIndex);
    auto m_trackingState = ovr_GetTrackingState(session, m_frameTiming, ovrTrue);

    // sensorSampleTime is fed into the layer later
    ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, EyeRenderPose, &sensorSampleTime);
}
#pragma endregion VR_RENDERING