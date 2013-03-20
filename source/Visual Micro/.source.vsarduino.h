//Board = Arduino Mega 2560 or Mega ADK
#define ARDUINO 102
#define __AVR_ATmega2560__
#define F_CPU 16000000L
#define __AVR__
#define __cplusplus
#define __attribute__(x)
#define __inline__
#define __asm__(x)
#define __extension__
#define __ATTR_PURE__
#define __ATTR_CONST__
#define __inline__
#define __asm__ 
#define __volatile__
#define __builtin_va_list
#define __builtin_va_start
#define __builtin_va_end
#define __DOXYGEN__
#define prog_void
#define PGM_VOID_P int
#define NOINLINE __attribute__((noinline))

typedef unsigned char byte;
extern "C" void __cxa_pure_virtual() {}

void waitForCNC();
//already defined in arduno.h
//already defined in arduno.h
void initCNC();
void setXYAxesZero();
void moveZAxis();
void setZAxisZero();
void moveToStartPos();
void sendCNC();
void moveToolAway();
void streamFileToServer(char *filename);
bool streamDataToFile();
void streamDataToCNC(char *filename);
int getFileList();
void removeDataFiles();
void okPressed();
void downPressed();
void lcdMenuTop(byte index);
void lcdConfigMenu(byte index);
void processUserInterface();

#include "C:\Program Files (x86)\Arduino\hardware\arduino\variants\mega\pins_arduino.h" 
#include "C:\Program Files (x86)\Arduino\hardware\arduino\cores\arduino\arduino.h"
#include "D:\MAS Thesis\Software\CNCInterface\source\CNCInterface.ino"
#include "D:\MAS Thesis\Software\CNCInterface\source\CNCHandler.ino"
#include "D:\MAS Thesis\Software\CNCInterface\source\Streaming.ino"
#include "D:\MAS Thesis\Software\CNCInterface\source\UI.ino"
#include "D:\MAS Thesis\Software\CNCInterface\source\UserHandler.ino"
