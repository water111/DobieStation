#ifndef ERRORS_HPP
#define ERRORS_HPP

#include <stdexcept>
#include <cstdint>

#define ERROR_STRING_MAX_LENGTH 255

class Errors
{
    public:
        static void breakpoint(uint32_t addr);
        static void die(const char* format, ...);
        static void nonfatal(const char* format, ...);
        static void print_warning(const char* format, ...);//ignores the error, just print it.
};

class Emulation_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class nonfatal_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

class breakpoint_exception : public std::exception
{
    public:
        uint32_t addr;

        breakpoint_exception(uint32_t addr) : addr(addr)
        {}

        virtual const char* what() const throw()
        {
            return "breakpoint exception";
        }
};

#endif
