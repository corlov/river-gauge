#!/usr/bin/python3

from flask import Flask, request, jsonify
import datetime
import csv
import json
from collections import deque


DATA_FILE = "/root/white-water/received_data.csv"
CONFIG_FILE = '/root/white-water/device_config.json'

LINES_TO_SHOW = 50

app = Flask(__name__)



# @app.route('/', methods=['POST'])
# def receive_data():
#     payload_bytes = request.data
#     if not payload_bytes:
#         print("Получен пустой запрос. Данные не записаны.")
#         return "Bad Request: Empty payload", 400
#     payload_string = payload_bytes.decode('utf-8')
#     timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
#     print(f"[{timestamp}] Получены данные: {payload_string}")
#     try:
#         with open(DATA_FILE, 'a', newline='') as f:
#             f.write(payload_string + '\n')
#         print(f"Данные успешно добавлены в файл {DATA_FILE}")
#     except Exception as e:
#         print(f"!!! ОШИБКА при записи в файл: {e}")
#         return "Internal Server Error", 500
#     return "OK", 200




@app.route('/', methods=['POST'])
def receive_data():
    
    payload_bytes = request.data
    if not payload_bytes:
        print("Получен пустой запрос.")
        return "Bad Request: Empty payload", 400
    
    payload_string = payload_bytes.decode('utf-8')
    timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    print(f"[{timestamp}] Получены данные: {payload_string}")

    try:
        with open(DATA_FILE, 'a', newline='') as f:
            f.write(payload_string + '\n')
        print(f"Данные успешно добавлены в файл {DATA_FILE}")
    except Exception as e:
        print(f"!!! ОШИБКА при записи в файл: {e}")
        return "Internal Server Error", 500

    
    try:
        with open(CONFIG_FILE, 'r') as f:
            config_data = json.load(f)

        config_data['current_server_time'] = datetime.datetime.now().isoformat(timespec='seconds')

        response_payload = {
            "status": "OK",
            "config": config_data
        }
        return jsonify(response_payload), 200
        
    except Exception as e:
        print(f"!!! ОШИБКА при чтении конфига или формировании ответа: {e}")
        # Если не удалось отправить настройки, отправляем простой OK, чтобы устройство не считало это ошибкой
        return "OK", 200





def calc_dates_delta(date_str1, date_str2):
    date_format = "%d.%m.%Y %H:%M:%S"
    try:
        dt_object1 = datetime.datetime.strptime(date_str1, date_format)
        dt_object2 = datetime.datetime.strptime(date_str2, date_format)
        delta = dt_object2 - dt_object1
        return delta.total_seconds()
    except (ValueError, TypeError):
        print(f"Ошибка: не удалось рассчитать дельту для '{date_str1}' и '{date_str2}'")
        return "N/A"



