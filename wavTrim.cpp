#include <iostream>
using namespace std;

#define BUFFER_LENGTH 4096
#define VERBOSE 1
#define DEFAULT_TRIM_RATIO 0.5f

// @details The WAVE file format is a subset of Microsoft's RIFF specification for the storage of multimedia files.
// A RIFF file starts out with a file header followed by a sequence of data chunks.
// A WAVE file is often just a RIFF file with a single "WAVE" chunk which consists of two sub-chunks.
typedef struct WAV_HEADER {
    // RIFF chunk descriptor
    char ChunkID[4];    // RIFF header, contains the word "RIFF"      Magic Number
    uint32_t ChunkSize; // RIFF Chunk size, the entire file in bytes minus 8 bytes (ChunkID and ChunkSize)

    char WAVE[4]; // WAVE header, contains the word "WAVE"

    // fmt sub-chunk, describes the sound data's format
    char Subchunk1ID[4];    // contains the letters "fmt "
    uint32_t Subchunk1Size; // fmt chunk size, minus 8 bytes (Subchunk1ID and Subchunk1Size)
    uint16_t AudioFormat;   // audio format 1=PCM, 6=mulaw, 7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t NumChannels;   // number of channels 1=Mono 2=Sterio
    uint32_t SampleRate;    // sampling Frequency in Hz
    uint32_t ByteRate;      // bytes per second
    uint16_t BlockAlign;    // 2=16-bit mono, 4=16-bit stereo
    uint16_t BitsPerSample; // number of bits per sample

    // data sub-chunk
    char Subchunk2ID[4];    // WAVE Header, contains the word "data"
    uint32_t Subchunk2Size; // data chunk size

} Wav_hdr;
// @brief Displays header information of .wav file
inline void display_header(const int& fileLength, const Wav_hdr& wavHeader) {
    cout << "\n----------------Header Info----------------\n";
    cout << "File size                  :" << fileLength << " bytes." << endl;

    cout << "RIFF header                :"
         << wavHeader.ChunkID[0]
         << wavHeader.ChunkID[1]
         << wavHeader.ChunkID[2]
         << wavHeader.ChunkID[3] << endl;

    cout << "Chunk size                 :" << wavHeader.ChunkSize << endl;

    cout << "WAVE header                :"
         << wavHeader.WAVE[0]
         << wavHeader.WAVE[1]
         << wavHeader.WAVE[2]
         << wavHeader.WAVE[3]
         << endl;

    cout << "FMT                        :"
         << wavHeader.Subchunk1ID[0]
         << wavHeader.Subchunk1ID[1]
         << wavHeader.Subchunk1ID[2]
         << wavHeader.Subchunk1ID[3]
         << endl;
    cout << "Subchunk1 Size             :" << wavHeader.Subchunk1Size << endl;

    // Audio format 1=PCM, 6=mulaw, 7=alaw, 257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    cout << "Audio Format               :" << wavHeader.AudioFormat << endl;
    cout << "Number of channels         :" << wavHeader.NumChannels << endl;
    cout << "Sampling Rate              :" << wavHeader.SampleRate << endl;
    cout << "Number of bytes per second :" << wavHeader.ByteRate << endl;
    cout << "Block align                :" << wavHeader.BlockAlign << endl;
    cout << "Number of bits per sample  :" << wavHeader.BitsPerSample << endl;

    cout << "Subchunk2ID                :"
         << wavHeader.Subchunk2ID[0]
         << wavHeader.Subchunk2ID[1]
         << wavHeader.Subchunk2ID[2]
         << wavHeader.Subchunk2ID[3]
         << endl;
    cout << "Subchunk2 (Data) Size      :" << wavHeader.Subchunk2Size << endl;
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
        cout << "Error opening wave file!\n";
        // @todo write to stderr
        exit(1);
    }
    cout << "Successfully opened wave file\n";
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
    cout << "Successfully read " << itemsRead << " header." << endl;

    if (sanity_check_header(wavHeader) == false) {
        cout << "Corrupt header, exiting" << endl;
        // @todo write to stderr
        exit(1);
    }
    return wavHeader;
}
// @brief Reads data chunk of .wav file
// @returns Pointer to data buffer
inline int8_t* read_data(FILE* wavFile, Wav_hdr& wavHeader, const float& trimRatio = DEFAULT_TRIM_RATIO) {
    // keep only <trimRatio> % of the data

    // @bug modifying subchunk2size before reading incorrectly reads the file
    const uint32_t Subchunk2Size = wavHeader.Subchunk2Size;
    // read Subchunk2Size bytes over 1 byte chunks
    int8_t* buffer = new int8_t[Subchunk2Size * trimRatio];
    while (fread(buffer, sizeof(buffer[0]), Subchunk2Size / sizeof(buffer[0]), wavFile) > 0)
        ;

    assert(sanity_check_header(wavHeader) == 1);

    wavHeader.ChunkSize -= ((1 - trimRatio) * wavHeader.Subchunk2Size);
    wavHeader.Subchunk2Size *= trimRatio;

    cout << "Successfully read " << Subchunk2Size << " bytes of data" << endl;
    return buffer;
}

inline void save_wav(const string& fileName, const Wav_hdr& wavHeader, int8_t* data) {
    FILE* wavFile = fopen(("trimmed_" + fileName + ".wav").c_str(), "wb");
    fwrite(&wavHeader, sizeof(Wav_hdr), 1, wavFile);
    fwrite(data, wavHeader.Subchunk2Size, 1, wavFile);
    fclose(wavFile);
}

inline void display_help_msg() {
    cout << "\nUsage:\n"
            "       wavTrim <infile> [options]\n"
            "Options:\n"
            "   -h                      Display this help message\n"
            "   -o <outfile>            Outfile name (Default= \"trimmed_\"+<infile>)\n"
            "   -v                      Specify verbosity\n"
            "   -r <ratio>              Trim .wav file by <ratio> (Default = 0.5)\n"
            "   -s                      Trim from start\n"
            "   -e                      Trim from end\n"
         << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        display_help_msg();
        return 0;
    }
    // @todo parse argv

    const string infile = argv[1];
    FILE* wavFile = load_wav(infile);
    const int fileLength = get_file_size(wavFile);

    Wav_hdr wavHeader = read_header(wavFile);
    if (VERBOSE) {
        display_header(fileLength, wavHeader);
    }

    int8_t* data = read_data(wavFile, wavHeader);
    const string outfile = infile.substr(0, infile.size() - 4);
    save_wav(outfile, wavHeader, data);
    if (VERBOSE) {
        // new file size after trimming <trim ratio>
        display_header(wavHeader.Subchunk2Size + sizeof(Wav_hdr), wavHeader);
    }

    delete[] data;
    data = nullptr;
    fclose(wavFile);

    cout << endl;
    return 0;
}