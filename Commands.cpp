// #include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
// #include <sched.h>
// #include <sys/wait.h>
// #include <sys/stat.h>
#include <iomanip>
#include "Commands.h"
#include <signal.h>
#include <sys/types.h>
#include <memory>
#include "./Exeptions.h"
// #include <thread>

#define SIGKILL 9
using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

//<---------------------------stuff functions--------------------------->

#if 0
#define FUNC_ENTRY() \
    cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
    cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
    return _rtrim(_ltrim(s));
}

void _reformatArgsVec(char **args, vector<string> vec)
{
    for (int i = 0; i < vec.size(); i++)
    {
        args[i] = new char[vec[i].size() + 1];
        memset(args[i], 0, vec[i].size() + 1);
        strcpy(args[i], vec[i].c_str());
    }
    args[vec.size()] = NULL;
}

int _parseCommandLine(const char *cmd_line, char **args)
{
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;)
    {
        args[i] = (char *)malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundCommand(const char *cmd_line)
{
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos)
    {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&')
    {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

void removeBackgroundSignString(string str)
{
    if (str.back() == '&')
        str.erase(str.size(), 1);
}

//<---------------------------stuff functions - end --------------------------->

std::vector<std::string> get_args_in_vec(const char *cmd_line)
{
    std::vector<std::string> res;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;)
        res.push_back(s);
    return res;
}

bool _isSimpleExternal(std::string str)
{
    return str.find_first_of("*?") == std::string::npos;
}

// Checks if a given string is made of digits only
bool isStringNumber(std::string str)
{
    for (int i = 0; i < str.size(); i++)
    {
        if (!isdigit(str[i]))
            return false;
    }
    return true;
}

//<---------------------------C'tors and D'tors--------------------------->

// Small Shell
SmallShell::SmallShell() : prompt("smash"), last_wd(""), current_command(nullptr), jobs_list()
{
}

// Command

Command::Command(const char *cmd_line) : job_id(-1), process_id(getpid()), cmd_l(cmd_line), args_vec()
{
    args_vec = get_args_in_vec(cmd_l);
};

RedirectionCommand::RedirectionCommand(const char *cmd_line, string sign) : Command::Command(cmd_line)
{
    SmallShell &smash = SmallShell::getInstance();

    // finding the > / >> sign and validating arguments
    string cmd_str = string(cmd_line);
    int sign_index = cmd_str.find(sign);
    if (sign_index == 0 || sign_index == cmd_str.size() + 1)
    {
        InvaildArgument e(sign);
        throw e;
    }

    // calculating the base command to be redirected (e.g., ls, showPid, ...) and the destination input file
    string base_cmd = cmd_str.substr(0, sign_index);
    dest = get_args_in_vec(cmd_str.substr(sign_index + 1, cmd_str.size() + 1).c_str())[0];
    base_command = smash.CreateCommand(base_cmd.c_str()).get();

    // out_pd = the index of a new FD that points to the standard output
    out_pd = dup(1);
}

void RedirectionCommand::execute()
{
    // changing the standard output to dest for smash itself
    prepare();

    // external cmd routine:
    if (base_command->isExternal())
    {
        int pid = fork();
        // -------------child------------//
        // notice we assume the cmd isn't running in the BackGround and won't get interrupted
        if (!pid)
        {
            int res = setpgrp();
            if (res < 0)
                perror("smash error: setpgrp failed");
            base_command->execute();
        }

        // ------------father----------//
        else
        {
            waitpid(pid);
        }
    }

    // in case the cmd isn't external
    else
    {
        base_command->execute();
    }

    // restores the correct stdout for smash
    cleanup();
}

void RedirectionNormalCommand::prepare()
{
    bool write_with_append = false;
    RedirectionCommand::prepare(write_with_append);
}

void RedirectionAppendCommand::prepare()
{
    bool write_with_append = true;
    RedirectionCommand::prepare(write_with_append);
}

void RedirectionCommand::prepare(bool write_with_append)
{
    // replacing stdout with dest
    int res_close = close(1);
    if (res_close < 0)
    {
        SystemCallFailed e("close");
        throw e;
    }
    int res_open;

    // permissions
    if (write_with_append)
        res_open = open(dest.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
    else
        res_open = open(dest.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);

    if (res_open < 0)
    {
        SystemCallFailed e("open");
        throw e;
    }
}

void RedirectionCommand::cleanup()
{
    int res = dup2(out_pd, 1);
    if (res < 0)
    {
        SystemCallFailed e("dup2");
        throw e;
    }

    // closing the new pd so the FDT won't get full
    res = close(out_pd);
    if (res < 0)
    {
        SystemCallFailed e("close");
        throw e;
    }
}

PipeCommand::PipeCommand(const char *cmd_line, string sign) : Command::Command(cmd_line)
{
    SmallShell &smash = SmallShell::getInstance();
    string cmd_str = string(cmd_line);
    int sign_index = cmd_str.find(sign);
    if (sign_index == 0 || sign_index == cmd_str.size() + 1)
    {
        InvaildArgument e(sign);
        throw e;
    }
    // calculating the commands for the pipe
    string first_cmd = cmd_str.substr(0, sign_index);
    string second_cmd = cmd_str.substr(sign_index + 1, cmd_str.size() - sign_index - 1);
    write_command = smash.CreateCommand(first_cmd.c_str()).get();
    read_command = smash.CreateCommand(second_cmd.c_str()).get();
    pipe(fd);

    // allocating new FD for the stdin(0) stdout(1)  and stderr(2)
    standard_in_pd = dup(0);
    standard_out_pd = dup(1);
    standard_error_pd = dup(2);
}

void PipeCommand::prepareWrite(int out_pid_num)
{
    dup2(fd[1], out_pid_num);
    close(fd[0]);
    close(fd[1]);
}

void PipeCommand::prepareRead()
{
    dup2(fd[0], 0);
    close(fd[0]);
    close(fd[1]);
}

void PipeNormalCommand::execute()
{
    PipeCommand::execute(1);
}

void PipeSterrCommand::execute()
{
    PipeCommand::execute(2);
}

void PipeCommand::execute(int pid_num)
{
    // fork the second process - for the read end of the pipe
    int pid1 = fork();
    if (!pid1)
    {
        setpgrp();
        prepareRead();
        read_command->execute();
    }

    prepareWrite(pid_num);

    // write end of the pipe
    if (write_command->isExternal())
    {
        int pid2 = fork();
        if (!pid2)
        {
            setpgrp();
            write_command->execute();
        }
        // wait for the "write-son" to finish writing
        waitpid(pid2);
    }
    else
    {
        write_command->execute();
    }

    // wait for the "read-son" to finish reading
    waitpid(pid1);

    // restoring the FDT for smash
    cleanUp();
}

void PipeCommand::cleanUp()
{
    int res1 = dup2(standard_in_pd, 0);
    int res2 = dup2(standard_out_pd, 1);
    int res3 = dup2(standard_error_pd, 2);
    if (res1 == -1 || res2 == -1 || res3 == -1)
    {
        SystemCallFailed e("dup2");
        throw e;
    }

    res1 = close(standard_out_pd);
    res2 = close(standard_in_pd);
    res3 = close(standard_error_pd);
    if (res1 == -1 || res2 == -1 || res3 == -1)
    {
        SystemCallFailed e("close");
        throw e;
    }
}

//<---------------------------C'tors and D'tors - end--------------------------->

//<---------------------------getters--------------------------->
const char *Command::getCmdL() const
{
    return cmd_l;
}

int Command::getJobId() const
{
    return job_id;
}

int Command::getProcessId() const
{
    return process_id;
}

bool JobsList::JobEntry::getStopped() const
{
    return is_stopped;
};

int JobsList::JobEntry::getJobId() const
{
    return command->getJobId();
};

shared_ptr<Command> JobsList::JobEntry::getCommand() const
{
    return command;
};

shared_ptr<Command> SmallShell::getCurrentCommand() const
{
    return current_command;
}

//<---------------------------getters - end--------------------------->

//<---------------------------setters--------------------------->
void Command::setJobId(int id)
{
    job_id = id;
}

void Command::setProcessId(int id)
{
    process_id = id;
}

void JobsList::JobEntry::setTime()
{
    init_time = time(NULL);
};

void JobsList::JobEntry::setStopped(bool is_stopped)
{
    this->is_stopped = is_stopped;
};

void SmallShell::setCurrentCommand(shared_ptr<Command> command)
{
    current_command = command;
}

//<---------------------------setters - end--------------------------->

//<---------------------------execute functions--------------------------->

void SmallShell::executeCommand(const char *cmd_line)
{
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("chprompt") == 0)
    {
        changeChprompt(cmd_line);
        return;
    }
    shared_ptr<Command> cmd = CreateCommand(cmd_line);
    if (!cmd->isExternal())
    {
        cmd->execute();
    }

    ///--------------------------Bonus start----------------------------------------
    /// -----------------------------else if (cmd == TimeOut)----------------
    ///             {
    ///                this->addTimeOutCommand(cmd);
    ///                cmd->execute();
    ///                this->addJob(cmd, false);
    ///            }
    ///---------------------------Bonus end-------------------------------------------

    else
    {
        int pid = fork();

        //--------------------- child -----------------------------//
        if (pid == 0)
        {
            if (setpgrp() == -1)
            {
                SystemCallFailed e("setpgrp");
                throw e;
            }
            cmd->execute();
        }

        //------------------------ father--------------------//
        else
        {
            cmd->setProcessId(pid);
            if (!(_isBackgroundCommand(cmd_line)))
            {
                current_command = cmd;
                waitpid(pid);
                current_command = (nullptr);
            }

            // add background Command to Joblist
            else
            {
                this->addJob(cmd, false);
            }
        }
    }
}

//    else if (isBuildInCommand(firstWord))
//    {
//        Command *cmd = CreateCommand(cmd_line);
//        cmd->execute();
//    }
//    else
//    {
//        Command *cmd = CreateCommand(cmd_line);
//        int pid = fork();
//        if (pid == 0)
//        {
//            setpgrp();
//            cmd->execute();
//        }
//        else
//        {
//            if (!(_isBackgroundCommand(cmd_line))) {
//                current_command = cmd;
//                waitpid(pid);
//                current_command = nullptr;
//            }
//        }

// TODO: Add your implementation here
// for example:
// Command* cmd = CreateCommand(cmd_line);
// cmd->execute();
// Please note that you must fork smash process for some commands (e.g., external commands....)

void ShowPidCommand::execute()
{
    int process_id = getpid();
    std::cout << "smash pid is " << process_id << std::endl;
}

void GetCurrDirCommand::execute()
{
    char cwd[256];

    // get the current working directory
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        std::cout << cwd << std::endl;
    }
    else
    {
        SystemCallFailed e("getcwd");
        throw e;
    }
}

void ChangeDirCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    std::string new_dir;

    //  Check amount of arguments
    if (args_vec.size() != 2)
    {
        TooManyArguments e("cd");
        throw e;
    }

    //  Check if going to last working directory or a new one
    if (args_vec[1] == "-")
    {
        //  if the last working directory doesn't exist
        if (smash.get_last_wd().empty())
        {
            OldPWDNotSet e;
            throw e;
        }
        else
        {
            new_dir = smash.get_last_wd();
        }
    }
    else
    {
        new_dir = args_vec[1];
    }

    //  Save current directory
    char buffer[256];
    if (getcwd(buffer, sizeof(buffer)) == NULL)
    {
        SystemCallFailed e("getcwd");
        throw e;
    }

    //  Change directory
    int res = chdir(new_dir.c_str());
    if (res == -1)
    {
        SystemCallFailed e("chdir");
        throw e;
    }
    else
    {
        //  Save last working directory
        smash.set_last_wd(string(buffer));
    }
}

