#include "manager.h"

int main(void) {

  // initialize
  MANAGER_initialize();

  for (;;) {
    MANAGER_cycle();
  }
}
