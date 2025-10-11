#pragma once

namespace Synthetic
{
// clang-format off

struct Synthetic1  { uint8_t data[15];    };  // 2^4  - 1  =   15 B
struct Synthetic2  { uint8_t data[31];    };  // 2^5  - 1  =   31 B
struct Synthetic3  { uint8_t data[63];    };  // 2^6  - 1  =   63 B
struct Synthetic4  { uint8_t data[127];   };  // 2^7  - 1  =  127 B
struct Synthetic5  { uint8_t data[255];   };  // 2^8  - 1  =  255 B
//struct Synthetic6  { uint8_t data[511];   };  // 2^9  - 1  =  511 B
//struct Synthetic7  { uint8_t data[1023];  };  // 2^10 - 1  = 1023 B
//struct Synthetic8  { uint8_t data[1535];  };  // ~1.5 KB
//struct Synthetic9  { uint8_t data[2047];  };  // ~2.0 KB
//struct Synthetic10 { uint8_t data[3071];  };  // ~3.0 KB
//struct Synthetic11 { uint8_t data[3583];  };  // ~3.5 KB
//struct Synthetic12 { uint8_t data[4095];  };  // 2^12 - 1 = ~4.0 KB

// clang-format on
}  // namespace Synthetic