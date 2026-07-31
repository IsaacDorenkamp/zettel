#pragma once

#include <exception>
#include <string>

#define DEFINE_EXCEPTION class Exception : public std::exception {\
public:\
    Exception(std::string message) : m_message(message) {}\
    const char* what() const throw() { return m_message.c_str(); }\
private:\
    std::string m_message;\
}
