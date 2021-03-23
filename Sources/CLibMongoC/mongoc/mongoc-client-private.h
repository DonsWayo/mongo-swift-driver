/*
 * Copyright 2013 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CLibMongoC_mongoc-prelude.h"

#ifndef MONGOC_CLIENT_PRIVATE_H
#define MONGOC_CLIENT_PRIVATE_H

#include <CLibMongoC_bson.h>

#include "mongoc-apm-private.h"
#include "mongoc-buffer-private.h"
#include "CLibMongoC_mongoc-client.h"
#include "mongoc-cluster-private.h"
#include "CLibMongoC_mongoc-config.h"
#include "CLibMongoC_mongoc-host-list.h"
#include "CLibMongoC_mongoc-read-prefs.h"
#include "mongoc-rpc-private.h"
#include "CLibMongoC_mongoc-opcode.h"
#ifdef MONGOC_ENABLE_SSL
#include "CLibMongoC_mongoc-ssl.h"
#endif

#include "CLibMongoC_mongoc-stream.h"
#include "mongoc-topology-private.h"
#include "CLibMongoC_mongoc-write-concern.h"
#include "mongoc-crypt-private.h"

BSON_BEGIN_DECLS

/* protocol versions this driver can speak */
#define WIRE_VERSION_MIN 3
#define WIRE_VERSION_MAX 9

/* first version that supported "find" and "getMore" commands */
#define WIRE_VERSION_FIND_CMD 4
/* first version with "killCursors" command */
#define WIRE_VERSION_KILLCURSORS_CMD 4
/* first version when findAndModify accepts writeConcern */
#define WIRE_VERSION_FAM_WRITE_CONCERN 4
/* first version to support readConcern */
#define WIRE_VERSION_READ_CONCERN 4
/* first version to support maxStalenessSeconds */
#define WIRE_VERSION_MAX_STALENESS 5
/* first version to support writeConcern */
#define WIRE_VERSION_CMD_WRITE_CONCERN 5
/* first version to support collation */
#define WIRE_VERSION_COLLATION 5
/* first version to support server-side errors for unsupported hint options */
#define WIRE_VERSION_HINT_SERVER_SIDE_ERROR 5
/* first version to support OP_MSG */
#define WIRE_VERSION_OP_MSG 6
/* first version to support array filters for "update" command */
#define WIRE_VERSION_ARRAY_FILTERS 6
/* first version to support retryable reads  */
#define WIRE_VERSION_RETRY_READS 6
/* first version to support retryable writes  */
#define WIRE_VERSION_RETRY_WRITES 6
/* version corresponding to server 4.0 release */
#define WIRE_VERSION_4_0 7
/* first version to support hint for "update" command */
#define WIRE_VERSION_UPDATE_HINT 8
/* version corresponding to server 4.2 release */
#define WIRE_VERSION_4_2 8
/* version corresponding to client side field level encryption support. */
#define WIRE_VERSION_CSE 8
/* first version to throw server-side errors for unsupported hint in
 * "findAndModify" command */
#define WIRE_VERSION_FIND_AND_MODIFY_HINT_SERVER_SIDE_ERROR 8
/* first version to support hint for "delete" command */
#define WIRE_VERSION_DELETE_HINT 9
/* first version to support hint for "findAndModify" command */
#define WIRE_VERSION_FIND_AND_MODIFY_HINT 9
/* version corresponding to server 4.4 release */
#define WIRE_VERSION_4_4 9
/* version corresponding to retryable writes error label */
#define WIRE_VERSION_RETRYABLE_WRITE_ERROR_LABEL 9
/* first version to support server hedged reads */
#define WIRE_VERSION_HEDGED_READS 9

struct _mongoc_collection_t;

struct _mongoc_client_t {
   mongoc_uri_t *uri;
   mongoc_cluster_t cluster;
   bool in_exhaust;
   bool is_pooled;

   mongoc_stream_initiator_t initiator;
   void *initiator_data;

#ifdef MONGOC_ENABLE_SSL
   bool use_ssl;
   mongoc_ssl_opt_t ssl_opts;
#endif

   mongoc_topology_t *topology;

   mongoc_read_prefs_t *read_prefs;
   mongoc_read_concern_t *read_concern;
   mongoc_write_concern_t *write_concern;

   mongoc_apm_callbacks_t apm_callbacks;
   void *apm_context;

   int32_t error_api_version;
   bool error_api_set;

   mongoc_server_api_t *api;

