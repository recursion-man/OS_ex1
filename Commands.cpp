#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

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

bool _isBackgroundComamnd(const char *cmd_line)
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

// TODO: Add your implementation for classes in Commands.h

void ShowPidCommand::execute() {}

SmallShell::SmallShell() : prompt("smash> ")
{
  // TODO: add your implementation
}

SmallShell::~SmallShell()
{
  // TODO: add your implementation
}

void SmallShell::changeChprompt(const char *cmd_line)
{
  vector<string> args = func(); // להוסיף את הפונקציה שמחזירה וקטור מהcmd_line
  string new_prompt = args.size() == 1 ? "smash> " : (args[1] + "> ");
  prompt = new_prompt;
}

SmallShell::printPrompt()
{
  std::cout << prompt;
}

/**
 * Creates and returns a pointer to Command class which matches the given command line (cmd_line)
 */
Command *SmallShell::CreateCommand(const char *cmd_line)
{
  // For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0)
  {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0)
  {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line, &this->last_wd);
  }
  ... else
  {
    return new ExternalCommand(cmd_line);
  }

  return nullptr;
}

bool isBuildInCommand(string firstWord)
{
  if (firstWord.compare("pwd") == 0 ||
      firstWord.compare("showpid") == 0 ||
      firstWord.compare("cd") == 0 ||
      firstWord.compare("jobs") == 0)
  {
    return true;
  }
  return false;
}

void SmallShell::executeCommand(const char *cmd_line)
{
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  if (firstWord.compare("chprompt") == 0)
  {
    changeChprompt(cmd_line);
  }
  else if (isBuildInCommand(firstWord))
  {
    Command *cmd = CreateCommand(cmd_line);
    cmd->execute();
  }
  else
  {
    int pid = fork();
    if (pid == 0)
    {
      Command *cmd = CreateCommand(cmd_line);
      cmd->execute();
    }
    else
    {
      current_process_id = pid;
      // חרא של האבא
    }
  }
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

void ShowPidCommand::execute()
{
  int process_id = getpid();
  std::cout << "smash pid is " << process_id << std::endl;
  is_finished = true;
}

void GetCurrDirCommand::execute()
{
  char cwd[256];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
  {
    std::cout << cwd << std::endl;
  }
  else
  {
    // perror("smash error: getcwd failed");
    // לזרוק חריגה שקראת מערכת נכשלה
  }
  is_finished = true;
}

void ChangeDirCommand::execute()
{
  char *new_dir;
  if (args.size() != 2)
  {
    // לזרוק שגיאה שאין ארגונטים
  }
  if (args[1] == "-")
  {
    if (p_last_wd == nullptr)
    {
      // לזרוק שגיאה שתיקיה אחרונה חוקית עוד לא הייתה
    }
    else
    {
      new_dir = *p_last_wd;
    }
  }
  else
  {
    new_dir = args[1];
  }

  char buffer[256];
  if (getcwd(buffer, sizeof(buffer)) == NULL)
  {
    // perror("smash error: getcwd failed");
    // לזרוק חריגה שקריאת מערכת נכשלה
  }

  int res = chdir(new_dir);
  // change failed
  if (res == -1)
  {
    // perror("smash error: chdir failed");
    // לזרוק חריגה שקריאת מערכת נכשלה
  }
  // change succeeded
  else
  {
    strcpy(*p_last_wd, buffer);
  }
  is_finished = true;
}

void Jobs::JobEntry::printInfo() const
{
  int current_time = time(NULL);
  int time_diff = difftime(current_time, init_time);
  string stopped_str = is_stopped ? "(stopped)" : "";
  std::cout << "[" << job_id << "]" << command->getCmdL() << " : " << process_id << " " << time_diff << " secs " << stopped_str << std::endl;
}

void JobEntry::setTime()
{
  init_time = time(NULL);
}

bool JobEntry::isJobFinished() const
{
  return command->getStatus();
}