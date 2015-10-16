#include <stdio.h> 
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>

#define   MAX_BUF  64

#define NETLINK_USER 31    //kernel module and application should have same netlink_user 
#define MAX_PAYLOAD 1024  /* maximum payload size*/

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;

int main(int argc, const char *argv[])
{
	struct pollfd fdset[1];
  char *MsgBuf ;

  sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if(sock_fd<0)
	{
      printf("socket cannot open \n");
      return -1;
	}
   nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
  
	  memset(&src_addr, 0, sizeof(src_addr));
	  src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();  /* self pid */
    /* interested in group 1<<0 */
	  bind(sock_fd, (struct sockaddr*)&src_addr,sizeof(src_addr)); //creating socket 

    memset(&dest_addr, 0, sizeof(dest_addr));
	  memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;   /* For Linux Kernel */
	  dest_addr.nl_groups = 0; /* unicast */
	
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	  nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;
    strcpy(NLMSG_DATA(nlh), "First time data send "); //sending data to kenrel module 
	  iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
	  msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    sendmsg(sock_fd,&msg,0) ;
	  recvmsg(sock_fd,&msg,0) ;
    MsgBuf = (char *) NLMSG_DATA(nlh);
    printf("Msg recived %c \n",*MsgBuf);
      
   close(sock_fd);
}

