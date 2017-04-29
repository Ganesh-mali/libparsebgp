/*
 * Copyright (c) 2013-2016 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 */

#include <algorithm>
#include "../include/parse_bgp.h"
#include "../include/mp_link_state_attr.h"
#include "../include/parse_bmpv1.h"

using namespace std;

void libparsebgp_parse_bgp_init(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data, obj_bgp_peer *peer_entry,
                                string router_addr, peer_info *peer_info) {
    //debug = false;
    //logger = logPtr;

    bgp_parsed_data->data_bytes_remaining = 0;
    bgp_parsed_data->data = NULL;

//    bzero(&common_hdr, sizeof(common_hdr));

    // Set our mysql pointer
    //this->mbus_ptr = mbus_ptr;

    // Set our peer entry
    bgp_parsed_data->p_entry = peer_entry;
    bgp_parsed_data->p_info = peer_info;

    bgp_parsed_data->router_addr = router_addr;
}

/**
 * Desctructor
 *
parseBGP::~parseBGP() {
}*/
/**
 * Update the Database path attributes
 *
 * \details This method will update the database for the supplied path attributes
 *
 * \param  attrs            Reference to the parsed attributes map
 */
static void libparsebgp_parse_bgp_update_db_attrs(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data, parsed_attrs_map &attrs) {

    /*
     * Setup the record
     */
    bgp_parsed_data->base_attr.as_path                  = (string)attrs[ATTR_TYPE_AS_PATH];
    bgp_parsed_data->base_attr.cluster_list             = (string)attrs[ATTR_TYPE_CLUSTER_LIST];
    bgp_parsed_data->base_attr.community_list           = (string)attrs[ATTR_TYPE_COMMUNITIES];
    bgp_parsed_data->base_attr.ext_community_list       = (string)attrs[ATTR_TYPE_EXT_COMMUNITY];

    bgp_parsed_data->base_attr.atomic_agg               = ((string)attrs[ATTR_TYPE_ATOMIC_AGGREGATE]).compare("1") == 0 ? true : false;

    if (((string)attrs[ATTR_TYPE_LOCAL_PREF]).length() > 0)
        bgp_parsed_data->base_attr.local_pref = std::stoul(((string)attrs[ATTR_TYPE_LOCAL_PREF]));
    else
        bgp_parsed_data->base_attr.local_pref = 0;

    if (((string)attrs[ATTR_TYPE_MED]).length() > 0)
        bgp_parsed_data->base_attr.med = std::stoul(((string)attrs[ATTR_TYPE_MED]));
    else
        bgp_parsed_data->base_attr.med = 0;

    if (((string)attrs[ATTR_TYPE_INTERNAL_AS_COUNT]).length() > 0)
        bgp_parsed_data->base_attr.as_path_count = std::stoi(((string)attrs[ATTR_TYPE_INTERNAL_AS_COUNT]));
    else
        bgp_parsed_data->base_attr.as_path_count = 0;

    if (((string)attrs[ATTR_TYPE_INTERNAL_AS_ORIGIN]).length() > 0)
        bgp_parsed_data->base_attr.origin_as = std::stoul(((string)attrs[ATTR_TYPE_INTERNAL_AS_ORIGIN]));
    else
        bgp_parsed_data->base_attr.origin_as = 0;

    if (((string)attrs[ATTR_TYPE_ORIGINATOR_ID]).length() > 0)
        strncpy(bgp_parsed_data->base_attr.originator_id, ((string)attrs[ATTR_TYPE_ORIGINATOR_ID]).c_str(), sizeof(bgp_parsed_data->base_attr.originator_id));
    else
        bzero(bgp_parsed_data->base_attr.originator_id, sizeof(bgp_parsed_data->base_attr.originator_id));

    if ( ((string)attrs[ATTR_TYPE_NEXT_HOP]).find_first_of(':') ==  string::npos)
        bgp_parsed_data->base_attr.nexthop_isIPv4 = true;
    else // is IPv6
        bgp_parsed_data->base_attr.nexthop_isIPv4 = false;

    if ( ((string)attrs[ATTR_TYPE_AGGEGATOR]).length() > 0)
        strncpy(bgp_parsed_data->base_attr.aggregator, ((string)attrs[ATTR_TYPE_AGGEGATOR]).c_str(), sizeof(bgp_parsed_data->base_attr.aggregator));
    else
        bzero(bgp_parsed_data->base_attr.aggregator, sizeof(bgp_parsed_data->base_attr.aggregator));

    if ( ((string)attrs[ATTR_TYPE_ORIGIN]).length() > 0)
        strncpy(bgp_parsed_data->base_attr.origin, ((string)attrs[ATTR_TYPE_ORIGIN]).c_str(), sizeof(bgp_parsed_data->base_attr.origin));
    else
        bzero(bgp_parsed_data->base_attr.origin, sizeof(bgp_parsed_data->base_attr.origin));

    if ( ((string)attrs[ATTR_TYPE_NEXT_HOP]).length() > 0)
        strncpy(bgp_parsed_data->base_attr.next_hop, ((string)attrs[ATTR_TYPE_NEXT_HOP]).c_str(), sizeof(bgp_parsed_data->base_attr.next_hop));

    else {
        // Skip adding path attributes if next hop is missing
        //    SELF_DEBUG("%s: no next-hop, must be unreach; not sending attributes to message bus", p_entry->peer_addr);
        bzero(bgp_parsed_data->base_attr.next_hop, sizeof(bgp_parsed_data->base_attr.next_hop));
        //bzero(bgp_parsed_data->path_hash_id, sizeof(bgp_parsed_data->path_hash_id));
        return;
    }

    //  SELF_DEBUG("%s: adding attributes to message bus", p_entry->peer_addr);

    // Update the DB entry
    // mbus_ptr->update_baseAttribute(*p_entry, base_attr, mbus_ptr->BASE_ATTR_ACTION_ADD);

    // Update the class instance variable path_hash_id
    //memcpy(bgp_parsed_data->path_hash_id, bgp_parsed_data->base_attr.hash_id, sizeof(bgp_parsed_data->path_hash_id));
}

/**
 * Update the Database for bgp-ls
 *
 * \details This method will update the database for the BGP-LS information
 *
 * \note    MUST BE called after adding the attributes since the path_hash_id must be set first.
 *
 * \param [in] remove      True if the records should be deleted, false if they are to be added/updated
 * \param [in] ls_data     Reference to the parsed link state nlri information
 * \param [in] ls_attrs    Reference to the parsed link state attribute information
 */
