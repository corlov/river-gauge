#include <Arduino.h>

#import "water_lvl_utils.h"

/**
 * @brief Вспомогательная функция для извлечения N-ного значения из CSV-строки.
 * @param data Входная CSV-строка.
 * @param index Индекс нужного значения (начиная с 0).
 * @return Строка с найденным значением или пустая строка, если не найдено.
 */
String getNthValue(const String& data, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == ',' || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}



