Простой Веб сервер. Работает по протоколу HTTP 1.0, реализована только команда GET, ответы 200 и 404, а также MIME-тип text/html
проект собирается с помощью команды cmake:
cmake .
make
в результате появляется исполняемый файл final, запускается командой:
final -h <ip> -p <port> -d <directory>
Сервер работает в многопроцесссорном режимес передачей дескрипторов.