static void libparsebgp_parse_bgp_update_db_bgp_ls(bool remove, parsed_data_ls ls_data, parsed_ls_attrs_map &ls_attrs) {
    /*
     * Update table entry with attributes based on NLRI
     */
    if (ls_data.nodes.size() > 0) {
        //SELF_DEBUG("%s: Updating BGP-LS: Nodes %d", p_entry->peer_addr, ls_data.nodes.size());

        // Merge attributes to each table entry
        for (list<obj_ls_node>::iterator it = ls_data.nodes.begin();it != ls_data.nodes.end(); it++) {

            if (ls_attrs.find(ATTR_NODE_NAME) != ls_attrs.end())
                memcpy((*it).name, ls_attrs[ATTR_NODE_NAME].data(), sizeof((*it).name));

            if (ls_attrs.find(ATTR_NODE_IPV4_ROUTER_ID_LOCAL) != ls_attrs.end())
                memcpy((*it).router_id, ls_attrs[ATTR_NODE_IPV4_ROUTER_ID_LOCAL].data(), 4);

            if (ls_attrs.find(ATTR_NODE_IPV6_ROUTER_ID_LOCAL) != ls_attrs.end()) {
                memcpy((*it).router_id, ls_attrs[ATTR_NODE_IPV6_ROUTER_ID_LOCAL].data(), 16);
                (*it).is_ipv4 = false;
            }

            if (ls_attrs.find(ATTR_NODE_MT_ID) != ls_attrs.end()) {
                strncpy((char *)&(*it).mt_id, (char *)ls_attrs[ATTR_NODE_MT_ID].data(),
                        strlen((char *)ls_attrs[ATTR_NODE_MT_ID].data()) + 1);
            }

            if (ls_attrs.find(ATTR_NODE_FLAG) != ls_attrs.end())
                memcpy((*it).flags, ls_attrs[ATTR_NODE_FLAG].data(), sizeof((*it).flags));

            if (ls_attrs.find(ATTR_NODE_ISIS_AREA_ID) != ls_attrs.end())
                memcpy((*it).isis_area_id, ls_attrs[ATTR_NODE_ISIS_AREA_ID].data(), sizeof((*it).isis_area_id));

            if (ls_attrs.find(ATTR_NODE_SR_CAPABILITIES) != ls_attrs.end()) {
                memcpy((*it).sr_capabilities_tlv, ls_attrs[ATTR_NODE_SR_CAPABILITIES].data(), sizeof((*it).sr_capabilities_tlv));
            }
        }
    }

    if (ls_data.links.size() > 0) {
        //SELF_DEBUG("%s: Updating BGP-LS: Links %d ", p_entry->peer_addr, ls_data.links.size());

        // Merge attributes to each table entry
        for (list<obj_ls_link>::iterator it = ls_data.links.begin();
             it != ls_data.links.end(); it++) {

            if (not (*it).is_ipv4 and ls_attrs.find(ATTR_NODE_IPV6_ROUTER_ID_LOCAL) != ls_attrs.end())
                memcpy((*it).router_id, ls_attrs[ATTR_NODE_IPV6_ROUTER_ID_LOCAL].data(), 16);

            else if (ls_attrs.find(ATTR_NODE_IPV4_ROUTER_ID_LOCAL) != ls_attrs.end()) {
                memcpy((*it).router_id, ls_attrs[ATTR_NODE_IPV4_ROUTER_ID_LOCAL].data(), 4);
                (*it).is_ipv4 = true;
            }

            if (not (*it).is_ipv4 and ls_attrs.find(ATTR_LINK_IPV6_ROUTER_ID_REMOTE) != ls_attrs.end())
                memcpy((*it).remote_router_id, ls_attrs[ATTR_LINK_IPV6_ROUTER_ID_REMOTE].data(), 16);

            else if (ls_attrs.find(ATTR_LINK_IPV4_ROUTER_ID_REMOTE) != ls_attrs.end()) {
                memcpy((*it).remote_router_id, ls_attrs[ATTR_LINK_IPV4_ROUTER_ID_REMOTE].data(), 4);
                //(*it).isIPv4 = true; // only set for local rid
            }


            if (ls_attrs.find(ATTR_NODE_ISIS_AREA_ID) != ls_attrs.end())
                memcpy((*it).isis_area_id, ls_attrs[ATTR_NODE_ISIS_AREA_ID].data(), sizeof((*it).isis_area_id));

            if (ls_attrs.find(ATTR_LINK_ADMIN_GROUP) != ls_attrs.end())
                memcpy(&(*it).admin_group, ls_attrs[ATTR_LINK_ADMIN_GROUP].data(), sizeof((*it).admin_group));

            if (ls_attrs.find(ATTR_LINK_MAX_LINK_BW) != ls_attrs.end())
                memcpy(&(*it).max_link_bw, ls_attrs[ATTR_LINK_MAX_LINK_BW].data(),
                       sizeof((*it).max_link_bw));

            if (ls_attrs.find(ATTR_LINK_MAX_RESV_BW) != ls_attrs.end())
                memcpy(&(*it).max_resv_bw, ls_attrs[ATTR_LINK_MAX_RESV_BW].data(), sizeof((*it).max_resv_bw));

            if (ls_attrs.find(ATTR_LINK_UNRESV_BW) != ls_attrs.end())
                memcpy(&(*it).unreserved_bw, ls_attrs[ATTR_LINK_UNRESV_BW].data(), sizeof((*it).unreserved_bw));

            if (ls_attrs.find(ATTR_LINK_TE_DEF_METRIC) != ls_attrs.end())
                memcpy(&(*it).te_def_metric, ls_attrs[ATTR_LINK_TE_DEF_METRIC].data(), sizeof((*it).te_def_metric));

            if (ls_attrs.find(ATTR_LINK_PROTECTION_TYPE) != ls_attrs.end())
                memcpy((*it).protection_type, ls_attrs[ATTR_LINK_PROTECTION_TYPE].data(), sizeof((*it).protection_type));

            if (ls_attrs.find(ATTR_LINK_MPLS_PROTO_MASK) != ls_attrs.end())
                memcpy((*it).mpls_proto_mask, ls_attrs[ATTR_LINK_MPLS_PROTO_MASK].data(), sizeof((*it).mpls_proto_mask));

            if (ls_attrs.find(ATTR_LINK_IGP_METRIC) != ls_attrs.end())
                memcpy(&(*it).igp_metric, ls_attrs[ATTR_LINK_IGP_METRIC].data(), sizeof((*it).igp_metric));

            if (ls_attrs.find(ATTR_LINK_SRLG) != ls_attrs.end())
                memcpy((*it).srlg, ls_attrs[ATTR_LINK_SRLG].data(), sizeof((*it).srlg));

            if (ls_attrs.find(ATTR_LINK_NAME) != ls_attrs.end())
                memcpy((*it).name, ls_attrs[ATTR_LINK_NAME].data(), sizeof((*it).name));

            if (ls_attrs.find(ATTR_LINK_PEER_EPE_NODE_SID) != ls_attrs.end())
                memcpy((*it).peer_node_sid, ls_attrs[ATTR_LINK_PEER_EPE_NODE_SID].data(), sizeof((*it).peer_node_sid));

            if (ls_attrs.find(ATTR_LINK_ADJACENCY_SID) != ls_attrs.end())
                memcpy((*it).peer_adj_sid, ls_attrs[ATTR_LINK_ADJACENCY_SID].data(), sizeof((*it).peer_adj_sid));
        }

    }

    if (ls_data.prefixes.size() > 0) {
        //SELF_DEBUG("%s: Updating BGP-LS: Prefixes %d ", p_entry->peer_addr, ls_data.prefixes.size());

        // Merge attributes to each table entry
        for (list<obj_ls_prefix>::iterator it = ls_data.prefixes.begin();it != ls_data.prefixes.end(); it++) {

            if (not (*it).isIPv4 and ls_attrs.find(ATTR_NODE_IPV6_ROUTER_ID_LOCAL) != ls_attrs.end())
                memcpy((*it).router_id, ls_attrs[ATTR_NODE_IPV6_ROUTER_ID_LOCAL].data(), 16);

            else if (ls_attrs.find(ATTR_NODE_IPV4_ROUTER_ID_LOCAL) != ls_attrs.end()) {
                memcpy((*it).router_id, ls_attrs[ATTR_NODE_IPV4_ROUTER_ID_LOCAL].data(), 4);
                (*it).isIPv4 = true;
            }

            if (ls_attrs.find(ATTR_NODE_ISIS_AREA_ID) != ls_attrs.end())
                memcpy((*it).isis_area_id, ls_attrs[ATTR_NODE_ISIS_AREA_ID].data(), sizeof((*it).isis_area_id));

            if (ls_attrs.find(ATTR_PREFIX_IGP_FLAGS) != ls_attrs.end())
                memcpy((*it).igp_flags, ls_attrs[ATTR_PREFIX_IGP_FLAGS].data(), sizeof((*it).igp_flags));

            if (ls_attrs.find(ATTR_PREFIX_ROUTE_TAG) != ls_attrs.end())
                memcpy(&(*it).route_tag, ls_attrs[ATTR_PREFIX_ROUTE_TAG].data(), sizeof((*it).route_tag));

            if (ls_attrs.find(ATTR_PREFIX_EXTEND_TAG) != ls_attrs.end())
                memcpy(&(*it).ext_route_tag, ls_attrs[ATTR_PREFIX_EXTEND_TAG].data(), sizeof((*it).ext_route_tag));

            if (ls_attrs.find(ATTR_PREFIX_PREFIX_METRIC) != ls_attrs.end())
                memcpy(&(*it).metric, ls_attrs[ATTR_PREFIX_PREFIX_METRIC].data(), sizeof((*it).metric));

            if (ls_attrs.find(ATTR_PREFIX_OSPF_FWD_ADDR) != ls_attrs.end())
                memcpy((*it).ospf_fwd_addr, ls_attrs[ATTR_PREFIX_OSPF_FWD_ADDR].data(), sizeof((*it).ospf_fwd_addr));

            if (ls_attrs.find(ATTR_PREFIX_SID) != ls_attrs.end())
                memcpy((*it).sid_tlv, ls_attrs[ATTR_PREFIX_SID].data(), sizeof((*it).sid_tlv));
        }

    }
}
/**
 * Update the Database advertised l3vpn
 *
 * \details This method will update the database for the supplied advertised prefixes
 *
 * \param [in] remove          True if the records should be deleted, false if they are to be added/updated
 * \param [in] prefixes        Reference to the list<vpn_tuple> of advertised vpns
 * \param [in] attrs           Reference to the parsed attributes map
 */
