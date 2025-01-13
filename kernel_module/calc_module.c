#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <net/genetlink.h>

#define FAMILY_NAME "calc_family"
#define COMMAND_CLIENT 1
#define COMMAND_SERVER 2

/**
 * @brief Определение атрибутов для Generic Netlink.
 *
 * Содержит список атрибутов, используемых в Netlink-сообщениях.
 */
enum {
    ATTR_UNSPEC, /**< Неопределенный атрибут, используется как заглушка. */
    ATTR_MSG,    /**< Основной атрибут, содержащий полезную нагрузку сообщения (строка). */
    __ATTR_MAX,  /**< Максимальное значение для атрибутов (служебный элемент). */
};
#define ATTR_MAX (__ATTR_MAX - 1) /**< Корректное определение верхней границы атрибутов. */

static __u32 pid_client = 0;
static __u32 pid_server = 0;
static int seq_client = 0;
static int seq_server = 0;

/**
 * @brief Отправляет сообщение через Netlink.
 *
 * Создает сообщение с указанной полезной нагрузкой, PID и номером последовательности,
 * а затем отправляет его через Generic Netlink.
 *
 * @param msg Строка с сообщением, которое нужно отправить.
 * @param pid PID получателя сообщения.
 * @param seq Номер последовательности для сообщения.
 *
 * @return 0 при успешной отправке, отрицательное значение кода ошибки в случае сбоя.
 */
static int send_message(const char *msg, int pid, int seq);
/**
 * @brief Обработчик команд от клиента.
 *
 * Обрабатывает сообщения, полученные от клиента, и перенаправляет их серверу
 * (если сервер зарегистрирован). Если сервер не зарегистрирован, отправляется сообщение об ошибке клиенту.
 *
 * @param skb Указатель на структуру sk_buff, содержащую сообщение.
 * @param info Структура, содержащая информацию о параметрах сообщения.
 *
 * @return 0 при успешной обработке, отрицательное значение кода ошибки в случае сбоя.
 */
static int calc_cmd_client(struct sk_buff *skb, struct genl_info *info);
/**
 * @brief Обработчик команд от сервера.
 *
 * Обрабатывает сообщения, полученные от сервера, и отправляет их клиенту
 * (если клиент зарегистрирован). Если сервер еще не зарегистрирован,
 * он регистрируется и сообщение отправляется самому серверу.
 *
 * @param skb Указатель на структуру sk_buff, содержащую сообщение.
 * @param info Структура, содержащая информацию о параметрах сообщения.
 *
 * @return 0 при успешной обработке, отрицательное значение кода ошибки в случае сбоя.
 */
static int calc_cmd_server(struct sk_buff *skb, struct genl_info *info);

/**
 * @brief Политика проверки атрибутов для Generic Netlink.
 *
 * Определяет правила валидации атрибутов, которые используются в сообщениях Netlink.
 * Здесь задается тип и ограничения атрибута `ATTR_MSG`.
 */
static const struct nla_policy calc_policy[ATTR_MAX + 1] = {
    [ATTR_MSG] =
        {
            .type = NLA_STRING, /**< Тип атрибута - строка. */
            .len = 1024,        /**< Максимальная длина строки - 1024 байта. */
        },
};
static const struct nla_policy calc_policy[ATTR_MAX + 1] = {
    [ATTR_MSG] = {.type = NLA_STRING, .len = 1024},
};

/**
 * @brief Список операций для Generic Netlink.
 *
 * Определяет команды и соответствующие обработчики (doit) для клиента и сервера.
 */
static const struct genl_ops calc_ops[] = {
    {
        .cmd = COMMAND_CLIENT,   /**< Команда для обработки сообщений клиента. */
        .flags = 0,              /**< Флаги для команды (нет специальных флагов). */
        .policy = calc_policy,   /**< Политика валидации атрибутов команды. */
        .doit = calc_cmd_client, /**< Указатель на функцию-обработчик команды клиента. */
        .dumpit = NULL,          /**< Поле для функции выгрузки (не используется). */
    },
    {
        .cmd = COMMAND_SERVER,   /**< Команда для обработки сообщений сервера. */
        .flags = 0,              /**< Флаги для команды (нет специальных флагов). */
        .policy = calc_policy,   /**< Политика валидации атрибутов команды. */
        .doit = calc_cmd_server, /**< Указатель на функцию-обработчик команды сервера. */
        .dumpit = NULL,          /**< Поле для функции выгрузки (не используется). */
    },
};

/**
 * @brief Описание семейства Generic Netlink.
 *
 * Определяет параметры семейства Netlink (имя, версию, операции, и пр.).
 */
