#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <string>

struct InvaildArgument : public std::exception
{
    std::string error_str;

public:
    InvaildArgument(std::string error_str) : error_str(error_str) {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct SystemCallFailed : public std::exception
{
    std::string error_str;

public:
    SystemCallFailed(std::string error_str) : error_str(error_str) {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct OldPWDNotSet : public std::exception
{
    std::string error_str;

public:
    OldPWDNotSet(std::string error_str) : error_str(error_str) {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct JobIdDoesntExist : public std::exception
{
    std::string error_str;

public:
    JobIdDoesntExist(std::string error_str) : error_str(error_str) {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct JobsListEmpty : public std::exception
{
    std::string error_str;

public:
    JobsListEmpty(std::string error_str) : error_str(error_str) {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct JobAlreadyRunning : public std::exception
{
    std::string error_str;

public:
    JobAlreadyRunning(std::string error_str) : error_str(error_str) {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct NoStoppedJobs : public std::exception
{
    std::string error_str;

public:
    NoStoppedJobs(std::string error_str) : error_str(error_str) {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct InvaildCoreNumber : public std::exception
{
    std::string error_str;

public:
    InvaildCoreNumber(std::string error_str) : error_str(error_str) {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

#endif // EXCEPTION_H