static void libparsebgp_parse_bgp_update_db_l3vpn(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data, bool remove, std::list<vpn_tuple> &prefixes,
                                                  parsed_attrs_map &attrs, vector<obj_vpn> &rib_list) {
    //vector<parseBMP::obj_vpn> rib_list;
    obj_vpn            rib_entry;
    uint32_t                         value_32bit;
    uint64_t                         value_64bit;

    /*
     * Loop through all vpn and add/update them in the DB
     */
    for (std::list<vpn_tuple>::iterator it = prefixes.begin();
         it != prefixes.end();
         it++) {
        vpn_tuple &tuple = (*it);

        //memcpy(rib_entry.path_attr_hash_id, bgp_parsed_data->path_hash_id, sizeof(rib_entry.path_attr_hash_id));
        //memcpy(rib_entry.peer_hash_id, bgp_parsed_data->p_entry->hash_id, sizeof(rib_entry.peer_hash_id));

        rib_entry.rd_type = tuple.rd_type;
        rib_entry.rd_assigned_number = tuple.rd_assigned_number;
        rib_entry.rd_administrator_subfield = tuple.rd_administrator_subfield;

        strncpy(rib_entry.prefix, tuple.prefix.c_str(), sizeof(rib_entry.prefix));

        rib_entry.prefix_len = tuple.len;

        snprintf(rib_entry.labels, sizeof(rib_entry.labels), "%s", tuple.labels.c_str());

        rib_entry.is_ipv4 = tuple.is_ipv4 ? 1 : 0;

        memcpy(rib_entry.prefix_bin, tuple.prefix_bin, sizeof(rib_entry.prefix_bin));

        // Add the ending IP for the prefix based on bits
        if (rib_entry.is_ipv4) {
            if (tuple.len < 32) {
                memcpy(&value_32bit, tuple.prefix_bin, 4);
                SWAP_BYTES(&value_32bit);

                value_32bit |= 0xFFFFFFFF >> tuple.len;
                SWAP_BYTES(&value_32bit);
                memcpy(rib_entry.prefix_bcast_bin, &value_32bit, 4);

            } else
                memcpy(rib_entry.prefix_bcast_bin, tuple.prefix_bin, sizeof(tuple.prefix_bin));

        } else {
            if (tuple.len < 128) {
                if (tuple.len >= 64) {
                    // High order bytes are left alone
                    memcpy(rib_entry.prefix_bcast_bin, tuple.prefix_bin, 8);

                    // Low order bytes are updated
                    memcpy(&value_64bit, &tuple.prefix_bin[8], 8);
                    SWAP_BYTES(&value_64bit);

                    value_64bit |= 0xFFFFFFFFFFFFFFFF >> (tuple.len - 64);
                    SWAP_BYTES(&value_64bit);
                    memcpy(&rib_entry.prefix_bcast_bin[8], &value_64bit, 8);

                } else {
                    // Low order types are all ones
                    value_64bit = 0xFFFFFFFFFFFFFFFF;
                    memcpy(&rib_entry.prefix_bcast_bin[8], &value_64bit, 8);

                    // High order bypes are updated
                    memcpy(&value_64bit, tuple.prefix_bin, 8);
                    SWAP_BYTES(&value_64bit);

                    value_64bit |= 0xFFFFFFFFFFFFFFFF >> tuple.len;
                    SWAP_BYTES(&value_64bit);
                    memcpy(rib_entry.prefix_bcast_bin, &value_64bit, 8);
                }
            } else
                memcpy(rib_entry.prefix_bcast_bin, tuple.prefix_bin, sizeof(tuple.prefix_bin));
        }

        rib_entry.path_id = tuple.path_id;
        snprintf(rib_entry.labels, sizeof(rib_entry.labels), "%s", tuple.labels.c_str());

//        SELF_DEBUG("%s: %s vpn=%s len=%d", p_entry->peer_addr, remove ? "removing" : "adding",rib_entry.prefix, rib_entry.prefix_len);

        // Add entry to the list
        rib_list.insert(rib_list.end(), rib_entry);
    }
}

