
#ifndef __GRAPH__
#define __GRAPH__

#include <assert.h>
#include "gluethread/glthread.h"
#include <stdlib.h>
#include <string.h>
#include "net.h"

typedef struct node_ node_t;
typedef struct link_ link_t;

#define NODE_NAME_SIZE   16
#define IF_NAME_SIZE     16
#define MAX_INTF_PER_NODE   10


typedef struct interface_ {
    char if_name[IF_NAME_SIZE];
    struct link_ *link;
    struct node_ *att_node;
    intf_nw_props_t intf_nw_props;
} interface_t;

struct link_ {
    interface_t intf1; // Host side connection of node
    interface_t intf2; // Peer side connection of nbr node
    unsigned int cost;
};

struct node_ {
    char node_name[NODE_NAME_SIZE];
    interface_t *intf[MAX_INTF_PER_NODE];
    glthread_t graph_glue;
    node_nw_prop_t node_nw_prop;
};

typedef struct graph_ {
    char topology_name[32];
    glthread_t node_list;
}graph_t;

GLTHREAD_TO_STRUCT(graph_glue_to_node, node_t, graph_glue);
/* 
 *Check who is my neighbour node/device connected to interface_t *interface
 */
static inline node_t*
get_nbr_node(interface_t *interface)
{
    link_t *link;
    assert(interface->att_node);
    assert(interface->link);

    /* Get interface link
     * Figure out which interface this link belongs to (host or peer)
     * i.e is this link belonging to intf1 or intf2.
     */
    link = interface->link;
    if (&link->intf1 == interface) {
        /* My nbr is intf2 */
        return (link->intf2.att_node);
    } else {
        /* My nbr is intf1 */
        return (link->intf1.att_node);
        
    }
}

/* 
 * Check which interface are not connected to node/device 
 */
static inline int 
get_node_intf_available_slot(node_t *node)
{
    int intf_idx;

    /* 
     * Loop the node interface to MAX and check if any node intf pointer is NULL
     * If NULL, it means node interface is available for connection.
     */
    for (intf_idx = 0; intf_idx < MAX_INTF_PER_NODE; intf_idx++) {
        if(node->intf[intf_idx]) {
            continue;
        }
        return intf_idx;
    }

    /* No empty interface in node is avoilable. */
    return -1;
}

static inline interface_t*
get_node_if_by_name(node_t *node, char *if_name)
{
    int i;
    for (i = 0; i < IF_NAME_SIZE; i++) {
        if(strncmp(node->intf[i]->if_name, if_name, IF_NAME_SIZE) == 0) {
            return node->intf[i];
        }
    }
    return NULL;
}

static inline node_t*
get_node_by_node_name(graph_t *topo, char *node_name)
{
    node_t *node;
    glthread_t *curr;

    ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr){
        node = graph_glue_to_node(curr);
        if(strncmp(node->node_name, node_name, strlen(node_name)) == 0)
            return node;
    } ITERATE_GLTHREAD_END(&topo->node_list, curr);
    return NULL;
}
node_t *
create_graph_node(graph_t* graph, char* node_name);

graph_t *
create_new_graph(char* topology_name);

void 
insert_link_between_two_nodes(node_t * node1, node_t* node2,
                              char *from_if_name,
                              char *to_if_name,
                              unsigned int cost);

/*Display Routines*/
void dump_graph(graph_t *graph);
void dump_node(node_t *node);
void dump_interface(interface_t *interface);

#endif 
