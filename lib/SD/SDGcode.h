#pragma once

#include <Arduino.h>
#include <SD.h>
#include <vector>

class SDGcode {
public:
    explicit SDGcode(uint8_t chipSelectPin = BUILTIN_SDCARD);

    bool begin();
    std::vector<String> getGcodeFiles(const String &directory = "/");
    std::vector<std::vector<String>> parseGcodeFile(const String &filePath);

    // Streaming API — avoids loading the whole file into RAM
    int  countGcodeLines(const String &filePath);
    bool openGcodeFile(const String &filePath);
    bool readLine(String &line);                           // next non-empty raw line (comments included)
    bool readNextGcodeLine(std::vector<String> &tokens);  // next executable line only
    void closeGcodeFile();

    static std::vector<String> tokenize(const String &line);

private:
    uint8_t chipSelectPin;
    bool initialized;
    File _streamFile;

    void collectGcodeFiles(const String &basePath, std::vector<String> &files);
    bool hasGcodeSuffix(const String &filename) const;
};
