#pragma once
#include <cstdint>
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;
namespace Tyra {struct alignas(16) Vec4 {
 float x=0,y=0,z=0,w=1;
 void set(float a,float b,float c,float d){x=a;y=b;z=c;w=d;}
 void operator*=(float v){x*=v;y*=v;z*=v;w*=v;}
};}