void ExternalCommand::execute()
{
    // removing & sign
    if (_isBackgroundCommand(cmd_l))
    {
        removeBackgroundSignString(args_vec.back().c_str());
    }
    // parse path depending on Command type (Simple or Complex)
    // inserting "-c" for Complex Command
    if (_isSimpleExternal(cmd_l))
    {
        args_vec[0] = "/bin/" + args_vec[0];
    }
    else
    {
        args_vec.insert(args_vec.begin(), "/bin/bash");
        args_vec.insert(args_vec.begin() + 1, "-c");
    }

    // reforming args_vec
    char **args = new char *[args_vec.size() + 1];
    _reformatArgsVec(args, args_vec);

    // executing Command
    if (execv(args[0], args) == -1)
    {
        perror("smash error: execv failed");
        exit(1);
    }

    // ---------------- validity check is in the father responsibility: in SmallShell::executeCommand-----//

    //    if (execv(args[0], args) == -1)
    //    {
    //        smash.removeJob(this->job_id);
    //        smash.setCurrentCommand(nullptr);
    //        SystemCallFailed e("exec");
    //        throw e;
    //    }
}

void JobsCommand::execute()
{
    //  Remove finised jobs
    jobs->removeFinishedJobs();

    //  Print jobs list
    jobs->printJobsList();
}

