#include "manager.h"
#include "io.h"

int main(void) {

  // initialize
  MANAGER_initialize();
  BTNS_initialize(MANAGER_btn_event);

  for (;;) {
    BTNS_cycle();
    MANAGER_cycle();
  }
}
