#include <fstream>
#include <iostream>
#include <string>
#include <bit>


namespace FSOAL {
    std::int32_t convert_to_int(char* buffer, std::size_t len) {
        std::int32_t a = 0;
        std::memcpy(&a, buffer, len);
        return a;
    }

    bool load_wav_file_header(std::ifstream& file,
        std::uint8_t& channels,
        std::int32_t& sampleRate,
        std::uint8_t& bitsPerSample,
        ALsizei& size) {
        char buffer[4];
        if (!file.is_open())
            return false;

        // the RIFF
        if (!file.read(buffer, 4).good()) {
            LOG_WARN("Could not read RIFF");
            return false;
        }
        if (std::strncmp(buffer, "RIFF", 4) != 0) {
            LOG_WARN("File is not a valid WAVE file (header doesn't begin with RIFF)");
            return false;
        }

        // the size of the file
        if (!file.read(buffer, 4).good()) {
            LOG_WARN("Could not read size of file");
            return false;
        }

        // the WAVE
        if (!file.read(buffer, 4).good()) {
            LOG_WARN("Could not read WAVE");
            return false;
        }
        if (std::strncmp(buffer, "WAVE", 4) != 0) {
            LOG_WARN("File is not a valid WAVE file (header doesn't contain WAVE)");
            return false;
        }

        // "fmt/0"
        if (!file.read(buffer, 4).good()) {
            LOG_WARN("Could not read fmt/0");
            return false;
        }

        // this is always 16, the size of the fmt data chunk
        if (!file.read(buffer, 4).good()) {
            LOG_WARN("Could not read the 16");
            return false;
        }

        // PCM should be 1?
        if (!file.read(buffer, 2).good()) {
            LOG_WARN("Could not read PCM");
            return false;
        }

        // the number of channels
        if (!file.read(buffer, 2).good()) {
            LOG_WARN("Could not read number of channels");
            return false;
        }
        channels = convert_to_int(buffer, 2);

        // sample rate
        if (!file.read(buffer, 4).good()) {
            LOG_WARN("Could not read sample rate");
            return false;
        }
        sampleRate = convert_to_int(buffer, 4);

        // (sampleRate * bitsPerSample * channels) / 8
        if (!file.read(buffer, 4).good()) {
            LOG_WARN("Could not read (sampleRate * bitsPerSample * channels) / 8");
            return false;
        }

        // ?? dafaq
        if (!file.read(buffer, 2).good()) {
            LOG_WARN("Could not read dafaq");
            return false;
        }

        // bitsPerSample
        if (!file.read(buffer, 2).good()) {
            LOG_WARN("Could not read bits per sample");
            return false;
        }
        bitsPerSample = convert_to_int(buffer, 2);

        // data chunk header "data"
        if (!file.read(buffer, 4).good()) {
            LOG_WARN("Could not read data chunk header");
            return false;
        }
        if (std::strncmp(buffer, "data", 4) != 0) {
            LOG_WARN("File is not a valid WAVE file (doesn't have 'data' tag)");
            return false;
        }

        // size of data
        if (!file.read(buffer, 4).good()) {
            LOG_WARN("Could not read data size");
            return false;
        }
        size = convert_to_int(buffer, 4);

        /* cannot be at the end of file */
        if (file.eof()) {
            LOG_WARN("Reached EOF on the file");
            return false;
        }
        if (file.fail()) {
            LOG_WARN("Fail state set on the file");
            return false;
        }

        return true;
    }

    char* load_wav(const std::string& filename,
        std::uint8_t& channels,
        std::int32_t& sampleRate,
        std::uint8_t& bitsPerSample,

        std::vector<char>& soundData) {
        ALsizei size = 0;
        std::ifstream in(filename, std::ios::binary);
        if (!in.is_open()) {
            LOG_ERRR("Could not open \"", filename.c_str(), "\"");
            return nullptr;
        }
        if (!load_wav_file_header(in, channels, sampleRate, bitsPerSample, size)) {
            LOG_ERRR("Could not load wav header of \"", filename.c_str(), "\"");
            return nullptr;
        }

        soundData.resize(size);

        in.read(soundData.data(), size);

        return soundData.data();
    }
}