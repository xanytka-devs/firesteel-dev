#include "defenitions.hpp"

// [!NOTICE]
// Code from [al_wrap](https://github.com/inobelar/al_wrap/tree/main)
// with slight modifications.

namespace FSOAL {

    static ALCdevice* DEVICE;
    static ALCcontext* CONTEXT;

    static bool initialize() {
        const ALboolean enumeration = alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT");
        if (enumeration == AL_FALSE)
            LOG_WARN("OpenAL: enum extension not available");

        const ALCchar* devices = (alcGetString(NULL, ALC_DEVICE_SPECIFIER));
        const ALCchar* _device = devices,
            * next = devices + 1;
        size_t len = 0;

        LOG_INFO("OpenAL Devices:");
        while ((_device && (*_device != '\0')) && (next && (*next != '\0'))) {
            fprintf(stdout, "%s\n", _device);
            len = strlen(_device);
            _device += (len + 1);
            next += (len + 2);
        }
        fflush(stdout);

        char const* device_name = nullptr;
        device_name = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

        DEVICE = alcOpenDevice(device_name);
        if(DEVICE == nullptr) {
            LOG_ERRR("OpenAL: unable to open default device");
            return false;
        }

        LOG_INFO("OpenAL Device: ", alcGetString(DEVICE, ALC_DEVICE_SPECIFIER));

        alGetError();

        CONTEXT = alcCreateContext(DEVICE, NULL);
        if (alcMakeContextCurrent(CONTEXT) == AL_FALSE) {
            LOG_ERRR("OpenAL: Failed to make default context");
            return false;
        }

        if (alGetError() != AL_NO_ERROR)
            LOG_ERRR("OpenAL: Failed to make default context");
        return true;
    }

    static void deinitialize() {
        DEVICE = alcGetContextsDevice(CONTEXT);
        alcMakeContextCurrent(NULL);
        alcDestroyContext(CONTEXT);
        alcCloseDevice(DEVICE);

        DEVICE = nullptr;
        CONTEXT = nullptr;
    }

}