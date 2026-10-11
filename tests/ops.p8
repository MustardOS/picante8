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
check("print x", print("abc", 0, 0), 12)
check("concat whole", "n"..3, "n3")
check("concat frac", "n"..0.5, "n0.5")
check("precision", 0x7fff.ffff > 32767, true)
poke(0x9000, 1, 2, 3)
check("poke many", peek(0x9000) + peek(0x9001) + peek(0x9002), 6)
check("peek many", select("#", peek(0x9000, 3)), 3)
check("peek wrap", peek(0x10000 + 0x9000), 1)
poke4(0x9100, -1.5)
check("poke4", peek4(0x9100), -1.5)
check("peek2 signed", (function() poke2(0x9200, -2) return peek2(0x9200) end)(), -2)
local hi = 0
for i = 1, 200 do if rnd(1) >= 1 then hi += 1 end end
check("rnd range", hi, 0)
local pick = rnd({7})
check("rnd table", pick, 7)
check("sget bounds", sget(-1, 400), 0)
local seen = {}
local list = {1, 2, 3, 4}
for v in all(list) do add(seen, v) if v == 2 then del(list, v) end end
check("all del", #seen, 4)
local hash = {a = 1, 2}
local n = 0
for v in all(hash) do n += 1 end
check("all array", n, 1)
spr(1, 0, 0, 2)
tline(0, 0, 10, 0, 0, 0)
local obj = {val = 3, time = nil}
local function env_call(_ENV) return flr(val + 0.5) end
check("api in custom env", env_call(obj), 3)
check("field miss stays nil", obj.time, nil)
check("print y", select(2, print("a\nb", 0, 0)), 12)
check("pairs nil", (function() for k in pairs(nil) do return 1 end return 0 end)(), 0)
check("for nil limit", (function() local n = 0 for i = 1, nil do n += 1 end return n end)(), 0)
check("tonum newline", tonum("1\n", 1), nil)
check("camera returns", (function() camera(5, 6) local x, y = camera() return x + y end)(), 11)
check("pal returns", (function() pal(3, 9) local p = pal(3, 3) pal() return p end)(), 9)
check("div zero", 1/0 > 32767, true)
check("pget camera", (function() cls() camera(-10, -10) pset(1, 1, 8) local c = pget(1, 1) camera() return c end)(), 8)
check("time moves", time() >= 0, true)
check("ord frac", ord("ab", 1.5), 97)
check("btnp range", btnp(9), false)
sfx(99) sfx(-1) music(99) music(-1)
check("serial", serial(), 0)
check("flr int idiv zero", flr(5)\0, 32767)
check("flr int mod zero", flr(5)%0, 0)
check("flr whole", flr(2.7) + 0.5, 2.5)
check("float key", ({[3] = "c"})[1.5 * 2], "c")
check("frac key", (function() local t = {} t[0.5] = 1 return t[1 / 2] end)(), 1)
check("literal ffff", 0xffff, -1)
check("literal 8000", 0x8000, -32768)
check("literal 8001", 0x8001, -32767)
check("literal decimal", 40000, -25536)
check("literal frac kept", 0.1 * 10, 1)
check("literal neg min", -32768 < 0, true)
check("folded wrap", 32767 + 1, -32768)
printh(fails == 0 and "ALL PASSED" or ("FAILURES "..fails))
