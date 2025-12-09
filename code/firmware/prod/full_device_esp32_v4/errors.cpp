#include "errors.h"
#include "water_lvl_settings.h"
#include <Arduino.h>


// Длительности в миллисекундах
const int BLINK_ON_DURATION = 200;      // Короткая вспышка
const int BLINK_OFF_DURATION = 400;     // Пауза между вспышками в одной цифре
const int INTER_DIGIT_PAUSE = 1000;      // Пауза между цифрами (между "2" и "1" в коде 213)

/**
 * @brief Промаргивает код ошибки, используя не более 3 вспышек на цифру.
 *        Функция блокирующая, использует delay().
 * @param errorCode Код ошибки для индикации (например, 213).
 *                  Цифры в коде должны быть от 1 до 3.
 *                  Цифра 0 игнорируется (используется как разделитель).
 */
void blinkErrorCode(int errorCode) {
  String codeStr = String(errorCode);

  for (int i = 0; i < codeStr.length(); i++) {
    char digitChar = codeStr.charAt(i);

    // Превращаем символ '1' в число 1 и т.д.
    int blinks = digitChar - '0';

    if (blinks <= 0 || blinks > 3) {
      continue;
    }

    // Промаргиваем текущую цифру
    for (int j = 0; j < blinks; j++) {
      digitalWrite(ERRORE_LED_PIN, HIGH);
      delay(BLINK_ON_DURATION);
      digitalWrite(ERRORE_LED_PIN, LOW);
      // После последней вспышки в цифре пауза не нужна
      if (j < blinks - 1) {
        delay(BLINK_OFF_DURATION);
      }
    }

    // После последней цифры в коде длинная пауза не нужна
    if (i < codeStr.length() - 1) {
      delay(INTER_DIGIT_PAUSE);
    }
  }

  delay(2000);
}