pico-8 cartridge // http://www.pico-8.com
version 29
__lua__
-- results are checked with assert so a failure stops with its message
local fails = 0
function check(name, got, want)
 if got ~= want then
  fails += 1
  printh("FAIL "..name.." got "..tostr(got).." want "..tostr(want))
 end
end
check("idiv", 7\2, 3)
check("idiv neg", -7\2, -4)
local a = 9 a \= 2 check("idiv assign", a, 4)
// a comment with // inside
check("comment", 1, 1) // trailing comment
check("shr frac", 3>>1, 1.5)
check("shr neg", -1>>1, -0.5)
check("shl", 1<<4, 16)
check("lshr", -1>>>16, 0x0000.ffff)
check("lshr 2", 0x8000>>>15, 1)
check("rotl", 0x8000<<>1, 0x0000.0001)
check("rotr", 1>><1, 0.5)
check("band frac", 1.5 & 1.25, 1)
check("bor", 0x0f | 0xf0, 0xff)
check("xor", 5^^3, 6)
local x = 5 x ^^= 1 check("xor assign", x, 4)
check("bnot", ~0, -0x0000.0001)
local s = "a" s ..= "b" check("concat assign", s, "ab")
local p = 2 p ^= 3 check("pow assign", p, 8)
local m = 6 m &= 3 check("band assign", m, 2)
local o = 4 o |= 1 check("bor assign", o, 5)
local l = 1 l <<= 3 check("shl assign", l, 8)
local r = 8 r >>= 2 check("shr assign", r, 2)
local u = 0x8000 u >>>= 15 check("lshr assign", u, 1)
check("binary", 0b101, 5)
check("binary frac", 0b1.1, 1.5)
check("hex frac", 0x1.8, 1.5)
for i=1,2do check("numeral then keyword", i < 3, true) end
if (1==1) check("short if", 1, 1)
local t = 0 if (t==0) t=1 else t=2
check("short if else", t, 1)
local w = 0 while (w < 5) w += 1
check("short while", w, 5)
local q = 0
if (q == 0) do
 q = 7
end
check("short if with block", q, 7)
local nn = 3
do local nn += 2 check("local compound", nn, 5) end
check("ne", 1 != 2, true)
check("split", #split("a,b,c"), 3)
check("split num", split("1,2,3")[2], 2)
check("split size", split("abcdef", 2)[3], "ef")
check("chr ord", ord(chr(65)), 65)
local d = {1,2,3} check("deli", deli(d, 1), 1) check("deli len", #d, 2)
local c = {1,2,2} check("count v", count(c, 2), 2)
local e = {1,3} add(e, 2, 2) check("add index", e[2], 2)
check("glyph var", ⬅️, 0)
check("glyph var 2", ❎, 5)
check("div zero", 1\0, 32767)
check("mod zero", 5%0, 0)
check("tonum word", tonum("obtained"), nil)
check("tonum num", tonum("12.5"), 12.5)
check("tonum hex", tonum("0x10"), 16)
check("tonum flag hex", tonum("ff", 1), 255)
check("tonum flag zero", tonum("x", 4), 0)
check("tostr int", tostr(3), "3")
check("tostr frac", tostr(0.5), "0.5")
check("tostr third", tostr(1/3), "0.3333")
check("tostr nil", tostr(nil), "[nil]")
check("tostr hex", tostr(1.5, true), "0x0001.8000")
check("tostr neg", tostr(-2), "-2")
check("sub rest", sub("hello", 2), "ello")
check("sub range", sub("hello", 2, 3), "el")
check("sub neg", sub("hello", -3), "llo")
check("sub neg end", sub("hello", 1, -2), "hell")
check("sub past", sub("hello", 9), "")
check("sub number", sub(12345, 2, 3), "23")
check("escape", #"\^w\#3", 4)
memset(0x4300, 42, 1)
check("peek op", @0x4300, 42)
? "print shorthand works"
printh(fails == 0 and "ALL PASSED" or ("FAILURES "..fails))
