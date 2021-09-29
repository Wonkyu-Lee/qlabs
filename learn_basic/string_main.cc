#include <iostream>
#include "base/strings/utf_string_conversions.h"

int main(int argc, const char* argv[]) {
  std::cout << base::WideToUTF8(L"This is a wide string.") << std::endl;
  std::wcout << base::UTF8ToWide(u8"This is an UTF8 string.") << std::endl;
  std::cout << base::UTF16ToUTF8(u"This is an UTF16 string.") << std::endl;
  return 0;
}
