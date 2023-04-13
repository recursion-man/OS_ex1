#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <ctime>
#include <string>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command
{
  // TODO: Add your data members
protected:
  int job_id;
  int process_id;
  const char *cmd_l;
  bool is_finished = true;
  std::vector<std::string> args_vec;

public:
  Command(const char *cmd_line); // : job_id(-1), process_id(getpid()), cmd_l(cmd_line), is_finished(false){};
  virtual ~Command();
  virtual void execute() = 0;
  virtual void preparePd(Command *);
  virtual void cleanup();

  // getters
  const char *getCmdL() const;
  bool getStatus() const;
  int getJobId() const;
  int getProcessId() const;

  // setters
  void setJobId(int id);
  void setProcessId(int id);

  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command
{
public:
  BuiltInCommand(const char *cmd_line) : Command::Command(cmd_line){};
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command
{
public:
  ExternalCommand(const char *cmd_line) : Command::Command(cmd_line){};
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command
{
  Command *write_command;
  Command *read_command;
  int standard_in_pd;
  int standard_out_pd;
  int fd[2];

public:
  PipeCommand(const char *cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
  void prepare() override;
  void prepareWrite();
  void prepareRead();
  void ReadCleanUp();
  void writeCleanUp();
};

class RedirectionCommand : public Command
{
  Command *base_command;
  std::string dest;
  int out_pd;

public:
  explicit RedirectionCommand(const char *cmd_line); // Command::Command(cmd_line)
  virtual ~RedirectionCommand() {}
  void execute() override;
  void prepare() override;
  void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand
{
private:
  char **plastPwd;

public:
  // TODO: Add your data members public:
  ChangeDirCommand(const char *cmd_line);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand
{
public:
  GetCurrDirCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line){};
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand
{
public:
  ShowPidCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line){};
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand
{
  JobsList *jobs;
  // TODO: Add your data members
public:
  QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~QuitCommand() {}
  void execute() override;
};

class JobsList
{
public:
  class JobEntry
  {
  private:
    Command *command;
    int init_time;
    bool is_stopped;

  public:
    JobEntry(Command *command, bool is_stopped) : command(command), init_time(time(NULL)), is_stopped(is_stopped){};
    ~JobEntry() = default;

    //  getters
    bool getStopped() const;
    bool isJobFinished() const;
    Command *getCommand() const;
    int getJobId() const;

    //  setters
    void setTime();
    void setStopped(bool is_stopped);

    //  aux
    void printInfo() const;

    // TODO: Add your data members
  };
  // TODO: Add your data members
  std::vector<std::shared_ptr<JobEntry>> jobs; // אולי נהפוך לרשימה מקושרת std::list?

public:
  JobsList();
  ~JobsList();

  //  getters
  JobEntry *getJobById(int jobId);
  JobEntry *getLastJob(int *lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  int getMaxId() const;

  //  aux
  void addJob(Command *cmd, bool isStopped = false);
  void removeJobById(int jobId);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();

  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand
{
  JobsList *jobs;
  // TODO: Add your data members
public:
  JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line){};
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand
{
  JobsList *jobs;
  // TODO: Add your data members
public:
  BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line), jobs(jobs){};
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand
{
  /* Bonus */
  // TODO: Add your data members
public:
  explicit TimeoutCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line){};
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  ChmodCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line){};
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  GetFileTypeCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line){};
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  SetcoreCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line){};
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line){};
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell
{
private:
  // TODO: Add your data members
  std::string prompt;
  std::string last_wd;
  Command *current_process;
  JobsList *jobs_list;

  SmallShell();

public:
  Command *CreateCommand(const char *cmd_line);
  SmallShell(SmallShell const &) = delete;     // disable copy ctor
  void operator=(SmallShell const &) = delete; // disable = operator
  static SmallShell &getInstance()             // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();

  //  aux
  void executeCommand(const char *cmd_line);
  // TODO: add extra methods as needed
  std::string get_last_wd() const;
  void set_last_wd(std::string);
  void printPrompt() const;
  void changeChprompt(const char *cmd_line);
  void addJob(Command *cmd, bool is_stopped = false);
  void removeJob(int job_id);
  void printPrompt() const;
  void setCurrentProcess(Command *cmd);
};

#endif // SMASH_COMMAND_H_
