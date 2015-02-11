/*
 * bgpstream
 *
 * Chiara Orsini, CAIDA, UC San Diego
 * chiara@caida.org
 *
 * Copyright (C) 2014 The Regents of the University of California.
 *
 * This file is part of bgpstream.
 *
 * bgpstream is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bgpstream is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bgpstream.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef __BGPSTREAM_UTILS_H
#define __BGPSTREAM_UTILS_H

/** @file
 *
 * @brief Header file that exposes all BGPStream utility types and functions.
 *
 * @author Chiara Orsini
 *
 */

/**
 * @name Public Constants
 *
 * @{ */

/** The maximum number of characters allowed in a collector name */
#define BGPSTREAM_UTILS_COLLECTOR_NAME_LEN 128

/** @} */

/* Include all utility headers */
#include <bgpstream_utils_addr.h>         /** < IP Address utilities */
#include <bgpstream_utils_as.h>           /** < AS/AS Path utilities */
#include <bgpstream_utils_id_set.h>       /** < ID Set utilities */
#include <bgpstream_utils_peer_sig_map.h> /** < Peer Signature utilities */
#include <bgpstream_utils_pfx.h>          /** < Prefix utilities */
#include <bgpstream_utils_str_set.h>      /** < String Set utilities */

#endif /* __BGPSTREAM_UTILS_H */

