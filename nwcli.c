#include "graph.h"
#include <stdio.h>
#include "CommandParser/libcli.h"
#include "CommandParser/cmdtlv.h"
#include "cmdcodes.h"

extern graph_t *topo;

/*Generic Topology Commands*/
static int
show_nw_topology_handler(param_t *param, ser_buff_t *tlv_buf, 
                         op_mode enable_or_disable)
{

    int CMDCODE = -1;
    CMDCODE = EXTRACT_CMD_CODE(tlv_buf);

    switch(CMDCODE){

        case CMDCODE_SHOW_NW_TOPOLOGY:
//            dump_nw_graph(topo);
            dump_graph(topo);
            break;
        default:
            ;
    }
    return 0;
}

/*General Validations*/
int
validate_node_existence(char *node_name){

    node_t *node = get_node_by_node_name(topo, node_name);
    printf(" %s() is called with node name = %s\n", __FUNCTION__, node_name);
    if(node)
        return VALIDATION_SUCCESS;
    printf("Error : Node %s do not exist\n", node_name);
    return VALIDATION_FAILED;
}

static int
arp_handler(param_t *param, ser_buff_t *tlv_buf,
                op_mode enable_or_disable)
{
    node_t *node;
    char *node_name;
    char *ip_addr;
    tlv_struct_t *tlv = NULL;

    TLV_LOOP_BEGIN(tlv_buf, tlv){
        if(strncmp(tlv->leaf_id, "node-name", strlen("node-name")) ==0)
            node_name = tlv->value;
        else if(strncmp(tlv->leaf_id, "ip-address", strlen("ip-address")) ==0)
            ip_addr = tlv->value;
    } TLV_LOOP_END;

    node = get_node_by_node_name(topo, node_name);
//    send_arp_broadcast_request(node, NULL, ip_addr);
    return 0;
}

void 
nw_init_cli(void)
{
    param_t *show = libcli_get_show_hook(); 
    param_t *debug = libcli_get_debug_hook();
    param_t *config = libcli_get_config_hook();
    param_t *run    = libcli_get_run_hook();
    param_t *debug_show = libcli_get_debug_show_hook();
    param_t *root = libcli_get_root();

    init_libcli(); 
    {
        /* Show topology */
        static param_t topology;
        init_param(&topology,
                   CMD,
                   "topology",
                   show_nw_topology_handler,
                   0,
                   INVALID,
                   0,
                   "Dump Complete Network Topology");
        libcli_register_param(show, &topology);
        set_param_cmd_code(&topology, CMDCODE_SHOW_NW_TOPOLOGY);
    }

    {
        /* CMD: run node <node name> resolve-arp <ip address> */
        /* run node */
        static param_t node;
        init_param(&node,
                   CMD,
                   "node",
                   0,
                   0,
                   INVALID,
                   0,
                   "\"node\" keyword");
        libcli_register_param(run, &node);

        {
            /* run node <node name> */
            static param_t node_name;
            init_param(&node_name,
                       LEAF,
                       0,
                       0,
                       validate_node_existence,
                       STRING,
                       "node-name",
                       "Node Name");
            libcli_register_param(&node, &node_name);
            {
                /* run node <node name> resolve-arp */
                static param_t resolve_arp;
                init_param(&resolve_arp,
                           CMD,
                           "resolve-arp",
                           0,
                           0,
                           INVALID,
                           0,
                           "Resolve ARP");
                libcli_register_param(&node_name, &resolve_arp);
                {
                    /* run node <node name> resolve-arp <ip-address> */
                    static param_t ip_addr;
                    init_param(&ip_addr, 
                               LEAF, 
                               0, 
                               arp_handler, 
                               0, 
                               IPV4, 
                               "ip-address", 
                               "Nbr IPv4 Address");
                    libcli_register_param(&resolve_arp, &ip_addr);
                    set_param_cmd_code(&ip_addr, CMDCODE_RUN_ARP);
                }
            }
        }
    }
}
