/* 
   HTTP session handling
   Copyright (C) 1999-2007, Joe Orton <joe@manyfish.co.uk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA

*/

#ifndef NE_SESSION_H
#define NE_SESSION_H 1

#include <sys/types.h>

#include "ne_ssl.h"
#include "ne_uri.h" /* for ne_uri */
#include "ne_defs.h"
#include "ne_socket.h"

NE_BEGIN_DECLS

typedef struct ne_session_s ne_session;

/* Create a session to the given server, using the given scheme.  If
 * "https" is passed as the scheme, SSL will be used to connect to the
 * server. */
ne_session *ne_session_create(const char *scheme,
			      const char *hostname, unsigned int port);

/* Finish an HTTP session */
void ne_session_destroy(ne_session *sess);

/* Prematurely force the connection to be closed for the given
 * session. */
void ne_close_connection(ne_session *sess);

/* Set the proxy server to be used for the session. */
void ne_session_proxy(ne_session *sess,
		      const char *hostname, unsigned int port);

/* Defined session flags: */
typedef enum ne_session_flag_e {
    NE_SESSFLAG_PERSIST = 0, /* disable this flag to prevent use of
                              * persistent connections. */

    NE_SESSFLAG_ICYPROTO, /* enable this flag to enable support for
                           * non-HTTP ShoutCast-style "ICY" responses. */

    NE_SESSFLAG_SSLv2, /* disable this flag to disable support for
                        * SSLv2, if supported by the SSL library. */

    NE_SESSFLAG_RFC4918, /* enable this flag to enable support for
                          * RFC4918-only WebDAV features; losing
                          * backwards-compatibility with RFC2518
                          * servers. */

    NE_SESSFLAG_CONNAUTH, /* enable this flag if an awful, broken,
                           * RFC-violating, connection-based HTTP
                           * authentication scheme is in use. */
    NE_SESSFLAG_NOBUFFER,  /* create a sock without buffer */

    NE_SESSFLAG_LAST /* enum sentinel value */
} ne_session_flag;

/* Set a new value for a particular session flag. */
void ne_set_session_flag(ne_session *sess, ne_session_flag flag, int value);

/* Return 0 if the given flag is not set, >0 it is set, or -1 if the
 * flag is not supported. */
int ne_get_session_flag(ne_session *sess, ne_session_flag flag);

/* Bypass the normal name resolution; force the use of specific set of
 * addresses for this session, addrs[0]...addrs[n-1].  The addrs array
 * must remain valid until the session is destroyed. */
void ne_set_addrlist(ne_session *sess, const ne_inet_addr **addrs, size_t n);

/* DEPRECATED: Progress callback. */
typedef void (*ne_progress)(void *userdata, ne_off_t progress, ne_off_t total);

/* DEPRECATED API: Set a progress callback for the session; this is
 * deprecated in favour of ne_set_notifier().  The progress callback
 * is invoked for after each block of the request and response body to
 * indicate request and response progress (there is no way to
 * distinguish between the two using this interface alone).
 *
 * NOTE: Use of this interface is mutually exclusive with the use of
 * ne_set_notifier().  A call to ne_set_progress() removes the
 * notifier callback, and vice versa. */
void ne_set_progress(ne_session *sess, ne_progress progress, void *userdata);

/* Store an opaque context for the session, 'priv' is returned by a
 * call to ne_session_get_private with the same ID. */
void ne_set_session_private(ne_session *sess, const char *id, void *priv);
void *ne_get_session_private(ne_session *sess, const char *id);

/* Status event type.  NOTE: More event types may be added in
 * subsequent releases, so callers must ignore unknown status types
 * for forwards-compatibility.  */
typedef enum {
    ne_status_lookup = 0, /* looking up hostname */
    ne_status_connecting, /* connecting to host */
    ne_status_connected, /* connected to host */
    ne_status_sending, /* sending a request body */
    ne_status_recving, /* receiving a response body */
    ne_status_disconnected /* disconnected from host */
} ne_session_status;

/* Status event information union; the relevant structure within
 * corresponds to the event type.  WARNING: the size of this union is
 * not limited by ABI constraint; it may be extended with additional
 * members of different size, or existing members may be extended. */
typedef union ne_session_status_info_u {
    struct /* ne_status_lookup */ {
        /* The hostname which is being resolved: */
        const char *hostname;
    } lu;
    struct /* ne_status_connecting */ {
        /* The hostname and network address to which a connection
         * attempt is being made: */
        const char *hostname;
        const ne_inet_addr *address;
    } ci;
    struct /* ne_status_connected, ne_status_disconnected */ {
        /* The hostname to which a connection has just been
         * established or closed: */
        const char *hostname;
    } cd;
    struct /* ne_status_sending and ne_status_recving */ {
        /* Request/response body transfer progress; if total == -1, the
         * total size is unknown; else 0 <= progress <= total:  */
        ne_off_t progress, total;
    } sr;
} ne_session_status_info;

/* Callback invoked to notify a new session status event, given by the
 * 'status' argument.  On invocation, the contents of exactly one of
 * the structures in the info union will be valid, as indicated
 * above. */
typedef void (*ne_notify_status)(void *userdata, ne_session_status status,
                                 const ne_session_status_info *info);

/* Set a status notification callback for the session, to report
 * session status events.  Only one notification callback per session
 * can be registered; the most recent of successive calls to this
 * function takes effect. Note that 
 *
 * NOTE: Use of this interface is mutually exclusive with the use of
 * ne_set_progress().  A call to ne_set_notifier() removes the
 * progress callback, and vice versa. */
