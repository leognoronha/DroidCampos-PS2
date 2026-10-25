#pragma once
#include "math/vec4.hpp"
namespace Tyra {
struct M4x4 {
  Vec4 operator*(const Vec4& v) const {return v;}
};
}
