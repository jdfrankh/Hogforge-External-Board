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

private:
    uint8_t chipSelectPin;
    bool initialized;

    void collectGcodeFiles(const String &basePath, std::vector<String> &files);
    bool hasGcodeSuffix(const String &filename) const;
};
