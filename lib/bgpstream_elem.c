/*
 * This file is part of bgpstream
 *
 * Copyright (C) 2015 The Regents of the University of California.
 * Authors: Alistair King, Chiara Orsini
 *
 * All rights reserved.
 *
 * This code has been developed by CAIDA at UC San Diego.
 * For more information, contact bgpstream-info@caida.org
 *
 * This source code is proprietary to the CAIDA group at UC San Diego and may
 * not be redistributed, published or disclosed without prior permission from
 * CAIDA.
 *
 * Report any bugs, questions or comments to bgpstream-info@caida.org
 *
 */

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "bgpdump_lib.h"
#include "utils.h"

#include "bgpstream_utils.h"

#include "bgpstream_debug.h"
#include "bgpstream_record.h"

#include "bgpstream_elem_int.h"

/* ==================== PROTECTED FUNCTIONS ==================== */

/* ==================== PUBLIC FUNCTIONS ==================== */

bgpstream_elem_t *bgpstream_elem_create() {
  // allocate memory for new element
  bgpstream_elem_t * ri;

  if((ri =
      (bgpstream_elem_t *) malloc_zero(sizeof(bgpstream_elem_t))) == NULL) {
    return NULL;
  }
  // all fields are initialized to zero

  // need to create as path
  if((ri->aspath = bgpstream_as_path_create()) == NULL) {
    return NULL;
  }

  return ri;
}

void bgpstream_elem_destroy(bgpstream_elem_t *elem) {

  if(elem == NULL) {
    return;
  }

  bgpstream_as_path_destroy(elem->aspath);
  elem->aspath = NULL;

  free(elem);
}

void bgpstream_elem_clear(bgpstream_elem_t *elem) {

}

bgpstream_elem_t *bgpstream_elem_copy(bgpstream_elem_t *dst,
                                      bgpstream_elem_t *src)
{
  /* do a memcpy and then manually copy the as path */
  memcpy(dst, src, sizeof(bgpstream_elem_t));

  if(bgpstream_as_path_copy(dst->aspath, src->aspath) != 0)
    {
      return NULL;
    }

  return dst;
}


int bgpstream_elem_type_snprintf(char *buf, size_t len,
                                 bgpstream_elem_type_t type)
{
  /* ensure we have enough bytes to write our single character */
  if(len == 0) {
    return 1;
  } else if(len == 1) {
    buf[0] = '\0';
    return 1;
  }

  switch(type)
    {
    case BGPSTREAM_ELEM_TYPE_RIB:
      buf[0] = 'R';
      break;

    case BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT:
      buf[0] = 'A';
      break;

    case BGPSTREAM_ELEM_TYPE_WITHDRAWAL:
      buf[0] = 'W';
      break;

    case BGPSTREAM_ELEM_TYPE_PEERSTATE:
      buf[0] = 'S';
      break;

    default:
      buf[0] = '\0';
      break;
    }

  buf[1] = '\0';
  return 1;
}

int bgpstream_elem_peerstate_snprintf(char *buf, size_t len,
                                      bgpstream_elem_peerstate_t state)
{
  size_t written = 0;

  switch(state)
    {
    case BGPSTREAM_ELEM_PEERSTATE_IDLE:
      strncpy(buf, "IDLE", len);
      written = strlen("IDLE");
      break;

    case BGPSTREAM_ELEM_PEERSTATE_CONNECT:
      strncpy(buf, "CONNECT", len);
      written = strlen("CONNECT");
      break;

    case BGPSTREAM_ELEM_PEERSTATE_ACTIVE:
      strncpy(buf, "ACTIVE", len);
      written = strlen("ACTIVE");
      break;

    case BGPSTREAM_ELEM_PEERSTATE_OPENSENT:
      strncpy(buf, "OPENSENT", len);
      written = strlen("OPENSENT");
      break;

    case BGPSTREAM_ELEM_PEERSTATE_OPENCONFIRM:
      strncpy(buf, "OPENCONFIRM", len);
      written = strlen("OPENCONFIRM");
      break;

    case BGPSTREAM_ELEM_PEERSTATE_ESTABLISHED:
      strncpy(buf, "ESTABLISHED", len);
      written = strlen("ESTABLISHED");
      break;

    default:
      if(len > 0) {
        buf[0] = '\0';
      }
      break;
    }

  /* we promise to always nul-terminate */
  if(written > len) {
    buf[len-1] = '\0';
  }

  return written;
}

#define B_REMAIN (len-written)
#define B_FULL   (written >= len)
#define ADD_PIPE                                \
  do {                                          \
  if(B_REMAIN > 1)                              \
    {                                           \
      *buf_p = '|';                             \
      buf_p++;                                  \
      *buf_p = '\0';                            \
      written++;                                \
    }                                           \
  else                                          \
    {                                           \
      return NULL;                              \
    }                                           \
  } while(0)