/**
 * Updates for either advertised or withdrawn Evpn NLRI's
 *
 * \param [in] remove          True if the records should be deleted, false if they are to be added/updated
 * \param [in] nlris           Reference to the list<evpn_tuple>
 * \param [in] attrs           Reference to the parsed attributes map
 */
static void libparsebgp_parse_bgp_update_db_evpn(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data, bool remove, std::list<evpn_tuple> &nlris,
                                                 parsed_attrs_map &attrs, vector<obj_evpn> &rib_list) {

    //vector<parseBMP::obj_evpn> rib_list;
    obj_evpn         rib_entry;

    /*
     * Loop through all vpn and add/update them in the DB
     */
    for (std::list<evpn_tuple>::iterator it = nlris.begin();
         it != nlris.end();
         it++) {
        evpn_tuple &tuple = (*it);

        //memcpy(rib_entry.path_attr_hash_id, bgp_parsed_data->path_hash_id, sizeof(rib_entry.path_attr_hash_id));
        //memcpy(rib_entry.peer_hash_id, bgp_parsed_data->p_entry->hash_id, sizeof(rib_entry.peer_hash_id));

        rib_entry.rd_type = tuple.rd_type;
        rib_entry.rd_assigned_number = tuple.rd_assigned_number;
        rib_entry.rd_administrator_subfield = tuple.rd_administrator_subfield;

        strcpy(rib_entry.ethernet_tag_id_hex, tuple.ethernet_tag_id_hex.c_str());
        rib_entry.mpls_label_1 = tuple.mpls_label_1;
        rib_entry.mac_len = tuple.mac_len;
        strcpy(rib_entry.mac, tuple.mac.c_str());
        rib_entry.ip_len = tuple.ip_len;
        strcpy(rib_entry.ip, tuple.ip.c_str());
        rib_entry.mpls_label_2 = tuple.mpls_label_2;
        rib_entry.originating_router_ip_len = tuple.originating_router_ip_len;
        strcpy(rib_entry.originating_router_ip, tuple.originating_router_ip.c_str());
        strcpy(rib_entry.ethernet_segment_identifier, tuple.ethernet_segment_identifier.c_str());


        rib_entry.path_id = tuple.path_id;

        //   SELF_DEBUG("%s: %s evpn mac=%s ip=%s", p_entry->peer_addr,remove ? "removing" : "adding", rib_entry.mac, rib_entry.ip);

        // Add entry to the list
        rib_list.insert(rib_list.end(), rib_entry);
    }
}


/**
 * Update the Database advertised prefixes
 *
 * \details This method will update the database for the supplied advertised prefixes
 *
 * \param  adv_prefixes         Reference to the list<prefix_tuple> of advertised prefixes
 * \param  attrs            Reference to the parsed attributes map
 */
static void libparsebgp_parse_bgp_update_db_adv_prefixes(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data, std::list<prefix_tuple> &adv_prefixes,
                                                         parsed_attrs_map &attrs, vector<obj_rib> &rib_list) {
    //vector<parseBMP::obj_rib> rib_list;
    obj_rib                         rib_entry;
    uint32_t                        value_32bit;
    uint64_t                        value_64bit;

    /*
     * Loop through all prefixes and add/update them in the DB
     */
    for (std::list<prefix_tuple>::iterator it = adv_prefixes.begin();
         it != adv_prefixes.end();
         it++) {
        prefix_tuple &tuple = (*it);

        //memcpy(rib_entry.path_attr_hash_id, bgp_parsed_data->path_hash_id, sizeof(rib_entry.path_attr_hash_id));
        //memcpy(rib_entry.peer_hash_id, bgp_parsed_data->p_entry->hash_id, sizeof(rib_entry.peer_hash_id));

        strncpy(rib_entry.prefix, tuple.prefix.c_str(), sizeof(rib_entry.prefix));

        rib_entry.prefix_len     = tuple.len;

        rib_entry.is_ipv4 = tuple.is_ipv4 ? 1 : 0;

        memcpy(rib_entry.prefix_bin, tuple.prefix_bin, sizeof(rib_entry.prefix_bin));

        // Add the ending IP for the prefix based on bits
        if (rib_entry.is_ipv4) {
            if (tuple.len < 32) {
                memcpy(&value_32bit, tuple.prefix_bin, 4);
                SWAP_BYTES(&value_32bit);

                value_32bit |= 0xFFFFFFFF >> tuple.len;
                SWAP_BYTES(&value_32bit);
                memcpy(rib_entry.prefix_bcast_bin, &value_32bit, 4);

            } else
                memcpy(rib_entry.prefix_bcast_bin, tuple.prefix_bin, sizeof(tuple.prefix_bin));

        } else {
            if (tuple.len < 128) {
                if (tuple.len >= 64) {
                    // High order bytes are left alone
                    memcpy(rib_entry.prefix_bcast_bin, tuple.prefix_bin, 8);

                    // Low order bytes are updated
                    memcpy(&value_64bit, &tuple.prefix_bin[8], 8);
                    SWAP_BYTES(&value_64bit);

                    value_64bit |= 0xFFFFFFFFFFFFFFFF >> (tuple.len - 64);
                    SWAP_BYTES(&value_64bit);
                    memcpy(&rib_entry.prefix_bcast_bin[8], &value_64bit, 8);

                } else {
                    // Low order types are all ones
                    value_64bit = 0xFFFFFFFFFFFFFFFF;
                    memcpy(&rib_entry.prefix_bcast_bin[8], &value_64bit, 8);

                    // High order bypes are updated
                    memcpy(&value_64bit, tuple.prefix_bin, 8);
                    SWAP_BYTES(&value_64bit);

                    value_64bit |= 0xFFFFFFFFFFFFFFFF >> tuple.len;
                    SWAP_BYTES(&value_64bit);
                    memcpy(rib_entry.prefix_bcast_bin, &value_64bit, 8);
                }
            } else
                memcpy(rib_entry.prefix_bcast_bin, tuple.prefix_bin, sizeof(tuple.prefix_bin));
        }

        rib_entry.path_id = tuple.path_id;
        snprintf(rib_entry.labels, sizeof(rib_entry.labels), "%s", tuple.labels.c_str());

        //   SELF_DEBUG("%s: Adding prefix=%s len=%d", p_entry->peer_addr, rib_entry.prefix, rib_entry.prefix_len);

        // Add entry to the list
        rib_list.insert(rib_list.end(), rib_entry);
    }
}

