#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <string>

class InvaildArgument : public std::exception
{
private:
    std::string error_str;

public:
    InvaildArgument(std::string error_type) : error_str("smash error: " + error_type + ": invalid arguments") {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

class TooManyArguments : public std::exception
{
    std::string error_str;

public:
    TooManyArguments(std::string error_type) : error_str("smash error: " + error_type + ": too many arguments") {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};


struct SystemCallFailed : public std::exception
{
    std::string error_str;

public:
    SystemCallFailed(std::string error_type) : error_str("smash error: " + error_type + " failed") {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

class OldPWDNotSet : public std::exception
{
    std::string error_str;
public:
    OldPWDNotSet(): error_str("smash error: cd: OLDPWD not set"){}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct JobIdDoesntExist : public std::exception
{
    std::string error_str;

public:
    JobIdDoesntExist(std::string error_type, int job_id) : error_str("smash error: " + error_type + ": "+ std::to_string(job_id)+ " does not exist") {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct JobsListEmpty : public std::exception
{
    std::string error_str;

public:
    JobsListEmpty() : error_str("smash error: fg: jobs list is empty") {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct JobAlreadyRunning : public std::exception
{
    std::string error_str;

public:
    JobAlreadyRunning(int job_id) : error_str("smash error: bg: job-id "+ std::to_string(job_id)+" is already running in the background") {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct NoStoppedJobs : public std::exception
{
    std::string error_str;

public:
    NoStoppedJobs() : error_str("smash error: bg: there is no stopped jobs to resume") {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

struct InvaildCoreNumber : public std::exception
{
    std::string error_str;

public:
    InvaildCoreNumber() : error_str("smash error: setcore: invalid core number") {}
    const char *what() const noexcept
    {
        return error_str.c_str();
    }
};

#endif // EXCEPTION_H