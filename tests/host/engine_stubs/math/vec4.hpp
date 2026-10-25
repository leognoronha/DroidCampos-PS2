#pragma once
#include <cstdint>
#include <string>
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;
namespace Tyra {
struct alignas(16) Vec4 {
  float x=0,y=0,z=0,w=1;
  Vec4()=default;
  Vec4(float a,float b,float c,float d=1):x(a),y(b),z(c),w(d){}
  void set(const Vec4& v){*this=v;}
  void operator/=(float v){x/=v;y/=v;z/=v;w/=v;}
  std::string getPrint() const {return "host Vec4";}
  static Vec4 getByLerp(const Vec4& a,const Vec4& b,float t){
    return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t};
  }
};
}
