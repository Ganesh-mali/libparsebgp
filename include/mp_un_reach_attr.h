/*
 * Copyright (c) 2013-2016 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 *
 */
#ifndef MPUNREACHATTR_H_
#define MPUNREACHATTR_H_

#include "bgp_common.h"
#include <list>
#include <string>
#include "mp_reach_attr.h"

/**
 * Parse the MP_UNREACH NLRI attribute data
 *
 * \details
 *      Will parse the MP_UNBREACH_NLRI data passed.  Parsed data will be stored
 *      in parsed_data.
 *
 * \param [in]   attr_len       Length of the attribute data
 * \param [in]   data           Pointer to the attribute data
 * \param [out]  parsed_data    Reference to parsed_update_data; will be updated with all parsed data
 *
 */
void libparsebgp_mp_un_reach_attr_parse_un_reach_nlri_attr(update_path_attrs *path_attrs, int attr_len, u_char *data, bool &has_end_of_rib_marker);

//} /* namespace bgp_msg */

#endif /* MPUNREACHATTR_H_ */