/**
 * Update the Database withdrawn prefixes
 *
 * \details This method will update the database for the supplied advertised prefixes
 *
 * \param  wdrawn_prefixes         Reference to the list<prefix_tuple> of withdrawn prefixes
 */
static void libparsebgp_parse_bgp_update_db_wdrawn_prefixes(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data, std::list<prefix_tuple> &wdrawn_prefixes,
                                                            vector<obj_rib> &rib_list) {
    //vector<parseBMP::obj_rib> rib_list;
    obj_rib         rib_entry;

    /*
     * Loop through all prefixes and add/update them in the DB
     *
     */
    for (std::list<prefix_tuple>::iterator it = wdrawn_prefixes.begin();
         it != wdrawn_prefixes.end();
         it++) {

        prefix_tuple &tuple = (*it);
        //memcpy(rib_entry.path_attr_hash_id, bgp_parsed_data->path_hash_id, sizeof(rib_entry.path_attr_hash_id));
        //memcpy(rib_entry.peer_hash_id, bgp_parsed_data->p_entry->hash_id, sizeof(rib_entry.peer_hash_id));
        strncpy(rib_entry.prefix, tuple.prefix.c_str(), sizeof(rib_entry.prefix));

        rib_entry.prefix_len     = tuple.len;

        rib_entry.is_ipv4 = tuple.is_ipv4 ? 1 : 0;

        memcpy(rib_entry.prefix_bin, tuple.prefix_bin, sizeof(rib_entry.prefix_bin));

        rib_entry.path_id = tuple.path_id;
        snprintf(rib_entry.labels, sizeof(rib_entry.labels), "%s", tuple.labels.c_str());

        //SELF_DEBUG("%s: Removing prefix=%s len=%d", p_entry->peer_addr, rib_entry.prefix, rib_entry.prefix_len);

        // Add entry to the list
        rib_list.insert(rib_list.end(), rib_entry);
    }
}

/**
 * Update the Database with the parsed updated data
 *
 * \details This method will update the database based on the supplied parsed update data
 *
 * \param  parsed_data          Reference to the parsed update data
 */
static void libparsebgp_parse_bgp_update_db(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data) {
    /*
     * Update the path attributes
     */
    libparsebgp_parse_bgp_update_db_attrs(bgp_parsed_data, bgp_parsed_data->parsed_data.update_msg.parsed_data.attrs);

    /*
     * Update the bgp-ls data
     */
    libparsebgp_parse_bgp_update_db_bgp_ls(false, bgp_parsed_data->parsed_data.update_msg.parsed_data.ls, bgp_parsed_data->parsed_data.update_msg.parsed_data.ls_attrs);
    libparsebgp_parse_bgp_update_db_bgp_ls(true, bgp_parsed_data->parsed_data.update_msg.parsed_data.ls_withdrawn, bgp_parsed_data->parsed_data.update_msg.parsed_data.ls_attrs);

    /*
     * Update the advertised prefixes (both ipv4 and ipv6)
     */
    libparsebgp_parse_bgp_update_db_adv_prefixes(bgp_parsed_data, bgp_parsed_data->parsed_data.update_msg.parsed_data.advertised,
                                                 bgp_parsed_data->parsed_data.update_msg.parsed_data.attrs,
                                                 bgp_parsed_data->parsed_data.update_msg.adv_obj_rib_list);

    libparsebgp_parse_bgp_update_db_l3vpn(bgp_parsed_data, false,bgp_parsed_data->parsed_data.update_msg.parsed_data.vpn,
                                          bgp_parsed_data->parsed_data.update_msg.parsed_data.attrs, bgp_parsed_data->parsed_data.update_msg.obj_vpn_rib_list);
    libparsebgp_parse_bgp_update_db_l3vpn(bgp_parsed_data, true,bgp_parsed_data->parsed_data.update_msg.parsed_data.vpn_withdrawn,
                                          bgp_parsed_data->parsed_data.update_msg.parsed_data.attrs, bgp_parsed_data->parsed_data.update_msg.obj_vpn_rib_list);

    libparsebgp_parse_bgp_update_db_evpn(bgp_parsed_data, false, bgp_parsed_data->parsed_data.update_msg.parsed_data.evpn,
                                         bgp_parsed_data->parsed_data.update_msg.parsed_data.attrs, bgp_parsed_data->parsed_data.update_msg.obj_evpn_rib_list);
    libparsebgp_parse_bgp_update_db_evpn(bgp_parsed_data, true, bgp_parsed_data->parsed_data.update_msg.parsed_data.evpn_withdrawn,
                                         bgp_parsed_data->parsed_data.update_msg.parsed_data.attrs, bgp_parsed_data->parsed_data.update_msg.obj_evpn_rib_list);

    /*
     * Update withdraws (both ipv4 and ipv6)
     */
    libparsebgp_parse_bgp_update_db_wdrawn_prefixes(bgp_parsed_data, bgp_parsed_data->parsed_data.update_msg.parsed_data.withdrawn,
                                                    bgp_parsed_data->parsed_data.update_msg.wdrawn_obj_rib_list);

}

/**
 * parse BGP messages in MRT
 *
 * \param [in] data             Pointer to the raw BGP message header
 * \param [in] size             length of the data buffer (used to prevent overrun)
 * \param [in] bgp_msg           Structure to store the bgp messages
 * \returns BGP message type
 */