void BackgroundCommand::execute()
{
    if (args_vec.size() != 2 || args_vec.size() != 1)
    {
        InvaildArgument e("bg");
        throw e;
    }

    // if a specific job was given
    if (args_vec.size() == 2)
    {
        int job_id_to_find;

        //  checking if valid argument(a number)
        if (isStringNumber(args_vec[1]))
        {
            job_id_to_find = stoi(args_vec[1]);
        }
        else
        {
            InvaildArgument e("bg");
            throw e;
        }

        //  getting the job from the list - if doesn't exist, a nullptr will return
        // Note: should we make sure jobs pointer was set currectly(isn't nullptr)?
        JobsList::JobEntry *job = this->jobs->getJobById(job_id_to_find);
        if (job != nullptr)
        {
            if (job->getStopped())
            {

                int pid = job->getCommand()->getProcessId();

                //  updating the command's status
                job->setStopped(false);

                //  printig the cmd_line of the command
                std::cout << job->getCommand()->getCmdL() << std::endl;

                // continue cammand without wating for it
                if (kill(pid, SIGCONT) == -1)
                {
                    SystemCallFailed e("kill");
                    throw e;
                }
            }
            else
            {
                JobAlreadyRunning e(job_id_to_find);
                throw e;
            }
        }
        else
        {
            JobIdDoesntExist e("bg", job_id_to_find);
            throw e;
        }
    }
    //  if no specific job was given
    else
    {
        JobsList::JobEntry *job = this->jobs->getLastStoppedJob(nullptr);
        if (job != nullptr)
        {
            int pid = job->getCommand()->getProcessId();

            //  updating the command's status
            job->setStopped(false);

            //  printig the cmd_line of the command
            std::cout << job->getCommand()->getCmdL() << std::endl;

            // continue cammand without wating for it
            if (kill(pid, SIGCONT) == -1)
            {
                SystemCallFailed e("kill");
                throw e;
            }
        }
        else
        {
            NoStoppedJobs e;
            throw e;
        }
    }
}

