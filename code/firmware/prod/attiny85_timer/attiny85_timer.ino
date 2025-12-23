#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>



#define MOSFET_PIN PB0
#define DONE_PIN PB1

#define DEVICE_ON 1
#define DEVICE_OFF 0

#define SLEEP_TIME_SECONDS 60*60*3
#define MAX_ACTIVE_TIMEOUT_SECONDS 60*5
#define PROGRAMMED_SLEEP_TIME 8

volatile bool wdt_flag = false;
volatile bool done_flag = false;


// Прерывание WDT (Когда WDT таймер досчитывает до установленного времени)
// Настройка WDT на 8 секунд → Спим → WDT считает → Через 8 сек → WDT_vect → Просыпаемся
ISR(WDT_vect) {
  wdt_flag = true;
}



// по низкому уровню прерывание
// Прерывание по DONE (Pin Change на PB1)
// Срабатывает при изменении состояния пина (LOW→HIGH или HIGH→LOW):
// Когда меняется состояние любого из пинов PB0-PB5
ISR(PCINT0_vect) {
  if (digitalRead(DONE_PIN) == LOW) {
    done_flag = true;
  }
}



// Настройка WDT таймера
void setupWDT() {
  // отключаем WDT: функция сбрасывает (перезапускает) watchdog таймер. Если этого не сделать, WDT досчитает до конца и перезагрузит МК.
  wdt_reset();
  MCUSR &= ~(1 << WDRF);

  // Разрешаем изменение настроек WDT
  WDTCR |= (1 << WDCE) | (1 << WDE);

  // 8 секунд (это самое большое что можно позволить без кварца и внешних датчиков времени в таком режиме сна)
  WDTCR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);
}



void configurePinModes() {
  pinMode(MOSFET_PIN, OUTPUT);
  pinMode(DONE_PIN, INPUT_PULLUP);

  // Неиспользуемые пины - INPUT, LOW для минимизации потребления
  // P5 - это ресет пин, обычно не трогается
  pinMode(PB2, INPUT); 
  digitalWrite(PB2, LOW);
  pinMode(PB3, INPUT); 
  digitalWrite(PB3, LOW);
  pinMode(PB4, INPUT); 
  digitalWrite(PB4, LOW);
}


void setupInterrupts() {
  // Настройка прерывания на DONE_PIN (PB1)
  // Включить систему Pin Change прерываний
  GIMSK |= (1 << PCIE); 
  // Подключить датчик на PB1 к системе, Когда на PB1 меняется состояние → срабатывает ISR(PCINT0_vect)
  PCMSK |= (1 << PCINT1); 
}


// Отключаем всё лишнее для экономии энергии
void disableUnusedHardware() {
  // Отключаем ADC
  ADCSRA = 0;
  // Отключаем аналоговый компаратор
  ACSR |= (1 << ACD);
  // Отключаем цифровые входы
  DIDR0 = 0xFF;
}



void setup() {
  configurePinModes();

  setupInterrupts();

  disableUnusedHardware();

  // set interruption enabled
  sei();
}



// SLEEP_MODE_PWR_DOWN (Самый экономичный)
// Отключает: ВСЕ источники тактирования, включая внутренний RC-генератор
// Остаются:
//      Watchdog Timer (если включён в режиме прерывания)
//      Pin Change Interrupt
//      Внешний Reset
// Потребление: 0.1-1.0 мкА (в идеальных условиях)
// Пробуждение: Только по WDT, Pin Change или внешнему Reset
// Особенность: millis(), delay() и таймеры НЕ работают
void goSleep() {
    wdt_flag = false;

    // SLEEP_MODE_PWR_DOWN (Самый экономичный)
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    // устанавливает бит SE (Sleep Enable) в регистре MCUCR (подготовка)
    sleep_enable();
    // МК мгновенно переходит в выбранный режим сна Выполнение кода останавливается на этой строке
    sleep_cpu();
    // Сбрасывает бит SE в регистре MCUCR
    // Зачем вызывать, если он вызывается автоматически? Для чистоты кода и на случай, если пробуждение произошло не по прерыванию.
    // ПРОСЫПАЕМСЯ здесь (через 8 секунд, задано командой setupWDT)
    sleep_disable();

    // Проверяем флаг (должен быть true)
    if (!wdt_flag) {
      // Что-то пошло не так - WDT не сработал
      // TODO: как это обработать?
    }
    // задержка для стабилизации
    _delay_us(10);
}



void deviceGoSleep() {
  setupWDT();

  for(uint16_t i = 0; i < SLEEP_TIME_SECONDS / PROGRAMMED_SLEEP_TIME; i++) {
    goSleep();
  }
}



void waitForDoneOrTimeout() {
  done_flag = false;

  for(uint8_t i = 0; i < MAX_ACTIVE_TIMEOUT_SECONDS; i++) {
    delay(1000);
    if (done_flag) {
      break;
    }
  }
}



void setDeviceMode(uint8_t mode) {
  if (mode) {
    digitalWrite(MOSFET_PIN, HIGH);
  }
  else {
    digitalWrite(MOSFET_PIN, LOW);  
  }
}



void loop() {
  setDeviceMode(DEVICE_ON);
  delay(10);

  waitForDoneOrTimeout();

  setDeviceMode(DEVICE_OFF);
  
  deviceGoSleep();
}
