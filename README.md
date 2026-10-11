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
- **API:** `split` `chr` `ord` `deli` `pack` `unpack` `inext` `stop` `oval` `ovalfill`, `add` with an index, `count`
  with a value, and arguments passed through `yield` and `coresume`.
- **Fixes:** `print(str, col)`, `srand()` without a seed, `reload` reading the correct arguments, map and memory calls
  that stay inside their bounds, and string to number conversion that no longer crashes on words containing a `b`.

## Building

```sh
make platform=unix
```

Cross-compile by passing the toolchain, for example `make CC=aarch64-linux-gnu-gcc CXX=aarch64-linux-gnu-g++`.

## Checking

`tools/check.sh` builds a host copy of the core around `tools/harness.cpp` and runs `tests/ops.p8`, which covers the
syntax and behaviour listed above. Pass a folder of carts to also report which of them load and run 30 frames.

```sh
tools/check.sh
tools/check.sh ~/carts/pico8
```

`data/api.lua` holds the Lua side of the API. Keep `src/gen/lua_api.h` in step with it, as that header is what the
core compiles in.

