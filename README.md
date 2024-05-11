## Отчет

### Работу выполнил Степанов Роман Андреевич, студент группы БПИ227

### Вариант 33

### Условие:

Пляшущие человечки. На тайном собрании глав преступного
мира города Лондона председатель собрания профессор Мориарти постановил: отныне вся переписка между преступниками должна
вестись тайнописью. В качестве стандарта были выбраны «пляшущие человечки», шифр, в котором каждой букве латинского
алфавита соответствует хитроумный значок.
Реализовать клиент–серверное приложение, шифрующее
исходный текст (в качестве ключа используется кодовая
таблица, устанавливающая однозначное соответствие
между каждой буквой и каким–нибудь числом).
Каждый процесс–шифровальщик является клиентом. Он кодирует свои кусочки общего текста. При решении использовать
парадигму портфеля задач. клиенты работают асинхронно, формируя свой закодированный фрагмент в случайное время.
Следовательно, при занесении информации в портфель–сервер необходимо проводить упорядочение фрагментов.
В программе необходимо вывести исходный текст, закодированные
фрагменты по мере их формирования, окончательный закодированный текст.

### Сценарий:

Программа сервер получает на вход сообщение, которое должно быть зашифровано, делит его на части и отдает клиентам.
Клиенты, после получения части сообщения, шифруют его и отправляют обратно на сервер

### Об аргументах:

Сервер принимает на вход: <ip> <port> <number> <message>.
number - число клиентов, которые будут шифровать сообщения.
Клиент принимает на вход: <ip> <port>, по которому клиент должен постучаться, чтобы попасть на сервер

# Предполагаемая оценка - 10 баллов

На каждую оценку разработана отдельная программа, которая является модификацией предыдущей. Базовые понятия описаны в
решении на 4-5 баллов, далее в каждом критерии указан список изменений

# Критерии на 4-5 баллов:

## Решение:

В папке 4-5 два файла: server.cpp и client.cpp.

server.cpp - сервер, который должен быть запущен первым. Принимает на вход ip, port, n, message. Ждет подключения n
клиентов, которые подтвердят свой статус шифровальщиков сообщением "worker". Делит message на части,
динной ```TASK_SIZE``` (#define в файле lib.h) и отправляет их клиентов. Отправление работает следующим образом: сервер
обходит список рабочих и отдает им кусок сообщения. Если список закончился - то сервер продолжает отсылать сообщения,
обнулив при этом счетчик списке клиентов. Далее сервер получает ответы от клиентов - в конфигруации на оценку 4-5
баллов, сервер синхронно собирает ответы в том порядке, в котором он их раздал (это изменено в решениях на большую
оценку).
После того как сервер собрал ответы, он выводит их на экран.
client.cpp - клиент, который должен быть запущен только после запуска сервера (иначе, он не сможет с ним соединиться).
Получает на вход ip адрес и порт сервар. После принятия сервером, отсылает ему сообщение "worker". Получает от сервера
сообщения, которые шифрует при помощи метода encode (из файла encode.h). Для этого метода используется словарь, который
находится в том же .h файле. Перед использованием - его надо проинициализировать. Функция инициализации позволяет при
отсутствии переданных аргументов сделать стандартную иницилазицию - каждому символу сопоставят символ, аски код которого
больше его самого на SHIFT (#define в lib.h) (в случае, если полученное число больше 128 - берется остаток от этого
числа по модулю 128). Словарь типа <char, char>, для дого чтобы длина задания и ответа совпадала - возможно
уницицированное чтение пакетов.
файлы lib.h и lib.cpp содержат определения и реализацию важных функций, используемых в программе.