#include "base/check.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"

// learn_logging --v=2
// learn_logging --v=2 --log-fatal
int main(int argc, const char* argv[]) {
  base::CommandLine::Init(argc, argv);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  logging::LoggingSettings settings;
  CHECK(logging::InitLogging(settings));

  // Read and set verbose level
  std::string verbose_switch = command_line.GetSwitchValueASCII("v");
  if (!verbose_switch.empty()) {
    int verbose_level = logging::GetVlogVerbosity();
    if (base::StringToInt(verbose_switch, &verbose_level)) {
      logging::SetMinLogLevel(-verbose_level);
    }
  }

  // check verbose level
  if (VLOG_IS_ON(1)) {
    LOG(INFO) << "VLOG_IS_ON(1) on";
  } else {
    LOG(INFO) << "VLOG_IS_ON(1) off";
  }
  if (VLOG_IS_ON(2)) {
    LOG(INFO) << "VLOG_IS_ON(2) on";
  } else {
    LOG(INFO) << "VLOG_IS_ON(2) off";
  }

  // Verbose log messages, disabled by default.
  VLOG(1) << "This is a log message with verbosity == 1";
  VLOG(2) << "This is a log message with verbosity == 2";

  // Verbose messages, can be enabled only in debug build.
  DVLOG(1) << "This is a DEBUG log message with verbosity == 1";
  DVLOG(2) << "This is a DEBUG log message with verbosity == 2";

  // Log messages visible by default.
  LOG(INFO) << "This is INFO log message.";
  LOG(WARNING) << "This is WARNING log message.";
  DLOG(INFO) << "This is DEBUG INFO log message.";
  DLOG(WARNING) << "This is DEBUG WARNING log message.";

  // FATAL log message will terminate our app.
  if (command_line.HasSwitch("log-fatal")) {
    LOG(FATAL) << "Program will terminate now!";
  }

  return 0;
}