/*
The function brings the job required to the foreground
input:
    job_id - the id of the job we need to continue - if it's 0, bring the last job in the jobs list to the foreground
    jobs - pointer to the jobs list variable in the smash

*/
void bringCommandToForegound(int job_id, JobsList *jobs)
{
    //  get the job required - if the job_id doesn't exist, nullptr will be returned
    JobsList::JobEntry *job_to_cont = job_id == 0 ? jobs->getLastJob(nullptr) : jobs->getJobById(job_id);

    //
    if (job_to_cont != nullptr)
    {
        //  print the cmd_line of the command
        std::cout << job_to_cont->getCommand()->getCmdL() << std::endl;

        int pid = job_to_cont->getCommand()->getProcessId();

        //  send a continue signal to the process
        if (kill(pid, SIGCONT) == -1)
        {
            SystemCallFailed e("kill");
            throw e;
        }

        //  update the job's status
        job_to_cont->setStopped(false);

        //  save current command running in the foreground
        SmallShell &smash = SmallShell::getInstance();
        smash.setCurrentCommand(job_to_cont->getCommand());

        //  wait for process to finish
        int *status;
        waitpid(pid, status);

        //  delete current process from current command
        smash.setCurrentCommand(nullptr);
    }
    else
    {
        JobsListEmpty e;
        throw e;
    }
}

void ForegroundCommand::execute()
{
    //  no specific job required
    if (args_vec.size() == 1)
    {
        bringCommandToForegound(0, jobs);
    }

    //    specific job required
    else if (args_vec.size() == 2)
    {
        int job_id_to_find;

        // check if valid
        if (isStringNumber(args_vec[1]))
        {
            job_id_to_find = stoi(args_vec[1]);
        }
        else
        {
            InvaildArgument e("bg");
            throw e;
        }
        if (job_id_to_find <= 0)
        {
            InvaildArgument e("bg");
            throw e;
        }
        else
        {
            bringCommandToForegound(job_id_to_find, jobs);
        }
    }
    else
    {
        InvaildArgument e("bg");
        throw e;
    }
}

void QuitCommand::execute()
{
    bool flag_kill = false;
    std::vector<std::string> args = get_args_in_vec(this->cmd_l);

    //  check if 'kill' was given as an argument
    for (int i = 1; i < args.size(); i++)
    {
        if (args[i] == "kill")
        {
            flag_kill = true;
            break;
        }
    }

    //  kill all jobs in the jobs list
    if (flag_kill)
    {
        jobs->killAllJobs();
    }
    exit(0);
}

