#include "comm.h"
#include "graph.h"
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h> /*for struct hostent*/
#include "net.h"
#include <unistd.h> // for close

static unsigned int udp_port_number = 40000;
static char recv_buffer[MAX_PACKET_BUFFER_SIZE];
static char send_buffer[MAX_PACKET_BUFFER_SIZE];

static unsigned int 
get_next_udp_port_number(void)
{
    return (udp_port_number++);
}

void 
init_udp_socket(node_t *node)
{
    int udp_sock_fd;
    struct sockaddr_in node_addr;

    if (node->udp_port_number) {
        /* This node UDP setup is already done. please go back */
        return;
    }
    node->udp_port_number = get_next_udp_port_number();

    udp_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); /* Communication type SOCK_DGRAM is used for UDP. TCP uses SOCK_STREAM */
    if (udp_sock_fd == -1) {
        printf("Socket Creation Failed for node %s\n");
        return;
    }
    node_addr.sin_family = AF_INET;
    node_addr.sin_port = node->udp_port_number;
    node_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(udp_sock_fd, (struct sockaddr *)&node_addr, sizeof(struct sockaddr)) == -1) {
        printf("Error: socket bind failed for Node %s\n", node->node_name);
        return;
    }
    node->udp_sock_fd = udp_sock_fd;

}

static int 
_send_pkt_out(int sock_fd, char * pkt_data, unsigned int pkt_size,
              unsigned int dst_udp_port_no)
{
    int rc;
    struct sockaddr_in dest_addr;

    struct hostent *host = (struct hostent *) gethostbyname("127.0.0.1");
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = dst_udp_port_no;
    dest_addr.sin_addr  = *((struct in_addr *)host->h_addr);

    rc = sendto(sock_fd, pkt_data, pkt_size, 0, 
                (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
    return rc;
}

int 
send_pkt_out(char *pkt, unsigned int pkt_size, interface_t *interface)
{
    int rc = 0;
    node_t *sending_node = interface->att_node;
    node_t *nbr_node = get_nbr_node(interface);
    if (!nbr_node) {
        return -1;
    }

    unsigned int dst_udp_port_no = nbr_node->udp_port_number;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    char *pkt_with_aux_data = NULL;
    interface_t *other_interface = &interface->link->intf1 == interface ? \
                                   &interface->link->intf2 : &interface->link->intf1;

    memset(send_buffer, 0 , MAX_PACKET_BUFFER_SIZE);
    pkt_with_aux_data = send_buffer;
    strncpy(pkt_with_aux_data, other_interface->if_name, IF_NAME_SIZE);
    pkt_with_aux_data[IF_NAME_SIZE - 1] = '\0';

    memcpy(pkt_with_aux_data + IF_NAME_SIZE, pkt, pkt_size);

    rc = _send_pkt_out(sock, pkt_with_aux_data, pkt_size + IF_NAME_SIZE, 
                       dst_udp_port_no);
    
    close(sock);
    return rc;
    
}

int
pkt_receive(node_t *node, interface_t *interface, char *pkt, 
            unsigned int pkt_size)
{
    /* 
     * This is a entry point to the Data link layer from Physical Layer.
     * Ingress journey of PKT starts from here in TCP/IP stack
     */
     printf("\nMsg recevied = %s, on node = %s, intf = %s\n\n", pkt, node->node_name, interface->if_name);
     return 0;
}
static void
_pkt_receive(node_t *receving_node, 
            char *pkt_with_aux_data, 
            unsigned int pkt_size)
{
    char * recv_intf_name = pkt_with_aux_data;
    interface_t *recv_intf = get_node_if_by_name(receving_node, recv_intf_name);
    if (!recv_intf) {
        printf("Error: Pkt recvd on unknown interface \n");
        return;
    }

    pkt_receive(receving_node, recv_intf, pkt_with_aux_data + IF_NAME_SIZE,
        pkt_size - IF_NAME_SIZE); // Actuall pkt size after removing if name from buffer
}

static void *
_network_start_pkt_receiver_thread(void *arg)
{
    node_t *node;
    graph_t *topo = (graph_t *) arg;
    glthread_t *curr;
    fd_set active_sock_fd_set, backup_sock_fd_set;
    int sock_max_fd = 0;
    int bytes_recvd = 0;
    struct sockaddr_in sender_addr;
    int addr_len = sizeof(struct sockaddr);
    
    FD_ZERO(&active_sock_fd_set);
    FD_ZERO(&backup_sock_fd_set);

    /* Need to check all node UDP socket if they recevied any pkt */
    ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr) {
        node = graph_glue_to_node(curr);
        if (!node->udp_sock_fd) {
            continue;
        }
        /* updae max socket fd */
        if (node->udp_sock_fd > sock_max_fd) {
            sock_max_fd = node->udp_sock_fd;
        }
        /* Set all node socket FDs which are present to backup_sockfd_set */ 
        FD_SET(node->udp_sock_fd, &backup_sock_fd_set);
    } ITERATE_GLTHREAD_END(&topo->node_list, curr);

    /* Listen to all node udp socket fds */
    while(1) {
        memcpy(&active_sock_fd_set, &backup_sock_fd_set, sizeof(fd_set));
		/* Blocking call, utill any data arrived to node udp socket which is in active_sock_fd_set */
        select(sock_max_fd + 1, &active_sock_fd_set,NULL,NULL,NULL);
        
        ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr) {
            node = graph_glue_to_node(curr);
            if (FD_ISSET(node->udp_sock_fd, &active_sock_fd_set)) {
                memset(recv_buffer, 0, MAX_PACKET_BUFFER_SIZE);
                bytes_recvd = recvfrom(node->udp_sock_fd, (char *)recv_buffer,
                MAX_PACKET_BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &addr_len);

                _pkt_receive(node, recv_buffer, bytes_recvd);
            }
        } ITERATE_GLTHREAD_END(&top->node_list, curr);
    }
}

void
network_start_pkt_receiver_thread(graph_t *topo)
{
    pthread_attr_t attr;
    pthread_t recv_pkt_thread;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&recv_pkt_thread, &attr, 
                   _network_start_pkt_receiver_thread,
                   (void *) topo);
}
