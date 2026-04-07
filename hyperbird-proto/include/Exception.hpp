#pragma once

// System includes
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

class Exception : public std::runtime_error
{

public:
    Exception(const char *file, int line, const std::string &arg) : std::runtime_error(arg)
    {
        // msg_ = std::string(file) + ":" + std::to_string(line) + ": " + arg;
        (void)file;
        (void)line;
        msg_ = arg;
    }

    ~Exception() throw() {}

    const char *what() const throw() override
    {
        return msg_.c_str();
    }

private:
    std::string msg_;
};

#define THROW_EXCEPT(arg) throw Exception(__FILE__, __LINE__, arg);