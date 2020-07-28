#include "graph.h"
#include "CommandParser/libcli.h"

extern graph_t* build_first_topo();
graph_t *topo = NULL;
int main(int argc, char **argv)
{
//    graph_t *topo = calloc(1, sizeof(graph_t));
    char msg[] = "hello, how are you?";
    nw_init_cli();
    topo = build_first_topo();
    
    sleep(2);
    node_t *snode = get_node_by_node_name(topo, "R0_re"); // Sender node
    interface_t *oif = get_node_if_by_name(snode, "eth0/0"); // out interface

    send_pkt_out(msg, strlen(msg), oif);
    start_shell();
    return 0;
//    dump_graph(topo);
//    dump_nw_graph(topo);
}
