#pragma once

#include <exception>
#include <string>

#define DEFINE_CUSTOM_EXCEPTION(exc_name) class exc_name : public std::exception {\
public:\
    exc_name(std::string message) : std::exception(), m_message(message) {}\
    const char* what() const throw() { return m_message.c_str(); }\
private:\
    std::string m_message;\
}
#define DEFINE_EXCEPTION DEFINE_CUSTOM_EXCEPTION(Exception)
