/* samptcpip -> samp-ftp
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-ftp/conn-init-destroy.inc - initializing and destroying connections
 *
 */

/**
    <summary>
        Initializes a connection.
    </summary>

    <param name="host[]">
        The host name or IP address that you want to connect to.
    </param>

    <param name="port = 21">
        The host name or IP address to connect to.
    </param>    

    <param name="user[] = \"anonymous\"">
        The user to log in as. If no user is specified, it defaults
        to "anonymous", a generic user that most FTP servers support.
    </param>

    <param name="password[] = \"anonymous@anonymous.com\"">
        The password of the user. By default, this is a string resembling
        an e-mail, as many FTP servers require anonymous users to log in
        with their e-mail addresses.
    </param>

    <param name="bool:tls">
        Whether TLS support is needed.
    </param>    
    
    <returns>
        The ID of the connection if one could be created,
        -1 otherwise.
    </returns>
*/
stock SampFTP_Connect(
                        host[],
                        port = 21,
                        bool:tls = false
                    )
{
    new connid = _SampFTP_InitConn();

    if( connid == -1 ) return -1;

    if( port < 1 || port > 65535 ) {
        SampFTP_errno = SAMPFTP_INVALID_PORT;
        return -1;
    }

    if( !host[0] ) {
        SampFTP_errno = SAMPFTP_NO_HOSTNAME_SET;
        return -1;
    }

    if( !tls && !_SampFTP_ConnectSock_NoTLS(connid, host, port) )
        return -1;

    return connid;
}

/**
    <summary>
        Logs into a remote FTP server.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect().
    </param>

    <param name="user[] = \"anonymous\"">
        The user to log in as. If no user is specified, it defaults
        to "anonymous", a generic user that most FTP servers support.
    </param>

    <param name="password[] = \"anonymous@anonymous.com\"">
        The password of the user. By default, this is a string resembling
        an e-mail address, as many FTP servers require anonymous users to
        log in with their e-mail address.
    </param>
    
    <returns>
        This function does not handle returns.
    </returns>
*/
stock SampFTP_Login(    connid,
                        user[] = "anonymous",
                        password[] = "anonymous@anonymous.com"
                    )
{
    new msg[128];

    if( !_SampFTP_IsConnValid(connid) ) return;

    if( strlen(user) > SAMPFTP_MAX_CREDENTIAL_LEN ||
        strlen(password) > SAMPFTP_MAX_CREDENTIAL_LEN ) {
        SampFTP_errno = SAMPFTP_STR_OUT_OF_BOUNDS;
        return;
    }

    memcpy( _SampFTP_Connections[connid][USERNAME], user, 0,
            strlen(user)*4, SAMPFTP_MAX_CREDENTIAL_LEN );

    memcpy( _SampFTP_Connections[connid][PASSWORD], password, 0,
            strlen(password)*4, SAMPFTP_MAX_CREDENTIAL_LEN );

    if( _SampFTP_Connections[connid][AUTHENTICATED] ) {
        SampFTP_errno = SAMPFTP_ALREADY_AUTHENTICATED;
        return;
    }

    // If the FTP service is ready, simply send the login command.
    // If it isn't, it's going to be sent when the server sends the
    // appropriate return code.
    if( _SampFTP_Connections[connid][SERVICE_READY] ) {
        format(msg, sizeof(msg), "USER %s\r\n", user);
        socket_send(_SampFTP_Connections[connid][CONTROL_CONN],
                    msg, strlen(msg));
    }

    return;
}

/**
    <summary>
        Sends a request to an FTP server.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect().
    </param>

    <param name="cmd[]">
        The command to be sent to the server.
    </param>

    <param name="callback[]">
        The callback function used to handle responses to this
        request.
    </param>

    <param name="id">
        A unique identifier for distinguishing different responses
        handled by the same callback function.
    </param>
    
    <returns>
        This function does not handle returns.
    </returns>
*/
stock SampFTP_SendCommand(connid, cmd[], callback[], id)
{
    if( !_SampFTP_IsConnValid(connid) ) return false;
    if( _SampFTP_IsSocketOccupied(connid) ) return false;
    if( !_SampFTP_IsAuthenticated(connid) ) return false;

    socket_send(_SampFTP_Connections[connid][CONTROL_CONN],
                cmd, strlen(cmd));
    socket_send(_SampFTP_Connections[connid][CONTROL_CONN],
                "\r\n", 2);

    _SampFTP_Connections[connid][MSG_SENT_AT] = GetTickCount();
    _SampFTP_Connections[connid][MSG_UNIQUE_ID] = id;

    memcpy( _SampFTP_Connections[connid][MSG_CALLBACK], callback, 0,
            strlen(callback)*4, SAMPFTP_MAX_CALLBACK_LEN );    
    return true;
}

