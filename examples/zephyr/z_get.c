//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zenoh-pico.h>

#define CLIENT_OR_PEER 0  // 0: Client mode; 1: Peer mode
#if CLIENT_OR_PEER == 0
#define MODE "client"
#define PEER ""  // If empty, it will scout
#elif CLIENT_OR_PEER == 1
#define MODE "peer"
#define PEER "udp/224.0.0.225:7447#iface=en0"
#else
#error "Unknown Zenoh operation mode. Check CLIENT_OR_PEER value."
#endif

#define KEYEXPR "demo/example/**"

void reply_dropper(void *ctx) { printf(" >> Received query final notification\n"); }

void reply_handler(z_owned_reply_t *oreply, void *ctx) {
    if (z_reply_is_ok(oreply)) {
        z_sample_t sample = z_reply_ok(oreply);
        char *keystr = z_keyexpr_to_string(sample.keyexpr);
        printf(" >> Received ('%s': '%.*s')\n", keystr, (int)sample.payload.len, sample.payload.start);
        free(keystr);
    } else {
        printf(" >> Received an error\n");
    }
}

int main(int argc, char **argv) {
    sleep(5);

    // Initialize Zenoh Session and other parameters
    z_owned_config_t config = z_config_default();
    zp_config_insert(z_loan(config), Z_CONFIG_MODE_KEY, z_string_make(MODE));
    if (strcmp(PEER, "") != 0) {
        zp_config_insert(z_loan(config), Z_CONFIG_PEER_KEY, z_string_make(PEER));
    }

    // Open Zenoh session
    printf("Opening Zenoh Session...");
    z_owned_session_t s = z_open(z_move(config));
    if (!z_check(s)) {
        printf("Unable to open session!\n");
        exit(-1);
    }
    printf("OK\n");

    // Start the receive and the session lease loop for zenoh-pico
    zp_start_read_task(z_loan(s), NULL);
    zp_start_lease_task(z_loan(s), NULL);

    while (1) {
        sleep(5);
        printf("Sending Query '%s'...\n", KEYEXPR);
        z_get_options_t opts = z_get_options_default();
        opts.target = Z_QUERY_TARGET_ALL;
        z_owned_closure_reply_t callback = z_closure(reply_handler, reply_dropper);
        if (z_get(z_loan(s), z_keyexpr(KEYEXPR), "", z_move(callback), &opts) < 0) {
            printf("Unable to send query.\n");
            exit(-1);
        }
    }

    printf("Closing Zenoh Session...");
    // Stop the receive and the session lease loop for zenoh-pico
    zp_stop_read_task(z_loan(s));
    zp_stop_lease_task(z_loan(s));

    z_close(z_move(s));
    printf("OK!\n");

    return 0;
}
