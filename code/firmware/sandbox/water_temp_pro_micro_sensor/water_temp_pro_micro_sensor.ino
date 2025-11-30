// --- СКЕТЧ-СКАНЕР ДЛЯ ШИНЫ 1-WIRE ---
#include <OneWire.h>

// Укажи пин, к которому подключен датчик
#define ONE_WIRE_BUS_PIN 4

OneWire ow(ONE_WIRE_BUS_PIN);

void setup() {
  Serial.begin(9600);
  while (!Serial); // Ждем открытия монитора порта
  Serial.println("--- Запущен сканер шины 1-Wire ---");
}

void loop() {
  byte i;
  byte present = 0;
  byte addr[8];

  Serial.print("Поиск устройств...");
  // ow.search() ищет следующее устройство на шине.
  // Если находит, возвращает 1. Если нет - 0.
  if (!ow.search(addr)) {
    Serial.println(" Устройства не найдены!");
    ow.reset_search(); // Сбрасываем поиск для следующей попытки
    delay(2000);
    return;
  }

  // Если устройство найдено, выводим его уникальный адрес
  Serial.println(" Устройство найдено!");
  Serial.print("  Адрес =");
  for (i = 0; i < 8; i++) {
    Serial.write(' ');
    if (addr[i] < 16) {
      Serial.print('0');
    }
    Serial.print(addr[i], HEX);
  }
  Serial.println();

  // Проверка CRC (контрольной суммы) адреса. Если она неверна,
  // значит, адрес считан с ошибками (плохой контакт).
  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("Ошибка CRC! Адрес считан неверно. Проверьте контакты!");
  }

  delay(5000); // Ждем 5 секунд перед следующим поиском
}
