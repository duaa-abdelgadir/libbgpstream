/*
 * This file is part of bgpstream
 *
 * CAIDA, UC San Diego
 * bgpstream-info@caida.org
 *
 * Copyright (C) 2012 The Regents of the University of California.
 * Authors: Alistair King, Chiara Orsini
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "bgpstream_int.h"
#include "bgpstream_debug.h"
#include "bgpstream_input.h"
#include "bgpstream_reader.h"
#include "bgpstream_di_mgr.h"
#include "bgpdump_lib.h"
#include "utils.h"

struct bgpstream {

  /* our input queue manager */
  bgpstream_input_mgr_t *input_mgr;

  /* our reader manager */
  bgpstream_reader_mgr_t *reader_mgr;

  /* filter manager instance */
  bgpstream_filter_mgr_t *filter_mgr;

  /* data interface manager */
  bgpstream_di_mgr_t *di_mgr;

  /* set to 1 once BGPStream has been started */
  int started;
};

/* ========== INTERNAL METHODS (see bgpstream_int.h) ========== */

bgpstream_filter_mgr_t *bgpstream_int_get_filter_mgr(bgpstream_t *bs)
{
  return bs->filter_mgr;
}

/* ========== PUBLIC METHODS (see bgpstream_int.h) ========== */

bgpstream_t *bgpstream_create()
{
  bgpstream_t *bs = NULL;

  if ((bs = malloc_zero(sizeof(bgpstream_t))) == NULL) {
    return NULL; // can't allocate memory
  }

  if ((bs->filter_mgr = bgpstream_filter_mgr_create()) == NULL) {
    goto err;
  }

  if ((bs->di_mgr = bgpstream_di_mgr_create(bs->filter_mgr)) == NULL) {
    goto err;
  }

  /* create an empty input mgr the input queue will be populated when a
   * bgpstream record is requested */
  if ((bs->input_mgr = bgpstream_input_mgr_create()) == NULL) {
    goto err;
  }

  if ((bs->reader_mgr = bgpstream_reader_mgr_create(bs->filter_mgr)) == NULL) {
    goto err;
  }

  return bs;

 err:
  bgpstream_destroy(bs);
  return NULL;
}

/* configure filters in order to select a subset of the bgp data available */
/* TODO: consider having these funcs return an error code */
void bgpstream_add_filter(bgpstream_t *bs, bgpstream_filter_type_t filter_type,
                          const char *filter_value)
{
  assert(!bs->started);
  bgpstream_filter_mgr_filter_add(bs->filter_mgr, filter_type, filter_value);
}

void bgpstream_add_rib_period_filter(bgpstream_t *bs, uint32_t period)
{
  assert(!bs->started);
  bgpstream_filter_mgr_rib_period_filter_add(bs->filter_mgr, period);
}

void bgpstream_add_recent_interval_filter(bgpstream_t *bs, const char *interval,
                                          uint8_t islive)
{

  uint32_t starttime, endtime;
  assert(!bs->started);

  if (bgpstream_time_calc_recent_interval(&starttime, &endtime, interval) ==
      0) {
    bgpstream_log_err("Failed to determine suitable time interval");
    return;
  }

  if (islive) {
    bgpstream_set_live_mode(bs);
    endtime = BGPSTREAM_FOREVER;
  }

  bgpstream_filter_mgr_interval_filter_add(bs->filter_mgr, starttime, endtime);
}

void bgpstream_add_interval_filter(bgpstream_t *bs, uint32_t begin_time,
                                   uint32_t end_time)
{
  assert(!bs->started);

  if (end_time == BGPSTREAM_FOREVER) {
    bgpstream_set_live_mode(bs);
  }
  bgpstream_filter_mgr_interval_filter_add(bs->filter_mgr, begin_time,
                                           end_time);
}

int bgpstream_get_data_interfaces(bgpstream_t *bs,
                                  bgpstream_data_interface_id_t **if_ids)
{
  return bgpstream_di_mgr_get_data_interfaces(bs->di_mgr, if_ids);
}

bgpstream_data_interface_id_t
bgpstream_get_data_interface_id_by_name(bgpstream_t *bs, const char *name)
{
  return bgpstream_di_mgr_get_data_interface_id_by_name(bs->di_mgr, name);
}

bgpstream_data_interface_info_t *
bgpstream_get_data_interface_info(bgpstream_t *bs,
                                  bgpstream_data_interface_id_t if_id)
{
  return bgpstream_di_mgr_get_data_interface_info(bs->di_mgr, if_id);
}

int bgpstream_get_data_interface_options(
  bgpstream_t *bs, bgpstream_data_interface_id_t if_id,
  bgpstream_data_interface_option_t **opts)
{
  return bgpstream_di_mgr_get_data_interface_options(bs->di_mgr, if_id, opts);
}