/* checks :
    -if the first char is '-'
    -if the string given is a number
    returns: the number if the string was given correctly. if Not, returns -1.
*/
int getSignalNumber(std::string str)
{
    if (str[0] != '-')
    {
        return -1;
    }

    //  remove the '-' from the string
    str.erase(0, 1);

    //  convert to integer
    int num;
    if (isStringNumber(str))
    {
        num = stoi(str);
        return num;
    }
    return -1;
}

void KillCommand::execute()
{
    //  check amount of arguments
    if (args_vec.size() != 3)
    {
        InvaildArgument e("kill");
        throw e;
    }

    //  get the number of the signal
    int signal_num = getSignalNumber(args_vec[1]); // return -1 if the format is wrong

    //  check if number is in range
    if (signal_num > 0 && signal_num < 32)
    {

        //  get the job - if does not exist, nullptr will be returned
        JobsList::JobEntry *job = jobs->getJobById(job_id);
        if (job == nullptr)
        {
            JobIdDoesntExist e("kill", job_id);
            throw e;
        }
        else
        {
            int pid = job->getCommand()->getProcessId();

            //  Note : to check if there are more signal numbers that fit
            //  kill signals
            if (signal_num == 9 || signal_num == 15 || signal_num == 6)
            {
                if (kill(pid, signal_num) == -1)
                {
                    SystemCallFailed e("kill");
                    throw e;
                }
                else
                {
                    //  remove the job from the jobs list for good
                    jobs->removeJobById(job->getJobId());
                }
            }

            //  stop signals
            else if (signal_num == 2 || signal_num == 19)
            {
                if (kill(pid, signal_num) == -1)
                {
                    SystemCallFailed e("kill");
                    throw e;
                }
                else
                {
                    //  update the jobs status
                    job->setStopped(true);
                }
            }

            // continute job signals
            else if (signal_num == 18)
            {
                if (kill(pid, signal_num) == -1)
                {
                    SystemCallFailed e("kill");
                    throw e;
                }
                else
                {
                    //  update the jobs status
                    job->setStopped(false);
                }
            }

            // ordinary signals
            else
            {
                if (kill(pid, signal_num) == -1)
                {
                    SystemCallFailed e("kill");
                    throw e;
                }
            }
        }
    }
    else
    {
        InvaildArgument e("kill");
        throw e;
    }
}

void SetcoreCommand::execute()
{
    //  checks amount of arguments
    if (args_vec.size() != 3)
    {
        InvaildArgument e("setcore");
        throw e;
    }

    //  check if the given core and the job id is indeed a number
    bool valid_arg1 = isStringNumber(args_vec[1]);
    bool valid_arg2 = isStringNumber(args_vec[2]);

    if (valid_arg1 && valid_arg2)
    {
        //  convert to integers
        int job_id = stoi(args_vec[1]);
        int core_number = stoi(args_vec[2]);

        //  get job required
        JobsList::JobEntry *job = jobs->getJobById(job_id);

        if (job == nullptr)
        {
            JobIdDoesntExist e("setcore", job_id);
            throw e;
        }
        else
        {

            //  checking if the command is 'sleep'
            string cmd_s = _trim(string(job->getCommand()->getCmdL()));
            string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
            if (firstWord.compare("sleep") == 0)
            {
                // Note : need to check if to do nothing or something else
                return;
            }

            int pid = job->getCommand()->getProcessId();

            //  get amount of cores in the cpu
            int cores_in_cpu = std::thread::hardware_concurrency();

            //  check if the core that was given is in range
            if (cores_in_cpu < core_number || core_number < 0)
            {
                InvaildCoreNumber e;
                throw e;
            }

            //  set the job's core
            //  Note : might be a problem with the bits?
            cpu_set_t set;
            CPU_SET(core_number, &set);
            if (sched_setaffinity(pid, sizeof(cpu_set_t), &set) == -1)
            {
                SystemCallFailed e("sched_setaffinity");
                throw e;
            }
        }
    }
    else
    {
        InvaildArgument e("setcore");
        throw e;
    }
}

