#include "sound.h"

#include "memory.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <type_traits>

#if SOUND_ENABLED

using namespace retro8;
using namespace retro8::sfx;

namespace
{
  constexpr int32_t LOWEST_KEY = -48;
  constexpr int32_t HIGHEST_KEY = 127;

  /* PICO-8 tunes pitch 33 to A 440 Hz, so pitch 0 is C at 65.4 Hz */
  struct KeyTable
  {
    float hz[HIGHEST_KEY - LOWEST_KEY + 1];

    KeyTable()
    {
      for (int32_t key = LOWEST_KEY; key <= HIGHEST_KEY; ++key)
        hz[key - LOWEST_KEY] = 440.0f * std::pow(2.0f, (key - 33) / 12.0f);
    }

    float operator()(int32_t key) const { return hz[std::min(std::max(key, LOWEST_KEY), HIGHEST_KEY) - LOWEST_KEY]; }
  };

  const KeyTable keyHz;

  inline float fraction(double value) { return float(value - std::floor(value)); }
  inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

  /* the waveforms follow measurements of PICO-8 output, t is the position within one period */
  float wave(Waveform waveform, float t, double phase)
  {
    switch (waveform)
    {
    case Waveform::TRIANGLE:
      return 0.5f * (std::fabs(4.0f * t - 2.0f) - 1.0f);
    case Waveform::TILTED_SAW:
    {
      constexpr float a = 0.875f;
      return 0.5f * (t < a ? 2.0f * t / a - 1.0f : 2.0f * (1.0f - t) / (1.0f - a) - 1.0f);
    }
    case Waveform::SAW:
      return 0.653f * (t < 0.5f ? t : t - 1.0f);
    case Waveform::SQUARE:
      return t < 0.5f ? 0.25f : -0.25f;
    case Waveform::PULSE:
      return t < 1.0f / 3.0f ? 0.25f : -0.25f;
    case Waveform::ORGAN:
      return (t < 0.5f ? 3.0f - std::fabs(24.0f * t - 6.0f) : 1.0f - std::fabs(16.0f * t - 12.0f)) / 9.0f;
    case Waveform::PHASER:
    {
      /* two triangles drift against each other at 1/128 of the note frequency */
      const float k = std::fabs(2.0f * fraction(phase / 128.0) - 1.0f);
      const float u = fraction(t + 0.5f * k);
      return (std::fabs(4.0f * u - 2.0f) - std::fabs(4.0f * t - 2.0f)) / 6.0f;
    }
    default:
      return 0.0f;
    }
  }
}

void Voice::start(sound_index_t sound, int32_t first, int32_t notes)
{
  index = sound;
  note = -1;
  limit = notes;
  offset = first;
  played = 0.0;
  clock = 0;
  released = false;
  lastKey = NO_KEY;
  lastVolume = 0.0f;
}

/* PICO-8 mixes at 22050 Hz, so its output stops just under 11 kHz. Measured against PICO-8 on a device it is flat to 9 kHz
   and almost silent above 11 kHz. A windowed sinc gives the same edge, which takes off the harsh top the waveforms
   otherwise have at 44100 Hz. */
void APU::initLowpass()
{
  constexpr double cutoff = 10000.0 / SAMPLE_RATE;
  constexpr double beta = 8.0;
  const auto bessel = [](double x) {
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 32; ++k)
    {
      term *= (x / (2.0 * k)) * (x / (2.0 * k));
      sum += term;
    }
    return sum;
  };

  const double middle = (LOWPASS_TAPS - 1) / 2.0;
  double total = 0.0;
  std::array<double, LOWPASS_TAPS> taps;
  for (size_t i = 0; i < LOWPASS_TAPS; ++i)
  {
    const double n = i - middle;
    const double sinc = n == 0.0 ? 2.0 * cutoff : std::sin(2.0 * M_PI * cutoff * n) / (M_PI * n);
    const double r = n / middle;
    const double window = bessel(beta * std::sqrt(std::max(0.0, 1.0 - r * r))) / bessel(beta);
    taps[i] = sinc * window;
    total += taps[i];
  }

  for (size_t i = 0; i < LOWPASS_TAPS; ++i)
    lowpassTaps[i] = float(taps[i] / total);

  lowpassHistory.fill(0.0f);
  lowpassAt = 0;
}