   /* mongoc_client_session_t's in use, to look up lsids and clusterTimes */
   mongoc_set_t *client_sessions;
   unsigned int csid_rand_seed;

   int64_t timeout_ms;

   uint32_t generation;
};

/* Defines whether _mongoc_client_command_with_opts() is acting as a read
 * command helper for a command like "distinct", or a write command helper for
 * a command like "createRole", or both, like "aggregate" with "$out".
 */
typedef enum {
   MONGOC_CMD_RAW = 0,
   MONGOC_CMD_READ = 1,
   MONGOC_CMD_WRITE = 2,
   MONGOC_CMD_RW = 3,
} mongoc_command_mode_t;

BSON_STATIC_ASSERT2 (mongoc_cmd_rw,
                     MONGOC_CMD_RW == (MONGOC_CMD_READ | MONGOC_CMD_WRITE));

typedef enum { MONGOC_RR_SRV, MONGOC_RR_TXT } mongoc_rr_type_t;

typedef struct _mongoc_rr_data_t {
   /* Number of records returned by DNS. */
   uint32_t count;

   /* Set to lowest TTL found when polling SRV records. */
   uint32_t min_ttl;

   /* Set to the resulting host list when polling SRV records */
   mongoc_host_list_t *hosts;

   /* Set to the TXT record when polling for TXT */
   char *txt_record_opts;
} mongoc_rr_data_t;

#define MONGOC_RR_DEFAULT_BUFFER_SIZE 1024
bool
_mongoc_client_get_rr (const char *service,
                       mongoc_rr_type_t rr_type,
                       mongoc_rr_data_t *rr_data,
                       size_t initial_buffer_size,
                       bson_error_t *error);

mongoc_client_t *
_mongoc_client_new_from_uri (mongoc_topology_t *topology);

bool
_mongoc_client_set_apm_callbacks_private (mongoc_client_t *client,
                                          mongoc_apm_callbacks_t *callbacks,
                                          void *context);

mongoc_stream_t *
mongoc_client_default_stream_initiator (const mongoc_uri_t *uri,
                                        const mongoc_host_list_t *host,
                                        void *user_data,
                                        bson_error_t *error);

mongoc_stream_t *
_mongoc_client_create_stream (mongoc_client_t *client,
                              const mongoc_host_list_t *host,
                              bson_error_t *error);

bool
_mongoc_client_recv (mongoc_client_t *client,
                     mongoc_rpc_t *rpc,
                     mongoc_buffer_t *buffer,
                     mongoc_server_stream_t *server_stream,
                     bson_error_t *error);

void
_mongoc_client_kill_cursor (mongoc_client_t *client,
                            uint32_t server_id,
                            int64_t cursor_id,
                            int64_t operation_id,
                            const char *db,
                            const char *collection,
                            mongoc_client_session_t *cs);
bool
_mongoc_client_command_with_opts (mongoc_client_t *client,
                                  const char *db_name,
                                  const bson_t *command,
                                  mongoc_command_mode_t mode,
                                  const bson_t *opts,
                                  mongoc_query_flags_t flags,
                                  const mongoc_read_prefs_t *user_prefs,
                                  const mongoc_read_prefs_t *default_prefs,
                                  mongoc_read_concern_t *default_rc,
                                  mongoc_write_concern_t *default_wc,
                                  bson_t *reply,
                                  bson_error_t *error);

mongoc_server_session_t *
_mongoc_client_pop_server_session (mongoc_client_t *client,
                                   bson_error_t *error);

bool
_mongoc_client_lookup_session (const mongoc_client_t *client,
                               uint32_t client_session_id,
                               mongoc_client_session_t **cs,
                               bson_error_t *error);

void
_mongoc_client_unregister_session (mongoc_client_t *client,
                                   mongoc_client_session_t *session);

void
_mongoc_client_push_server_session (mongoc_client_t *client,
                                    mongoc_server_session_t *server_session);
void
_mongoc_client_end_sessions (mongoc_client_t *client);

mongoc_stream_t *
mongoc_client_connect_tcp (int32_t connecttimeoutms,
                           const mongoc_host_list_t *host,
                           bson_error_t *error);

mongoc_stream_t *
mongoc_client_connect (bool buffered,
                       bool use_ssl,
                       void *ssl_opts_void,
                       const mongoc_uri_t *uri,
                       const mongoc_host_list_t *host,
                       bson_error_t *error);
BSON_END_DECLS

#endif /* MONGOC_CLIENT_PRIVATE_H */