@app.route('/latest', methods=['GET'])
def get_latest_data():
    print(f"Запрос на /latest. Отдаю последние {LINES_TO_SHOW} записей.")
    try:
        with open(DATA_FILE, 'r') as f:
            last_lines = deque(f, LINES_TO_SHOW)
    except FileNotFoundError:
        return f"Ошибка: Файл с данными {DATA_FILE} еще не создан.", 404
    if not last_lines:
        return "Файл с данными пуст.", 200


    html = """
    <!DOCTYPE html>
    <html lang="ru">
    <head>
        <meta charset="UTF-8">
        <title>Последние данные</title>
        <style>
            body {{ font-family: sans-serif; margin: 2em; background-color: #f4f4f9; }}
            h1 {{ color: #333; }}
            table {{ border-collapse: collapse; width: 80%; margin-top: 20px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }}
            th, td {{ border: 1px solid #ddd; padding: 12px; text-align: left; }}
            th {{ background-color: #8A2BE2; color: white; }}
            tr:nth-child(even) {{ background-color: #f2f2f2; }}
            tr:hover {{ background-color: #ddd; }}

            .highlight-water {{ background-color: lightyellow; color: blue; font-weight: bold; }}
            .highlight-voltage {{ background-color: lightyellow; color: blue; font-weight: bold; }}
            .service-info {{ color: grey; font-size: 0.9em; }}
        </style>
    </head>
    <body>
        <h1>Гидропост v.1.0.0</h1>
        <h4>Последние {lines_count} записей</h4>
        <table>
            <thead>
                <tr>
                    <th>(1)</th>
                    <th>(2)</th>
                    <th>(3)</th>
                    <th>(4)</th>
                    <th>(5)</th>
                    <th>(6)</th>
                    <th>(7)</th>
                    <th>(8)</th>
                    <th>(9)</th>
                    <th>(10)</th>
                    <th>(11)</th>
                    <th>(12)</th>
                    <th>(13)</th>
                    <th>(14)</th>
                    <th>(15)</th>
                    <th>(16)</th>
                    <th>(17)</th>
                    <th>(18)</th>
                    <th>(19)</th>
                    <th>(20)</th>
                    <th>(21)</th>
                    <th>(22)</th>
                    <th>(23)</th>                    
                </tr>
                <tr>
                    <th>Сч.запусков устройства</th>
                    <th>Время пробуждения</th>
                    <th>Время отправки</th>
                    <th>Поиск GPRS, сек</th>
                    <th>T возд. дат1, °C</th>
                    <th>Влажн., %</th>
                    <th>Давл.атм., мПа</th>
                    <th>T воздуха дат2, °C</th>
                    <th>T воды, °C</th>
                    <th>Уровень воды реки, м</th>
                    <th>U АКБ, В</th>
                    <th>I потребления, мА</th>
                    <th>P потребления, мВт</th>
                    
                    <th>Качество сигнала</th>
                    <th>Оператор</th>
                    <th>% батареи</th>
                    <th>U модема, мВ</th>
                    <th>IMEI</th>
                    <th>Время оператора</th>
                    
                    <th>Dev_id</th>
                    <th>GPS</th>
                    <th>Дата монтажа</th>
                    <th>Версия ПО</th>
                </tr>
            </thead>
            <tbody>
    """.format(lines_count=len(last_lines))

    for line in reversed(last_lines):
        try:
            row = list(csv.reader([line.strip()]))[0]

            def get_value(index, default="N/A"):
                return row[index] if len(row) > index else default

            boot_counter = get_value(0)
            time_start = get_value(1)
            time_send = get_value(22)
            delta = calc_dates_delta(time_start, time_send)

            t_sensor_1 = get_value(2)
            hum = get_value(3)
            pres = get_value(4)
            t_sensor_2 = get_value(5)
            t_h2o = get_value(6)
            deep = get_value(12)
            volts = get_value(13)
            current = get_value(14)
            power = get_value(15)

            dev_id = get_value(7)
            gps = get_value(8) + ',' + get_value(9)
            install_date = get_value(11)
            ver = get_value(10)
            
            
            quality = get_value(16)
            operator = get_value(17)
            bat = get_value(18)
            u_modem = get_value(19)
            imei = get_value(20)
            operator_time = get_value(21)
            

            html += f"""
                <tr>
                    <td>{boot_counter}</td>
                    <td>{time_start}</td>
                    <td>{time_send}</td>
                    <td>{delta}</td>
                    <td>{t_sensor_1}</td>
                    <td>{hum}</td>
                    <td>{pres}</td>
                    <td>{t_sensor_2}</td>
                    <td class="highlight-voltage">{t_h2o}</td>
                    <td class="highlight-water">{deep}</td>
                    <td>{volts}</td>
                    <td>{current}</td>
                    <td>{power}</td>
                    
                    <td>{quality}</td>
                    <td>{operator}</td>
                    <td>{bat}</td>
                    <td>{u_modem}</td>
                    <td>{imei}</td>
                    <td>{operator_time}</td>
                    
                    <td class="service-info">{dev_id}</td>
                    <td class="service-info">{gps}</td>
                    <td class="service-info">{install_date}</td>
                    <td class="service-info">{ver}</td>
                </tr>
            """
        except Exception as e:
            print(f"Не удалось разобрать строку: '{line.strip()}'. Ошибка: {e}")
            continue

    html += """
            </tbody>
        </table>
    </body>
    </html>
    """
    return html, 200





if __name__ == '__main__':
    print(f"Сервер запущен.")
    print(f" - POST-запросы для сохранения данных принимаются на: http://<ваш_ip>:8001/")
    print(f" - Последние данные можно посмотреть по адресу: http://<ваш_ip>:8001/latest")
    print(f"Данные сохраняются в файл: {DATA_FILE}")
    app.run(host='0.0.0.0', port=8001)


