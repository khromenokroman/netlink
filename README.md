# Netlink

### Задача 

````
Нужно написать два userspace приложения (клиент, сервер) которые общаются по протоколу generic-netlink
Сервер ожидает от клиента запрос вида json:
{ "action": "add", "arg1": 4, "arg2": 5}
И возвращает результат вида:
{ "result": 9 }
Возможные действия (action): add, sub, mul
````

#### Сборка
Модуль ядра
````bash
cd kernel_module
make
````
Клиент, сервер
````bash
cd ..
mkdir -p build && cd build
cmake ..
cmake --build .
````
#### Запуск
````bash
cd ../kernel_module/
insmod calc_module.ko
cd ../build
./server
./client
````

Как это работает
![work](video/work_app.gif)

В хорошем качестве `video/work_app.mp4`

чтобы сделать gif
````bash
apt install ffmpeg
ffmpeg -i video/work_app.mp4 -vf "fps=15,scale=2048:-1:flags=lanczos" -c:v gif video/work_app.gif
````