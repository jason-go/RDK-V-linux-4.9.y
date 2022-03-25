#ifdef LINUX_OS

#include <linux/err.h>
#include <linux/in.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/delay.h>

#include "kernelcalls.h"

static int q_id[10] = {0};

int gx_queue_create(unsigned int queue_depth, unsigned int data_size)
{
	int                     tmpSkt;
	int                     returnStat;
	struct sockaddr_in      servaddr;
	struct socket *sock;
	int i = 0;

	for (i = 0; i < 10; i++)
	{
		if (q_id[i] == 0)
			break;
	}

	tmpSkt = sock_create(AF_INET, SOCK_DGRAM, 0, &sock);
	if (tmpSkt < 0)
	{
		sock_release(sock);
		return tmpSkt;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_port = htons(55000 + i);
	servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	/*
	 ** bind the input socket to a pipe
	 ** port numbers are GXCORE_BASE_PORT + queue_id
	 */
	returnStat = security_socket_bind(sock,(struct sockaddr *)&servaddr, sizeof(servaddr));

	if ( returnStat == -1 )
	{
		sock_release(sock);
		return returnStat;
	}

	q_id[i] = (int)sock;
	/* store socket handle */
	return q_id[i];
}

int gx_queue_delete (int queue_id)
{
	int i;

	sock_release((struct socket *)queue_id);

	for (i = 0; i < 10; i++)
	{
		if (q_id[i] == queue_id)
		{
			q_id[i] = 0;
			return 0;
		}
	}
	return -1;
}

int gx_queue_get(int queue_id, char *data, unsigned int size, int timeout)
{
	int sizeCopied;
	struct msghdr msg;
	struct iovec iov;
	char address[128];
	if(data == NULL || size == 0)
		return -2;

	msg.msg_control=NULL;
	msg.msg_controllen=0;
	msg.msg_iovlen=1;
	msg.msg_iov=&iov;
	iov.iov_len=size;
	iov.iov_base=data;
	msg.msg_name=address;
	msg.msg_namelen=128;

	/* Read the socket for data */
	if (timeout == 0 || timeout == -1)
	{
		if (size != sock_recvmsg((struct socket *)queue_id, &msg, size, 0))
			return -1;
	}
	else
	{
		/* timeout */
		int timeloop;

		for ( timeloop = timeout; timeloop > 0; timeloop = timeloop - 100 ) {
			sizeCopied = sock_recvmsg((struct socket *)queue_id, &msg, size, 0);

			if ( sizeCopied == size ) {
				return 0;
			}
			else if (sizeCopied == -1) {
				/* Sleep for 100 milliseconds */
				msleep(100);
			}
			else {
				return -2;
			}
		}
		return -3;
	} /* END timeout */

	return 0;
}

int gx_queue_put(int queue_id, char *data, unsigned int size)
{
	struct sockaddr_in serva;
	struct socket *sock;
	int bytesSent    = 0;
	int tempSkt      = 0;
	struct msghdr msg;
	struct iovec iov;
	char address[128];
	int i;

	/* Check Parameters */
	if (data == NULL || size == 0)
		return -2;

	for (i = 0; i < 255; i++)
	{
		if (q_id[i] == queue_id)
			break;
	}

	iov.iov_base=data;
	iov.iov_len=size;
	msg.msg_name=NULL;
	msg.msg_iov=&iov;
	msg.msg_iovlen=1;
	msg.msg_control=NULL;
	msg.msg_controllen=0;
	msg.msg_namelen=0;

	/* specify the IP addres and port number of destination */
	memset(&serva, 0, sizeof(serva));
	serva.sin_family      = AF_INET;
	serva.sin_port        = htons(55000 + i);
	serva.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	/* open a temporary socket to transfer the packet to MR */
	tempSkt = sock_create(AF_INET, SOCK_DGRAM, 0, &sock);
	if (tempSkt < 0)
	{
		sock_release(sock);
		return tempSkt;
	}

	/* send the packet to the message router thread (MR) */
	if(__copy_from_user((struct sockaddr *)&address[0],(struct sockaddr *)&serva, sizeof(serva)))
        {
                return -1;
        }

	msg.msg_name=address;
	msg.msg_namelen=sizeof(serva);

	bytesSent = sock_sendmsg(sock, &msg, size);
	if( bytesSent != size )
		return -1;

	/* close socket */
	sock_release(sock);

	return 0;
}

#endif

