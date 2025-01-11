#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <net/genetlink.h>

#define FAMILY_NAME "calc_family"
#define COMMAND_CLIENT 1
#define COMMAND_SERVER 2

enum {
    ATTR_UNSPEC,
    ATTR_MSG,
    __ATTR_MAX,
};
#define ATTR_MAX (__ATTR_MAX - 1)

static __u32 pid_client = 0;
static __u32 pid_server = 0;

static int send_message(const char *msg, int pid);
static int calc_cmd_client(struct sk_buff *skb, struct genl_info *info);
static int calc_cmd_server(struct sk_buff *skb, struct genl_info *info);

static const struct nla_policy calc_policy[ATTR_MAX + 1] = {
    [ATTR_MSG] = {.type = NLA_STRING, .len = 1024},
};

// Декларация операций
static const struct genl_ops calc_ops[] = {
    {
        .cmd = COMMAND_CLIENT, // Обработчик клиента
        .flags = 0,
        .policy = calc_policy,
        .doit = calc_cmd_client,
        .dumpit = NULL,
    },
    {
        .cmd = COMMAND_SERVER, // Обработчик сервера
        .flags = 0,
        .policy = calc_policy,
        .doit = calc_cmd_server,
        .dumpit = NULL,
    },
};

static struct genl_family calc_family = {
    .name = FAMILY_NAME,
    .version = 1,
    .maxattr = 1,
    .module = THIS_MODULE,
    .ops = calc_ops,               // Указываем зарегистрированные операции
    .n_ops = ARRAY_SIZE(calc_ops), // Количество операций
};

// Модуль инициализации
static int __init calc_init(void) {
    int ret;

    pr_info("Initializing calc_family...\n");

    ret = genl_register_family(&calc_family);
    if (ret) {
        pr_err("Failed to register Generic Netlink family: %d\n", ret);
        return ret;
    }
    pr_info("\"%s\" registered with family ID: %d\n", FAMILY_NAME, calc_family.id);

    pr_info("\"%s\" initialized successfully.\n", FAMILY_NAME);
    return 0;
}

// Выгрузка модуля
static void __exit calc_exit(void) {
    pr_info("Exiting calc_family...\n");

    genl_unregister_family(&calc_family);

    pr_info("[Kernel] calc_family exited successfully.\n");
}

module_init(calc_init);
module_exit(calc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Khromenok Roman");
MODULE_DESCRIPTION("Kernel module for Generic Netlink communication.");
MODULE_VERSION("1.0");

static int send_message(const char *msg, int pid) {
    struct sk_buff *skb;
    void *hdr;

    // Проверка PID
    if (pid == 0) {
        pr_err("Not valid PID.\n");
        return -EINVAL;
    }

    pr_info("Sending message: %s to %d\n", msg, pid);

    // Создание нового сообщения
    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (!skb) {
        pr_err("Failed to allocate new sk_buff.\n");
        return -ENOMEM;
    }

    // Создаем заголовок Generic Netlink
    hdr = genlmsg_put(skb, 0, 0, &calc_family, 0, COMMAND_SERVER);
    if (!hdr) {
        pr_err("Failed to create genlmsg header.\n");
        kfree_skb(skb);
        return -ENOMEM;
    }

    // Добавляем полезные данные
    if (nla_put_string(skb, ATTR_MSG, msg)) {
        pr_err("Failed to add message payload to genlmsg.\n");
        kfree_skb(skb);
        return -EMSGSIZE;
    }

    genlmsg_end(skb, hdr);

    int ret = genlmsg_unicast(&init_net, skb, pid);
    if (ret) {
        pr_err("[Kernel] Failed sending message. Error: %d\n", ret);
    } else {
        pr_info("[Kernel] Sended message\n");
    }
    return ret;
}

static int calc_cmd_client(struct sk_buff *skb, struct genl_info *info) {
    struct nlattr *na;
    char *msg;
    int result = -EINVAL;

    pr_info("Client receiving message. Processing...\n");

    na = info->attrs[ATTR_MSG];
    if (!na) {
        pr_err("Received message with no payload.\n");
        return result;
    }

    pid_client = info->snd_portid;
    msg = nla_data(na);

    pr_info("Received message from client (PID %d): %s\n", pid_client, msg);

    if (pid_server != 0) {
        result = send_message(msg, pid_server);
        if (result != 0) {
            pr_err("Failed sending message. Error: %d\n", result);
        } else {
            pr_info("Successfully sending client message.\n");
        }
    } else {
        pr_info("Server not connected\n");
        result = 0;
    }

    return result;
}

static int calc_cmd_server(struct sk_buff *skb, struct genl_info *info) {
    struct nlattr *na;
    char *msg;
    int result = -EINVAL;

    pr_info("Server receiving message. Processing...\n");

    na = info->attrs[ATTR_MSG];
    if (!na) {
        pr_err("Received message with no payload.\n");
        return result;
    }

    msg = nla_data(na);
    pr_info("Received message from server (PID %d): %s\n", pid_client, msg);

    if (pid_server != 0) {
        result = send_message(msg, pid_client);
        if (result != 0) {
            pr_err("Failed sending message. Error: %d\n", result);
        } else {
            pr_info("Successfully sending client message.\n");
        }
    } else {
        pid_server = info->snd_portid;
        pr_info("Set PID Server %d\n", pid_server);
        result = 0;
    }

    return result;
}
