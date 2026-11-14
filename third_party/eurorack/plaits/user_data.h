// Örgü: desktop replacement for the original Plaits user_data.h.
// The hardware reads user wavetables / patches from STM32 flash; the plugin
// has no flash, so every slot reports "no user data" and the engines fall
// back to their built-in tables (same behaviour as the firmware's TEST build).

#ifndef PLAITS_USER_DATA_H_
#define PLAITS_USER_DATA_H_

#include "stmlib/stmlib.h"

namespace plaits {

class UserData {
 public:
  enum {
    ADDRESS = 0,
    SIZE = 0x1000
  };

  UserData() { }
  ~UserData() { }

  inline const uint8_t* ptr(int slot) const {
    (void)slot;
    return NULL;
  }
};

}  // namespace plaits

#endif  // PLAITS_USER_DATA_H_
