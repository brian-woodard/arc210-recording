
#pragma once

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float    f32;
typedef double   f64;

#define ArrayCount(array) sizeof(array)/sizeof(array[0])

struct TBuffer
{
   u8* Data;
   u64 Size;
};

struct TCommandWord
{
   u16 WordCount      : 5;
   u16 Subaddress     : 5;
   u16 Transmit       : 1;
   u16 RemoteTerminal : 5;
};

struct T1553Data
{
   TCommandWord CommandWord;
   u16          StatusWord;
   u16          Words[32];
};

struct T1553Record
{
   f32       Time;
   u16       Record;
   u16       Channel;
   u16       Bus;
   u16       Spare;
   T1553Data Data;
};