bgpstream_data_interface_option_t *bgpstream_get_data_interface_option_by_name(
  bgpstream_t *bs, bgpstream_data_interface_id_t if_id, const char *name)
{
  bgpstream_data_interface_option_t *options;
  int opt_cnt = 0;
  int i;

  opt_cnt = bgpstream_get_data_interface_options(bs, if_id, &options);

  if (options == NULL || opt_cnt == 0) {
    return NULL;
  }

  for (i = 0; i < opt_cnt; i++) {
    if (strcmp(options[i].name, name) == 0) {
      return &options[i];
    }
  }

  return NULL;
}

/* configure the data interface options */

int bgpstream_set_data_interface_option(
  bgpstream_t *bs, bgpstream_data_interface_option_t *option_type,
  const char *option_value)
{
  assert(!bs->started);
  return bgpstream_di_mgr_set_data_interface_option(bs->di_mgr,
                                                    option_type, option_value);
}

/* configure the interface so that it connects
 * to a specific data interface
 */
void bgpstream_set_data_interface(bgpstream_t *bs,
                                  bgpstream_data_interface_id_t di)
{
  assert(!bs->started);
  bgpstream_di_mgr_set_data_interface(bs->di_mgr, di);
}

bgpstream_data_interface_id_t bgpstream_get_data_interface_id(bgpstream_t *bs)
{
  return bgpstream_di_mgr_get_data_interface_id(bs->di_mgr);
}

/* configure the interface so that it blocks
 * waiting for new data
 */
void bgpstream_set_live_mode(bgpstream_t *bs)
{
  assert(!bs->started);
  bgpstream_di_mgr_set_blocking(bs->di_mgr);
}

/* turn on the bgpstream interface, i.e.:
 * it makes the interface ready
 * for a new get next call
*/
int bgpstream_start(bgpstream_t *bs)
{
  assert(!bs->started);

  // validate the filters that have been set
  int rc;
  if ((rc = bgpstream_filter_mgr_validate(bs->filter_mgr)) != 0) {
    return rc;
  }

  // start the data interface
  if (bgpstream_di_mgr_start(bs->di_mgr) != 0) {
    return -1;
  }

  bs->started = 1;
  return 0;
}

int bgpstream_get_next_record(bgpstream_t *bs, bgpstream_record_t *record)
{
  assert(bs->started);
  int md_queue_len = 0;
  bgpstream_input_t *md_queue = NULL;

  // if bs_record contains an initialized bgpdump entry we destroy it
  bgpstream_record_clear(record);
  record->bs = bs; // in case the user is using one record with two streams...

  // while we have no data in our local queues, try and get some
  while (bgpstream_reader_mgr_is_empty(bs->reader_mgr)) {

    // while the list of "file" metadata is empty, try and get some more files
    while (bgpstream_input_mgr_is_empty(bs->input_mgr)) {
      // ask the data interface for more "files"
      // this call will block if we're in blocking mode
      if ((md_queue_len =
           bgpstream_di_mgr_get_queue(bs->di_mgr, bs->input_mgr)) < 0) {
        // error from the data interface
        return -1;
      }
      if (md_queue_len == 0) {
        // no more data (only returned if not in live mode)
        return 0;
      }
    }

    // if we're here then the input manager has metadata in its queue for us to
    // process
    md_queue = bgpstream_input_mgr_get_queue_to_process(bs->input_mgr);
    // tell the reader manager about the new input metadata
    if (bgpstream_reader_mgr_add(bs->reader_mgr, md_queue,
                                 bs->filter_mgr) != 0) {
      return -1;
    }
    // we own the metadata queue, so destroy it
    bgpstream_input_mgr_destroy_queue(md_queue);
    md_queue = NULL;
  }

  // if we're here, then the reader manager has data we can get
  return bgpstream_reader_mgr_get_next_record(bs->reader_mgr, record,
                                              bs->filter_mgr);
}

/* destroy a bgpstream interface istance
 */
void bgpstream_destroy(bgpstream_t *bs)
{
  if (bs == NULL) {
    return;
  }

  bgpstream_input_mgr_destroy(bs->input_mgr);
  bs->input_mgr = NULL;

  bgpstream_reader_mgr_destroy(bs->reader_mgr);
  bs->reader_mgr = NULL;

  bgpstream_filter_mgr_destroy(bs->filter_mgr);
  bs->filter_mgr = NULL;

  bgpstream_di_mgr_destroy(bs->di_mgr);
  bs->di_mgr = NULL;

  bs->started = 0;

  free(bs);
}
