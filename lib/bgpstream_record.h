/*
 * libbgpstream
 *
 * Chiara Orsini, CAIDA, UC San Diego
 * chiara@caida.org
 *
 * Copyright (C) 2013 The Regents of the University of California.
 *
 * This file is part of libbgpstream.
 *
 * libbgpstream is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libbgpstream is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libbgpstream.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _BGPSTREAM_RECORD_H
#define _BGPSTREAM_RECORD_H

#include <bgpdump_lib.h>
#include <stdbool.h>


#define BGPSTREAM_PAR_LEN 512


typedef enum {BGPSTREAM_UPDATE,
	      BGPSTREAM_RIB
} bgpstream_record_dump_type_t;

typedef enum {DUMP_START,        /* first entry in dump */
	      DUMP_MIDDLE,       /* intermediate entry in dump */
	      DUMP_END           /* last entry in dump */
} bgpstream_dump_position_t;

typedef enum {VALID_RECORD     = 0,    /* valid entry found in dump */
	      FILTERED_SOURCE  = 1,    /* fltered source: source is not empty, but no valid record found */
	      EMPTY_SOURCE     = 2,   /* empty source: source has no entries */
	      CORRUPTED_SOURCE = 3,   /* corrupted source: error in opening dump */	      
	      CORRUPTED_RECORD = 4    /* corrupted record: dump corrupted at some point */	      
} bgpstream_record_status_t;

#define BGPSTREAM_RECORD_TYPE_MAX 5

typedef struct struct_bgpstream_record_attributes_t {
  // define a list of useful attributes to associate to bgp dump entry
  char dump_project[BGPSTREAM_PAR_LEN];    // project name
  char dump_collector[BGPSTREAM_PAR_LEN];  // collector name
  bgpstream_record_dump_type_t dump_type;  // dump type
  long dump_time;   // timestamp associated with the time the bgp data was "aggregated"
  long record_time; // timestamp associated with the time the bgp data was last seen 
} bgpstream_record_attributes_t;


typedef struct struct_bgpstream_record_t {
  BGPDUMP_ENTRY *bd_entry;
  bgpstream_record_attributes_t attributes;
  bgpstream_record_status_t status; 
  bgpstream_dump_position_t dump_pos; 
} bgpstream_record_t;

#endif /* _BGPSTREAM_RECORD_H */
