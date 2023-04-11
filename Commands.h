#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <ctime>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command
{
  // TODO: Add your data members
protected:
  int job_id;
  const char *cmd_l;
  bool is_finished = true;

public:
  Command(const char *cmd_line) : job_id(-1), cmd_l(cmd_line), is_finished(false){};
  virtual ~Command();
  virtual void execute() = 0;
  // virtual void prepare();
  // virtual void cleanup();
  const char *getCmdL() const
  {
    return cmd_l;
  }
  bool getStatus() const
  {
    return is_finished;
  }
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
  ExternalCommand(const char *cmd_line) : Command::Command(cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override
  {
    SmallShell &smash = SmallShell::getInstance();

    JobEntry job = smash.addJob(this);
    // לעשות בדיקה אם יש * או ? בcmd

    if (isSimpleExternal(cmd))
    {
      char **args;
      _parseCommandLine(cmd, args);
      if (exec(args[0], args) == -1)
      {
        smash.removeJob(job->getJobId());
        // אז למחוק את המשימה מהרשימה
        // smash.removeJob();
        // ולזרוק שגיאה שפעולת מערכת לא הצליחה
      }
    }
    else
    {
      char *args = {"-c", cmd_line};
      if (exec("/bin/bash", args) == -1)
      {
        smash.removeJob(job->getJobId());
        // אז למחוק את המשימה מהרשימה
        // smash.removeJob();
        // ולזרוק שגיאה שפעולת מערכת לא הצליחה
      }
    }
  }
};

class PipeCommand : public Command
{
  // TODO: Add your data members
public:
  PipeCommand(const char *cmd_line) : Command::Command(cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command
{
  // TODO: Add your data members
public:
  explicit RedirectionCommand(const char *cmd_line) : Command::Command(cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  // void prepare() override;
  // void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand
{
  // TODO: Add your data members public:
  vector<string> args = func(); // להוסיף את הפונקציה שמפרידה את המילים בשורת קוד
  char **last_wd;
  ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand::BuiltInCommand(cmd_line), args(func()),
                                                            last_wd(plastPwd)
  {
    if (args.size() != 2)
    {
      // לזרוק שגיאה על כמות לא נכונה של ארגומנטים או שלא נוספו ארגומנטים
      // throw error that "smash error: cd: too many arguments"
    }
    else if (*plastPwd == "")
    {
      // לזרוק חריגה שתיקיה אחרונה חוקית לא קיימת עדיין
      // throw error that "smash error: cd: OLDPWD not set"
    }
    else
    {
    }
  };
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand
{
public:
  GetCurrDirCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand
{
public:
  ShowPidCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line);
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
    int process_id;
    int job_id;
    int init_time;
    bool is_stopped;

  public:
    JobEntry(Command *command, int process_id, int job_id, bool is_stopped) : command(command), process_id(process_id), job_id(job_id), init_time(time(NULL)), is_stopped(is_stopped){};
    ~JobEntry() = default;
    void printInfo() const;
    void setTime();
    void getJobStatus() const;
    // TODO: Add your data members
  };
  // TODO: Add your data members
  std::queue<JobEntry> jobs;
  JobEntry current_job;

public:
  JobsList();
  ~JobsList();
  void addJob(Command *cmd, bool isStopped = false)
  {
  }
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry *getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry *getLastJob(int *lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line);

  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand
{
  /* Bonus */
  // TODO: Add your data members
public:
  explicit TimeoutCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  ChmodCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  GetFileTypeCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  SetcoreCommand(const char *cmd_line) : BuiltInCommand::BuiltInCommand(cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand::BuiltInCommand(cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell
{
private:
  // TODO: Add your data members
  std::string prompt;
  char *last_wd;
  int current_process_id;
  JobsList *jobs_list;

  SmallShell() : prompt("smash"), last_wd(new char[256]), current_process_id(-1), jobs_list()
  {
    last_wd = nullptr;
  };

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
  ~SmallShell()
  {
    delete last_wd;
  };
  void executeCommand(const char *cmd_line);
  // TODO: add extra methods as needed
  void printPrompt() const;
  void changeChprompt(const char *cmd_line);

  void addJob(Command *cmd, bool is_stopped = false)
  {
    jobs_list = > addJob(cmd, is_stopped);
  };
  void removeJob(int job_id)
  {
    jobs_list->removeJobById(job_id);
  };
};

#endif // SMASH_COMMAND_H_
