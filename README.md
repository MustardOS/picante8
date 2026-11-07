# Picante8

Picante8 is the MustardOS PICO-8 Libretro core for Pickles specifically.

Picante8 is a fork of [Retro8](https://github.com/libretro/retro8) by Jack (jakz), and keeps its GPL-3.0 licence. The
original copyright notices remain in the source files.

## What changed from Retro8

Retro8 stopped short of the newer PICO-8 syntax, so about half of a typical cart collection failed to load. Picante8
adds:

- **Syntax:** `\` integer division, `//` comments, the `@` `%` `$` peek operators, `^^` xor, `>>>` `<<>` `>><`, every
  compound assignment (`\=` `..=` `^=` `&=` `|=` `^^=` `<<=` `>>=` `>>>=`), `local x += y`, binary literals, numbers
  that run straight into a keyword (`10do`, `0then`), glyph variables, and the `?` print shorthand.
- **Short statements:** `while (cond) stat` on one line, `if (cond) ... else ...` without running both branches, and
  `do ... end` blocks that span several lines inside a short `if`.
- **Numbers:** bitwise operators and `band` `bor` `bxor` `bnot` `shl` `shr` `lshr` `rotl` `rotr` work on 16.16 fixed
  point like PICO-8, and integer division or modulo by zero no longer stops the cart.
- **Strings:** the P8SCII escapes `\*` `\#` `\-` `\|` `\+` `\^`, a working `sub`, and `tostr` and `tonum` with their
  PICO-8 formats and flags.
- **Custom fonts:** a font poked into `0x5600` is used after the P8SCII `\14` code or by default once `0x5f58` has
  `0x81` set, with its own width, height, offsets and per character adjustments, until `\15` switches back.
- **API:** `split` `chr` `ord` `deli` `pack` `unpack` `inext` `stop` `oval` `ovalfill`, `add` with an index, `count`
  with a value, and arguments passed through `yield` and `coresume`.
- **Fixes:** `print(str, col)`, `srand()` without a seed, `reload` reading the correct arguments, map and memory calls
  that stay inside their bounds, and string to number conversion that no longer crashes on words containing a `b`.
- **Running:** carts that run their own loop with `flip()`, or wait with the P8SCII `\^1` to `\^9` codes, hand each
  frame back and carry on from the same place. `time()` moves on by one frame every frame, `stat()` reports the frame
  rate, the CPU load, sound channels, music and the date, and `cartdata` keeps saves in PICO-8's `.p8d.txt` layout.
- **Numbers:** arithmetic is 16.16 fixed point like PICO-8, so results wrap at 32768, multiplication and division round
  the same way, and numbers print to the same four decimal places. Literals such as `0xffff` and `0x8000` wrap to the
  signed PICO-8 range, and division by zero gives the largest number of the matching sign.
- **More API:** `tline`, `reset`, `serial`, `rnd(table)`, `pal(table)`, previous values from `camera`, `pal`, `color`,
  `cursor` and `clip`, `print` returning its right and bottom edges with wide and tall text, string indexing with
  `s[i]`, glyph variables for fill patterns, and the API staying reachable from a cart that replaces `_ENV`.
- **Loading:** PNG carts with plain text code load, and a damaged or uncompilable cart fails cleanly instead of
  stopping the frontend.
- **Speed:** filled circles draw one span per row, and whole number table keys skip the float conversion.
- **Sound:** a new sound engine with all eight waveforms and effects, custom instruments, looping sounds, music fades
  and channel masks, the reverb, dampen and distortion filters, and output limited to PICO-8's bandwidth. Its
  behaviour was checked against recordings of PICO-8 itself. The buzz, detune, reverb and dampen settings of each
  sound follow zepto8's measurements for every waveform.
- **Multicart:** `load()` switches to another cart with its parameter and breadcrumb, `reload()` reads another cart's
  data, and text carts can `#include` files from their folder.
- **Graphics and memory:** memory remapping, screen modes, the secret palette, inverted fills, `tline` precision,
  `rrect` and `rrectfill`, and the out of range defaults for `sget`, `mget` and `pget`.
- **Save states:** the cart's Lua state, memory and sound are saved and restored, including carts paused inside a
  `flip()` loop or a waiting `print`.
- **Mouse:** carts that turn on the mouse with `poke(0x5f2d, 1)` read it through `stat(32)` to `stat(39)`. A stick
  moves the cursor, and holding Y lets the D-Pad move it on devices without sticks. L1 is the left click, R1 the right
  click, and L2 the middle click. A Libretro mouse or touch screen works as well. None of this changes the controls
  of carts that never turn the mouse on.
- **Keyboard:** the same carts read the devkit keyboard through `stat(28)`, `stat(30)` and `stat(31)`. The D-Pad holds
  the arrow keys, WASD, ESDF or IJKL, O and X hold Z and X, and the X button holds Space or another chosen key. A
  Libretro keyboard works as well.

## Core Options

| Option               | Choices                                              | Default  |
|:---------------------|:-----------------------------------------------------|:---------|
| Mouse Control        | Auto, Right Stick, Left Stick, D-Pad Hold, Disabled  | Auto     |
| Mouse Speed          | Slow, Normal, Fast, Very Fast                        | Normal   |
| Mouse Stick Deadzone | 10% to 30%                                           | 20%      |
| Mouse Hold Button    | Y, Select                                            | Y        |
| Mouse Cursor         | Hidden, Shown                                        | Hidden   |
| Mouse Function       | Free Movement, Return to Centre                      | Free     |
| Keyboard D-Pad       | Arrow Keys, WASD, ESDF, IJKL, Disabled               | Arrows   |
| Keyboard X Button    | Space, Enter, Shift, Tab, Escape, Disabled           | Space    |

Auto uses whichever stick moves, with the right stick first, and also allows the hold button. Mouse Cursor draws an
arrow for carts that do not draw their own. Return to Centre places the cursor out from the middle as far as the stick
is pushed and glides it back when the stick is let go.

## Building

```sh
make platform=unix
```

Cross-compile by passing the toolchain, for example `make CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++`.

## Checking

`tools/check.sh` builds a host copy of the core around `tools/harness.cpp` and runs `tests/ops.p8` and `tests/api.p8`,
which cover the syntax and behaviour listed above. Pass a folder of carts to also report which of them load and run 30
frames.

The harness also takes these environment variables:

- `P8_TIME=1` prints the average milliseconds per frame.
- `P8_PROFILE=1` samples the cart's Lua lines and prints the busiest ones.
- `P8_TRACE_AFTER=N` prints a Lua traceback after N seconds, for carts that hang.
- `P8_SAVE_DIR=path` keeps `cartdata` saves in that folder.
- `P8_DUMP=dir` writes a frame image every `P8_DUMP_EVERY` frames.
- `P8_PRESS=frame:button:length,...` presses buttons at the given frames.
- `P8_MOUSE=frame:x:y:buttons,...` places the mouse and sets its buttons from the given frames.
- `P8_KEYS=frame:scancode:frames,...` holds a devkit keyboard key by SDL scancode for the given number of frames.
- `P8_WAV=file` writes the sound to a WAV file.
- `P8_STATE=S:L` saves a state after frame S and restores it after frame L, with `P8_STATE_OUT` and `P8_STATE_IN`
  to write or read the state as a file.

```sh
tools/check.sh
tools/check.sh ~/carts/pico8
```

`data/api.lua` holds the Lua side of the API. Keep `src/gen/lua_api.h` in step with it, as that header is what the
core compiles in.

## Credits

- [Retro8](https://github.com/libretro/retro8) by Jack (jakz), which Picante8 is a fork of.
- [Eris](https://github.com/fnuecke/eris) by Florian Nuecke, MIT licence, which saves and restores the Lua state for
  save states. Its notice is kept in `src/lua/eris.c`.
- [zepto8](https://github.com/samhocevar/zepto8) by Sam Hocevar, whose measurements of PICO-8's waveforms and noise the
  sound engine follows.
