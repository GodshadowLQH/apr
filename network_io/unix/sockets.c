/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include "networkio.h"
#include "apr_portable.h"

static ap_status_t socket_cleanup(void *sock)
{
    struct socket_t *thesocket = sock;
    if (close(thesocket->socketdes) == 0) {
        thesocket->socketdes = -1;
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_create_tcp_socket(ap_socket_t **new, ap_context_t *cont)
 *    Create a socket for tcp communication.
 * arg 1) The new socket that has been setup. 
 * arg 2) The context to use
 */
ap_status_t ap_create_tcp_socket(struct socket_t **new, ap_context_t *cont)
{
    (*new) = (struct socket_t *)ap_palloc(cont, sizeof(struct socket_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont; 
    (*new)->local_addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt,
                         sizeof(struct sockaddr_in));
    (*new)->remote_addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt,
                          sizeof(struct sockaddr_in));

    if ((*new)->local_addr == NULL || (*new)->remote_addr == NULL) {
        return APR_ENOMEM;
    }
 
    (*new)->socketdes = socket(AF_INET ,SOCK_STREAM, IPPROTO_TCP);

    (*new)->local_addr->sin_family = AF_INET;
    (*new)->remote_addr->sin_family = AF_INET;

    (*new)->addr_len = sizeof(*(*new)->local_addr);
    
    if ((*new)->socketdes < 0) {
        return errno;
    }
    (*new)->timeout = -1;
    ap_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
} 

/* ***APRDOC********************************************************
 * ap_status_t ap_shutdown(ap_socket_t *thesocket, ap_shutdown_how_e how)
 *    Shutdown either reading, writing, or both sides of a tcp socket.
 * arg 1) The socket to close 
 * arg 2) How to shutdown the socket.  One of:
 *            APR_SHUTDOWN_READ      -- no longer allow read requests
 *            APR_SHUTDOWN_WRITE     -- no longer allow write requests
 *            APR_SHUTDOWN_READWRITE -- no longer allow read or write requests 
 * NOTE:  This does not actually close the socket descriptor, it just
 *        controls which calls are still valid on the socket.
 */
ap_status_t ap_shutdown(struct socket_t *thesocket, ap_shutdown_how_e how)
{
    if (shutdown(thesocket->socketdes, how) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_close_socket(ap_socket_t *thesocket)
 *    Close a tcp socket.
 * arg 1) The socket to close 
 */
ap_status_t ap_close_socket(struct socket_t *thesocket)
{
    ap_kill_cleanup(thesocket->cntxt, thesocket, socket_cleanup);
    return socket_cleanup(thesocket);
}

/* ***APRDOC********************************************************
 * ap_status_t ap_bind(ap_socket_t *sock)
 *    Bind the socket to it's assocaited port
 * arg 1) The socket to bind 
 * NOTE:  This is where we will find out if there is any other process
 *        using the selected port.
 */
ap_status_t ap_bind(struct socket_t *sock)
{
    if (bind(sock->socketdes, (struct sockaddr *)sock->local_addr, sock->addr_len) == -1)
        return errno;
    else
        return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_listen(ap_socket_t *sock, ap_int32_t backlog)
 *    Listen to a bound socketi for connections. 
 * arg 1) The socket to listen on 
 * arg 2) The number of outstanding connections allowed in the sockets
 *        listen queue.  If this value is less than zero, the listen
 *        queue size is set to zero.  
 */
ap_status_t ap_listen(struct socket_t *sock, ap_int32_t backlog)
{
    if (listen(sock->socketdes, backlog) == -1)
        return errno;
    else
        return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_accept(ap_socket_t **new, ap_socket_t *sock, 
                         ap_context_t *connection_context)
 *    Accept a new connection request
 * arg 1) A copy of the socket that is connected to the socket that
 *        made the connection request.  This is the socket which should
 *        be used for all future communication.
 * arg 2) The socket we are listening on.
 * arg 3) The context for the new socket.
 */
ap_status_t ap_accept(struct socket_t **new, const struct socket_t *sock, struct context_t *connection_context)
{
    (*new) = (struct socket_t *)ap_palloc(connection_context, 
                            sizeof(struct socket_t));

    (*new)->cntxt = connection_context;
    (*new)->local_addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt, 
                 sizeof(struct sockaddr_in));

    (*new)->remote_addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt, 
                 sizeof(struct sockaddr_in));
    (*new)->addr_len = sizeof(struct sockaddr_in);
#ifndef HAVE_POLL
    (*new)->connected = 1;