void libparsebgp_parse_bgp_parse_msg_from_mrt(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data, u_char *data, size_t size,
                                                uint32_t asn, bool is_local_msg) {
    u_char  bgp_msg_type = libparsebgp_parse_bgp_parse_header(bgp_parsed_data, data, size);
    switch (bgp_msg_type) {
        case BGP_MSG_UPDATE: {
            int read_size = 0;
            data += BGP_MSG_HDR_LEN;
            //libparsebgp_update_msg_data u_msg;
            libparsebgp_update_msg_init(&bgp_parsed_data->parsed_data.update_msg, bgp_parsed_data->p_entry->peer_addr, bgp_parsed_data->router_addr,
                                        bgp_parsed_data->p_info);
            //UpdateMsg uMsg(bgp_parsed_data->p_entry->peer_addr, bgp_parsed_data->router_addr, bgp_parsed_data->p_info);

            //if ((read_size=u_msg.parseUpdateMsg(data, bgp_parsed_data->data_bytes_remaining, bgp_msg->parsed_data, bgp_msg->has_end_of_rib_marker)) != (size - BGP_MSG_HDR_LEN)) {
            if ((read_size=libparsebgp_update_msg_parse_update_msg(&bgp_parsed_data->parsed_data.update_msg, data,
                                                                   bgp_parsed_data->data_bytes_remaining,
                                                                   bgp_parsed_data->has_end_of_rib_marker)) != (size - BGP_MSG_HDR_LEN)) {
                //LOG_NOTICE("%s: rtr=%s: Failed to parse the update message, read %d expected %d", p_entry->peer_addr, router_addr.c_str(), read_size, (size - read_size));
                //return true;
                throw "Failed to parse BGP update message";
            }

            bgp_parsed_data->data_bytes_remaining -= read_size;

            libparsebgp_parse_bgp_update_db(bgp_parsed_data);
            break;
        }
        case BGP_MSG_NOTIFICATION: {
            //bool rval;
            data += BGP_MSG_HDR_LEN;

            libparsebgp_notify_msg parsed_msg;
            //NotificationMsg nMsg;
            if (libparsebgp_notification_parse_notify(data, bgp_parsed_data->data_bytes_remaining, parsed_msg))
            {
                // LOG_ERR("%s: rtr=%s: Failed to parse the BGP notification message", p_entry->peer_addr, router_addr.c_str());
                throw "Failed to parse the BGP notification message";
            }
            else {
                data += 2;                                                 // Move pointer past notification message
                bgp_parsed_data->data_bytes_remaining -= 2;

                bgp_parsed_data->parsed_data.notification_msg.error_code = parsed_msg.error_code;
                bgp_parsed_data->parsed_data.notification_msg.error_subcode = parsed_msg.error_subcode;
                strncpy(bgp_parsed_data->parsed_data.notification_msg.error_text, parsed_msg.error_text,
                        sizeof(bgp_parsed_data->parsed_data.notification_msg.error_text));
            }
            //return rval;
            break;
        }
        case BGP_MSG_KEEPALIVE: {
            break;
        }
        case BGP_MSG_OPEN: {
            libparsebgp_open_msg_data *open_msg_data;
            libparsebgp_open_msg_init(open_msg_data, bgp_parsed_data->p_entry->peer_addr, bgp_parsed_data->p_info);
            list <string>       cap_list;
//            string              local_bgp_id, remote_bgp_id;
            size_t              read_size;

            bgp_parsed_data->p_info->recv_four_octet_asn = false;
            bgp_parsed_data->p_info->sent_four_octet_asn = false;
            bgp_parsed_data->p_info->using_2_octet_asn = false;

            data += BGP_MSG_HDR_LEN;
            read_size = libparsebgp_open_msg_parse_open_msg(open_msg_data,data, bgp_parsed_data->data_bytes_remaining, is_local_msg);

            if (!read_size) {
                //       LOG_ERR("%s: rtr=%s: Failed to read sent open message",  p_entry->peer_addr, router_addr.c_str());
                throw "Failed to read open message";
            }

            data += read_size;                                          // Move the pointer pase the sent open message
            bgp_parsed_data->data_bytes_remaining -= read_size;

//            if (is_local_msg) {
//                read_size = libparsebgp_open_msg_parse_open_msg(open_msg_data,data, bgp_parsed_data->data_bytes_remaining, true);
//
//                if (!read_size) {
//                    //       LOG_ERR("%s: rtr=%s: Failed to read sent open message",  p_entry->peer_addr, router_addr.c_str());
//                    throw "Failed to read sent open message";
//                }
//
//                data += read_size;                                          // Move the pointer pase the sent open message
//                bgp_parsed_data->data_bytes_remaining -= read_size;
//
//                //strncpy(up_event->local_bgp_id, local_bgp_id.c_str(), sizeof(up_event->local_bgp_id));
//
//                // Convert the list to string
//                //bzero(up_event->sent_cap, sizeof(up_event->sent_cap));
//
//                string cap_str;
//                for (list<string>::iterator it = cap_list.begin(); it != cap_list.end(); it++) {
//                    if ( it != cap_list.begin())
//                        cap_str.append(", ");
//
//                    // Check for 4 octet ASN support
//                    if ((*it).find("4 Octet ASN") != std::string::npos)
//                        bgp_parsed_data->p_info->sent_four_octet_asn = true;
//
//                    cap_str.append((*it));
//                }
//
//                //strncpy(up_event->sent_cap, cap_str.c_str(), sizeof(up_event->sent_cap));
//
//            }
//
//            /*
//             * Process the received open message
//             */
//
//            else {
//                read_size = libparsebgp_open_msg_parse_open_msg(open_msg_data,data, bgp_parsed_data->data_bytes_remaining, false);
//
//                if (!read_size) {
//                    //       LOG_ERR("%s: rtr=%s: Failed to read sent open message", p_entry->peer_addr, router_addr.c_str());
//                    throw "Failed to read received open message";
//                }
//
//                data += read_size;                                          // Move the pointer pase the sent open message
//                bgp_parsed_data->data_bytes_remaining -= read_size;
//
//                //strncpy(up_event->remote_bgp_id, remote_bgp_id.c_str(), sizeof(up_event->remote_bgp_id));
//
//                // Convert the list to string
//                //bzero(up_event->recv_cap, sizeof(up_event->recv_cap));
//
//                string cap_str;
//                for (list<string>::iterator it = cap_list.begin(); it != cap_list.end(); it++) {
//                    if ( it != cap_list.begin())
//                        cap_str.append(", ");
//
//                    // Check for 4 octet ASN support - reset to false if
//                    if ((*it).find("4 Octet ASN") != std::string::npos)
//                        bgp_parsed_data->p_info->recv_four_octet_asn = true;
//
//                    cap_str.append((*it));
//                }
//
//                //strncpy(up_event->recv_cap, cap_str.c_str(), sizeof(up_event->recv_cap));
//
//            }

            /*
            data += BGP_MSG_HDR_LEN;

            read_size = oMsg.parseOpenMsg(data, data_bytes_remaining, isLocalMsg, asn, up_event->local_hold_time, bgp_id, cap_list);

            if (!read_size) {
                //       LOG_ERR("%s: rtr=%s: Failed to read sent open message",  p_entry->peer_addr, router_addr.c_str());
                throw "Failed to read open message";
            }

            data += read_size;                                          // Move the pointer pase the sent open message
            data_bytes_remaining -= read_size;

            string cap_str;
            for (list<string>::iterator it = cap_list.begin(); it != cap_list.end(); it++) {
                if ( it != cap_list.begin())
                    cap_str.append(", ");

                // Check for 4 octet ASN support
                if ((*it).find("4 Octet ASN") != std::string::npos) {
                    if (isLocalMsg)
                        p_info->sent_four_octet_asn = true;
                    else
                        p_info->recv_four_octet_asn = true;
                }

                cap_str.append((*it));
            }*/

            break;
        }
        default: {
            throw "BGP message type does not match";
        }
    }
    //return bgp_msg_type;
}

