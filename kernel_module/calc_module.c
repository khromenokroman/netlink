#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/genetlink.h>

// Определение
#define FAMILY_NAME "calc_family"
#define COMMAND_CLIENT 1
#define COMMAND_SERVER 2

// PID клиента и сервера
static u32 pid_client = 0;
static u32 pid_server = 0;

// Перечисление атрибутов
enum calc_attrs {
    ATTR_MSG, // Полезная нагрузка — JSON-строка
    __ATTR_MAX,
};

static struct nla_policy calc_policy[__ATTR_MAX] = {
    [ATTR_MSG] = { .type = NLA_NUL_STRING },
};

// Прототип операций (заранее объявляем, чтобы оперировать внутри семейства)
static int calc_cmd_client(struct sk_buff *skb, struct genl_info *info);
static int calc_cmd_server(struct sk_buff *skb, struct genl_info *info);

// Декларация операций
static const struct genl_ops calc_ops[] = {
    {
        .cmd = COMMAND_CLIENT,  // Обработчик клиента
        .flags = 0,
        .policy = calc_policy,
        .doit = calc_cmd_client,
        .dumpit = NULL,
    },
    {
        .cmd = COMMAND_SERVER,  // Обработчик сервера
        .flags = 0,
        .policy = calc_policy,
        .doit = calc_cmd_server,
        .dumpit = NULL,
    },
};

// Семейство Generic Netlink (объявляется наверху)
static struct genl_family calc_family = {
    .name = FAMILY_NAME,
    .version = 1,
    .maxattr = 1,
    .module = THIS_MODULE,
    .ops = calc_ops, // Указываем зарегистрированные операции
    .n_ops = ARRAY_SIZE(calc_ops), // Количество операций
};

// Пересылка сообщения серверу с логированием
static int forward_to_server(const char *msg) {
    struct sk_buff *skb;
    void *hdr;

    // Проверка наличия PID сервера
    if (pid_server == 0) {
        pr_err("[Kernel] Server PID is not registered. Unable to forward message.\n");
        return -EINVAL;
    }

    pr_info("[Kernel] Preparing to forward message to server. Client PID: %d, Server PID: %d, Message: %s\n",
            pid_client, pid_server, msg);

    // Создание нового сообщения
    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (!skb) {
        pr_err("[Kernel] Failed to allocate new sk_buff.\n");
        return -ENOMEM;
    }

    // Создаем заголовок Generic Netlink
    hdr = genlmsg_put(skb, 0, 0, &calc_family, 0, COMMAND_SERVER);
    if (!hdr) {
        pr_err("[Kernel] Failed to create genlmsg header.\n");
        kfree_skb(skb);
        return -ENOMEM;
    }

    // Добавляем полезные данные
    if (nla_put_string(skb, ATTR_MSG, msg)) {
        pr_err("[Kernel] Failed to add message payload to genlmsg.\n");
        kfree_skb(skb);
        return -EMSGSIZE;
    }

    genlmsg_end(skb, hdr);

    // Отправляем сообщение серверу
    pr_info("[Kernel] Forwarding message to server (PID %d). Message: %s\n", pid_server, msg);
    return genlmsg_unicast(&init_net, skb, pid_server);
}

// Обработка команд от клиента
static int calc_cmd_client(struct sk_buff *skb, struct genl_info *info) {
    struct nlattr *na;
    char *msg;

    pr_info("[Kernel] Client command received. Processing...\n");

    // Получаем полезную нагрузку
    na = info->attrs[ATTR_MSG];
    if (!na) {
        pr_err("[Kernel] Received message with no payload.\n");
        return -EINVAL;
    }

    // Запоминаем PID клиента для обратного ответа
    pid_client = info->snd_portid;
    msg = nla_data(na);

    // Логируем сообщение
    pr_info("[Kernel] Received message from client (PID %d): %s\n", pid_client, msg);

    // Пересылаем запрос на сервер
    int ret = forward_to_server(msg);
    if (ret != 0) {
        pr_err("[Kernel] Failed to forward message to server. Error: %d\n", ret);
    } else {
        pr_info("[Kernel] Successfully forwarded client message to server.\n");
    }

    return ret;
}

// Обработка команд от сервера
static int calc_cmd_server(struct sk_buff *skb, struct genl_info *info) {
    struct nlattr *na;
    char *msg;

    pr_info("[Kernel] Server command received. Processing...\n");

    // Получаем полезную нагрузку
    na = info->attrs[ATTR_MSG];
    if (!na) {
        pr_err("[Kernel] Received server message with no payload.\n");
        return -EINVAL;
    }

    // Запоминаем PID сервера (в первый раз)
    if (pid_server == 0) {
        pid_server = info->snd_portid;
        pr_info("[Kernel] Registered server PID: %d\n", pid_server);
    }

    msg = nla_data(na);

    // Логируем сообщение от сервера
    pr_info("[Kernel] Received response from server (PID %d): %s\n", pid_server, msg);

    // Проверяем наличие PID клиента
    if (pid_client == 0) {
        pr_err("[Kernel] No client PID registered to send server response.\n");
        return -EINVAL;
    }

    pr_info("[Kernel] Forwarding server response to client (PID %d). Message: %s\n", pid_client, msg);

    struct sk_buff *skb_out;
    void *hdr;

    // Создание нового сообщения
    skb_out = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (!skb_out) {
        pr_err("[Kernel] Failed to allocate sk_buff for server response.\n");
        return -ENOMEM;
    }

    hdr = genlmsg_put(skb_out, 0, 0, &calc_family, 0, COMMAND_CLIENT);
    if (!hdr) {
        pr_err("[Kernel] Failed to create genlmsg header for client.\n");
        kfree_skb(skb_out);
        return -ENOMEM;
    }

    if (nla_put_string(skb_out, ATTR_MSG, msg)) {
        pr_err("[Kernel] Failed to add server response payload.\n");
        kfree_skb(skb_out);
        return -EMSGSIZE;
    }

    genlmsg_end(skb_out, hdr);

    int ret = genlmsg_unicast(&init_net, skb_out, pid_client);
    if (ret != 0) {
        pr_err("[Kernel] Failed to send server response to client (PID %d). Error: %d\n", pid_client, ret);
    } else {
        pr_info("[Kernel] Successfully forwarded server response to client.\n");
    }

    return ret;
}

// Модуль инициализации
static int __init calc_init(void) {
    int ret;

    pr_info("[Kernel] Initializing calc_family...\n");

    pr_info("Family name: %s\n", FAMILY_NAME);
    pr_info("Family version: %d\n", calc_family.version);
    pr_info("Family maxattr: %d\n", calc_family.maxattr);
    pr_info("Number of operations: %ld\n", ARRAY_SIZE(calc_ops));

    // Регистрируем семейство
    ret = genl_register_family(&calc_family);
    if (ret) {
        pr_err("[Kernel] Failed to register Generic Netlink family: %d\n", ret);
        return ret;
    }
    pr_info("[Kernel] calc_family registered with family ID: %d\n", calc_family.id);

    pr_info("[Kernel] calc_family initialized successfully.\n");
    return 0;
}

// Выгрузка модуля
static void __exit calc_exit(void) {
    pr_info("[Kernel] Exiting calc_family...\n");

    // Освобождаем семейство
    genl_unregister_family(&calc_family);

    // Сброс PID при выгрузке
    pid_client = 0;
    pid_server = 0;

    pr_info("[Kernel] calc_family exited successfully.\n");
}

module_init(calc_init);
module_exit(calc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Kernel module for Generic Netlink communication.");
MODULE_VERSION("1.0");