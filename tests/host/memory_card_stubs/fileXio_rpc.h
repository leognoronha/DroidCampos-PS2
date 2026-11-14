#pragma once

constexpr int FXIO_WAIT = 0, FXIO_NOWAIT = 1;
constexpr int FXIO_COMPLETE = 1, FXIO_INCOMPLETE = 0;

int fileXioInitSkipOverride();
void fileXioSetBlockMode(int);
int fileXioWaitAsync(int, int*);
int fileXioOpen(const char*, int);
int fileXioRead(int, void*, int);
int fileXioClose(int);
