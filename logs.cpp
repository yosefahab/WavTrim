#include <iostream>
using namespace std;

extern bool VERBOSE;
template <typename... Args>
void log(const char* format, Args... args) {
    if (VERBOSE == false) {
        return;
    }
    printf(format, args...);
}

// @brief Writes error message to stderr and exits with error 1
inline void logErr(const string& err) {
    cerr << err << endl;
    exit(1);
}

inline void display_help_msg() {
    cout << "Usage:\n"
            "       wavTrim <infile> [options]\n"
            "Options:\n"
            "   -h                      Display this help message\n"
            "   -o <outfile>            Outfile name (Default= \"trimmed_\"+<infile>)\n"
            "   -v                      Verbose output\n"
            "   -r <ratio>              Trim .wav file by <ratio> (Default = 0.5)\n"
            "   -s <offset>             Seek to specified offset\n" // @todo implement
            "   -e                      Trim from end\n"
         << endl;
}