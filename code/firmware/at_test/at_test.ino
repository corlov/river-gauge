// Serial Passthrough Sketch
// Pro Micro <-> SIM800

void setup() {
  // Serial для общения с компьютером (Монитор порта)
  Serial.begin(9600);
  while (!Serial); // Ждем открытия монитора

  // Serial1 для общения с модемом SIM800 (пины 0, 1)
  Serial1.begin(9600);

  Serial.println("Ready to talk to modem. Type AT commands.");
}

void loop() {
  // Если что-то пришло от модема, отправляем это в компьютер
  if (Serial1.available()) {
    Serial.write(Serial1.read());
  }
  // Если что-то пришло от компьютера, отправляем это модему
  if (Serial.available()) {
    Serial1.write(Serial.read());
  }
}
