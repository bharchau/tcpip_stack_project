#include "graph.h"
#include "CommandParser/libcli.h"

extern graph_t* build_first_topo();
graph_t *topo = NULL;
int main(int argc, char **argv)
{
//    graph_t *topo = calloc(1, sizeof(graph_t));
    nw_init_cli();
    topo = build_first_topo();
    start_shell();
    return 0;
//    dump_graph(topo);
//    dump_nw_graph(topo);
}
