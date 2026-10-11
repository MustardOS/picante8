local string, math = ...

function all(t)
  if t ~= nil then
    local nt = {}
    local ni = 1
    for _,v in pairs(t) do
      nt[ni] = v
      ni = ni + 1
    end
    for k,v in pairs(nt) do
    end

    local i = 0
    return function() i = i + 1; return nt[i] end
  end
  return function() end
end

function add(t, v, i)
  if t ~= nil then
    if i ~= nil then
      table.insert(t, i, v)
    else
      t[#t+1] = v
    end
    return v
  end
end

function foreach(c, f)
  if c ~= nil then
    for value in all(c) do
      f(value)
    end
  end
end


function mapdraw(...)
  map(...)
end

function count(t, v)
  if t ~= nil then
    if v == nil then
      return #t
    end
    local n = 0
    for i = 1, #t do
      if t[i] == v then
        n = n + 1
      end
    end
    return n
  end
end

function del(t, v)
  if t ~= nil then
    local found = false
    for i = 1, #t do
      if t[i] == v then
        found = true
      end
      if found then
        t[i] = t[i+1]
      end
    end
    if found then
      return v
    end
  end
end

function cocreate(f)
  return coroutine.create(f)
end

function yield(...)
  return coroutine.yield(...)
end

function coresume(f, ...)
  return coroutine.resume(f, ...)
end

function costatus(f)
  return coroutine.status(f)
end

function deli(t, i)
  if t ~= nil and #t > 0 then
    if i == nil then
      i = #t
    end
    if i >= 1 and i <= #t then
      return table.remove(t, i)
    end
  end
end

function split(s, sep, convert)
  local r = {}
  if s == nil then
    return r
  end
  s = tostring(s)
  if sep == nil then
    sep = ","
  end
  if convert == nil then
    convert = true
  end
  local function push(x)
    if convert then
      local v = tonumber(x)
      if v ~= nil then
        x = v
      end
    end
    r[#r+1] = x
  end
  if type(sep) == "number" then
    sep = math.max(1, math.floor(sep))
    for i = 1, #s, sep do
      push(string.sub(s, i, i + sep - 1))
    end
  elseif sep == "" then
    for i = 1, #s do
      push(string.sub(s, i, i))
    end
  else
    local start = 1
    while true do
      local a, b = string.find(s, sep, start, true)
      if a == nil then
        push(string.sub(s, start))
        break
      end
      push(string.sub(s, start, a - 1))
      start = b + 1
    end
  end
  return r
end

function chr(...)
  local t = {...}
  for i = 1, select("#", ...) do
    t[i] = string.char(math.floor(tonumber(t[i]) or 0) % 256)
  end
  return table.concat(t)
end

function ord(s, i, n)
  if type(s) ~= "string" then
    return nil
  end
  i = i or 1
  n = n or 1
  return string.byte(s, i, i + n - 1)
end

function pack(...)
  return table.pack(...)
end

function unpack(t, i, j)
  return table.unpack(t, i, j)
end

function inext(t, i)
  i = (i or 0) + 1
  local v = t[i]
  if v ~= nil then
    return i, v
  end
end

function stop()
end

function ovalfill(x0, y0, x1, y1, c)
  local cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
  local rx, ry = math.abs(x1 - x0) / 2 + 0.5, math.abs(y1 - y0) / 2 + 0.5
  for y = math.floor(math.min(y0, y1)), math.floor(math.max(y0, y1)) do
    local dy = (y - cy) / ry
    if dy >= -1 and dy <= 1 then
      local w = rx * math.sqrt(1 - dy * dy)
      rectfill(cx - w + 0.5, y, cx + w - 0.5, y, c)
    end
  end
end

function oval(x0, y0, x1, y1, c)
  local cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
  local rx, ry = math.abs(x1 - x0) / 2 + 0.5, math.abs(y1 - y0) / 2 + 0.5
  for y = math.floor(math.min(y0, y1)), math.floor(math.max(y0, y1)) do
    local dy = (y - cy) / ry
    if dy >= -1 and dy <= 1 then
      local w = rx * math.sqrt(1 - dy * dy)
      pset(cx - w + 0.5, y, c)
      pset(cx + w - 0.5, y, c)
    end
  end
  for x = math.floor(math.min(x0, x1)), math.floor(math.max(x0, x1)) do
    local dx = (x - cx) / rx
    if dx >= -1 and dx <= 1 then
      local h = ry * math.sqrt(1 - dx * dx)
      pset(x, cy - h + 0.5, c)
      pset(x, cy + h - 0.5, c)
    end
  end
end

-- button glyphs are variables in PICO-8, in both P8SCII and UTF-8 form
for k, v in pairs({
  ["\x8b"] = 0, ["\x91"] = 1, ["\x94"] = 2, ["\x83"] = 3, ["\x8e"] = 4, ["\x97"] = 5,
  ["\xe2\xac\x85"] = 0, ["\xe2\x9e\xa1"] = 1, ["\xe2\xac\x86"] = 2, ["\xe2\xac\x87"] = 3,
  ["\xf0\x9f\x85\xbe"] = 4, ["\xe2\x9d\x8e"] = 5,
}) do
  _ENV[k] = v
  _ENV[k .. "\xef\xb8\x8f"] = v
end