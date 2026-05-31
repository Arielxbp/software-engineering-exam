#pragma once

#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace SELib {

// ============================================================================
// 11. PARAMETER PARSER
// ============================================================================

// ParameterParser: A utility to parse and retrieve parameters from
// configuration files. Purpose:
//  - Reads a file and stores key-value pairs for parameters.
//  - Allows retrieval of parameters as integers, doubles, or strings.
//  - Provides structured data for advanced parsing.
// Usage:
//  ParameterParser parser;
//  if (parser.parseFile("config.txt")) {
//      int param = parser.getInt("example_key", 42);
//  }
class ParameterParser {
private:
  std::map<std::string, std::string> params;
  std::vector<std::vector<std::string>> structuredData;

public:
  bool parseFile(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open())
      return false;

    std::string line;
    while (std::getline(file, line)) {
      if (line.empty())
        continue;

      std::istringstream iss(line);
      std::string key;
      iss >> key;

      std::string value;
      std::getline(iss, value);
      if (!value.empty() && value[0] == ' ') {
        value = value.substr(1);
      }

      params[key] = value;

      std::vector<std::string> tokens;
      std::istringstream tokenStream(line);
      std::string token;
      while (tokenStream >> token) {
        tokens.push_back(token);
      }
      structuredData.push_back(tokens);
    }
    return true;
  }

  int getInt(const std::string &key, int defaultValue = 0) const {
    auto it = params.find(key);
    if (it != params.end()) {
      return std::stoi(it->second);
    }
    return defaultValue;
  }

  double getDouble(const std::string &key, double defaultValue = 0.0) const {
    auto it = params.find(key);
    if (it != params.end()) {
      return std::stod(it->second);
    }
    return defaultValue;
  }

  std::string getString(const std::string &key,
                        const std::string &defaultValue = "") const {
    auto it = params.find(key);
    if (it != params.end()) {
      return it->second;
    }
    return defaultValue;
  }

  const std::vector<std::vector<std::string>> &getStructuredData() const {
    return structuredData;
  }

  bool hasKey(const std::string &key) const {
    return params.find(key) != params.end();
  }
};

// ============================================================================
// 12. OUTPUT WRITER
// ============================================================================

// OutputWriter: A utility class for writing data to output files.
// Purpose:
//  - Simplifies the process of writing data to files.
//  - Supports writing lines, key-value pairs, and managing file headers.
// Usage:
//  OutputWriter writer;
//  if (writer.open("output.txt", "Header Info")) {
//    writer.writeLine("Example Line");
//    writer.writeKeyValue("Key", 42.0);
//    writer.close();
//  }
class OutputWriter {
private:
  std::ofstream file;

public:
  bool open(const std::string &filename, const std::string &header) {
    file.open(filename);
    if (!file.is_open())
      return false;
    file << header << std::endl;
    return true;
  }

  template <typename T> void writeLine(const T &value) {
    file << value << std::endl;
  }

  template <typename T, typename... Args>
  void writeLine(const T &first, const Args &...args) {
    file << first << " ";
    writeLine(args...);
  }

  void writeKeyValue(const std::string &key, double value, int precision = 2) {
    file << key << " " << std::fixed << std::setprecision(precision) << value
         << std::endl;
  }

  void close() {
    if (file.is_open()) {
      file.close();
    }
  }

  ~OutputWriter() { close(); }
};
} // namespace SELib
