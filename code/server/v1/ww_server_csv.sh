#!/bin/bash

# --- Конфигурация ---
PORT=8001
CSV_FILE="sensor_data.csv"

# --- Создаем заголовок CSV, если файл не существует ---
# Это гарантирует, что заголовок будет записан только один раз.
if [ ! -f "$CSV_FILE" ]; then
  # Важно: имена колонок должны соответствовать данным, которые вы будете записывать
  # 1-й столбец: наше время сервера
  # 2-й - 5-й столбцы: данные, приходящие от устройства
  echo "server_time,device_id,device_datetime,temperature,humidity,pressure" > "$CSV_FILE"
  echo "CSV file '$CSV_FILE' created with header."
fi

echo "Listening on port $PORT. Appending data to '$CSV_FILE'. Press Ctrl+C to stop."

# --- Основной цикл для прослушивания порта ---
while true; do
  # Ждем подключения и считываем ВСЕ, что придет, в переменную.
  # nc закроет соединение, как только клиент его закроет.
  incoming_data=$(nc -l "$PORT")

  # Проверяем, что мы получили непустые данные, чтобы не записывать пустые строки
  if [ -n "$incoming_data" ]; then
    # Получаем текущее время сервера в нужном формате (ISO 8601)
    timestamp=$(date +'%Y-%m-%d %H:%M:%S')

    # Объединяем временную метку сервера и полученные данные через запятую
    # и добавляем в конец CSV-файла.
    echo "$timestamp,$incoming_data" >> "$CSV_FILE"

    # (Опционально) Выводим в консоль то, что только что записали, для отладки
    echo "Logged: $timestamp,$incoming_data"
  fi
done
