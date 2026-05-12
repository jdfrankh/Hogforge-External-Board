#include "SDGcode.h"

#include <SD.h>

namespace {
std::vector<String> tokenizeByWhitespace(const String &line) {
    std::vector<String> tokens;
    int length = line.length();
    int i = 0;

    while (i < length) {
        while (i < length && isspace(static_cast<unsigned char>(line[i]))) {
            i++;
        }

        if (i >= length) {
            break;
        }

        int start = i;
        while (i < length && !isspace(static_cast<unsigned char>(line[i]))) {
            i++;
        }

        tokens.push_back(line.substring(start, i));
    }

    return tokens;
}
}

SDGcode::SDGcode(uint8_t chipSelectPin)
    : chipSelectPin(chipSelectPin), initialized(false) {}

bool SDGcode::begin() {
    initialized = SD.begin(chipSelectPin);
    return initialized;
}

std::vector<String> SDGcode::getGcodeFiles(const String &directory) {
    std::vector<String> files;

    if (!initialized) {
        return files;
    }

    collectGcodeFiles(directory, files);
    return files;
}

std::vector<std::vector<String>> SDGcode::parseGcodeFile(const String &filePath) {
    std::vector<std::vector<String>> parsedLines;

    if (!initialized) {
        return parsedLines;
    }

    File file = SD.open(filePath.c_str());
    if (!file || file.isDirectory()) {
        if (file) {
            file.close();
        }
        return parsedLines;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.replace("\r", "");
        line.trim();

        if (line.length() == 0) {
            continue;
        }

        int commentStart = line.indexOf(';');
        if (commentStart >= 0) {
            line = line.substring(0, commentStart);
            line.trim();
        }

        if (line.length() == 0) {
            continue;
        }

        std::vector<String> tokens = tokenizeByWhitespace(line);
        if (!tokens.empty()) {
            parsedLines.push_back(tokens);
        }
    }

    file.close();
    return parsedLines;
}

void SDGcode::collectGcodeFiles(const String &basePath, std::vector<String> &files) {
    File dir = SD.open(basePath.c_str());
    if (!dir || !dir.isDirectory()) {
        if (dir) {
            dir.close();
        }
        return;
    }

    while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
            break;
        }

        String name = String(entry.name());
        String fullPath = basePath;

        if (!fullPath.endsWith("/")) {
            fullPath += "/";
        }
        fullPath += name;

        if (entry.isDirectory()) {
            collectGcodeFiles(fullPath, files);
        } else if (hasGcodeSuffix(name)) {
            files.push_back(fullPath);
        }

        entry.close();
    }

    dir.close();
}

bool SDGcode::hasGcodeSuffix(const String &filename) const {
    String lower = filename;
    lower.toLowerCase();
    return lower.endsWith(".gcode");
}
