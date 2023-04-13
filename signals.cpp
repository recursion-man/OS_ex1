#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num)
{
  // TODO: Add your implementation
  std::cout << "smash: got ctrl-C" << std::endl;
  SmallShell &smash = SmallShell::getInstance();
  Command *current_command = smash.getCurrentCommand();
  if (current_command != nullptr)
  {
    int pid = current_command->getProcessId();
    if (kill(pid, SIGKILL) == -1)
    {
      // לזרוק שגיאה שkill לא עבדה
      // perror("...kill...");
    }
    else
    {
      if (current_command->getJobId() != -1)
      {
        smash.removeJob(current_command->getJobId());
      }
      smash.setCurrentCommand(nullptr);
      std::cout << "smash: process " << pid << " was killed" << std::endl;
    }
  }
}

void ctrlZHandler(int sig_num)
{
  std::cout << "smash: got ctrl-Z" << std::endl;
  SmallShell &smash = SmallShell::getInstance();
  Command *current_command = smash.getCurrentCommand();
  if (current_command != nullptr)
  {
    int pid = current_command->getProcessId();
    if (kill(pid, SIGSTOP) == -1)
    {
      // לזרוק שגיאה שkill לא עבדה
      // perror("...kill...");
    }
    else
    {
      smash.addJob(current_command, true);
      smash.setCurrentCommand(nullptr);
      std::cout << "smash: process " << pid << " was stopped" << std::endl;
    }
  }
}

void alarmHandler(int sig_num)
{
  // TODO: Add your implementation
}