/**
 * handle BGP update message and store in DB
 *
 * \details Parses the bgp update message and store it in the DB.
 *
 * \param [in]     data             Pointer to the raw BGP message header
 * \param [in]     size             length of the data buffer (used to prevent overrun)
 *
 * \returns True if error, false if no error.
 */
bool libparsebgp_parse_bgp_handle_update(libparsebgp_parsed_bmp_rm_msg *bgp_parsed_data, u_char *data, size_t size) {
    //UpdateMsg::parsed_update_data parsed_data;
    int read_size = 0;

    if (libparsebgp_parse_bgp_parse_header(bgp_parsed_data, data, size) == BGP_MSG_UPDATE) {
        data += BGP_MSG_HDR_LEN;

        /*
         * Parse the update message - stored results will be in parsed_data
         */
        //UpdateMsg uMsg(bgp_parsed_data->p_entry->peer_addr, bgp_parsed_data->router_addr, bgp_parsed_data->p_info);
        //libparsebgp_update_msg_data u_msg;
        libparsebgp_update_msg_init(&bgp_parsed_data->parsed_data.update_msg, bgp_parsed_data->p_entry->peer_addr,
                                    bgp_parsed_data->router_addr, bgp_parsed_data->p_info);

        //if ((read_size=uMsg.parseUpdateMsg(data, bgp_parsed_data->data_bytes_remaining, bgp_msg->parsed_data, bgp_msg->has_end_of_rib_marker)) != (size - BGP_MSG_HDR_LEN)) {
        if ((read_size=libparsebgp_update_msg_parse_update_msg(&bgp_parsed_data->parsed_data.update_msg, data, bgp_parsed_data->data_bytes_remaining,
                                                               bgp_parsed_data->has_end_of_rib_marker)) != (size - BGP_MSG_HDR_LEN)) {
            //LOG_NOTICE("%s: rtr=%s: Failed to parse the update message, read %d expected %d", p_entry->peer_addr, router_addr.c_str(), read_size, (size - read_size));
            return true;
        }

        bgp_parsed_data->data_bytes_remaining -= read_size;

        /*
         * Update the DB with the update data
         */
        libparsebgp_parse_bgp_update_db(bgp_parsed_data);
    }

    return false;
}

/**
 * handle BGP notify event - updates the down event with parsed data
 *
 * \details
 *  The notify message does not directly add to Db, so the calling
 *  method/function must handle that.
 *
 * \param [in]     data             Pointer to the raw BGP message header
 * \param [in]     size             length of the data buffer (used to prevent overrun)
 * \param [out]    down_event       Reference to the down event/notification storage buffer
 *
 * \returns True if error, false if no error.
 */
bool libparsebgp_parse_bgp_handle_down_event(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data, u_char *data, size_t size) {
    bool        rval;

    // Process the BGP message normally
    if (libparsebgp_parse_bgp_parse_header(bgp_parsed_data, data, size) == BGP_MSG_NOTIFICATION) {

        data += BGP_MSG_HDR_LEN;

        libparsebgp_notify_msg notify_msg;
        //NotificationMsg nMsg;
        if ( (rval=libparsebgp_notification_parse_notify(data, bgp_parsed_data->data_bytes_remaining, notify_msg)))
        {
            // LOG_ERR("%s: rtr=%s: Failed to parse the BGP notification message", p_entry->peer_addr, router_addr.c_str());
            throw "Failed to parse the BGP notification message";
        }
        else {
            data += 2;                                                 // Move pointer past notification message
            bgp_parsed_data->data_bytes_remaining -= 2;

            bgp_parsed_data->parsed_data.notification_msg.error_code = notify_msg.error_code;
            bgp_parsed_data->parsed_data.notification_msg.error_subcode = notify_msg.error_subcode;
            strncpy(bgp_parsed_data->parsed_data.notification_msg.error_text, notify_msg.error_text,
                    sizeof(bgp_parsed_data->parsed_data.notification_msg.error_text));
            /*
            down_event->bgp_err_code = notify_msg.error_code;
            down_event->bgp_err_subcode = notify_msg.error_subcode;
            strncpy(down_event->error_text, notify_msg.error_text, sizeof(down_event->error_text));*/
        }
    }
    else {
        //LOG_ERR("%s: rtr=%s: BGP message type is not a BGP notification, cannot parse the notification",
         //       p_entry->peer_addr, router_addr.c_str());
        throw "ERROR: Invalid BGP MSG for BMP down event, expected NOTIFICATION message.";
    }

    return rval;
}

/**
 * Handles the up event by parsing the BGP open messages - Up event will be updated
 *
 * \details
 *  This method will read the expected sent and receive open messages.
 *
 * \param [in]     data             Pointer to the raw BGP message header
 * \param [in]     size             length of the data buffer (used to prevent overrun)
 *
 * \returns True if error, false if no error.
 */
