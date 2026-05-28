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

// --- Streaming API ---

static bool parseLine(const String &raw, std::vector<String> &tokens) {
    String line = raw;
    line.replace("\r", "");
    line.trim();
    if (line.length() == 0) return false;
    int cs = line.indexOf(';');
    if (cs == 0) return false;
    if (cs > 0) { line = line.substring(0, cs); line.trim(); }
    if (line.length() == 0) return false;
    tokens = tokenizeByWhitespace(line);
    return !tokens.empty();
}

int SDGcode::countGcodeLines(const String &filePath) {
    if (!initialized) return 0;
    File file = SD.open(filePath.c_str());
    if (!file || file.isDirectory()) { if (file) file.close(); return 0; }
    int count = 0;
    std::vector<String> dummy;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (parseLine(line, dummy)) count++;
    }
    file.close();
    return count;
}

bool SDGcode::openGcodeFile(const String &filePath) {
    if (!initialized) return false;
    if (_streamFile) _streamFile.close();
    _streamFile = SD.open(filePath.c_str());
    return (_streamFile && !_streamFile.isDirectory());
}

bool SDGcode::readLine(String &line) {
    if (!_streamFile) return false;
    while (_streamFile.available()) {
        line = _streamFile.readStringUntil('\n');
        line.replace("\r", "");
        line.trim();
        if (line.length() > 0) return true;
    }
    return false; // EOF
}

bool SDGcode::readNextGcodeLine(std::vector<String> &tokens) {
    tokens.clear();
    String line;
    while (readLine(line)) {
        if (line.charAt(0) == ';') continue; // skip comment-only lines
        int cs = line.indexOf(';');
        if (cs > 0) { line = line.substring(0, cs); line.trim(); }
        if (line.length() == 0) continue;
        tokens = tokenizeByWhitespace(line);
        if (!tokens.empty()) return true;
    }
    return false; // EOF
}

std::vector<String> SDGcode::tokenize(const String &line) {
    return tokenizeByWhitespace(line);
}

void SDGcode::closeGcodeFile() {
    if (_streamFile) _streamFile.close();
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
