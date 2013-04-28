
#include "fuzzy.h"

Fuzzy_Distance Fuzzy_IRs[4], Fuzzy_Ping;

// convert a crisp input into a fuzzy distance
// input  : distance in centimeters
// input  : pointer to Fuzzy_Distance variable to write the result to
// outputs: none (writes fuzzy values to struct passed in via pointer)
void Fuzzify(unsigned long dist_cm, Fuzzy_Distance *fuzzy) {
  fuzzy->close = 64;
  fuzzy->ok = 64;
  fuzzy->far = 64;
}
