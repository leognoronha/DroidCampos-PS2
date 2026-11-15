/*
# _____        ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2022, tyra - https://github.com/h4570/tyra
# Licensed under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#pragma once

#include <string>
#include <vector>
#include "math/vec4.hpp"
#include "debug/debug.hpp"
#include "renderer/3d/pipeline/static/core/bag/stapip_bag.hpp"
#include "renderer/3d/pipeline/static/core/bag/packaging/stapip_bag_package.hpp"

namespace Tyra {

class StaPipBag;
class StaPipBagPackage;

class StaPipQBuffer {
 public:
  StaPipQBuffer();
  ~StaPipQBuffer();
  StaPipQBuffer(const StaPipQBuffer&) = delete;
  StaPipQBuffer& operator=(const StaPipQBuffer&) = delete;

  void setMaxVertCount(const u32& count);

  /**
   * @brief Dont allocate any dynamic data in buffer.
   * Just copy pointers to input data.
   */
  void fillByPointer(const StaPipBagPackage& pkg);

  /**
   * @brief Allocate dynamic data in buffer
   * And copy input data to it.
   */
  void fillByCopyMax(const StaPipBagPackage& pkg1, const StaPipBagPackage& pkg2,
                     const StaPipBagPackage& pkg3);

  /**
   * @brief Allocate dynamic data in buffer
   * And copy input data to it.
   */
  void fillByCopy1By2(const StaPipBagPackage& pkg1,
                      const StaPipBagPackage& pkg2);

  /**
   * @brief Allocate dynamic data in buffer
   * And copy input data to it.
   */
  void fillByCopy1By3(const StaPipBagPackage& pkg);

  /**
   * @brief Select owned storage, growing only when required. Caller must have
   * acquired this buffer after its previous DMA transfer completed.
   */
  void reallocateManually(const u16& size);

  bool any() const;

  StaPipBag* bag;

  Vec4* vertices;
  Vec4* sts;
  Vec4* colors;
  Vec4* normals;
  u32 size;

  void print() const;
  void print(const char* name) const;
  void print(const std::string& name) const { print(name.c_str()); }
  std::string getPrint(const char* name = nullptr) const;

 private:
  u32 maxVertCount;
  void allocateDynamicData(u16 size, StaPipBag* bag);
  std::vector<Vec4> vertexStorage, stStorage, colorStorage, normalStorage;
};

}  // namespace Tyra