static struct genl_family calc_family = {
    .name = FAMILY_NAME,           /**< Название семейства Netlink. */
    .version = 1,                  /**< Версия семейства Netlink. */
    .maxattr = ATTR_MAX,           /**< Максимальное количество поддерживаемых атрибутов. */
    .module = THIS_MODULE,         /**< Указатель на текущий модуль ядра. */
    .ops = calc_ops,               /**< Список операций (команд), поддерживаемых семейством. */
    .n_ops = ARRAY_SIZE(calc_ops), /**< Количество операций, объявленных в calc_ops. */
};
static int __init calc_init(void) {
    int ret;

    pr_info("Initializing Generic Netlink family \"%s\"\n", FAMILY_NAME);

    ret = genl_register_family(&calc_family);
    if (ret) {
        pr_err("Failed to register Generic Netlink family \"%s\": %d\n", FAMILY_NAME, ret);
        return ret;
    }
    pr_info("Generic Netlink family \"%s\" registered successfully with family ID: %d\n", FAMILY_NAME, calc_family.id);
    return 0;
}

static void __exit calc_exit(void) {
    pr_info("Unregistering Generic Netlink family \"%s\"\n", FAMILY_NAME);

    genl_unregister_family(&calc_family);

    pr_info("Generic Netlink family \"%s\" unregistered successfully\n", FAMILY_NAME);
}

module_init(calc_init);
module_exit(calc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Khromenok Roman");
MODULE_DESCRIPTION("Kernel module for Generic Netlink communication");
MODULE_VERSION("1.0");

static int send_message(const char *msg, int pid, int seq) {
    struct sk_buff *skb = NULL;
    void *hdr = NULL;

    if (!msg) {
        pr_err("Message is NULL, sending will be skipped\n");
        return -EINVAL;
    }

    if (pid == 0) {
        pr_err("Invalid PID specified.\n");
        return -EINVAL;
    }

    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (!skb) {
        pr_err("Failed to allocate sk_buff.\n");
        return -ENOMEM;
    }

    hdr = genlmsg_put(skb, 0, seq, &calc_family, 0, COMMAND_SERVER);
    if (!hdr) {
        pr_err("Failed to create Generic Netlink header.\n");
        kfree_skb(skb);
        return -ENOMEM;
    }

    if (nla_put_string(skb, ATTR_MSG, msg)) {
        pr_err("Failed to add message payload.\n");
        kfree_skb(skb);
        return -EMSGSIZE;
    }

    genlmsg_end(skb, hdr);

    int ret = genlmsg_unicast(&init_net, skb, pid);
    if (ret) {
        pr_err("Failed to send message to PID %d, seq %d. Error: %d\n", pid, seq, ret);
    } else {
        pr_info("Message sent to PID %d with sequence number %d\n", pid, seq);
    }
    return ret;
}

static int calc_cmd_client(struct sk_buff *skb, struct genl_info *info) {
    struct nlattr *na = NULL;
    char *msg = NULL;
    char const *message_pass = "No server registered yet. Message will be dropped";
    int result = -EINVAL;

    na = info->attrs[ATTR_MSG];
    if (!na) {
        pr_err("Received a message with no payload.\n");
        return result;
    }

    //@todo: не самое хорошее решение, сходу не могу придумать, как красиво сделать
    pid_client = info->snd_portid;
    seq_client = nlmsg_hdr(skb)->nlmsg_seq;
    pr_info("Registered client with PID %d and sequence number %d\n", pid_client, seq_client);

    msg = nla_data(na);
    pr_info("Message from client (PID %d): %s\n", pid_client, msg);

    if (pid_server != 0) {
        result = send_message(msg, pid_server, seq_server);
        if (result != 0) {
            pr_err("Failed to forward client message to server. Error: %d\n", result);
        }
    } else {
        pr_info("%s\n", message_pass);
        result = send_message(message_pass, pid_client, seq_client);
        if (result != 0) {
            pr_err("Failed to sending error message. Error: %d\n", result);
        }
        result = 0;
    }

    return result;
}

//@todo: из описания задания не понятно как надо обрабатывать разрыв соединия,
// в текущей реализации надо выгружать и загружать модуль ядра, если сервер разъединился
static int calc_cmd_server(struct sk_buff *skb, struct genl_info *info) {
    struct nlattr *na = NULL;
    char *msg = NULL;
    int result = -EINVAL;

    na = info->attrs[ATTR_MSG];
    if (!na) {
        pr_err("Received a message with no payload.\n");
        return result;
    }

    msg = nla_data(na);

    //@todo: не самое хорошее решение, сходу не могу придумать, как красиво сделать
    if (pid_server == 0) {
        pid_server = info->snd_portid;
        seq_server = nlmsg_hdr(skb)->nlmsg_seq;
        pr_info("Registered server with PID %d and sequence number %d\n", pid_server, seq_server);

        result = send_message(msg, pid_server, seq_server);
        if (result) {
            pr_err("Failed to send initial server message. Error: %d\n", result);
        }
        return result;
    } else {
        result = send_message(msg, pid_client, seq_client);
        if (result) {
            pr_err("Failed to forward server message to client. Error: %d\n", result);
        } else {
            pr_info("Message from server forwarded to client.\n");
        }
        return result;
    }
}