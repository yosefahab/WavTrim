# WavTrim


## Overview
Command line tool for trimming .wav files by the specified ratio.


## Command line flags
    -h                      Display help message

    -o <outfile>            Specify output file name,
                            by default wavTrim will add "trimmed_" prefix to input file (trimmed_<infile>.wav)

    -v                      Allow log information to be printed to stdout

    -r <ratio>              Amount of data to KEEP (Default = 0.5)

    -s                      Trim from start

    -e                      Trim from end

## Get started
```
make
```

## Demo
```c++
wavTrim test.wav
```

**Currently a WIP.**
