# WiFi Module for AVR_Relay

This project is a WiFi module for the relay control system [AVR_Relay](https://github.com/viteck1048/AVR_Relay). It provides the ability to monitor the status and remotely control the relay via WiFi.

## Installation

1. Install the required libraries via PlatformIO or Arduino
2. Configure the connection parameters in the `conf.h` file
3. Upload the firmware to the device

## Configuration

To configure the parameters, edit the `conf.h` file and specify your encryption keys and paths for the server https://github.com/viteck1048/javaIoTserver:

```c
#define KEY_1 123456
#define KEY_2 789012
// other parameters...
```

## Usage

After starting, the module activates the network settings menu in AVR_Program_Relay, receives the settings from it and starts sending the AVRRelay state to the server.

## License

This project is distributed under the MIT license. See the `LICENSE` file for more information.

# WiFi модуль для AVR_Relay

Цей проект є WiFi-модулем для системи релейного управління [AVR_Relay](https://github.com/viteck1048/AVR_Relay). Він надає можливість моніторингу стану та дистанційного керування реле через WiFi-мережу.

## Встановлення

1. Встановіть необхідні бібліотеки через PlatformIO або ардуїно
2. Налаштуйте параметри підключення у файлі `conf.h`
3. Завантажте прошивку на пристрій

## Конфігурація

Для налаштування параметрів віддреедагуйте файл `conf.h` та вкажіть свої ключі шифрування та шляхи для сервера https://github.com/viteck1048/javaIoTserver:

```c
#define KEY_1 123456
#define KEY_2 789012
// інші параметри...
```

## Використання

Після запуску  модуль активує в AVR_Program_Relay меню налаштувань мережі, отримає від нього налаштування й почне відправляти на сервер стан AVRRelay.

## Ліцензія

Цей проект поширюється під ліцензією MIT. Див. файл `LICENSE` для отримання додаткової інформації.
