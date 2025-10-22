### Настройка Arduino IDE

1) Скачать под Линукс файл с офиц.сайта (работает только под ВПН) arduino-ide_2.3.6_Linux_64bit.AppImage - его можно запускать это и есть среда
2) для того чтобы запрограммировать чип на базе ESP32 нужно скачать ПО для обновления прошивок
        https://downloads.arduino.cc/tools/dfu-util-0.11-arduino5-linux_amd64.tar.gz
        и положить его ув каталог /home/kostya/.arduino15/staging/packages


### Загрузка на плату ESP32 через Arduino IDE

1) Обязательно только через разъем отмеченый как COM производить программирование, второй разъем USB когда не будем программировать с ноута
2) Выбрать тип платы ESP32S3 DevModule, выбрать устройство /dev/ttyUSB0
3) Возможно чтобы не было просадо питания программирвоать через USB-хаб с отдельным питанием
4) sudo systemctl stop ModemManager
5) sudo usermod -a -G dialout $USER
6) sudo apt-get remove brltty

