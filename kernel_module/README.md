# CALC_MODULE

#### Проверена работоспособность на `linux-headers-6.1.0-28-amd64`

Сборка
````bash
apt install linux-headers-$(uname -r)
make
````

Очистка
````bash
make clean
````

Установка
````bash
su -l
insmod calc_module.ko
````

Удаление
````bash
su -l
rmmod calc_module
````

Как проверить, что все хорошо 
````bash
$ lsmod | grep calc
calc_module            16384  0
````

Журнал (например)
````bash
dmesg
# Установка
[494987.513857] Generic Netlink family 'calc_family' registered.
# Удаление
[495022.689707] Generic Netlink family 'calc_family' unregistered.
````