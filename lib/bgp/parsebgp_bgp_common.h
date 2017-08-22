#ifndef __PARSEBGP_BGP_COMMON_H
#define __PARSEBGP_BGP_COMMON_H

#include <inttypes.h>

/**
 * BGP Address Families (AFI)
 */
typedef enum {

  /** IPv4 Address */
  PARSEBGP_BGP_AFI_IPV4 = 1,

  /** IPv6 Address */
  PARSEBGP_BGP_AFI_IPV6 = 2,

  // TODO: add support for L2VPN, BGPLS

} parsebgp_bgp_afi_t;

/**
 * BGP Subsequnt Address Families (SAFI)
 */
typedef enum {

  /** Unicast */
  PARSEBGP_BGP_SAFI_UNICAST = 1,

  /** Multicast (PARSEBGP_NOT_IMPLEMENTED) */
  PARSEBGP_BGP_SAFI_MULTICAST = 2,

  // TODO: add support for MULTICAST, EVPN, NLRI_LABEL

  /** MPLS */
  PARSEBGP_BGP_SAFI_MPLS = 128,

} parsebgp_bgp_safi_t;

/**
 * BGP Prefix Types (based on AFI/SAFI)
 */
typedef enum {

  /** Unicast IPv4 Prefix */
  PARSEBGP_BGP_PREFIX_UNICAST_IPV4 = 1,

  /** Unicast IPv6 Prefix */
  PARSEBGP_BGP_PREFIX_UNICAST_IPV6 = 2,

  // TODO: add support for BGPLS and L2VPN etc.
} parsebgp_bgp_prefix_type_t;

/**
 * BGP Prefix Tuple
 */
typedef struct parsebgp_bgp_prefix {

  /** Prefix Type (parsebgp_bgp_prefix_type_t) */
  uint8_t type;

  /** Prefix Length (number of bits in the mask) */
  uint8_t len;

  /** Prefix Address */
  uint8_t addr[16];

} parsebgp_bgp_prefix_t;

#endif /* __PARSEBGP_BGP_COMMON_H */