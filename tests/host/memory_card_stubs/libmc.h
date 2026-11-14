#pragma once

#include <cstdint>

constexpr int MC_TYPE_XMC = 1, MC_NOWAIT = 1;
constexpr int sceMcResChangedCard = -1, sceMcResNoFormat = -2, sceMcResFullDevice = -3;
constexpr int sceMcResNoEntry = -4, sceMcResDeniedPermit = -5, sceMcResDeniedPS1Permit = -51;

struct mcIcon {
  char head[4];
  unsigned short type, nlOffset;
  unsigned unknown2, trans;
  int bgCol[4][4];
  float lightDir[3][4], lightCol[3][4], lightAmbient[4];
  unsigned short title[34];
  unsigned char view[64], copy[64], del[64], unknown3[512];
};

int mcInit(int);
int mcSync(int, int*, int*);
int mcGetInfo(int, int, int*, int*, int*);
int mcOpen(int, int, const char*, int);
int mcWrite(int, const void*, int);
int mcClose(int);
int mcFlush(int);
int mcMkDir(int, int, const char*);
