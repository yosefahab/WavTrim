#include <algorithm>
#include <iostream>
using namespace std;

bool VERBOSE = false;
float TRIM_RATIO = 0.5f;

template <typename... Args>
void log(const char* format, Args... args) {
    if (VERBOSE == false) {
        return;
    }
    printf(format, args...);
}

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
inline void display_header(const int& fileLength, const Wav_hdr& wavHeader) {
    cout << "\n----------------Header Info----------------\n";
    cout << "File size                  :" << fileLength << " bytes.\n";

    cout << "RIFF header                :"
         << wavHeader.ChunkID[0]
         << wavHeader.ChunkID[1]
         << wavHeader.ChunkID[2]
         << wavHeader.ChunkID[3] << '\n';

    cout << "Chunk size                 :" << wavHeader.ChunkSize << '\n';

    cout
        << "WAVE header                :"
        << wavHeader.WAVE[0]
        << wavHeader.WAVE[1]
        << wavHeader.WAVE[2]
        << wavHeader.WAVE[3]
        << '\n';

    cout << "FMT                        :"
         << wavHeader.Subchunk1ID[0]
         << wavHeader.Subchunk1ID[1]
         << wavHeader.Subchunk1ID[2]
         << wavHeader.Subchunk1ID[3]
         << '\n';
    cout << "Subchunk1 Size             :" << wavHeader.Subchunk1Size << '\n';

    // Audio format 1=PCM, 6=mulaw, 7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    cout << "Audio Format               :" << wavHeader.AudioFormat << '\n';
    cout << "Number of channels         :" << wavHeader.NumChannels << '\n';
    cout << "Sampling Rate              :" << wavHeader.SampleRate << '\n';
    cout << "Number of bytes per second :" << wavHeader.ByteRate << '\n';
    cout << "Block align                :" << wavHeader.BlockAlign << '\n';
    cout << "Number of bits per sample  :" << wavHeader.BitsPerSample << '\n';

    cout << "Subchunk2ID                :"
         << wavHeader.Subchunk2ID[0]
         << wavHeader.Subchunk2ID[1]
         << wavHeader.Subchunk2ID[2]
         << wavHeader.Subchunk2ID[3]
         << '\n';
    cout << "Subchunk2 (Data) Size      :" << wavHeader.Subchunk2Size << '\n';
    cout << "-------------------------------------------";
    cout << endl;
}

inline int get_file_size(FILE* file) {
    // move pointer to end of file
    fseek(file, 0, SEEK_END);

    const int fileSize = ftell(file);

    // move pointer back to start of file
    rewind(file);
    return fileSize;
}

inline FILE* load_wav(const string& path) {
    FILE* wavFile = fopen(path.c_str(), "rb");
    if (wavFile == NULL) {
        cerr << "Error opening wave file! check the file path!!\n";
        exit(1);
    }
    log("%s", "Successfully opened wave file\n");
    return wavFile;
}

// @todo more sanity checks
inline bool sanity_check_header(const Wav_hdr& wavHeader) {
    return ((wavHeader.SampleRate * wavHeader.NumChannels * wavHeader.BitsPerSample / 8) == wavHeader.ByteRate) &&
           ((wavHeader.NumChannels * wavHeader.BitsPerSample / 8) == wavHeader.BlockAlign);
}

// @brief Reads header chunk of .wav file
inline Wav_hdr read_header(FILE* wavFile) {
    Wav_hdr wavHeader;
    size_t itemsRead = fread(&wavHeader, sizeof(Wav_hdr), 1, wavFile);
    assert(itemsRead == 1);

    if (sanity_check_header(wavHeader) == false) {
        cerr << "Corrupt header, exiting" << endl;
        exit(1);
    }
    log("Successfully read %zu header.\n", itemsRead);
    return wavHeader;
}

// @brief Reads data chunk of .wav file
// @param trimRatio: Percentage of data to KEEP
// @returns Pointer to data buffer
inline int8_t* read_data(FILE* wavFile, Wav_hdr& wavHeader, const float& trimRatio) {
    // keep only <trimRatio> % of the data

    // update header to new size after trim
    wavHeader.ChunkSize -= ((1 - trimRatio) * wavHeader.Subchunk2Size);
    wavHeader.Subchunk2Size *= trimRatio;

    // read Subchunk2Size bytes over 1 byte chunks
    int8_t* buffer = new int8_t[wavHeader.Subchunk2Size];
    const size_t bytesRead = fread(buffer, sizeof(buffer[0]), wavHeader.Subchunk2Size / sizeof(buffer[0]), wavFile);

    assert(bytesRead == wavHeader.Subchunk2Size);
    assert(sanity_check_header(wavHeader) == 1);
    log("Successfully read %zu bytes of data.\n", bytesRead);
    return buffer;
}

inline void save_wav(const string& fileName, const Wav_hdr& wavHeader, int8_t* data) {
    FILE* wavFile = fopen(fileName.c_str(), "wb");
    fwrite(&wavHeader, sizeof(Wav_hdr), 1, wavFile);
    fwrite(data, wavHeader.Subchunk2Size, 1, wavFile);
    fclose(wavFile);
    log("%s", "Successfully saved file.\n");
}

inline void display_help_msg() {
    cout << "Usage:\n"
            "       wavTrim <infile> [options]\n"
            "Options:\n"
            "   -h                      Display this help message\n"
            "   -o <outfile>            Outfile name (Default= \"trimmed_\"+<infile>)\n"
            "   -v                      Verbose output\n"
            "   -r <ratio>              Trim .wav file by <ratio> (Default = 0.5)\n"
            "   -s                      Trim from start\n"
            "   -e                      Trim from end\n" // @todo implement
         << endl;
}

char* getCmdOption(char** begin, char** end, const string& option) {
    char** itr = find(begin, end, option);
    if (itr != end && ++itr != end) {
        return *itr;
    }
    return nullptr;
}

inline bool cmdOptionExists(char** begin, char** end, const string& option) {
    return find(begin, end, option) != end;
}

inline void parse_argv(const int& argc, char* argv[]) {
    if (argc < 2 || cmdOptionExists(argv, argv + argc, "-h")) {
        display_help_msg();
        exit(0);
    }

    VERBOSE = cmdOptionExists(argv, argv + argc, "-v");

    // @warning unsafe conversion
    if (cmdOptionExists(argv, argv + argc, "-r")) {
        TRIM_RATIO = atof(getCmdOption(argv, argv + argc, "-r"));
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

    // ################read wav data################//
    int8_t* data = read_data(wavFile, wavHeader, TRIM_RATIO);
    fclose(wavFile);

    // ################get output file name################//
    string outFile("trimmed_" + infile.substr(0, infile.size() - 4) + ".wav");
    if (cmdOptionExists(argv, argv + argc, "-o")) {
        outFile = getCmdOption(argv, argv + argc, "-o");
    }

    // ################save wav file################//
    save_wav(outFile, wavHeader, data);
    delete[] data;
    data = nullptr;
    if (VERBOSE) {
        // new file size after trimming <trim ratio>
        display_header(wavHeader.Subchunk2Size + sizeof(Wav_hdr), wavHeader);
    }

    cout << endl;
    return 0;
}