bool libparsebgp_parse_bgp_handle_up_event(u_char *data, size_t size, libparsebgp_parsed_bmp_peer_up_event *up_event) {
    libparsebgp_open_msg_data *open_msg_data;
    libparsebgp_open_msg_init(open_msg_data, bgp_parsed_data->p_entry->peer_addr, bgp_parsed_data->p_info);
    list <string>       cap_list;
    string              local_bgp_id, remote_bgp_id;
    size_t              read_size;

    bgp_parsed_data->p_info->recv_four_octet_asn = false;
    bgp_parsed_data->p_info->sent_four_octet_asn = false;
    bgp_parsed_data->p_info->using_2_octet_asn = false;


    /*
     * Process the sent open message
     */
    if (libparsebgp_parse_bgp_parse_header(bgp_parsed_data, data, size) == BGP_MSG_OPEN) {
        data += BGP_MSG_HDR_LEN;

        read_size = libparsebgp_open_msg_parse_open_msg(open_msg_data,data, bgp_parsed_data->data_bytes_remaining, true,
                                                        up_event->local_asn, up_event->local_hold_time,local_bgp_id, cap_list);

        if (!read_size) {
     //       LOG_ERR("%s: rtr=%s: Failed to read sent open message",  p_entry->peer_addr, router_addr.c_str());
            throw "Failed to read sent open message";
        }

        data += read_size;                                          // Move the pointer pase the sent open message
        bgp_parsed_data->data_bytes_remaining -= read_size;

        strncpy(up_event->local_bgp_id, local_bgp_id.c_str(), sizeof(up_event->local_bgp_id));

        // Convert the list to string
        bzero(up_event->sent_cap, sizeof(up_event->sent_cap));

        string cap_str;
        for (list<string>::iterator it = cap_list.begin(); it != cap_list.end(); it++) {
            if ( it != cap_list.begin())
                cap_str.append(", ");

            // Check for 4 octet ASN support
            if ((*it).find("4 Octet ASN") != std::string::npos)
                bgp_parsed_data->p_info->sent_four_octet_asn = true;

            cap_str.append((*it));
        }

        strncpy(up_event->sent_cap, cap_str.c_str(), sizeof(up_event->sent_cap));

    } else {
  //      LOG_ERR("%s: rtr=%s: BGP message type is not BGP OPEN, cannot parse the open message",  p_entry->peer_addr, router_addr.c_str());
        throw "ERROR: Invalid BGP MSG for BMP Sent OPEN message, expected OPEN message.";
    }

    /*
     * Process the received open message
     */
    cap_list.clear();

    if (libparsebgp_parse_bgp_parse_header(bgp_parsed_data, data, size) == BGP_MSG_OPEN) {
        data += BGP_MSG_HDR_LEN;

        read_size = libparsebgp_open_msg_parse_open_msg(open_msg_data,data, bgp_parsed_data->data_bytes_remaining, false, up_event->remote_asn, up_event->remote_hold_time, remote_bgp_id, cap_list);

        if (!read_size) {
     //       LOG_ERR("%s: rtr=%s: Failed to read sent open message", p_entry->peer_addr, router_addr.c_str());
            throw "Failed to read received open message";
        }

        data += read_size;                                          // Move the pointer pase the sent open message
        bgp_parsed_data->data_bytes_remaining -= read_size;

        strncpy(up_event->remote_bgp_id, remote_bgp_id.c_str(), sizeof(up_event->remote_bgp_id));

        // Convert the list to string
        bzero(up_event->recv_cap, sizeof(up_event->recv_cap));

        string cap_str;
        for (list<string>::iterator it = cap_list.begin(); it != cap_list.end(); it++) {
            if ( it != cap_list.begin())
                cap_str.append(", ");

            // Check for 4 octet ASN support - reset to false if
            if ((*it).find("4 Octet ASN") != std::string::npos)
                bgp_parsed_data->p_info->recv_four_octet_asn = true;

            cap_str.append((*it));
        }

        strncpy(up_event->recv_cap, cap_str.c_str(), sizeof(up_event->recv_cap));

    } else {
  //      LOG_ERR("%s: rtr=%s: BGP message type is not BGP OPEN, cannot parse the open message",p_entry->peer_addr, router_addr.c_str());
        throw "ERROR: Invalid BGP MSG for BMP Received OPEN message, expected OPEN message.";
    }

    return false;
}

/**
 * Parses the BGP common header
 *
 * \details
 *      This method will parse the bgp common header and will upload the global
 *      c_hdr structure, instance data pointer, and remaining bytes of message.
 *      The return value of this method will be the BGP message type.
 *
 * \param [in]      data            Pointer to the raw BGP message header
 * \param [in]      size            length of the data buffer (used to prevent overrun)
 *
 * \returns BGP message type
 */
u_char libparsebgp_parse_bgp_parse_header(libparsebgp_parse_bgp_parsed_data *bgp_parsed_data, u_char *data, size_t size) {
    //bzero(&common_hdr, sizeof(common_hdr));

    /*
     * Error out if data size is not large enough for common header
     */
    if (size < BGP_MSG_HDR_LEN) {
 //       LOG_WARN("%s: rtr=%s: BGP message is being parsed is %d but expected at least %d in size",p_entry->peer_addr, router_addr.c_str(), size, BGP_MSG_HDR_LEN);
        return 0;
    }

    memcpy(&bgp_parsed_data->c_hdr, data, BGP_MSG_HDR_LEN);

    // Change length to host byte order
    SWAP_BYTES(&(bgp_parsed_data->c_hdr.len));

    // Update remaining bytes left of the message
    bgp_parsed_data->data_bytes_remaining = bgp_parsed_data->c_hdr.len - BGP_MSG_HDR_LEN;

    /*
     * Error out if the remaining size of the BGP message is grater than passed bgp message buffer
     *      It is expected that the passed bgp message buffer holds the complete BGP message to be parsed
     */
//    if (common_hdr.len > size) {
//        LOG_WARN("%s: rtr=%s: BGP message size of %hu is greater than passed data buffer, cannot parse the BGP message",p_entry->peer_addr, router_addr.c_str(), common_hdr.len, size);
//    }

 //   SELF_DEBUG("%s: rtr=%s: BGP hdr len = %u, type = %d", p_entry->peer_addr, router_addr.c_str(), common_hdr.len, common_hdr.type);

    /*
     * Validate the message type as being allowed/accepted
     */
    switch (bgp_parsed_data->c_hdr.type) {
        case BGP_MSG_UPDATE         : // Update Message
        case BGP_MSG_NOTIFICATION   : // Notification message
        case BGP_MSG_OPEN           : // OPEN message
            // Message(s) are allowed - calling method will request further parsing of the bgp message type
            break;

        case BGP_MSG_ROUTE_REFRESH: // Route Refresh message
 //           LOG_NOTICE("%s: rtr=%s: Received route refresh, nothing to do with this message currently.",p_entry->peer_addr, router_addr.c_str());
            break;

        default :
 //           LOG_WARN("%s: rtr=%s: Unsupported BGP message type = %d", p_entry->peer_addr, router_addr.c_str(), common_hdr.type);
            break;
    }

    return bgp_parsed_data->c_hdr.type;
}
