#include "../include/lib_parse_common.h"

#define BUFFER_SIZE 2048

/**
 * Function to read from file and store in buffer
 * @param fp        file pointer
 * @param buffer    char array to store data
 * @param position  position in buffer to start adding data
 */
void file_read(FILE *fp, u_char *buffer, int position) {
    char *array = (char *) malloc(BUFFER_SIZE*sizeof(char));
    if(fp!=NULL)
    {
        fread(array, 1, BUFFER_SIZE, fp);
        for (int i = 0; i < BUFFER_SIZE; i ++) {
            buffer[position++] = array[i];
        }
        printf("%d", position);
    }
    else //file could not be opened
        printf("File could not be opened.\n");
    free(array);
}

/**
 * Function to shift the buffer to read more data
 * @param buffer      array containing data
 * @param bytes_read  number of bytes read
 * @param buf_len     length of buffer
 */
int shift(u_char *buffer, int bytes_read, int buf_len) {
    memmove(buffer, buffer+bytes_read, buf_len - bytes_read);
    memset(buffer+bytes_read, 0, buf_len - bytes_read);
    return buf_len - bytes_read;
}

/**
 * Function to print address stored in char array
 * @param array char array containing the address
 * @param len length of the array
 */
void print_addr_from_char_array(u_char *array, int len) {
    for(int i = 0;i<len;i++ ) {
        if (i)
            printf(":");
        printf("%d", (unsigned int)array[i]);
    }
}

/**
 * Function to generate the output in ELEM format:
 * <dump-type>|<elem-type>|<record-ts>|<project>|<collector>|<peer-ASn>|<peer-IP>|<prefix>|<next-hop-IP>|<AS-path>|
 * <origin-AS>|<communities>|<old-state>|<new-state>
 * @param parse_msg structure holding the parsed message
 */
