#pragma once

#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cctype>

class String {
private:
    std::string s_;
public:
    String() : s_("") {}
    String(const char* s) : s_(s ? s : "") {}
    String(const std::string& s) : s_(s) {}
    String(char c) : s_(1, c) {}
    String(float val, int decimals) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(decimals) << val;
        s_ = ss.str();
    }

    const char* c_str() const { return s_.c_str(); }
    int length() const { return static_cast<int>(s_.length()); }

    void trim() {
        // remove leading whitespace
        s_.erase(s_.begin(), std::find_if(s_.begin(), s_.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        // remove trailing whitespace
        s_.erase(std::find_if(s_.rbegin(), s_.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s_.end());
    }

    int indexOf(char c, int start = 0) const {
        if (start < 0 || start > static_cast<int>(s_.length())) {
            return -1;
        }
        size_t pos = s_.find(c, start);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }

    String substring(int start, int end = -1) const {
        int len = length();
        if (start < 0) start = 0;
        if (start > len) start = len;
        if (end < 0 || end > len) end = len;
        if (end < start) end = start;
        return String(s_.substr(start, end - start));
    }

    float toFloat() const {
        try {
            return std::stof(s_);
        } catch (...) {
            return 0.0f;
        }
    }

    char operator[](int index) const {
        if (index < 0 || index >= length()) return '\0';
        return s_[index];
    }

    char& operator[](int index) {
        static char dummy = '\0';
        if (index < 0 || index >= length()) return dummy;
        return s_[index];
    }

    String& operator+=(char c) {
        s_ += c;
        return *this;
    }

    String& operator+=(const String& other) {
        s_ += other.s_;
        return *this;
    }

    String& operator+=(const char* other) {
        if (other) s_ += other;
        return *this;
    }

    friend String operator+(String lhs, const String& rhs) {
        lhs += rhs;
        return lhs;
    }

    friend String operator+(String lhs, const char* rhs) {
        lhs += rhs;
        return lhs;
    }

    friend String operator+(const char* lhs, const String& rhs) {
        String s(lhs);
        s += rhs;
        return s;
    }

    friend String operator+(String lhs, char rhs) {
        lhs += rhs;
        return lhs;
    }

    bool operator==(const String& other) const {
        return s_ == other.s_;
    }

    bool operator==(const char* other) const {
        return s_ == (other ? other : "");
    }
};

struct MockSerial {
    std::vector<std::string> printed_lines;
    std::string current_output;

    void print(const String& s) {
        current_output += s.c_str();
        size_t pos;
        while ((pos = current_output.find('\n')) != std::string::npos) {
            printed_lines.push_back(current_output.substr(0, pos + 1));
            current_output = current_output.substr(pos + 1);
        }
    }
    void println(const String& s) {
        print(s + "\n");
    }
    void begin(unsigned long baud) {}
    void begin(unsigned long baud, int config, int rxPin, int txPin) {}
    
    // For loop reading mock
    std::string input_stream;
    size_t input_idx = 0;
    bool available() {
        return input_idx < input_stream.length();
    }
    char read() {
        if (available()) {
            return input_stream[input_idx++];
        }
        return '\0';
    }

    void clear() {
        printed_lines.clear();
        current_output = "";
        input_stream = "";
        input_idx = 0;
    }
};

extern MockSerial Serial;
extern MockSerial Serial1;

#define SERIAL_8N1 0
inline void delay(int ms) {}
