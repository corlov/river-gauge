#!/usr/bin/python3 

from flask import Flask, request
from datetime import datetime
import logging

# --- НАСТРОЙКИ ---
# Укажите IP-адрес, на котором будет работать сервер.
# '0.0.0.0' означает, что он будет доступен со всех сетевых интерфейсов
# (т.е. с других устройств в вашей локальной сети).
HOST = '0.0.0.0'
PORT = 8001
LOG_FILE = "data.log"
LOG_FILE_STAT = "statistica.log"

# --- НАСТРОЙКА ЛОГГИРОВАНИЯ ---
# Настраиваем логирование, чтобы сообщения писались и в консоль, и в файл.
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(message)s',
    handlers=[
        logging.FileHandler(LOG_FILE, encoding='utf-8'),
        logging.StreamHandler() # Вывод в консоль
    ]
)

# --- СОЗДАНИЕ ПРИЛОЖЕНИЯ ---
app = Flask(__name__)

# --- ОБРАБОТЧИК ЗАПРОСОВ ---
@app.route('/', methods=['POST'])
def handle_data():
    """
    Эта функция будет вызываться каждый раз, когда на сервер приходит POST-запрос.
    """
    try:
        # Получаем данные из тела запроса и декодируем их
        data = request.data.decode('utf-8')

        # Логгируем полученные данные
        logging.info(f"Получены данные: {data}")
        
        server_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        print(f"{server_time},{data}")
        with open(LOG_FILE_STAT, "a", encoding="utf-8") as f:
            f.write(f"{server_time},{data}\n")

        # Возвращаем клиенту (ESP32) ответ, что все хорошо
        return "OK", 200

    except Exception as e:
        logging.error(f"Ошибка при обработке запроса: {e}")
        return "Error", 500

# --- ЗАПУСК СЕРВЕРА ---
if __name__ == '__main__':
    print(f"Сервер Flask запущен на http://{HOST}:{PORT}")
    print(f"Данные сохраняются в файл: {LOG_FILE}")
    print("Для остановки сервера нажмите Ctrl+C")
    # Запускаем сервер
    app.run(host=HOST, port=PORT)
