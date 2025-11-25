#ifndef TYPES_H_
#define TYPES_H_

#include <Arduino.h> // Для uint32_t и bool

struct PrevState {
  uint32_t bootCount;
  bool success;
};



typedef uint32_t color_t;

const color_t CL_OFF     = 0x000000; // Черный (выключен)

const color_t CL_RED     = 0xFF0000; // Красный (R=255, G=0, B=0)
const color_t CL_GREEN   = 0x00FF00; // Зеленый (R=0, G=255, B=0)
const color_t CL_BLUE    = 0x0000FF; // Синий   (R=0, G=0, B=255)

const color_t CL_YELLOW  = 0xFFFF00; // Желтый  (R=255, G=255, B=0)
const color_t CL_PURPLE  = 0xFF00FF; // Фиолетовый (R=255, G=0, B=255)
const color_t CL_CYAN    = 0x00FFFF; // Голубой (R=0, G=255, B=255)

const color_t CL_WHITE   = 0xFFFFFF; // Белый (R=255, G=255, B=255)



#endif
  