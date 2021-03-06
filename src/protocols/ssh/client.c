/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "blank.h"
#include "client.h"
#include "guac_handlers.h"
#include "ibar.h"
#include "ssh_client.h"
#include "terminal.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#define GUAC_SSH_DEFAULT_FONT_NAME "monospace" 
#define GUAC_SSH_DEFAULT_FONT_SIZE 12
#define GUAC_SSH_DEFAULT_PORT      "22"

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "username",
    "password",
    "font-name",
    "font-size",
    "enable-sftp",
    "private-key",
    "passphrase",
#ifdef ENABLE_SSH_AGENT
    "enable-agent",
#endif
    NULL
};

enum __SSH_ARGS_IDX {

    /**
     * The hostname to connect to. Required.
     */
    IDX_HOSTNAME,

    /**
     * The port to connect to. Optional.
     */
    IDX_PORT,

    /**
     * The name of the user to login as. Optional.
     */
    IDX_USERNAME,

    /**
     * The password to use when logging in. Optional.
     */
    IDX_PASSWORD,

    /**
     * The name of the font to use within the terminal.
     */
    IDX_FONT_NAME,

    /**
     * The size of the font to use within the terminal, in points.
     */
    IDX_FONT_SIZE,

    /**
     * Whether SFTP should be enabled.
     */
    IDX_ENABLE_SFTP,

    /**
     * The private key to use for authentication, if any.
     */
    IDX_PRIVATE_KEY,

    /**
     * The passphrase required to decrypt the private key, if any.
     */
    IDX_PASSPHRASE,

#ifdef ENABLE_SSH_AGENT
    /**
     * Whether SSH agent forwarding support should be enabled.
     */
    IDX_ENABLE_AGENT,
#endif

    SSH_ARGS_COUNT
};

int guac_client_init(guac_client* client, int argc, char** argv) {

    guac_socket* socket = client->socket;

    ssh_guac_client_data* client_data = malloc(sizeof(ssh_guac_client_data));

    /* Init client data */
    client->data = client_data;
    client_data->mod_alt   =
    client_data->mod_ctrl  =
    client_data->mod_shift = 0;
    client_data->clipboard_data = NULL;
    client_data->term_channel = NULL;

    if (argc != SSH_ARGS_COUNT) {
        guac_client_log_error(client, "Wrong number of arguments");
        return -1;
    }

    /* Read parameters */
    strcpy(client_data->hostname,  argv[IDX_HOSTNAME]);
    strcpy(client_data->username,  argv[IDX_USERNAME]);
    strcpy(client_data->password,  argv[IDX_PASSWORD]);

    /* Init public key auth information */
    client_data->key = NULL;
    strcpy(client_data->key_base64,     argv[IDX_PRIVATE_KEY]);
    strcpy(client_data->key_passphrase, argv[IDX_PASSPHRASE]);

    /* Read font name */
    if (argv[IDX_FONT_NAME][0] != 0)
        strcpy(client_data->font_name, argv[IDX_FONT_NAME]);
    else
        strcpy(client_data->font_name, GUAC_SSH_DEFAULT_FONT_NAME );

    /* Read font size */
    if (argv[IDX_FONT_SIZE][0] != 0)
        client_data->font_size = atoi(argv[IDX_FONT_SIZE]);
    else
        client_data->font_size = GUAC_SSH_DEFAULT_FONT_SIZE;

    /* Parse SFTP enable */
    client_data->enable_sftp = strcmp(argv[IDX_ENABLE_SFTP], "true") == 0;
    client_data->sftp_session = NULL;
    client_data->sftp_ssh_session = NULL;
    strcpy(client_data->sftp_upload_path, ".");

#ifdef ENABLE_SSH_AGENT
    client_data->enable_agent = strcmp(argv[IDX_ENABLE_AGENT], "true") == 0;
#endif

    /* Read port */
    if (argv[IDX_PORT][0] != 0)
        strcpy(client_data->port, argv[IDX_PORT]);
    else
        strcpy(client_data->port, GUAC_SSH_DEFAULT_PORT);

    /* Create terminal */
    client_data->term = guac_terminal_create(client,
            client_data->font_name, client_data->font_size,
            client->info.optimal_resolution,
            client->info.optimal_width, client->info.optimal_height);

    /* Fail if terminal init failed */
    if (client_data->term == NULL) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Terminal initialization failed";
        return -1;
    }

    /* Set up I-bar pointer */
    client_data->ibar_cursor = guac_ssh_create_ibar(client);

    /* Set up blank pointer */
    client_data->blank_cursor = guac_ssh_create_blank(client);

    /* Send initial name */
    guac_protocol_send_name(socket, client_data->hostname);

    /* Initialize pointer */
    client_data->current_cursor = client_data->blank_cursor;
    guac_ssh_set_cursor(client, client_data->current_cursor);

    guac_socket_flush(socket);

    /* Set basic handlers */
    client->handle_messages   = ssh_guac_client_handle_messages;
    client->clipboard_handler = ssh_guac_client_clipboard_handler;
    client->key_handler       = ssh_guac_client_key_handler;
    client->mouse_handler     = ssh_guac_client_mouse_handler;
    client->size_handler      = ssh_guac_client_size_handler;
    client->free_handler      = ssh_guac_client_free_handler;

    /* Start client thread */
    if (pthread_create(&(client_data->client_thread), NULL, ssh_client_thread, (void*) client)) {
        guac_client_log_error(client, "Unable to SSH client thread");
        return -1;
    }

    /* Success */
    return 0;

}

