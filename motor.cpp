#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/time.h"
#include "hardware/sync.h"
#include "cyw42_arch.h"

/* Developer's Note:
 * If this code is being run on a Pico 2 W,
 * keep in mind that the onboard LED can be used only by initialising
 * the WiFi board. In the interest of saving power (however little it
 * it might be) that you use an external LED if you do not otherwise
 * plan to use the WiFi chip.
*/

/* Define pins */


/* End pin definitions */

/* Function Definitions */

int callibration() {
  
  return 0;
}

int servo(float angle) {

  return 0;
}

int housekeeper() {

  return 0;
}

static void core1_entry(void) {
  tight_loop_contents();
}

int main(void) {

}
