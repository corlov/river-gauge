const int ledPin = LED_BUILTIN;
const int pinDone = 2; // Вместо 5
const int pinModemPwr = 12; // Вместо 6

void setup() {
  Serial.begin(9600);
  Serial.println("Программа запущена. Настраиваю пины...");

  pinMode(ledPin, OUTPUT);  
  pinMode(pinDone, OUTPUT);
  pinMode(pinModemPwr, OUTPUT);
  
  digitalWrite(pinDone, LOW);
}

void loop() {  
  digitalWrite(ledPin, HIGH);
  digitalWrite(pinDone, LOW);
  digitalWrite(pinModemPwr, HIGH);
  Serial.println("On");
  delay(10000);

  
  digitalWrite(ledPin, LOW);
  digitalWrite(pinDone, HIGH);
  digitalWrite(pinModemPwr, LOW);

  delay(100);
  digitalWrite(pinDone, LOW);
  
  Serial.println("Off");
  delay(10000);
}
