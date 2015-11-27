/* This the kernel module 
   this is used for Spi
   this is under GNU/GPL 
   You can use it 
*/



#include <linux/module.h>
#include <net/sock.h> 
#include <linux/netlink.h> 
#include <linux/skbuff.h> 
#include <linux/kernel.h> 
#include<linux/init.h> 
#include <linux/moduleparam.h> 
#include <linux/gpio.h> 
#include <linux/spi/spi.h> 
#include <linux/fs.h>
#include <linux/cdev.h> 
#include <linux/errno.h>
#include <linux/irq.h> 
#include <linux/interrupt.h> 
#include <linux/time.h>
#include <linux/ioport.h>
#include <asm/io.h>

#define NETLINK_USER 31   // for client application 
#define   AM33XX_CONTROL_BASE		0x44e10000  //for using gpio of beaglebone black 

struct sock *nl_sk = NULL;
static uint spi_bus = 1;
static uint spi_cs = 1;
static uint speed_hz = 1500000; /* found experimentally */
static struct spi_device *spi_device_1;
uint16_t gpio_pin_chipselect = 60 ;  //external chipselect for controlling the slave   

//for receve and send of message to client application 
static void nl_recv_send_msg(struct sk_buff *skb)
{
    static uint16_t  *buf ;
    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    int res;
    uint32_t ret ;
    uint32_t *read_buf;
	
    printk(KERN_INFO "Entering to kernel module \n");
    nlh=(struct nlmsghdr*)skb->data;
    printk(KERN_INFO "Netlink received msg payload: %s\n",(char*)nlmsg_data(nlh));
    pid = nlh->nlmsg_pid; /*pid of sending process */
    buf  =  nlmsg_data(nlh);
    printk(KERN_INFO"data of buf before sending %x %x of size %d\n",*buf,*(buf+1), sizeof(buf));

	gpio_set_value(gpio_pin_chipselect,0);  // manually controlling chipselect by making it low 
	
	/* spi_write function sends data using our spi */	
    res =  spi_write(spi_pot_device,&buf,sizeof buf) ;
    printk(KERN_INFO "Write Result %d Size of Ret is %d\n",res,sizeof(ret)) ;
	/* spi_read to read the data form our spi */
	res = spi_read(spi_pot_device,&ret,sizeof ret);
    printk(KERN_INFO "Got Result and ret val: %d  %d\n",ret,res) ;

    gpio_set_value(gpio_pin_chipselect,1);  // making again chipselect high 
 
    read_buf  = &ret;
    msg_size= strlen(read_buf);
    printk(KERN_INFO " buf value  = %x \n",*read_buf);

    skb_out = nlmsg_new(msg_size,0);

    if(!skb_out)
    {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }
    nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);

    NETLINK_CB(skb_out).dst_group = 0; // not in mcast group
    strncpy(nlmsg_data(nlh),read_buf,msg_size);
    res=nlmsg_unicast(nl_sk,skb_out,pid);
    if(res<0)
        printk(KERN_INFO "Error while sending bak to user\n");

}

static int setup_pinmux(void)
{
   int i,err;
   static u32 pins[] = {
      AM33XX_CONTROL_BASE + 0x878,   // gpio1_28 (beaglebone p9/12)
      0x7 | (2 << 3),                //       mode 7 , PULLUP, OUTPUT   according to data sheet 
   };

   for (i=0; i<2; i+=2) {
      void* addr = ioremap(pins[i], 2);  // remap of io 

      if (NULL == addr)
         return -EBUSY;

      iowrite32(pins[i+1], addr);
      iounmap(addr);
   }
   gpio_pin_chipselect =60 ;
   err = gpio_request_one(gpio_pin_chipselect, GPIOF_OUT_INIT_HIGH,"chip select");
   return err;
}

static inline __init int spi_int(void)
{
	int ret;
	struct spi_board_info spi_device_info_1 = {
                .modalias = "spi_int",
                .max_speed_hz = speed_hz,
                .bus_num = spi_bus,
                .mode = 1,  // depends on the mode we have to use this 
        };
		
 	struct spi_master *master1;  // for specifying I am using this driver for spi_master
	master1 = spi_busnum_to_master( spi_device_info_1.bus_num ); //assigning bus_number to master 
        if( !master1 )
        	{
             		printk( KERN_INFO"spi bus information for master  not found"); 
                	return -ENODEV; // error to the kernel 
       		}
    spi_device_1 = spi_new_device( master1, &spi_device_info_1 ); // creation of spi device in kernel module 
        if( !spi_device_1 )
		{
            printk(KERN_INFO"spi device not able to create "); // this error is generated if we are using the same channel which bydefault spi is using
            return -ENODEV;
		}
		
    spi_device_1->bits_per_word = 32;  // specifying the lenght of word that can spi transfer
	spi_device_1->cs_gpio =60;  // for external GPIO as chip select
    ret = spi_setup( spi_pot_device);
        if( ret )
                spi_unregister_device( spi_device_1 );
        else
                printk( KERN_INFO "%d bus no andcs no %d", spi_bus, gpio_pin_chipselect );
        return ret;
}

static inline void spi_release(void) 
{
        spi_unregister_device( spi_pot_device );

}

static int __init spi_init(void)
{
    int ret ;
	ret = spi_int();
	if( ret )
	{
		printk(KERN_INFO "SPI was not able to set ");
		return ret;
	}
    ret = setup_pinmux();  // way of setting up gpio in linux kernel 
    if(ret)
	{
	  printk(KERN_INFO "CHIP SELECT OF GPIO WAS NOT ABLE TO SET");
    }
    
	static struct netlink_kernel_cfg cfg;

    printk("Entering: %s\n",__FUNCTION__);
//  nl_sk=netlink_kernel_create(&init_net, NETLINK_USER, NULL);  //used if kernel version is less than 2.60
    cfg.input = nl_recv_send_msg;  //for kernel version < 2.60
    nl_sk=netlink_kernel_create(&init_net, NETLINK_USER,  &cfg );
  
  if(!nl_sk)
    {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }
    return 0;
}

static void __exit spi_exit(void){
    printk(KERN_INFO "exiting SPI module\n");
    spi_release(); 
    gpio_free(gpio_pin_chipselect);
    netlink_kernel_release(nl_sk);
}

module_init(spi_init);
module_exit(spi_exit);

MODULE_LICENSE("GPL");
MODULE_OWNER("SUMIK");
