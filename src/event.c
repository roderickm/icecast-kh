/* Icecast
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright 2000-2004, Jack Moffitt <jack@xiph.org, 
 *                      Michael Smith <msmith@xiph.org>,
 *                      oddsock <oddsock@xiph.org>,
 *                      Karl Heyes <karl@xiph.org>
 *                      and others (see AUTHORS for details).
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "event.h"
#include "cfgfile.h"
#include "yp.h"
#include "source.h"

#include "refbuf.h"
#include "client.h"
#include "logging.h"
#include "slave.h"
#include "fserve.h"
#include "stats.h"

#define CATMODULE "event"

void event_config_read (void)
{
    int ret;
    ice_config_t *config;
    ice_config_t new_config, old_config;
    /* reread config file */

    INFO0("Re-reading XML");
    config = config_grab_config(); /* Both to get the lock, and to be able
                                     to find out the config filename */
    xmlSetGenericErrorFunc (config->config_filename, log_parse_failure);
    ret = config_parse_file(config->config_filename, &new_config);
    if(ret < 0) {
        ERROR0("Error parsing config, not replacing existing config");
        switch(ret) {
            case CONFIG_EINSANE:
                ERROR0("Config filename null or blank");
                break;
            case CONFIG_ENOROOT:
                ERROR1("Root element not found in %s", config->config_filename);
                break;
            case CONFIG_EBADROOT:
                ERROR1("Not an icecast2 config file: %s",
                        config->config_filename);
                break;
            default:
                ERROR1("Parse error in reading %s", config->config_filename);
                break;
        }
        config_release_config();
    }
    else {
        restart_logging (&new_config);
        config_set_config (&new_config, &old_config);
        config_release_config();

        connection_thread_shutdown();
        redirector_clearall();
        fserve_scan ((time_t)0);
        config = config_get_config();
        yp_recheck_config (config);
        fserve_recheck_mime_types (config);
        stats_global (config);
        workers_adjust (config->workers_count);
        connection_listen_sockets_close (config, 0);
        redirector_setup (config);
        config_release_config();

        slave_restart();
        config_clear (&old_config);
    }
}

