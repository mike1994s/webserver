Простой Веб сервер. Работает по протоколу HTTP 1.0, реализована только команда GET, ответы 200 и 404, а также MIME-тип text/html
проект собирается с помощью команды cmake:<br/>
cmake . <br/>
make<br/>
в результате появляется исполняемый файл final, запускается командой:<br/>
final -h <ip> -p <port> -d <directory><br/>
Сервер работает в многопроцесссорном режиме с передачей дескрипторов.
