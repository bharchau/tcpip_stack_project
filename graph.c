#include "graph.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>


extern void 
init_udp_socket(node_t *node);

void 
insert_link_between_two_nodes(node_t *node1, node_t* node2, 
                            char *from_if_name,char *to_if_name,
                            unsigned int cost)
{
    link_t *link = calloc(1, sizeof(link_t));
    int empty_intf_slot;

    strncpy(link->intf1.if_name, from_if_name, IF_NAME_SIZE);
    link->intf1.if_name[IF_NAME_SIZE - 1] = '\0';
    strncpy(link->intf2.if_name, to_if_name, IF_NAME_SIZE); 
    link->intf2.if_name[IF_NAME_SIZE - 1] = '\0';

    link->intf1.link = link;
    link->intf2.link = link;

    link->intf1.att_node = node1;
    link->intf2.att_node = node2;
    link->cost = cost;

    /* 
     * interface link properties is created.
     * Now insert these links to the empty interface node.
     */
    empty_intf_slot = get_node_intf_available_slot(node1);
    node1->intf[empty_intf_slot] = &link->intf1;
    
    empty_intf_slot = get_node_intf_available_slot(node2);
    node2->intf[empty_intf_slot] = &link->intf2;

    init_intf_nw_prop(&link->intf1.intf_nw_props);
    init_intf_nw_prop(&link->intf2.intf_nw_props);
    
    interface_assign_mac_address(&link->intf1);
    interface_assign_mac_address(&link->intf2);
}
graph_t*
create_new_graph(char *topology_name)
{
    graph_t *graph = calloc(1,sizeof(graph_t));
    strncpy(graph->topology_name, topology_name, 32);
    graph->topology_name[31] = '\0';
    
    init_glthread(&graph->node_list);
    return graph;
}

node_t*
create_graph_node(graph_t *graph, char *node_name)
{
    node_t *node = calloc(1, sizeof(node_t));
    strncpy(node->node_name, node_name,NODE_NAME_SIZE);
    node->node_name[NODE_NAME_SIZE - 1] = '\0';

    /* Create unique UDP port number and UDP socket */
    init_udp_socket(node);

    init_node_nw_prop(&node->node_nw_prop);
    init_glthread(&node->graph_glue);
    glthread_add_next(&graph->node_list, &node->graph_glue);
    return node;
}

void dump_graph(graph_t *graph)
{
    node_t *node;
    glthread_t *curr;

    printf("%s:\n", graph->topology_name);
    printf("Node Name\tLB Addr\t\tInterface Name\t\tIP Addr\t\tMAC Addr\t\t\tNbr Node\tLocal Node\tCost\n");
    printf("===========================================================================================================================================\n");
    ITERATE_GLTHREAD_BEGIN(&graph->node_list, curr) {
        node = graph_glue_to_node(curr);
        dump_node(node);
    } ITERATE_GLTHREAD_END(&graph->node_list, curr);
}

void dump_node(node_t *node) 
{
    unsigned int i = 0;
    interface_t *intf;

    printf("\r%s\t\t%s/32\t\t", node->node_name,node->node_nw_prop.lb_addr.ip_addr);

    for (i = 0; i < MAX_INTF_PER_NODE; i++) {
        intf = node->intf[i];
        if(!intf) break; // Empty interface slot
        dump_interface(intf);
    }
    printf("\n");
}

void dump_interface(interface_t *interface)
{
    link_t *link = interface->link;
    node_t *nbr_node = get_nbr_node(interface);

    printf("%s\t\t%s/%u\t%3u:%3u:%3u:%3u:%3u:%3u\t\t%s\t\t%s\t\t%u\n\t\t\t\t\t",
            interface->if_name, IF_IP(interface), interface->intf_nw_props.mask, IF_MAC(interface)[0], IF_MAC(interface)[1], IF_MAC(interface)[2],IF_MAC(interface)[3], IF_MAC(interface)[4], IF_MAC(interface)[5], nbr_node->node_name, 
            interface->att_node->node_name, link->cost);
}
