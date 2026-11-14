# Third-party DSP code

`plaits/` and `stmlib/` are taken unchanged from Emilie Gillet's open-source
Eurorack firmware (MIT license, see `LICENSE-plaits.txt` and `stmlib/LICENSE`):

- https://github.com/pichenettes/eurorack — commit f4cfa7697095a28d292bff758beb51deb49b5a86 (`plaits/dsp`, `plaits/resources.*`)
- https://github.com/pichenettes/stmlib — commit d18def816c51d1da0c108236928b2bbd25c17481 (`dsp/`, `utils/`, `stmlib.h`)

Only one file differs from upstream: `plaits/user_data.h` is replaced by a
desktop stub, because the hardware reads user data from STM32 flash memory.

This project is not affiliated with or endorsed by Mutable Instruments.