#define SEEK_STR_END                            \
  do {                                          \
    while(*buf_p != '\0')                       \
      {                                         \
        written++;                              \
        buf_p++;                                \
      }                                         \
 } while(0)

char *bgpstream_elem_snprintf(char *buf, size_t len,
                              bgpstream_elem_t *elem)
{
  assert(elem);

  size_t written = 0; /* < how many bytes we wanted to write */
  size_t c = 0; /* < how many chars were written */
  char *buf_p = buf;

  bgpstream_as_path_seg_t *seg;

  /* common fields */

  // timestamp|peer_ip|peer_asn|message_type|

  /* TIMESTAMP */
  c = snprintf(buf_p, B_REMAIN, "%"PRIu32"|", elem->timestamp);
  written += c;
  buf_p += c;

  if(B_FULL)
    return NULL;

  /* PEER IP */
  if(bgpstream_addr_ntop(buf_p, B_REMAIN, &elem->peer_address) == NULL)
    return NULL;
  SEEK_STR_END;

  /* PEER ASN */
  c = snprintf(buf_p, B_REMAIN, "|%"PRIu32"|", elem->peer_asnumber);
  written += c;
  buf_p += c;

  if(B_FULL)
    return NULL;

  /* MESSAGE TYPE */
  c = bgpstream_elem_type_snprintf(buf_p, B_REMAIN, elem->type);
  written += c;
  buf_p += c;

  if(B_FULL)
    return NULL;

  ADD_PIPE;

  /* conditional fields */
  switch(elem->type)
    {
    case BGPSTREAM_ELEM_TYPE_RIB:
    case BGPSTREAM_ELEM_TYPE_ANNOUNCEMENT:

      /* PREFIX */
      if(bgpstream_pfx_snprintf(buf_p, B_REMAIN,
                                (bgpstream_pfx_t*)&(elem->prefix)) == NULL)
        {
          return NULL;
        }
      SEEK_STR_END;
      ADD_PIPE;

      /* NEXT HOP */
      if(bgpstream_addr_ntop(buf_p, B_REMAIN, &elem->nexthop) == NULL)
        {
          return NULL;
        }
      SEEK_STR_END;
      ADD_PIPE;

      /* AS PATH */
      c = bgpstream_as_path_snprintf(buf_p, B_REMAIN, elem->aspath);
      written += c;
      buf_p += c;

      if(B_FULL)
        return NULL;

      ADD_PIPE;

      /* ORIGIN AS */
      if((seg = bgpstream_as_path_get_origin_as(elem->aspath)) != NULL)
        {
          c = bgpstream_as_path_seg_snprintf(buf_p, B_REMAIN, seg);
          written += c;
          buf_p += c;
        }

      ADD_PIPE;

      /* OLD STATE (empty) */
      ADD_PIPE;

      /* NEW STATE (empty) */
      if(B_FULL)
        return NULL;

      /* END OF LINE */
      break;

    case BGPSTREAM_ELEM_TYPE_WITHDRAWAL:

      /* PREFIX */
      if(bgpstream_pfx_snprintf(buf_p, B_REMAIN,
                                (bgpstream_pfx_t*)&(elem->prefix)) == NULL)
        {
          return NULL;
        }
      SEEK_STR_END;
      ADD_PIPE;
      /* NEXT HOP (empty) */
      ADD_PIPE;
      /* AS PATH (empty) */
      ADD_PIPE;
      /* ORIGIN AS (empty) */
      ADD_PIPE;
      /* OLD STATE (empty) */
      ADD_PIPE;
      /* NEW STATE (empty) */
      if(B_FULL)
        return NULL;
      /* END OF LINE */
      break;

    case BGPSTREAM_ELEM_TYPE_PEERSTATE:

      /* PREFIX (empty) */
      ADD_PIPE;
      /* NEXT HOP (empty) */
      ADD_PIPE;
      /* AS PATH (empty) */
      ADD_PIPE;
      /* ORIGIN AS (empty) */
      ADD_PIPE;

      /* OLD STATE */
      c = bgpstream_elem_peerstate_snprintf(buf_p, B_REMAIN,
                                            elem->old_state);
      written += c;
      buf_p += c;

      if(B_FULL)
        return NULL;

      ADD_PIPE;

      /* NEW STATE (empty) */
      c = bgpstream_elem_peerstate_snprintf(buf_p, B_REMAIN, elem->new_state);
      written += c;
      buf_p += c;

      if(B_FULL)
        return NULL;
      /* END OF LINE */
      break;

    default:
      fprintf(stderr, "Error during elem processing\n");
      return NULL;
    }

  return buf;
}

