#include "logs.cpp"
#include <algorithm>
#include <iostream>

using namespace std;

bool VERBOSE = false;
float TRIM_RATIO = 0.5f;

// The WAVE file format is a subset of Microsoft's RIFF specification for the storage of multimedia files.
// A RIFF file starts out with a file header followed by a sequence of data chunks.
// A WAVE file is often just a RIFF file with a single "WAVE" chunk which consists of two sub-chunks.
typedef struct WAV_HEADER {
    // RIFF chunk descriptor
    char ChunkID[4];    // RIFF header, contains the word "RIFF"      Magic Number
    uint32_t ChunkSize; // RIFF Chunk size in bytes, the entire file in bytes minus 8 bytes (ChunkID and ChunkSize)

    char WAVE[4]; // WAVE header, contains the word "WAVE"

    // fmt sub-chunk, describes the sound data's format
    char Subchunk1ID[4];    // contains the letters "fmt "
    uint32_t Subchunk1Size; // fmt chunk size in bytes, minus 8 bytes (Subchunk1ID and Subchunk1Size)
    uint16_t AudioFormat;   // audio format 1=PCM, 6=mulaw, 7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t NumChannels;   // number of channels 1=Mono 2=Sterio
    uint32_t SampleRate;    // sampling Frequency in Hz
    uint32_t ByteRate;      // bytes per second
    uint16_t BlockAlign;    // 2=16-bit mono, 4=16-bit stereo
    uint16_t BitsPerSample; // number of bits per sample

    // data sub-chunk
    char Subchunk2ID[4];    // WAVE Header, contains the word "data"
    uint32_t Subchunk2Size; // data chunk size in bytes

} Wav_hdr;

// @brief Displays header information of .wav file
void display_header(const uint32_t& fileLength, const Wav_hdr& wavHeader) {
    log("%s", "\n----------------Header Info----------------\n");
    log("File size                  :%d bytes.\n", fileLength);

    log("RIFF header                :%.4s\n", wavHeader.ChunkID);

    log("Chunk size                 :%d bytes.\n", wavHeader.ChunkSize);

    log("WAVE header                :%.4s\n", wavHeader.WAVE);

    log("FMT                        :%s\n", wavHeader.Subchunk1ID);

    log("Subchunk1 Size             :%d bytes.\n", wavHeader.Subchunk1Size);

    // Audio format 1=PCM, 6=mulaw, 7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    log("Audio Format               :%d\n", wavHeader.AudioFormat);
    log("Number of channels         :%d\n", wavHeader.NumChannels);
    log("Sampling Rate              :%d\n", wavHeader.SampleRate);
    log("Number of bytes per second :%d\n", wavHeader.ByteRate);
    log("Block align                :%d\n", wavHeader.BlockAlign);
    log("Number of bits per sample  :%d\n", wavHeader.BitsPerSample);

    log("Subchunk2ID                :%.4s\n", wavHeader.Subchunk2ID);

    log("Subchunk2 (Data) Size      :%d bytes.\n", wavHeader.Subchunk2Size);
    log("%s", "-------------------------------------------\n");
}

uint32_t get_file_size(FILE* file) {
    fseek(file, 0, SEEK_END);
    const uint32_t fileSize = ftell(file);
    rewind(file);
    return fileSize;
}

inline FILE* load_wav(const string& path) {
    FILE* wavFile = fopen(path.c_str(), "rb");
    if (wavFile == NULL) {
        logErr("Error opening wave file, check the file path!");
    }
    log("%s", "Successfully opened wave file.\n");
    return wavFile;
}

// @todo more sanity checks
constexpr bool sanity_check_header(const Wav_hdr& wavHeader) {
    return ((wavHeader.SampleRate * wavHeader.NumChannels * wavHeader.BitsPerSample / 8) == wavHeader.ByteRate) &&
           ((wavHeader.NumChannels * wavHeader.BitsPerSample / 8) == wavHeader.BlockAlign);
}

// @brief Reads header chunk of .wav file
inline Wav_hdr read_header(FILE* wavFile) {
    Wav_hdr wavHeader;
    size_t itemsRead = fread(&wavHeader, sizeof(Wav_hdr), 1, wavFile);
    assert(itemsRead == 1);

    if (sanity_check_header(wavHeader) == false) {
        logErr("Corrupt header, exiting");
    }
    log("Successfully read %zu header.\n", itemsRead);
    return wavHeader;
}

