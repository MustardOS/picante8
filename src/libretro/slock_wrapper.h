#pragma once

#if defined(_3DS) || defined(__PSP__) || defined(PS2) || defined(WIIU) || defined(GEKKO) || defined(__DJGPP__)
#define USE_SLOCK_WRAPPER 1
#endif

#ifdef USE_SLOCK_WRAPPER
typedef struct slock slock_t;

class slock_wrap
{
 public:
  slock_wrap();
  ~slock_wrap();
  void lock();
  void unlock();
  bool try_lock();
 private:
  slock_t *slck;
};
#endif
