Простой Веб сервер. Работает по протоколу HTTP 1.0, реализована только команда GET, ответы 200 и 404, а также MIME-тип text/html
проект собирается с помощью команды cmake:<br/>
<b>cmake . </b> <br/>
<b>make</b><br/>
в результате появляется исполняемый файл final, запускается командой:<br/>
<b>final -h ip -p port -d directory</b><br/>
Сервер работает в многопроцесссорном режиме с передачей дескрипторов.