#endif

    (*new)->socketdes = accept(sock->socketdes, (struct sockaddr *)(*new)->remote_addr,
                        &(*new)->addr_len);

    if ((*new)->socketdes < 0) {
        return errno;
    }

    if (getsockname((*new)->socketdes, (*new)->local_addr, &((*new)->addr_len)) < 0) {
	return errno;
    }

    ap_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_connect(ap_socket_t *sock, char *hostname)
 *    Issue a connection request to a socket either on the same machine
 *    or a different one. 
 * arg 1) The socket we wish to use for our side of the connection 
 * arg 2) The hostname of the machine we wish to connect to.  If NULL,
 *        APR assumes that the sockaddr_in in the apr_socket is completely
 *        filled out.
 */
ap_status_t ap_connect(struct socket_t *sock, char *hostname)
{
    struct hostent *hp;

    if (hostname != NULL) {
        hp = gethostbyname(hostname);

        if ((sock->socketdes < 0) || (!sock->remote_addr)) {
            return APR_ENOTSOCK;
        }
        if (!hp)  {
            if (h_errno == TRY_AGAIN) {
                return EAGAIN;
            }
            return h_errno;
        }
    
        memcpy((char *)&sock->remote_addr->sin_addr, hp->h_addr_list[0], hp->h_length);

        sock->addr_len = sizeof(*sock->remote_addr);
    }

    if ((connect(sock->socketdes, (const struct sockaddr *)sock->remote_addr, sock->addr_len) < 0) &&
        (errno != EINPROGRESS)) {
        return errno;
    }
    else {
        int namelen = sizeof(*sock->local_addr);
        getsockname(sock->socketdes, (struct sockaddr *)sock->local_addr, &namelen);
#ifndef HAVE_POLL
	sock->connected=1;
#endif
        return APR_SUCCESS;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_socketdata(void **data, char *key, ap_socket_t *sock)
 *    Return the context associated with the current socket.
 * arg 1) The currently open socket.
 * arg 2) The user data associated with the socket.
 */
ap_status_t ap_get_socketdata(void **data, char *key, struct socket_t *sock)
{
    if (sock != NULL) {
        return ap_get_userdata(data, key, sock->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOSOCKET;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_socketdata(ap_socket_t *sock, void *data, char *key,
                                 ap_status_t (*cleanup) (void *))
 *    Set the context associated with the current socket.
 * arg 1) The currently open socket.
 * arg 2) The user data to associate with the socket.
 * arg 3) The key to associate with the data.
 * arg 4) The cleanup to call when the socket is destroyed.
 */
ap_status_t ap_set_socketdata(struct socket_t *sock, void *data, char *key,
                              ap_status_t (*cleanup) (void *))
{
    if (sock != NULL) {
        return ap_set_userdata(data, key, cleanup, sock->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOSOCKET;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_sock(ap_os_sock_t *thesock, ap_socket_t *sock)
 *    Convert the socket from an apr type to an OS specific socket
 * arg 1) The socket to convert.
 * arg 2) The os specifc equivelant of the apr socket..
 */
ap_status_t ap_get_os_sock(ap_os_sock_t *thesock, struct socket_t *sock)
{
    if (sock == NULL) {
        return APR_ENOSOCKET;
    }
    *thesock = sock->socketdes;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_sock(ap_socket_t **sock, ap_os_socket_t *thesock, 
 *                            ap_context_t *cont)
 *    Convert a socket from the os specific type to the apr type
 * arg 1) The context to use.
 * arg 2) The socket to convert to.
 * arg 3) The socket we are converting to an apr type.
 */
ap_status_t ap_put_os_sock(struct socket_t **sock, ap_os_sock_t *thesock, 
                           ap_context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if ((*sock) == NULL) {
        (*sock) = (struct socket_t *)ap_palloc(cont, sizeof(struct socket_t));
        (*sock)->cntxt = cont;
        (*sock)->local_addr = (struct sockaddr_in *)ap_palloc((*sock)->cntxt,
                             sizeof(struct sockaddr_in));
        (*sock)->remote_addr = (struct sockaddr_in *)ap_palloc((*sock)->cntxt,
                              sizeof(struct sockaddr_in));

        if ((*sock)->local_addr == NULL || (*sock)->remote_addr == NULL) {
            return APR_ENOMEM;
        }
     
        (*sock)->addr_len = sizeof(*(*sock)->local_addr);
        (*sock)->timeout = -1;
        if (getsockname(*thesock, (*sock)->local_addr, &((*sock)->addr_len)) < 0) {
            return errno;
        }
    }
    (*sock)->socketdes = *thesock;
    return APR_SUCCESS;
}

