#include <iostream>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"

int main(int argc, const char* argv[]) {
  base::FilePath current_dir;
  CHECK(base::GetCurrentDirectory(&current_dir));
  std::cout << "Enumerating files and directories in path: "
            << current_dir.AsUTF8Unsafe() << std::endl;

  base::FileEnumerator file_enumerator(
      current_dir, false,
      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);

  for (base::FilePath name = file_enumerator.Next(); !name.empty();
       name = file_enumerator.Next()) {
    std::cout << (file_enumerator.GetInfo().IsDirectory() ? "[dir ] "
                                                          : "[file] ")
              << name.AsUTF8Unsafe() << std::endl;
  }

  return 0;
}
