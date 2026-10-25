#pragma once
namespace Tyra {
struct Vec4 {
  float x=0,y=0,z=0,w=1;
  Vec4()=default;
  Vec4(float a,float b,float c,float d=1):x(a),y(b),z(c),w(d){}
  static Vec4 getByLerp(const Vec4& a,const Vec4& b,float t){
    return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.w+(b.w-a.w)*t};
  }
};
}