float APU::lowpass(float sample)
{
  lowpassHistory[lowpassAt] = sample;

  float out = 0.0f;
  size_t at = lowpassAt;
  for (size_t i = 0; i < LOWPASS_TAPS; ++i)
  {
    out += lowpassTaps[i] * lowpassHistory[at];
    at = at == 0 ? LOWPASS_TAPS - 1 : at - 1;
  }

  lowpassAt = (lowpassAt + 1) % LOWPASS_TAPS;
  return out;
}

void APU::init()
{
  static_assert(sizeof(SoundSample) == 2, "Must be 2 bytes");
  static_assert(sizeof(Sound) == 68, "Must be 68 bytes");

  for (size_t i = 0; i < CHANNEL_COUNT; ++i)
  {
    sfxChannels[i] = Channel();
    musicChannels[i] = Channel();
  }

  initLowpass();

  pattern = -1;
  musicVolume = 1.0f;
  musicFade = 0.0f;
  musicStopping = false;
}

void APU::play(sound_index_t index, channel_index_t channel, int32_t start, int32_t length)
{
  mutex.lock();
  playSound(index, channel, start, length);
  mutex.unlock();
}

void APU::music(music_index_t index, int32_t fadeMs, int32_t mask)
{
  mutex.lock();
  startMusic(index, fadeMs, mask);
  mutex.unlock();
}

void APU::playSound(sound_index_t index, channel_index_t channel, int32_t start, int32_t length)
{
  const bool all = channel < 0 || channel >= (channel_index_t)CHANNEL_COUNT;

  if (index == -1 || index == -2)
  {
    for (size_t i = 0; i < CHANNEL_COUNT; ++i)
    {
      if (!all && (channel_index_t)i != channel)
        continue;

      if (index == -1)
      {
        sfxChannels[i].voice.stop();
        sfxChannels[i].instrument.stop();
      }
      else
      {
        sfxChannels[i].voice.released = true;
        sfxChannels[i].instrument.released = true;
      }
    }
    return;
  }

  if (index < 0 || index >= (sound_index_t)SOUND_COUNT)
    return;

  if (channel == -2)
  {
    for (auto& c : sfxChannels)
      if (c.voice.index == index)
      {
        c.voice.stop();
        c.instrument.stop();
      }
    return;
  }

  if (all)
  {
    /* the same sound restarts where it plays, then free channels outside the music mask are taken, then the oldest sound */
    channel = -1;

    for (size_t i = 0; i < CHANNEL_COUNT && channel < 0; ++i)
      if (sfxChannels[i].voice.index == index)
        channel = i;

    for (size_t i = 0; i < CHANNEL_COUNT && channel < 0; ++i)
      if (!(musicMask & (1 << i)) && !sfxChannels[i].voice.active() && !musicChannels[i].voice.active())
        channel = i;

    for (size_t i = 0; i < CHANNEL_COUNT && channel < 0; ++i)
      if (!(musicMask & (1 << i)) && !sfxChannels[i].voice.active())
        channel = i;

    for (size_t i = 0; i < CHANNEL_COUNT; ++i)
      if (!(musicMask & (1 << i)) && (channel < 0 || (sfxChannels[i].voice.active() && sfxChannels[channel].voice.active() && sfxChannels[i].started < sfxChannels[channel].started)))
        channel = i;

    if (channel < 0)
      return;
  }

  Channel& c = sfxChannels[channel];
  c.voice.start(index, std::max(0, start), length > 0 ? length : -1);
  c.instrument.stop();
  c.started = ++starts;
}

void APU::startMusic(music_index_t index, int32_t fadeMs, int32_t mask)
{
  if (index < 0)
  {
    if (fadeMs > 0 && pattern >= 0)
    {
      musicStopping = true;
      musicFade = -1.0f / (fadeMs * (SAMPLE_RATE / 1000.0f));
    }
    else
      stopMusic();
    return;
  }

  musicMask = mask & 0x0f;
  musicStopping = false;
  musicVolume = fadeMs > 0 ? 0.0f : 1.0f;
  musicFade = fadeMs > 0 ? 1.0f / (fadeMs * (SAMPLE_RATE / 1000.0f)) : 0.0f;
  startPattern(index);
}

void APU::stopMusic()
{
  pattern = -1;
  musicStopping = false;
  musicMask = 0;
  for (auto& c : musicChannels)
  {
    c.voice.stop();
    c.instrument.stop();
  }
}

