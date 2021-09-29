#include <iostream>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"

// learn_command_line --bool-switch --string-switch=SOME_VALUE

int main(int argc, const char* argv[]) {
  CHECK(base::CommandLine::Init(argc, argv))
      << "Failed to parse a command line argument.";

  using base::CommandLine;
  DCHECK(CommandLine::ForCurrentProcess())
      << "Command line for process wasn't initialized.";

  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  std::cout << "Application program name is "
            << command_line.GetProgram().AsUTF8Unsafe() << std::endl;

  if (command_line.HasSwitch("bool-switch")) {
    std::cout << "Detected a boolean switch!" << std::endl;
  }

  std::string string_switch = command_line.GetSwitchValueASCII("string-switch");
  if (!string_switch.empty()) {
    std::cout << "Got a string switch value: " << string_switch << std::endl;
  }

  return 0;
}
