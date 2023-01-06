# WavTrim


## Overview
Command line tool for trimming .wav files by the specified ratio.


## Command line flags
    -h                      Display help message

    -o <outfile>            Specify output file name,
                            by default wavTrim will add "trimmed_" prefix to input file (trimmed_<infile>.wav)

    -v                      Allow log information to be printed to stdout

    -r <ratio>              Amount of data to KEEP (Default = 0.5)

    -e                      Trim from end

## Get started
```
make
```

## Demo
To trim test.wav by 50%
```
./wavTrim test.wav
```
or
```
./wavTrim test.wav -r 0.5
```

To specify outfile
```
./wavTrim test.wav -o output_file_name.wav
```

To trim last first 30%
```
./wavTrim test.wav -r 0.7 -e
```