/* the pattern lasts as long as its leftmost sound that does not loop, or 32 notes of its leftmost sound when all of them loop */
void APU::startPattern(music_index_t index)
{
  const Music* m = index >= 0 && index < (music_index_t)MUSIC_COUNT ? memory.music(index) : nullptr;
  bool any = false;

  for (size_t i = 0; m && i < CHANNEL_COUNT; ++i)
    any |= m->isChannelEnabled(i);

  if (!any)
  {
    stopMusic();
    return;
  }

  int64_t length = -1, looping = -1;

  for (size_t i = 0; i < CHANNEL_COUNT; ++i)
  {
    Channel& c = musicChannels[i];
    c.instrument.stop();

    if (!m->isChannelEnabled(i))
    {
      c.voice.stop();
      continue;
    }

    const Sound* sound = memory.sound(m->sound(i));
    c.voice.start(m->sound(i), 0, -1);

    const int64_t note = noteSamples(sound, i);
    if (!sound->loops() && length < 0)
      length = note * sound->length();
    if (looping < 0)
      looping = note * 32;
  }

  pattern = index;
  patternPlayed = 0;
  patternLength = length >= 0 ? length : looping;
}

void APU::advancePattern()
{
  const Music* m = memory.music(pattern);

  if (m->isStop())
    stopMusic();
  else if (m->isLoopEnd())
  {
    music_index_t i = pattern;
    while (i > 0 && !memory.music(i)->isLoopBegin())
      --i;
    startPattern(i);
  }
  else
    startPattern(pattern + 1);
}

/* 0x5f40 halves the speed, 0x5f41 adds reverb, 0x5f42 distorts and 0x5f43 damps, one bit per channel */
uint8_t APU::control(int32_t offset) const
{
  return memory.base()[0x5f40 + offset];
}

int32_t APU::noteSamples(const Sound* sound, channel_index_t channel) const
{
  const int32_t speed = std::max<int32_t>(1, sound->speed) * SAMPLES_PER_TICK;
  return (control(0) & (1 << channel)) ? speed * 2 : speed;
}

float APU::noise(Voice& voice, float frequency, const Sound* sound)
{
  /* zepto8's measurement: random steps through a low pass that follows the pitch, louder the lower the note, worked at 22050 Hz */
  voice.noiseHalf = !voice.noiseHalf;
  if (voice.noiseHalf)
  {
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    const float white = (random >> 8) * (2.0f / 16777216.0f) - 1.0f;

    const float scale = frequency / keyHz(63);
    voice.noiseLast = voice.noise;
    voice.noise = (voice.noiseLast + scale * white) / (1.0f + scale);
  }

  const float key = frequency > 0.0f ? 33.0f + 12.0f * std::log2(frequency / 440.0f) : 0.0f;
  const float factor = 1.0f - std::max(0.0f, std::min(63.0f, key)) / 63.0f;
  float out = voice.noise * 1.5f * (1.0f + factor * factor);

  if (sound->flags & 0x02)
  {
    const float t = fraction(voice.phase);
    out *= 2.0f * (t < 0.5f ? t : t - 1.0f);
  }

  return out;
}

float APU::oscillator(Voice& voice, Waveform waveform, float frequency, const Sound* sound)
{
  float out;

  if (waveform == Waveform::NOISE)
    out = noise(voice, frequency, sound);
  else
  {
    out = wave(waveform, fraction(voice.phase), voice.phase);

    const int32_t detune = sound->detune();
    if (detune)
    {
      out = 0.5f * (out + wave(waveform, fraction(voice.detunePhase), voice.detunePhase));
      voice.detunePhase += frequency * (detune == 1 ? 1.006f : 2.0f) / SAMPLE_RATE;
    }

    if (sound->buzz())
      out = 0.75f * out + (fraction(voice.phase) < 0.25f ? 0.0625f : -0.0625f);
  }

  voice.phase += double(frequency) / SAMPLE_RATE;
  if (voice.phase > 1048576.0)
    voice.phase -= 1048576.0;

  return out;
}

