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
static int seq_client = 0;
static int seq_server = 0;

static int send_message(const char *msg, int pid, int seq);
static int calc_cmd_client(struct sk_buff *skb, struct genl_info *info);
static int calc_cmd_server(struct sk_buff *skb, struct genl_info *info);

static const struct nla_policy calc_policy[ATTR_MAX + 1] = {
    [ATTR_MSG] = {.type = NLA_STRING, .len = 1024},
};

static const struct genl_ops calc_ops[] = {
    {
        .cmd = COMMAND_CLIENT,
        .flags = 0,
        .policy = calc_policy,
        .doit = calc_cmd_client,
        .dumpit = NULL,
    },
    {
        .cmd = COMMAND_SERVER,
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
    .ops = calc_ops,
    .n_ops = ARRAY_SIZE(calc_ops),
};
static int __init calc_init(void) {
    int ret;

    pr_info("Initializing Generic Netlink family \"%s\"\n", FAMILY_NAME);

    ret = genl_register_family(&calc_family);
    if (ret) {
        pr_err("Failed to register Generic Netlink family \"%s\": %d\n", FAMILY_NAME, ret);
        return ret;
    }
    pr_info("Generic Netlink family \"%s\" registered successfully with family ID: %d\n",
            FAMILY_NAME, calc_family.id);
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
    struct sk_buff *skb;
    void *hdr;

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
    struct nlattr *na;
    char *msg;
    int result = -EINVAL;

    na = info->attrs[ATTR_MSG];
    if (!na) {
        pr_err("Received a message with no payload.\n");
        return result;
    }

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
        pr_info("No server registered yet. Message will be dropped\n");
        result = 0;
    }

    return result;
}

static int calc_cmd_server(struct sk_buff *skb, struct genl_info *info) {
    struct nlattr *na;
    char *msg;
    int result = -EINVAL;

    na = info->attrs[ATTR_MSG];
    if (!na) {
        pr_err("Received a message with no payload.\n");
        return result;
    }

    msg = nla_data(na);

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