void ne_set_notifier(ne_session *sess, ne_notify_status status, void *userdata);

/* Certificate verification failures.
 * The certificate is not yet valid: */
#define NE_SSL_NOTYETVALID (0x01)
/* The certificate has expired: */
#define NE_SSL_EXPIRED (0x02)
/* The hostname for which the certificate was issued does not
 * match the hostname of the server; this could mean that the
 * connection is being intercepted: */
#define NE_SSL_IDMISMATCH (0x04)
/* The certificate authority which signed the server certificate is
 * not trusted: there is no indicatation the server is who they claim
 * to be: */
#define NE_SSL_UNTRUSTED (0x08)

/* The bitmask of known failure bits: if (failures & ~NE_SSL_FAILMASK)
 * is non-zero, an unrecognized failure is given, and the verification
 * should be failed. */
#define NE_SSL_FAILMASK (0x0f)

/* A callback which is used when server certificate verification is
 * needed.  The reasons for verification failure are given in the
 * 'failures' parameter, which is a binary OR of one or more of the
 * above NE_SSL_* values. failures is guaranteed to be non-zero.  The
 * callback must return zero to accept the certificate: a non-zero
 * return value will fail the SSL negotiation. */
typedef int (*ne_ssl_verify_fn)(void *userdata, int failures,
				const ne_ssl_certificate *cert);

/* Install a callback to handle server certificate verification.  This
 * is required when the CA certificate is not known for the server
 * certificate, or the server cert has other verification problems. */
void ne_ssl_set_verify(ne_session *sess, ne_ssl_verify_fn fn, void *userdata);

/* Use the given client certificate for the session.  The client cert
 * MUST be in the decrypted state, otherwise behaviour is undefined.
 * The 'clicert' object is duplicated internally so can be destroyed
 * by the caller.  */
void ne_ssl_set_clicert(ne_session *sess, const ne_ssl_client_cert *clicert);

/* Indicate that the certificate 'cert' is trusted; the 'cert' object
 * is duplicated internally so can be destroyed by the caller.  This
 * function has no effect for non-SSL sessions. */
void ne_ssl_trust_cert(ne_session *sess, const ne_ssl_certificate *cert);

/* If the SSL library provided a default set of CA certificates, trust
 * this set of CAs. */
void ne_ssl_trust_default_ca(ne_session *sess);

/* Callback used to load a client certificate on demand.  If dncount
 * is > 0, the 'dnames' array dnames[0] through dnames[dncount-1]
 * gives the list of CA names which the server indicated were
 * acceptable.  The callback should load an appropriate client
 * certificate and then pass it to 'ne_ssl_set_clicert'. */
typedef void (*ne_ssl_provide_fn)(void *userdata, ne_session *sess,
				  const ne_ssl_dname *const *dnames,
                                  int dncount);

/* Register a function to be called when the server requests a client
 * certificate. */
void ne_ssl_provide_clicert(ne_session *sess, 
                            ne_ssl_provide_fn fn, void *userdata);

/* Set the timeout (in seconds) used when reading from a socket.  The
 * timeout value must be greater than zero. */
void ne_set_read_timeout(ne_session *sess, int timeout);

/* Set the timeout (in seconds) used when making a connection.  The
 * timeout value must be greater than zero. */
void ne_set_connect_timeout(ne_session *sess, int timeout);

/* Sets the user-agent string. neon/VERSION will be appended, to make
 * the full header "User-Agent: product neon/VERSION".
 * If this function is not called, the User-Agent header is not sent.
 * The product string must follow the RFC2616 format, i.e.
 *       product         = token ["/" product-version]
 *       product-version = token
 * where token is any alpha-numeric-y string [a-zA-Z0-9]* */
void ne_set_useragent(ne_session *sess, const char *product);

/* Returns non-zero if next-hop server does not claim compliance to
 * HTTP/1.1 or later. */
int ne_version_pre_http11(ne_session *sess);

/* Returns the 'hostport' URI segment for the end-server, e.g.
 * "my.server.com:8080". */
const char *ne_get_server_hostport(ne_session *sess);

/* Returns the URL scheme being used for the current session, omitting
 * the trailing ':'; e.g. "http" or "https". */
const char *ne_get_scheme(ne_session *sess);

/* Sets the host, scheme, and port fields of the given URI structure
 * to that of the configured server and scheme for the session; host
 * and scheme are malloc-allocated.  No other fields in the URI
 * structure are changed. */
void ne_fill_server_uri(ne_session *sess, ne_uri *uri);

/* If a proxy is configured, sets the host and port fields in the
 * given URI structure to that of the proxy.  The hostname is
 * malloc-allocated.  No other fields in the URI structure are
 * changed; if a proxy is not configured, no fields are changed. */
void ne_fill_proxy_uri(ne_session *sess, ne_uri *uri);

/* Set the error string for the session; takes printf-like format
 * string. */
void ne_set_error(ne_session *sess, const char *format, ...)
    ne_attribute((format (printf, 2, 3)));

/* Retrieve the error string for the session */
const char *ne_get_error(ne_session *sess);

/* Retrieve the net socket fd, if sess is connected, return -1
 * otherwise */
int ne_get_socket(ne_session *sess);

size_t ne_get_buffered(ne_session *sess);

NE_END_DECLS

#endif /* NE_SESSION_H */