void GetFileTypeCommand::execute()
{

    // check amount of arguments
    if (args_vec.size() != 2)
    {
        InvaildArgument e("gettype");
        throw e;
    }

    //  get info on path
    std::string path = args_vec[1];
    struct stat stats;
    if (stat(path.c_str(), &stats) == -1)
    {
        SystemCallFailed e("stat");
        throw e;
    }

    int file_size = stats.st_size;

    //  get file's type
    std::string file_type;
    switch (stats.st_mode & S_IFMT)
    {
    case S_IFBLK:
        file_type = "block device";
        break;
    case S_IFCHR:
        file_type = "character device";
        break;
    case S_IFDIR:
        file_type = "directory";
        break;
    case S_IFIFO:
        file_type = "FIFO";
        break;
    case S_IFLNK:
        file_type = "symlink";
        break;
    case S_IFREG:
        file_type = "regular file";
        break;
    case S_IFSOCK:
        file_type = "socket";
        break;
    default:
        //  Note: to check what to do if it is not one of those options
        file_type = "unknown";
        break;
    }

    //  print info
    std::cout << path << "'s type is \"" << file_type << "\" and takes up " << file_size << " bytes" << std::endl;
}

// assume chmod takes up to 4 args
bool isChmodArgsValid(const string args)
{
    int count = 0;
    for (const char c : args)
    {
        count++;
        int i = c - '0';
        if (count > 4 || i > 7 || i < 0)
            return false;
    }
    return true;
}

void ChmodCommand::execute()
{
    if (args_vec.size() != 3 || !isChmodArgsValid(args_vec[1]))
    {
        InvaildArgument e("chmod");
        throw e;
    }
    {
        int i;
        i = strtol(args_vec[1].c_str(), 0, 8);
        if (chmod(args_vec[2].c_str(), i) < 0)
        {
            SystemCallFailed e("chmod");
            throw e;
        }
    }
}
//<--------------------------- execute functions - end--------------------------->

//<--------------------------- Jobs List functions--------------------------->

void JobsList::JobEntry::printInfo() const
{
    //  calculate the time passed
    int current_time = time(NULL);
    int time_diff = difftime(current_time, init_time);

    // get status of job
    string stopped_str = is_stopped ? "(stopped)" : "";

    //  print info
    std::cout << "[" << command->getJobId() << "]" << command->getCmdL() << " : " << command->getProcessId() << " " << time_diff << " secs " << stopped_str << std::endl;
};

//  returns the max id that is currently in the jobs list
int JobsList::getMaxId() const
{
    int max_id = 0;
    for (int i = 0; i < jobs.size(); i++)
    {
        if (jobs[i]->getJobId() > max_id)
        {
            max_id = jobs[i]->getJobId();
        }
    }
    return max_id;
}

void JobsList::addJob(shared_ptr<Command> command, bool isStopped)
{
    //  remove finished job before checking max id
    this->removeFinishedJobs();

    if (command->getJobId() == -1)
    {
        //<----------- command was Not in the jobs list before ----------->

        // get job id for new command
        int job_id = getMaxId() + 1;

        //  update the command's job id
        command->setJobId(job_id);

        //  add job
        std::shared_ptr<JobEntry> new_job(new JobEntry(command, isStopped));
        jobs.push_back(new_job);
    }
    else
    {
        //<----------- command was in the jobs list before ----------->

        //  find the job in the jobs list
        for (int i = 0; i < jobs.size(); i++)
        {
            if (jobs[i]->getJobId() == command->getJobId())
            {
                //  reset time to current time
                jobs[i]->setTime();
                break;
            }
        }
    }
}