// @brief Reads data chunk of .wav file
// @param trimRatio: Percentage of data to KEEP
// @returns Pointer to data buffer
inline int8_t* read_data(FILE* wavFile, Wav_hdr& wavHeader, const float& trimRatio, const bool& fromEnd) {
    // if trim from end, seek to the last <bytesToRead> bytes
    const uint32_t bytesToRead = wavHeader.Subchunk2Size * trimRatio;
    if (fromEnd == true) {
        fseek(wavFile, -static_cast<int32_t>(bytesToRead), SEEK_END);
    }

    // keep only <trimRatio> % of the data
    // update header to new size after trim
    wavHeader.ChunkSize -= (wavHeader.Subchunk2Size - bytesToRead);
    wavHeader.Subchunk2Size = bytesToRead;

    // read Subchunk2Size bytes over 1 byte chunks
    int8_t* buffer = new int8_t[wavHeader.Subchunk2Size];
    const size_t bytesRead = fread(buffer, sizeof(buffer[0]), wavHeader.Subchunk2Size / sizeof(buffer[0]), wavFile);

    assert(bytesRead == bytesToRead);
    assert(sanity_check_header(wavHeader) == 1);
    log("Successfully read %zu bytes of data.\n", bytesRead);
    return buffer;
}

inline void save_wav(const string& fileName, const Wav_hdr& wavHeader, int8_t* data) {
    FILE* wavFile = fopen(fileName.c_str(), "wb");
    fwrite(&wavHeader, sizeof(Wav_hdr), 1, wavFile);
    fwrite(data, wavHeader.Subchunk2Size, 1, wavFile);
    fclose(wavFile);
    log("Successfully saved to file: %s\n", fileName.c_str());
}

char* getCmdOption(char** begin, char** end, const string& option) {
    char** itr = find(begin, end, option);
    if (itr != end && ++itr != end) {
        return *itr;
    }
    return nullptr;
}

bool cmdOptionExists(char** begin, char** end, const string& option) {
    return find(begin, end, option) != end;
}

inline void parse_argv(const int& argc, char* argv[]) {
    if (argc < 2 || cmdOptionExists(argv, argv + argc, "-h")) {
        display_help_msg();
        exit(0);
    }

    if (cmdOptionExists(argv, argv + argc, "-e") && cmdOptionExists(argv, argv + argc, "-s")) {
        logErr("Please only specify one of {-e, -s} flags, not both.");
    }

    VERBOSE = cmdOptionExists(argv, argv + argc, "-v");

    if (cmdOptionExists(argv, argv + argc, "-r")) {
        try {
            // @bug empty <ratio> causes seg fault
            TRIM_RATIO = stof(getCmdOption(argv, argv + argc, "-r"));
            if (TRIM_RATIO > 1) {
                logErr("Ratio must not be greater than 1!.");
            }
        } catch (exception) {
            logErr("Invalid ratio passed, terminating.");
        }
    }
}

int main(int argc, char* argv[]) {
    parse_argv(argc, argv);

    // ################load wav file################//
    const string infile = argv[1];
    FILE* wavFile = load_wav(infile);

    // ################read wav header################//
    Wav_hdr wavHeader = read_header(wavFile);
    if (VERBOSE) {
        display_header(get_file_size(wavFile), wavHeader);
    }

    // @attention fromEnd is guaranteed to be false if the seek flag is specified.
    const bool fromEnd = cmdOptionExists(argv, argv + argc, "-e");
    // float offset = 0;
    // if (cmdOptionExists(argv, argv + argc, "-s")) {
    //     offset = stof(getCmdOption(argv, argv + argc, "-s"));
    // }
    // ################read wav data################//
    int8_t* data = read_data(wavFile, wavHeader, TRIM_RATIO, fromEnd);
    fclose(wavFile);
    if (VERBOSE) {
        // new file size after trimming <trim ratio>
        display_header(wavHeader.Subchunk2Size + sizeof(Wav_hdr), wavHeader);
    }

    // ################get output file name################//
    string outFile("trimmed_" + infile.substr(0, infile.size() - 4) + ".wav");
    if (cmdOptionExists(argv, argv + argc, "-o")) {
        outFile = getCmdOption(argv, argv + argc, "-o");
    }

    // ################save wav file################//
    save_wav(outFile, wavHeader, data);
    delete[] data;
    data = nullptr;

    cout << endl;
    return 0;
}