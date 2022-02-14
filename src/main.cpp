#include <dirent.h>
#include "sol.h"

int main() {
  sol::state state;
  state.open_libraries();

  // Could use dirent here to find lua files from a directory tree to execute

  state.script("print('hello from lua!')");

  return 0;
}
