#!/usr/bin/python3


from flask import Flask, request
import datetime

# --- КОНФИГУРАЦИЯ ---
# Имя файла, куда будут добавляться данные.
# Он будет создан в той же папке, где запущен скрипт.
DATA_FILE = "/root/white-water/received_data.csv"

# Создаем экземпляр Flask-приложения
app = Flask(__name__)

# Определяем маршрут (route) для обработки запросов
# Мы ждем POST-запросы на главный URL ("/")
@app.route('/', methods=['POST'])
def receive_data():
    """
    Эта функция будет вызываться каждый раз, когда на сервер
    приходит POST-запрос.
    """
    # 1. Получаем сырые данные из тела запроса.
    # request.data содержит payload в виде байтов (bytes).
    payload_bytes = request.data

    # Проверяем, что данные не пустые
    if not payload_bytes:
        print("Получен пустой запрос. Данные не записаны.")
        # Возвращаем ошибку клиенту
        return "Bad Request: Empty payload", 400

    # 2. Декодируем байты в строку (стандартная кодировка UTF-8)
    payload_string = payload_bytes.decode('utf-8')

    # 3. Печатаем полученные данные на консоль с временной меткой
    timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    print(f"[{timestamp}] Получены данные: {payload_string}")

    # 4. Добавляем данные в конец файла
    try:
        # Открываем файл в режиме 'a' (append - дозапись в конец)
        # 'with' гарантирует, что файл будет корректно закрыт
        with open(DATA_FILE, 'a') as f:
            # Добавляем саму строку данных и символ новой строки,
            # чтобы каждая запись была на новой строке.
            f.write(payload_string + '\n')

        print(f"Данные успешно добавлены в файл {DATA_FILE}")

    except Exception as e:
        # Если произошла ошибка при записи в файл
        print(f"!!! ОШИБКА при записи в файл: {e}")
        return "Internal Server Error", 500

    # 5. Отправляем ответ клиенту (Arduino), что все прошло успешно
    return "OK", 200

# Эта часть кода запускает сервер
if __name__ == '__main__':
    # host='0.0.0.0' делает сервер видимым в локальной сети,
    # а не только на этом компьютере. ЭТО КРИТИЧЕСКИ ВАЖНО!
    # port=5000 - стандартный порт для Flask, можно изменить.
    print(f"Сервер запущен. Ожидание данных на порту 5000...")
    print(f"Данные будут сохраняться в файл: {DATA_FILE}")
    app.run(host='0.0.0.0', port=8001)
