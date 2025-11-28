void setup() {
  pinMode(17, OUTPUT); // Используем пин 17 (RX LED)
}
void loop() {
  digitalWrite(17, HIGH);
  delay(700); // Уменьшим задержку, чтобы быстрее увидеть результат
  digitalWrite(17, LOW);
  delay(200);
}