/* owner is set for a top level sound, whose notes may play sfx 0 to 7 as instruments through the owner's second voice */
float APU::renderVoice(Voice& voice, channel_index_t channel, int32_t keyShift, float pitchScale, float volumeScale, Channel* owner)
{
  if (!voice.active())
    return 0.0f;

  const Sound* sound = memory.sound(voice.index);
  int32_t n = int32_t(voice.offset);

  if (n != voice.note)
  {
    bool looped = false;
    if (sound->loops() && !voice.released && voice.note == sound->loopEnd - 1 && n >= sound->loopEnd)
    {
      voice.offset -= sound->loopEnd - sound->loopStart;
      n = int32_t(voice.offset);
      looped = true;
    }

    if (n >= sound->length() || (voice.limit > 0 && voice.played >= voice.limit))
    {
      voice.stop();
      if (owner)
        owner->instrument.stop();
      return 0.0f;
    }

    const bool hadNote = voice.note >= 0;
    SoundSample previous = {0};
    if (hadNote)
    {
      previous = sound->samples[voice.note];
      voice.lastKey = previous.pitch() + keyShift;
      voice.lastVolume = previous.volume() / 7.0f;
    }

    voice.note = n;

    /* an instrument carries on through notes of the same instrument and pitch, and restarts when either changes, when the sound loops or once it has ended */
    const SoundSample& next = sound->samples[n];
    if (owner && next.useSfx())
    {
      const sound_index_t instrument = sound_index_t(next.waveform());
      const bool carries = hadNote && !looped && previous.useSfx() && previous.waveform() == next.waveform() && previous.pitch() == next.pitch();
      if (!carries || !owner->instrument.active() || owner->instrument.index != instrument)
        owner->instrument.start(instrument, 0, -1);
    }
  }

  const SoundSample note = sound->samples[n];
  const float t = float(voice.offset - n);
  const int32_t key = note.pitch() + keyShift;
  const float base = keyHz(key);
  float frequency = base;
  float volume = note.volume() / 7.0f;

  switch (note.effect())
  {
  case Effect::SLIDE:
    /* slides from the previous note and its volume, not towards the next one */
    if (voice.lastKey != NO_KEY)
    {
      frequency = lerp(keyHz(voice.lastKey), base, t);
      if (voice.lastVolume > 0.0f)
        volume = lerp(voice.lastVolume, volume, t);
    }
    break;
  case Effect::VIBRATO:
  {
    /* half a semitone at 7.5 Hz whatever the speed */
    const float wobble = std::fabs(fraction(7.5 * voice.clock / SAMPLE_RATE) - 0.5f) - 0.25f;
    frequency = lerp(base, base * 1.059463094f, wobble);
    break;
  }
  case Effect::DROP:
    frequency *= 1.0f - t;
    break;
  case Effect::FADE_IN:
    volume *= t;
    break;
  case Effect::FADE_OUT:
    volume *= 1.0f - t;
    break;
  case Effect::ARPEGGIO_FAST:
  case Effect::ARPEGGIO_SLOW:
  {
    /* steps through the group of four notes every 4 or 8 ticks, twice as fast at speed 8 or below */
    int64_t ticks = note.effect() == Effect::ARPEGGIO_FAST ? 4 : 8;
    if (sound->speed <= 8)
      ticks /= 2;
    const int32_t step = int32_t(voice.clock / (ticks * SAMPLES_PER_TICK));
    frequency = keyHz(sound->samples[(n & ~3) | (step & 3)].pitch() + keyShift);
    break;
  }
  default:
    break;
  }

  frequency *= pitchScale;
  volume *= volumeScale;

  /* a short glide between volumes keeps note changes from clicking */
  voice.volume += (volume - voice.volume) * 0.04f;

  float out;
  if (owner && note.useSfx())
    out = renderVoice(owner->instrument, channel, key - 24, frequency / base, voice.volume, nullptr);
  else
    out = oscillator(voice, note.waveform(), frequency, sound) * voice.volume;

  const double step = 1.0 / noteSamples(sound, channel);
  voice.offset += step;
  voice.played += step;
  ++voice.clock;

  return out;
}

float APU::renderChannel(Channel& c, channel_index_t index)
{
  if (c.voice.active())
    c.flags = memory.sound(c.voice.index)->flags;

  float out = renderVoice(c.voice, index, 0, 1.0f, 1.0f, &c);

  Sound flags;
  flags.flags = c.flags;
  const uint8_t bit = 1 << index, strong = 0x10 << index;

  int32_t dampen = flags.dampen();
  if (control(3) & bit) dampen = std::max(dampen, 1);
  if (control(3) & strong) dampen = 2;
  if (dampen)
  {
    c.damped += (out - c.damped) * (dampen == 1 ? 0.5f : 0.25f);
    out = c.damped;
  }

  if (control(2) & bit)
    out = std::max(-0.25f, std::min(0.25f, out * 3.0f));

  int32_t reverb = flags.reverb();
  if (control(1) & bit) reverb = std::max(reverb, 1);
  if (control(1) & strong) reverb = 2;
  if (reverb)
  {
    const size_t delay = reverb == 1 ? 1464 : 2928;
    const float echoed = c.echo[(c.echoAt + c.echo.size() - delay) % c.echo.size()];
    out += echoed * 0.5f;
    c.echo[c.echoAt] = out;
  }
  else
    c.echo[c.echoAt] = 0.0f;
  c.echoAt = (c.echoAt + 1) % c.echo.size();

  return out;
}

