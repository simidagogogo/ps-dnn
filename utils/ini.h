
// INI 文件的全称是 Initialization File，即初始化文件
// 它是一种简单的配置文件格式，通常用于存储配置信息，例如程序的设置、参数或其他初始化数据。
// INI 文件的结构简单，易于阅读和编辑，因此在许多软件和系统中被广泛使用。

#ifndef INICPP_H_
#define INICPP_H_

#include <string>
#include <algorithm>
#include <fstream>
#include <istream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <iostream>

using std::string;

// IniField表示INI文件中的一个字段（键值对中的值部分）
// 提供了多种赋值运算符()=和类型转换运算符(explicit operator), 支持将字段值转换为常见的C++类型(如int、float、bool等)
class IniField {
private:
  std::string value;

public:
  IniField() : value() {}
  IniField(const std::string& value) : value(value) {}
  IniField(const IniField& field) : value(field.value) {}

  ~IniField() {}

  template<typename T>
  T as() const {
    return static_cast<T>(*this);
  }

  // Assignment Operators
  IniField& operator=(const char* value) {
    this->value = std::string(value);
    return *this;
  }

  IniField& operator=(const std::string& value) {
    this->value = value;
    return *this;
  }

  IniField& operator=(const IniField& field) {
    this->value = field.value;
    return *this;
  }

  IniField& operator=(const int value) {
    std::stringstream ss;
    ss << value;
    this->value = ss.str();
    return *this;
  }

  IniField& operator=(const unsigned int value) {
    std::stringstream ss;
    ss << value;
    this->value = ss.str();
    return *this;
  }

  IniField& operator=(const double value) {
    std::stringstream ss;
    ss << value;
    this->value = ss.str();
    return *this;
  }

  IniField& operator=(const float value) {
    std::stringstream ss;
    ss << value;
    this->value = ss.str();
    return *this;
  }

  IniField& operator=(const bool value) {
    if (value) {
      this->value = "true";
    } else {
      this->value = "false";
    }
    return *this;
  }

  // Cast Operators
  explicit operator const char* () const {
    return value.c_str();
  }

  explicit operator std::string() const {
    return value;
  }

  explicit operator int() const {
    char* endptr;

    // check if decimal
    int result = std::strtol(value.c_str(), &endptr, 10);
    if (*endptr == '\0') return result;

    // check if octal
    result = std::strtol(value.c_str(), &endptr, 8);
    if (*endptr == '\0') return result;

    // check if hex
    result = std::strtol(value.c_str(), &endptr, 16);
    if (*endptr == '\0') return result;

    throw std::invalid_argument("field is not an int");
  }

  explicit operator unsigned int() const {
    char* endptr;
    // check if decimal
    int result = std::strtoul(value.c_str(), &endptr, 10);
    if (*endptr == '\0') return result;

    // check if octal
    result = std::strtoul(value.c_str(), &endptr, 8);
    if (*endptr == '\0') return result;

    // check if hex
    result = std::strtoul(value.c_str(), &endptr, 16);
    if (*endptr == '\0') return result;

    throw std::invalid_argument("field is not an unsigned int");
  }

  explicit operator float() const {
    return std::stof(value);
  }

  explicit operator double() const {
    return std::stod(value);
  }

  explicit operator bool() const {
    std::string str(value);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    if (str == "TRUE") {
      return true;
    } else if (str == "FALSE") {
      return false;
    } else {
      throw std::invalid_argument("field is not a bool");
    }
  }
};


// 表示INI文件中的一个节 section
// 每个节包含多个字段, 字段以键值对的形式存储
class IniSection : public std::map<std::string, IniField> {
public:
  IniSection() {}
  ~IniSection() {}
};


// 表示整个INI文件
// 提供了加载（load）、保存（save）、解码（decode）和编码（encode）等方法，用于从文件或字符串中读取INI数据，或将数据写回文件或字符串。
// 支持自定义字段分隔符（默认为=）和注释字符（默认为#）
class IniFile : public std::map<std::string, IniSection> {
private:
  char field_sep; // 自定义字段分隔符
  char comment;   // 注释字符

public:
  IniFile() : IniFile('=', '#') {}

