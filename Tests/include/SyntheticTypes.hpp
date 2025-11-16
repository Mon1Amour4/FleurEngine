#pragma once

namespace Synthetic
{
// clang-format off

struct Synthetic1  // 2^4  - 1  =   15 B
{ 
	// Non-trivial Ctor&Dctor
	Synthetic1() 
	{
		data[0] = 128;
		std::cout << "Synthetic1 constructor" << std::endl;
	}
	~Synthetic1() 
	{
		data[0] = 64;
		std::cout << "Synthetic1 destructor" << std::endl;
	}

	uint8_t data[15];    
};  
struct Synthetic2  
{
	//Synthetic2(bool){};
	uint8_t data[31];    
};  // 2^5  - 1  =   31 B
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

inline uint32_t SizeOf(uint32_t idx) 
{
	switch (idx)
	{
		case 0:
			return sizeof(Synthetic1);
			break;

		case 1:
			return sizeof(Synthetic2);
			break;

		case 2:
			return sizeof(Synthetic3);
			break;

		case 3:
			return sizeof(Synthetic4);
			break;

		case 4:
			return sizeof(Synthetic5);
			break;
	}
}
// clang-format on
}  // namespace Synthetic