/**
    <summary>
        Checks if the connection instance can be used to
        send FTP commands.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect()
        (or, in this case, _SampFTP_InitConn()).
    </param>

    <returns>
        -1 if the connection doesn't exist, 0 if the connection
        exists but no one has logged in yet, 1 otherwise.
    </returns>
*/
stock SampFTP_CanProceed(connid)
{
    if( !_SampFTP_IsConnValid(connid) ) return -1;

    return _SampFTP_Connections[connid][AUTHENTICATED] ? 1 : 0;
}

/**
    <summary>
        Checks if the FTP server is ready to provide services
        to the user. More specifically, this function checks
        whether the protocol interpreter has received a message
        indicating that the FTP server is ready to accept login
        credentials.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect()
        (or, in this case, _SampFTP_InitConn()).
    </param>

    <returns>
        -1 if the connection doesn't exist, 0 if the connection
        exists but the server hasn't sent the appropriate status code,
        1 otherwise.
    </returns>

    <remarks>
        This command does not check if the line is actually occupied by
        another request. If you try to send a message through an occupied
        socket, the error code SAMPFTP_MESSAGE_IS_BEING_SENT will be
        set by the appropriate function.
    </remarks>
*/
stock SampFTP_IsReady(connid)
{
    if( !_SampFTP_IsConnValid(connid) ) return -1;

    return _SampFTP_Connections[connid][SERVICE_READY] ? 1 : 0;
}

/**
    <summary>
        Initializes a socket and connects to the
        given host.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect()
        (or, in this case, _SampFTP_InitConn()).
    </param>

    <param name="host[]">
        The host name or IP address to connect to.
    </param>

    <param name="port">
        The port (21 is the default ftp port) of the server.
    </param>

    <returns>
        True if the connection was successful, false otherwise.
    </returns>
*/
stock _SampFTP_ConnectSock_NoTLS(connid, host[], port)
{
    new Socket:sock =
        _SampFTP_Connections[connid][CONTROL_CONN] =
        socket_create(TCP);

    if( !is_socket_valid(sock) ) {
        SampFTP_errno = SAMPFTP_SOCKET_CREATION_FAILED;
        return false;     
    }

    if( !socket_connect(sock, host, port) ) {
        SampFTP_errno = SAMPFTP_CONNECTION_FAILED;
        return false;
    }

    return true;
}

/**
    <summary>
        Initializes a connection instance.
    </summary>

    <returns>
        The ID of the connection if one could be
        created, -1 otherwise.
    </returns>
*/
stock _SampFTP_InitConn()
{
    _samptcpip_dummy(SampFTP_errno);
    for( new i = 0; i<SAMPFTP_MAX_CONNS; i++ ) {
        if( !_SampFTP_Connections[i][INITIALIZED] ) {
            _SampFTP_Connections[i][INITIALIZED] = true;
            return i;
        }
    }
    
    SampFTP_errno = SAMPFTP_MAX_CONNS_REACHED;
    return -1;
}

/**
    <summary>
        Destroys a connection instance.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect()
    </param>
    
    <returns>
        True if the action was completed successfully, false otherwise.
    </returns>
*/
stock _SampFTP_DestroyConn(connid)
{
    if( !_SampFTP_CheckConnBounds(connid) ) return false;

    if( !_SampFTP_Connections[connid][INITIALIZED] ) {
        SampFTP_errno = SAMPFTP_OBJ_NOT_INITIALIZED;
        return false;
    }
    
    _SampFTP_ClearConn(connid);
    return true;
}