void elem_generate(libparsebgp_parse_msg *parse_msg) {
    switch (parse_msg->msg_type) {
        case MRT_MESSAGE_TYPE: {
            switch (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.type) {
                case TABLE_DUMP: {
                    printf("R|");
                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.has_end_of_rib_marker)
                        printf("E|");
                    else
                        printf("R|");
                    printf("%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.time_stamp,
                           parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.peer_as);
                    //print peer IP address:
                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.peer_ip,
                                               parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.sub_type == AFI_IPv6 ? 16 : 4);
                    printf("|");
                    //print prefix:
                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.prefix,
                                               parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.sub_type == AFI_IPv6 ? 16 : 4);
                    printf("|");
                    //print next hop address:
                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs_count; ++i) {
                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->attr_type.attr_type_code == ATTR_TYPE_NEXT_HOP) {
                            print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->attr_value.next_hop, 4);
                            break;
                        }
                    }
                    printf("|");
                    uint32_t origin_as = 0;
                    //print as_path
                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs_count; i++) {
                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->attr_type.attr_type_code == ATTR_TYPE_AS_PATH) {
                            for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->count_as_path; ++j) {
                                for (int k = 0; k < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->attr_value.as_path->count_seg_asn; ++k) {
                                    printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->attr_value.as_path[j].seg_asn[k]);
                                    origin_as = parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->attr_value.as_path[j].seg_asn[k];
                                }
                            }
                            break;
                        }
                    }
                    printf("|%d|", origin_as);
                    //print communities
                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs_count; ++i) {
                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->attr_type.attr_type_code == ATTR_TYPE_COMMUNITIES) {
                            for(int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->count_attr_type_comm; ++j) {
                                printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump.bgp_attrs[i]->attr_value.attr_type_comm[j]);
                            }
                            break;
                        }
                    }
                    printf("|");
                    break;
                }
                case TABLE_DUMP_V2: {
                    printf("R|");
                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.has_end_of_rib_marker)
                        printf("E|");
                    else
                        printf("R|");
                    switch (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.sub_type) {
                        case PEER_INDEX_TABLE: {
                            printf("%d||", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.time_stamp);
                            //print bgp-id
                            print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.peer_index_tbl.collector_bgp_id, 4);
                            printf("|||||||||");
                            break;
                        }
                        case RIB_IPV4_UNICAST:
                        case RIB_IPV6_UNICAST: {
                            bool found = false;
                            printf("%d||||||", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.time_stamp);
                            //print next_hop address
                            for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.entry_count && !found; ++i) {
                                for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs_count; ++j) {
                                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs[j]->attr_type.attr_type_code == ATTR_TYPE_NEXT_HOP) {
                                        print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs[j]->attr_value.next_hop, 4);
                                        found = true;
                                        break;
                                    }
                                }
                            }
                            found = false;
                            printf("|");
                            uint32_t origin_as = 0;
                            //print as_path
                            for(int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.entry_count && !found; ++i) {
                                for(int l = 0; l < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs_count; ++l) {
                                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs[l]->attr_type.attr_type_code == ATTR_TYPE_AS_PATH) {
                                        for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs[l]->count_as_path; ++j) {
                                            for (int k = 0; k < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs[l]->attr_value.as_path->count_seg_asn; ++k) {
                                                printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs[l]->attr_value.as_path[j].seg_asn[k]);
                                                origin_as = parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs[l]->attr_value.as_path[j].seg_asn[k];
                                            }
                                        }
                                        found = true;
                                        break;
                                    }
                                }
                            }
                            printf("|%d|", origin_as);
                            found = false;
                            //print communities
                            for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.entry_count && !found; ++i) {
                                for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs_count; ++j) {
                                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[j]->attr_type.attr_type_code == ATTR_TYPE_COMMUNITIES) {
                                        for (int k = 0; k < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs[j]->count_attr_type_comm; ++k) {
                                            printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_entry_hdr.rib_entries[i].bgp_attrs[j]->attr_value.attr_type_comm[k]);
                                        }
                                        found = true;
                                        break;
                                    }
                                }
                            }
                            printf("||");
                            break;
                        }
                        case RIB_GENERIC: {
                            printf("%d|||||", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.time_stamp);
                            int len = parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.address_family_identifier == AFI_IPv4 ? 4 : 16;
                            //print prefix
                            print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.nlri_entry.prefix, len);
                            bool found = false;
                            printf("|%d||||||", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.time_stamp);
                            //print next-hop address
                            for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.entry_count && !found; ++i) {
                                for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs_count; ++j) {
                                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[j]->attr_type.attr_type_code == ATTR_TYPE_NEXT_HOP) {
                                        print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[j]->attr_value.next_hop, 4);
                                        found = true;
                                        break;
                                    }
                                }
                            }
                            found = false;
                            printf("|");
                            uint32_t origin_as = 0;
                            //print as_path
                            for(int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.entry_count && !found; ++i) {
                                for(int l = 0; l < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs_count; ++l) {
                                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[l]->attr_type.attr_type_code == ATTR_TYPE_AS_PATH) {
                                        for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[l]->count_as_path; ++j) {
                                            for (int k = 0; k < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[l]->attr_value.as_path->count_seg_asn; ++k) {
                                                printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[l]->attr_value.as_path[j].seg_asn[k]);
                                                origin_as = parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[l]->attr_value.as_path[j].seg_asn[k];
                                            }
                                        }
                                        found = true;
                                        break;
                                    }
                                }
                            }
                            printf("|%d|", origin_as);
                            found = false;
                            //print communities
                            for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.entry_count && !found; ++i) {
                                for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs_count; ++j) {
                                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[j]->attr_type.attr_type_code == ATTR_TYPE_COMMUNITIES) {
                                        for (int k = 0; k < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[j]->count_attr_type_comm; ++k) {
                                            printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.table_dump_v2.rib_generic_entry_hdr.rib_entries[i].bgp_attrs[j]->attr_value.attr_type_comm[k]);
                                        }
                                        found = true;
                                        break;
                                    }
                                }
                            }
                            printf("||");
                            break;
                        }
                    }
                    break;
                }

                case BGP4MP:
                case BGP4MP_ET: {
                    switch (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.sub_type) {
                        case BGP4MP_STATE_CHANGE:
                        case BGP4MP_STATE_CHANGE_AS4: {
                            printf("U||%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.time_stamp,
                                   parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_state_change_msg.peer_asn);
                            int ip_len = parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_state_change_msg.address_family == AFI_IPv4 ? 4 : 16;
                            //print peer ip address
                            print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_state_change_msg.peer_ip, ip_len);
                            printf("||||||%d|%d", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_state_change_msg.old_state,
                                   parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_state_change_msg.new_state);

                            break;
                        }
                        case BGP4MP_MESSAGE:
                        case BGP4MP_MESSAGE_LOCAL:
                        case BGP4MP_MESSAGE_AS4_LOCAL:
                        case BGP4MP_MESSAGE_AS4: {
                            switch (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.c_hdr.type) {
                                case BGP_MSG_OPEN: {
                                    printf("R|R|%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.time_stamp,
                                           parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.peer_asn);
                                    int ip_len = parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.address_family == AFI_IPv4 ? 4 : 16;
                                    //print peer ip address
                                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.peer_ip, ip_len);
                                    printf("|||||||");
                                    break;
                                }
                                case BGP_MSG_UPDATE: {
                                    printf("U|");
                                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.wdrawn_route_len > 0)
                                        printf("W|");
                                    else if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.total_path_attr_len > 0)
                                        printf("A|");
                                    else
                                        printf("U|");
                                    printf("%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.time_stamp,
                                           parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.peer_asn);
                                    int ip_len = parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.address_family == AFI_IPv6 ? 16 : 4;
                                    //print peer ip address
                                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.peer_ip, ip_len);
                                    printf("|");
                                    //print next hop address
                                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.count_path_attr; ++i) {
                                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_type.attr_type_code == ATTR_TYPE_NEXT_HOP) {
                                            print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.next_hop, 4);
                                            break;
                                        }
                                    }
                                    printf("|||");
                                    int origin_as = 0;
                                    //print as_path
                                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.count_path_attr; ++i) {
                                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_type.attr_type_code == ATTR_TYPE_AS_PATH) {
                                            for(int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->count_as_path; ++j) {
                                                for (int k = 0; k < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.as_path[j].count_seg_asn; ++k) {
                                                    printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.as_path[j].seg_asn[k]);
                                                    origin_as = parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.as_path[j].seg_asn[k];
                                                }
                                            }
                                            break;
                                        }
                                    }
                                    printf("|%d|", origin_as);
                                    //print communities
                                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.count_path_attr; ++i) {
                                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_type.attr_type_code == ATTR_TYPE_COMMUNITIES) {
                                            for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->count_attr_type_comm; ++j) {
                                                printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.attr_type_comm[j]);
                                                break;
                                            }

                                        }
                                    }
                                    printf("||");
                                    break;
                                }
                                case BGP_MSG_NOTIFICATION: {
                                    printf("R|R|%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.c_hdr.time_stamp,
                                           parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.peer_asn);
                                    int ip_len = parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.address_family == AFI_IPv4 ? 4 : 16;
                                    //print peer ip address
                                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_mrt_msg.parsed_data.bgp4mp.bgp4mp_msg.peer_ip, ip_len);
                                    printf("|||||||");
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
        case BMP_MESSAGE_TYPE: {
            uint8_t type = 0;
            if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.version == 3)
                type = parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_hdr.c_hdr_v3.type;
            else if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.version == 1 || parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.version == 2)
                type = parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_hdr.c_hdr_old.type;
            else
                break;

            switch (type) {
                case TYPE_ROUTE_MON: {
                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.has_end_of_rib_marker)
                        printf("R|E|"); //End of RIB
                    else
                        printf("U||");
                    printf("%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.ts_secs,
                           parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_as);
                    //print peer IP address
                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_addr, 16);
                    printf("||");
                    //print next hop address
                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.count_path_attr; ++i) {
                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->attr_type.attr_type_code == ATTR_TYPE_NEXT_HOP) {
                            print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->attr_value.next_hop, 4);
                            break;
                        }
                    }
                    printf("|||");
                    int origin_as = 0;
                    //print as_path
                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.count_path_attr; ++i) {
                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->attr_type.attr_type_code == ATTR_TYPE_AS_PATH) {
                            for(int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->count_as_path; ++j) {
                                for (int k = 0; k < parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->attr_value.as_path[j].count_seg_asn; ++k) {
                                    printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->attr_value.as_path[j].seg_asn[k]);
                                    origin_as = parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->attr_value.as_path[j].seg_asn[k];
                                }
                            }
                            break;
                        }
                    }
                    printf("|%d|", origin_as);
                    //print communities
                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.count_path_attr; ++i) {
                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->attr_type.attr_type_code == ATTR_TYPE_COMMUNITIES) {
                            for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->count_attr_type_comm; ++j) {
                                printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_bmp_msg.parsed_rm_msg.parsed_data.update_msg.path_attributes[i]->attr_value.attr_type_comm[j]);
                                break;
                            }

                        }
                    }
                    printf("||");
                    break;
                }
                case TYPE_STATS_REPORT: {
                    break;
                }
                case TYPE_TERM_MSG: {
                    printf("R|R|%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.ts_secs,
                           parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_as);
                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_addr, 16);
                    printf("|||||||");
                    break;
                }
                case TYPE_INIT_MSG: {
                    printf("R|R|%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.ts_secs,
                           parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_as);
                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_addr, 16);
                    printf("|||||||");
                    break;
                }
                case TYPE_PEER_UP: {
                    printf("R|R|%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.ts_secs,
                           parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_as);
                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_addr, 16);
                    printf("|||||||");
                    break;
                }
                case TYPE_PEER_DOWN: {
                    printf("R|R|%d|||%d|", parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.ts_secs,
                           parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_as);
                    print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_bmp_msg.libparsebgp_parsed_peer_hdr.peer_addr, 16);
                    printf("|||||||");
                    break;
                }
            }
            break;
        }
        case BGP_MESSAGE_TYPE: {
            switch (parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.c_hdr.type) {
                case BGP_MSG_OPEN: {
                    printf("R|R||||||||||||");
                    break;
                }
                case BGP_MSG_UPDATE: {
                    if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.has_end_of_rib_marker)
                        printf("R|E|"); //End of RIB
                    else
                        printf("U||");
                    printf("|||||||");
                    //print next hop address
                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.count_path_attr; ++i) {
                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_type.attr_type_code == ATTR_TYPE_NEXT_HOP) {
                            print_addr_from_char_array(parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.next_hop, 4);
                            break;
                        }
                    }
                    printf("|||");
                    int origin_as = 0;
                    //print as path
                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.count_path_attr; ++i) {
                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_type.attr_type_code == ATTR_TYPE_AS_PATH) {
                            for(int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->count_as_path; ++j) {
                                for (int k = 0; k < parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.as_path[j].count_seg_asn; ++k) {
                                    printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.as_path[j].seg_asn[k]);
                                    origin_as = parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.as_path[j].seg_asn[k];
                                }
                            }
                            break;
                        }
                    }
                    printf("|%d|", origin_as);
                    //print communities
                    for (int i = 0; i < parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.count_path_attr; ++i) {
                        if (parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_type.attr_type_code == ATTR_TYPE_COMMUNITIES) {
                            for (int j = 0; j < parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->count_attr_type_comm; ++j) {
                                printf("%d ", parse_msg->libparsebgp_parse_msg_parsed.parsed_bgp_msg.parsed_data.update_msg.path_attributes[i]->attr_value.attr_type_comm[j]);
                                break;
                            }
                        }
                    }
                    printf("||");
                    break;
                }
                case BGP_MSG_NOTIFICATION: {
                    printf("R|R||||||||||||");
                    break;
                }
            }
            break;
        }
    }
    printf("\n");
}