  IniFile(const char& fieldSep, const char& comment) : field_sep(fieldSep), comment(comment) {}

  IniFile(const std::string& filename, const char sep = '=', const char comment = '#') : IniFile(sep, comment) {
    load(filename);
  }

  IniFile(std::istream& is, const char& sep = '=', const char& comment = '#') : IniFile(sep, comment) {
    decode(is);
  }

  ~IniFile() {}

  void setFieldSep(const char& sep) {
    this->field_sep = sep;
  }

  void setCommentChar(const char& comment) {
    this->comment = comment;
  }

  void decode(std::istream& istream) {
    clear();
    int cnt = 0;
    IniSection* currentSection = NULL;

    // iterate file by line
    while (!istream.eof() && !istream.fail()) {
      std::string line;
      std::getline(istream, line, '\n');
      ++cnt;

      // 1. skip if line is empty
      if (line.size() == 0) continue;

      // 2. skip if line is a comment
      if (line[0] == comment) continue;

      // 3. line is a section
      if (line[0] == '[') {
        // 3.1 check if the section is also closed on same line
        std::size_t pos = line.find("]");
        if (pos == std::string::npos) {
          std::stringstream ss;
          ss << "l" << cnt << ": ini parsing failed, section not closed";
          throw std::logic_error(ss.str());
        }

        // 3.2 check if the section name is empty
        if (pos == 1) {
          std::stringstream ss;
          ss << "l" << cnt << ": ini parsing failed, section is empty";
          throw std::logic_error(ss.str());
        }

        // 3.3 check if there is a newline following closing bracket
        // 方括号后面不能有多余的字符(例如空格或注释), 否则会被认为是格式错误
        if (pos + 1 != line.length()) {
          std::stringstream ss;
          ss << "l" << cnt << ": ini parsing failed, no end of line after section";
          throw std::logic_error(ss.str());
        }

        // 3.4 retrieve section name
        std::string secName = line.substr(1, pos - 1);
        currentSection = &((*this)[secName]);
      } else {
        // 4. line is a field definition
        // 4.1 check if section was already opened
        if (!currentSection) {
          std::stringstream ss;
          ss << "l" << cnt << ": ini parsing failed, field has no section";
          throw std::logic_error(ss.str());
        }

        // 4.2 find key value separator
        std::size_t pos = line.find(field_sep);
        if (pos == std::string::npos) {
          std::stringstream ss;
          ss << "l" << cnt << ": ini parsing failed, no '=' found";
          throw std::logic_error(ss.str());
        }

        // 4.3 retrieve field name and value
        std::string name = line.substr(0, pos);
        std::string value = line.substr(pos + 1, std::string::npos);
        (*currentSection)[name] = value;
      }
    }
  }

  void decode(const std::string& content) {
    std::istringstream ss(content);
    decode(ss);
  }

  void load(const std::string& fileName) {
    // std::ifstream是std::istream的派生类
    std::ifstream file(fileName.c_str());
    if (!file) {
      std::cerr << "Ini File " << fileName << " doesn't exist" << std::endl;
      exit(1);
    }
    decode(file);
  }

  void encode(std::ostream& os) const {
    // iterate through all sections in this file
    for (const auto& filePair : *this) {
      os << "[" << filePair.first << "]" << std::endl;
      
      // iterate through all fields in the section
      for (const auto& kv : filePair.second) {
        os << kv.first << field_sep << kv.second.as<std::string>() << std::endl;
      }
    }
  }

  std::string encode() const {
    std::ostringstream ss;
    encode(ss);
    return ss.str();
  }

  void save(const std::string& fileName) const {
    std::ofstream os(fileName.c_str());
    if (!os) {
      throw std::ios_base::failure("Failed to open file: " + fileName);
    }
    encode(os);
  }
};

#endif