void JobsList::removeJobById(int jobId)
{
    //  find job and remove it from the jobs list's vector
    for (int i = 0; i < jobs.size(); i++)
    {
        if (jobs[i]->getJobId() == jobId)
        {
            jobs.erase(jobs.begin() + i);
            break;
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId)
{

    for (int i = 0; i < jobs.size(); i++)
    {
        if (jobs[i]->getJobId() == jobId)
        {
            return jobs[i].get();
        }
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId)
{
    if (jobs.size() == 0)
        return nullptr;
    return jobs.back().get();
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId)
{
    int i = jobs.size() - 1;

    //  goes from back to start in order to get the biggest one
    while (i >= 0)
    {
        if (jobs[i]->getStopped())
        {
            return jobs[i].get();
        }
        i--;
    }
    return nullptr;
}

void JobsList::printJobsList()
{
    for (int i = 0; i < jobs.size(); i++)
    {
        jobs[i]->printInfo();
    }
}

void JobsList::killAllJobs()
{
    // remove finished jobs in order to prevent a signal from sending
    this->removeFinishedJobs();

    // print info according to assignment
    std::cout << " sending SIGKILL signal to " << jobs.size() << " jobs:" << std::endl;
    for (int i = 0; i < jobs.size(); i++)
    {
        std::cout << jobs[i]->getCommand()->getProcessId() << ": " << jobs[i]->getCommand()->getCmdL() << std::endl;
    }

    //  send kill signals to all processes
    for (int i = jobs.size() - 1; i >= 0; i--)
    {
        int pid = jobs[i]->getCommand()->getProcessId();
        int job_id = jobs[i]->getJobId();

        //  send kill signal
        if (kill(pid, SIGKILL) == -1)
        {
            SystemCallFailed e("kill");
            throw e;
        }

        //  remove from jobs list
        this->removeJobById(job_id);
    }
}
void JobsList::removeFinishedJobs()
{
    std::vector<int> jobs_to_delete;
    for (int i = 0; jobs.size(); i++)
    {
        //  check if a process is finished
        if (waitpid(jobs[i]->getCommand()->getProcessId(), nullptr, WNOHANG))
        {
            jobs_to_delete.push_back(jobs[i]->getJobId());
        }

        //  updates the stopped field in each job while we're here
        if (waitpid(jobs[i]->getCommand()->getProcessId(), nullptr, WUNTRACED))
        {
            jobs[i]->setStopped(true);
        }
        else
        {
            jobs[i]->setStopped(false);
        }
    }

    //  remove the finished jobs
    for (int i = 0; i < jobs_to_delete.size(); i++)
    {
        this->removeJobById(jobs_to_delete[i]);
    }
}

//<--------------------------- Jobs List functions - end--------------------------->

//<--------------------------- Smash List functions--------------------------->

/**
 * Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 */

bool isAppendRedirect(string cmd_str)
{
    return cmd_str.find(">>") != string::npos;
}
bool isSterrPipe(string cmd_str)
{
    return cmd_str.find("|&") != string::npos;
}

bool isRedirect(string cmd_str)
{
    return cmd_str.find(">") != string::npos;
}
bool isPipe(string cmd_str)
{
    return cmd_str.find("|") != string::npos;
}

shared_ptr<Command> SmallShell::CreateCommand(const char *cmd_line)
{
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (isAppendRedirect(string(cmd_line)))
    {
        return shared_ptr<Command>(new RedirectionAppendCommand(cmd_line));
    }
    else if (isSterrPipe(string(cmd_line)))
    {
        return shared_ptr<Command>(new PipeSterrCommand(cmd_line));
    }
    if (isRedirect(string(cmd_line)))
    {
        return shared_ptr<Command>(new RedirectionNormalCommand(cmd_line));
    }
    else if (isPipe(string(cmd_line)))
    {
        return shared_ptr<Command>(new PipeNormalCommand(cmd_line));
    }
    else if (firstWord.compare("pwd") == 0)
    {
        return shared_ptr<Command>(new GetCurrDirCommand(cmd_line));
    }
    else if (firstWord.compare("showpid") == 0)
    {
        return shared_ptr<Command>(new ShowPidCommand(cmd_line));
    }
    else if (firstWord.compare("cd") == 0)
    {
        return shared_ptr<Command>(new ChangeDirCommand(cmd_line));
    }
    else if (firstWord.compare("bg") == 0)
    {
        return shared_ptr<Command>(new BackgroundCommand(cmd_line, this->jobs_list));
    }
    else if (firstWord.compare("fg") == 0)
    {
        return shared_ptr<Command>(new ForegroundCommand(cmd_line, this->jobs_list));
    }
    else if (firstWord.compare("kill") == 0)
    {
        return shared_ptr<Command>(new KillCommand(cmd_line, this->jobs_list));
    }
    else if (firstWord.compare("quit") == 0)
    {
        return shared_ptr<Command>(new QuitCommand(cmd_line, this->jobs_list));
    }
    else if (firstWord.compare("setcore") == 0)
    {
        return shared_ptr<Command>(new SetcoreCommand(cmd_line, this->jobs_list));
    }
    else if (firstWord.compare("getfileinfo") == 0)
    {
        return shared_ptr<Command>(new GetFileTypeCommand(cmd_line));
    }
    else if (firstWord.compare("chmod") == 0)
    {
        return shared_ptr<Command>(new ChmodCommand(cmd_line));
    }
    else
    {
        return shared_ptr<Command>(new ExternalCommand(cmd_line));
    }

    return nullptr;
}

void SmallShell::changeChprompt(const char *cmd_line)
{
    vector<string> args = get_args_in_vec(cmd_line);
    string new_prompt = args.size() == 1 ? "smash> " : (args[1] + "> ");
    prompt = new_prompt;
}

void SmallShell::printPrompt() const
{
    std::cout << prompt << " " << std::endl;
}

void SmallShell::addJob(shared_ptr<Command> cmd, bool is_stopped)
{
    jobs_list->addJob(cmd, is_stopped);
};

void SmallShell::removeJob(int job_id)
{
    jobs_list->removeJobById(job_id);
};

//<--------------------------- Smash functions - end--------------------------->

//<--------------------------- Aux functions--------------------------->

//  Note : is irrelevent?
bool isBuildInCommand(string firstWord)
{
    if (firstWord.compare("pwd") == 0 ||
        firstWord.compare("showpid") == 0 ||
        firstWord.compare("cd") == 0 ||
        firstWord.compare("jobs") == 0 ||
        firstWord.compare("bg") == 0 ||
        firstWord.compare("fg") == 0 ||
        firstWord.compare("quit") == 0) // להוסיף עוד
    {
        return true;
    }
    return false;
}

//<--------------------------- Aux functions - end--------------------------->

//// bonus

void SmallShell::addTimeOutCommand(std::shared_ptr<TimeoutCommand> cmd)
{
    timeOutList->addToList(cmd);
}

void SmallShell::handleAlarm()
{
    timeOutList->handleSignal();
}

TimeoutCommand::TimeoutCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line)
{
    SmallShell &smash = SmallShell::getInstance();
    string target_cmd_str = "";
    for (int i = 2; i < args_vec.size(); i++)
        target_cmd_str += args_vec[i];

    target_cmd = smash.CreateCommand(target_cmd_str.c_str());
    if (!isStringNumber(args_vec[1]))
    {
        InvaildArgument e("timeout");
        throw e;
    }
    int time_to_alarm = stoi(args_vec[1]);
    dest_time = time(nullptr) + time_to_alarm;
    m_pid = -1;
};

void TimeoutCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();

    // we assume all the commands are External, because a built-in command will end very quick...
    if (!target_cmd->isExternal())
        throw std::runtime_error("got Built-in Command in Timeout!!");

    int pid = fork();

    // ------------------------------child-------------------------//
    if (!pid)
    {
        target_cmd->execute();
    }

    //------------------------ father--------------------//
    else
    {
        // store the pid of the child in the list
        setProcessId(pid);

        if (!(_isBackgroundCommand(target_cmd->getCmdL())))
        {
            smash.setCurrentCommand(target_cmd.get());
            waitpid(pid);
            smash.setCurrentCommand(nullptr);
        }
    }
}

void TimeOutList::removeNext()
{
    time_out_list.pop_front();
    if (time_out_list.empty())
        return;
    next_cmd = time_out_list.front().get();
    makeAlarm();
}

void TimeOutList::makeAlarm()
{
    int cmd_time = next_cmd->getTime();
    time_to_next = cmd_time - time(nullptr);
    alarm(time_to_next);
}
void TimeOutList::handleSignal()
{
    int target_pid = next_cmd->getProcessId();

    //    // check if the command already stopped before killing it
    //    int res = waitpid(target_pid, nullptr, WNOHANG);

    if (target_pid != getpid())
    {
        //
        if (kill(target_pid, SIGKILL) == -1)
        {
            SystemCallFailed e("kill");
            throw e;
        }

        std::cout << "smash: " + string(next_cmd->getCmdL()) + " timed out!" << std::endl;
    }
    removeNext();
}
void TimeOutList::addToList(std::shared_ptr<TimeoutCommand> new_cmd)
{
    int new_cmd_time = new_cmd->getTime();
    if (new_cmd_time < time_to_next + time(nullptr))
    {
        next_cmd = new_cmd.get();
        makeAlarm();
    }
    for (auto it = time_out_list.begin(); it != time_out_list.end(); it++)
    {
        if ((*it)->getTime() > new_cmd_time)
        {
            time_out_list.insert(it, new_cmd);
            return;
        }
    }
    time_out_list.push_back(new_cmd);
}