const Voice* APU::audible(channel_index_t channel) const
{
  if (sfxChannels[channel].voice.active())
    return &sfxChannels[channel].voice;
  if (musicChannels[channel].voice.active())
    return &musicChannels[channel].voice;
  return nullptr;
}

int APU::channelSound(int channel) const
{
  if (channel < 0 || channel >= (int)CHANNEL_COUNT)
    return -1;
  const Voice* voice = audible(channel);
  return voice ? voice->index : -1;
}

int APU::channelNote(int channel) const
{
  if (channel < 0 || channel >= (int)CHANNEL_COUNT)
    return -1;
  const Voice* voice = audible(channel);
  return voice ? (voice->note >= 0 ? voice->note : int(voice->offset)) : -1;
}

void APU::renderSounds(int16_t* dest, size_t totalSamples)
{
  mutex.lock();

  for (size_t s = 0; s < totalSamples; ++s)
  {
    if (pattern >= 0 && ++patternPlayed > patternLength)
      advancePattern();

    if (pattern >= 0 && musicFade != 0.0f)
    {
      musicVolume = std::max(0.0f, std::min(1.0f, musicVolume + musicFade));
      if (musicStopping && musicVolume <= 0.0f)
        stopMusic();
      else if (!musicStopping && musicVolume >= 1.0f)
        musicFade = 0.0f;
    }

    float mix = 0.0f;

    for (size_t i = 0; i < CHANNEL_COUNT; ++i)
    {
      const bool overridden = sfxChannels[i].voice.active();
      const float music = renderChannel(musicChannels[i], i) * musicVolume;
      const float sound = renderChannel(sfxChannels[i], i);

      if (_soundEnabled)
        mix += sound;
      if (_musicEnabled && !overridden)
        mix += music;
    }

    mix = lowpass(mix);
    dest[s] = int16_t(std::max(-32767.0f, std::min(32767.0f, mix * 32767.0f)));
  }

  mutex.unlock();
}

namespace
{
  template<typename T> void putRaw(std::string& out, const T& value)
  {
    static_assert(std::is_trivially_copyable<T>::value, "save state fields must be plain data");
    out.append(reinterpret_cast<const char*>(&value), sizeof(T));
  }

  template<typename T> bool getRaw(const char*& at, const char* end, T& value)
  {
    if (size_t(end - at) < sizeof(T)) return false;
    std::memcpy(&value, at, sizeof(T));
    at += sizeof(T);
    return true;
  }
}

void APU::saveState(std::string& out)
{
  mutex.lock();
  out.clear();
  putRaw(out, sfxChannels);
  putRaw(out, musicChannels);
  putRaw(out, pattern);
  putRaw(out, patternPlayed);
  putRaw(out, patternLength);
  putRaw(out, musicMask);
  putRaw(out, musicVolume);
  putRaw(out, musicFade);
  putRaw(out, musicStopping);
  putRaw(out, starts);
  putRaw(out, random);
  putRaw(out, lowpassHistory);
  putRaw(out, lowpassAt);
  mutex.unlock();
}

bool APU::loadState(const std::string& in)
{
  const char* at = in.data();
  const char* end = at + in.size();

  mutex.lock();
  const bool ok = getRaw(at, end, sfxChannels) && getRaw(at, end, musicChannels) && getRaw(at, end, pattern) &&
    getRaw(at, end, patternPlayed) && getRaw(at, end, patternLength) && getRaw(at, end, musicMask) &&
    getRaw(at, end, musicVolume) && getRaw(at, end, musicFade) && getRaw(at, end, musicStopping) &&
    getRaw(at, end, starts) && getRaw(at, end, random) && getRaw(at, end, lowpassHistory) && getRaw(at, end, lowpassAt);
  if (!ok || lowpassAt >= LOWPASS_TAPS)
  {
    init();
    lowpassAt = 0;
  }
  mutex.unlock();
  return ok;
}

#endif
