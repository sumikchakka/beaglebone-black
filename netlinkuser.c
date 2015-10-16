/*
   Kernel module for netlink
*/
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#define NETLINK_USER 31

struct sock *nl_sk = NULL;
int pid;

static void hello_nl_recv_msg(struct sk_buff *skb)
{
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int msg_size;
    char *msg1 ="hie"  ;
    int res;
    nlh = (struct nlmsghdr *)skb->data;
    printk(KERN_INFO "Netlink received msg payload:%s\n", (char *)nlmsg_data(nlh));
    pid = nlh->nlmsg_pid; /*pid of sending process */
    msg_size = strlen(msg1);;
    skb_out = nlmsg_new(msg_size, 0);

    if (!skb_out)
    {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }
    printk(KERN_INFO "Putting Message\n") ;
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0; // not in mcast group 
    strncpy(nlmsg_data(nlh),msg1, msg_size);
    res = nlmsg_unicast(nl_sk, skb_out, pid);

    if (res < 0)
        printk(KERN_INFO "Error while sending bak to user\n");
}

static int __init netlinkuser_init(void)
{
    printk("Entering: %s\n", __FUNCTION__);

//    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, hello_nl_recv_msg); // kernel verion below 2.6
    static struct netlink_kernel_cfg cfg = {0};
    cfg.input = hello_nl_recv_msg;
    nl_sk=netlink_kernel_create(&init_net, NETLINK_USER, &cfg );
   if (!nl_sk)
    {

        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }

   return 0;
}

static void __exit netlinkuser_exit(void)
{
    printk(KERN_INFO "Exiting netlinkuser module\n");
    netlink_kernel_release(nl_sk);
}

module_init(netlinkuser_init); module_exit(netlinkuser_exit);

MODULE_LICENSE("GPL");
MODULE_NAME("Sumik");