int main(int argc, char * argv[]) {

    //fie_path is used to store the path of the file to be parsed
    char file_path[20];

    //Deafaut Message Type to be Parsed
    int msg_type = MRT_MESSAGE_TYPE;
    if (argc>1)
        strcpy(file_path, argv[1]);
    else
        strcpy(file_path, "../updates.20170601.2055");

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-f")) {
            // We expect the next arg to be the filename
            if (i + 1 >= argc) {
                printf("INVALID ARG: -f expects the filename to be specified\n");
                return true;
            }
            // Set the new filename
            strcpy(file_path, argv[++i]);
        }
        else if (!strcmp(argv[i], "-t")) {
            // We expect the next arg to be the type of message
            if (i + 1 >= argc) {
                printf("INVALID ARG: -t expects the type to be specified\n");
                return true;
            }
            // Set the message type
            msg_type = atoi(argv[++i]);
        }
    }

    int position = 0, len = BUFFER_SIZE, count = 0;
    ssize_t bytes_read = 0;

    //Allocating memory to buffer the messaged from file
    u_char *buffer= (u_char *)malloc(BUFFER_SIZE*sizeof(u_char));

    //msg_read is used to determine if the message is read properly
    bool msg_read = true;

    //Opening file in reading mode
    FILE *fp = fopen(file_path, "r");

    //Initializing the parsed message structure.
    libparsebgp_parse_msg *parse_msg = (libparsebgp_parse_msg *)malloc(sizeof(libparsebgp_parse_msg));
    if (fp != NULL) {

        //Read the file unless End Of File is encountered
        while (!feof(fp)) {

            //Allocation of extra buffer to parse additional messages from file
            if (position)
                buffer = (u_char *)realloc(buffer, (BUFFER_SIZE+position)*sizeof(u_char));

            //Reading the messages from the file and storing it in buffer
            file_read(fp, buffer, position);
            printf("\n");
            len = BUFFER_SIZE + position;
            int tlen = len;
            msg_read = true;

            //Setting the initial position to start parsing
            position = 0;

            while (msg_read && len > 0) {

                //Memsetting the structure to NULL values
                memset(parse_msg, 0, sizeof(parse_msg));

                //Moving the pointer to the next message
                u_char *buffer_to_pass = buffer+position;

                //Invoking the parse message API
                bytes_read = libparsebgp_parse_msg_common_wrapper(parse_msg, &buffer_to_pass, len, msg_type);

                //Checking if error has occurred in parsing the message
                if (bytes_read < 0) {
                    msg_read = false;       //Setting the msg_read to false
                    printf("\n Crashed. Error code: %ld\n", bytes_read);
                }
                //Checking if no message is read
                else if (bytes_read == 0)
                    msg_read = false;
                else {
                    //Moving the pointer to point towards the next message.
                    position += bytes_read;
                    printf("\nMessage %d Parsed Successfully\n", count+1);
                    len -= bytes_read;
                    printf("Bytes read in parsing this message: %ld Remaining Length of Buffer: %d\n", bytes_read, len);

                    //Generate the output in elem format
                    elem_generate(parse_msg);

                    //Destructor for the message
                    libparsebgp_parse_msg_common_destructor(parse_msg);

                    //Incrementing the Message count
                    count++;
                }
            }
            //Shift the buffer to accommodate new message from the file
            position = shift(buffer, position, tlen);
        }
        printf("*******File Parsed completely*******\n");
    } else
        //Unable to open file.
        printf("File could not be opened\n");

    //Freeing the pointers
    free(parse_msg);
    free(buffer);
    return 0;
}
