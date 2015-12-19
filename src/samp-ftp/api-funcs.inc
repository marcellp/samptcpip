/* samptcpip -> samp-ftp
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-ftp/api-funcs.inc - samp-ftp api functions
 *
 */

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

    <param name="callback[]=\"\"">
        The callback function used to handle responses to this
        request. If no callback function is specified, the response
        will still be interpreted but it won't be reported on.
    </param>

    <param name="id = 0">
        A unique identifier for distinguishing different responses
        handled by the same callback function.
    </param>
    
    <returns>
        False if the message could not be sent through the socket,
        true otherwise.
    </returns>
*/
stock bool:SampFTP_SendCommand(connid, cmd[], callback[] = "", id = 0)
{
    if( !_SampFTP_IsConnValid(connid) ) return false;
    if( !_SampFTP_IsAuthenticated(connid) ) return false;
    if( _SampFTP_IsSocketOccupied(connid) ) return false;

    socket_send(_SampFTP_Connections[connid][CONTROL_CONN],
                cmd, strlen(cmd));
    socket_send(_SampFTP_Connections[connid][CONTROL_CONN],
                "\r\n", 2);

    if( callback[0] != '\0' ) {
        _SampFTP_Connections[connid][MSG_SENT_AT] = GetTickCount();
        _SampFTP_Connections[connid][MSG_UNIQUE_ID] = id;
        
        memcpy( _SampFTP_Connections[connid][MSG_CALLBACK], callback, 0,
                strlen(callback)*4 + 1, SAMPFTP_MAX_CALLBACK_LEN );
    }

    return true;
}

/**
    <summary>
        Enable passive mode.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect().
    </param>

    <param name="callback[]">
        The callback function used to handle the server's response
        to this request.
    </param>

    <param name="id">
        A unique identifier for distinguishing different responses
        handled by the same callback function.
    </param>
    
    <returns>
        False if the message could not be sent through the socket,
        true otherwise.
    </returns>
*/
stock bool:SampFTP_EnablePassiveMode(connid, callback[], id = 0)
{
    if( !_SampFTP_IsConnValid(connid) ) return false;
    if( !_SampFTP_IsAuthenticated(connid) ) return false;
    if( _SampFTP_IsSocketOccupied(connid) ) return false;   
    
    memcpy( _SampFTP_Connections[connid][TRANSFER_CALLBACK], callback, 0,
            strlen(callback)*4 + 1, SAMPFTP_MAX_CALLBACK_LEN );
    _SampFTP_Connections[connid][TRANSFER_STATE] = _SAMPFTP_PASV_CALLBACK;
    _SampFTP_Connections[connid][TRANSFER_UNIQUE_ID] = id;

    return SampFTP_SendCommand(connid, "PASV", "_SampFTP_Passive_Call");
}

/**
    <summary>
        Retrieves a file, a directory or some other data sent by
        the server through the data connection.
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
        False if the message could not be sent through the socket,
        true otherwise.
    </returns>
*/
stock bool:SampFTP_RetrieveData(connid, cmd[], callback[], id = 0)
{
    if( !_SampFTP_IsConnValid(connid) ) return false;
    if( !_SampFTP_IsAuthenticated(connid) ) return false;
    if( _SampFTP_IsSocketOccupied(connid) ) return false;
    if( !_SampFTP_IsDataConnUp(connid) ) return false;

    memcpy( _SampFTP_Connections[connid][TRANSFER_CALLBACK], callback, 0,
            strlen(callback)*4 + 1, SAMPFTP_MAX_CALLBACK_LEN );
    _SampFTP_Connections[connid][TRANSFER_STATE] = _SAMPFTP_RET_TEXT;
    _SampFTP_Connections[connid][TRANSFER_UNIQUE_ID] = id;

    return SampFTP_SendCommand(connid, cmd, "");
}

forward _SampFTP_Passive_Call(connid, code, id, data[]);
public _SampFTP_Passive_Call(connid, code, id, data[])
{
    if( code == 227 ) {
        new ip = 0;
        new ipstr[16];
        new port = 0;
        new Socket:sock;
        new bool:success;

        success = _SampFTP_ParsePASV(data, ip, port);

        if( success && ip != 0 && port != 0 ) {

            if( is_socket_valid(_SampFTP_Connections[connid][DATA_CONN]) ) {
                socket_destroy(_SampFTP_Connections[connid][DATA_CONN]);
            }

            sock = _SampFTP_Connections[connid][DATA_CONN] = socket_create(TCP);

            if( !is_socket_valid(sock) ) {
                SampFTP_errno = SAMPFTP_SOCKET_CREATION_FAILED;
            }

            format( ipstr, 16, "%i.%i.%i.%i",
                    (ip >> 24) & 0xFF,
                    (ip >> 16) & 0xFF,
                    (ip >> 8) & 0xFF,
                    (ip) & 0xFF);

            if( !_SampFTP_Connections[connid][TLS] )
                _SampFTP_ConnectSock_NoTLS(connid, ipstr, port, true);
        }
    }

    // CallLocalFunction() will call the callback before
    // MSG_CALLBACK gets set to zero. This means that any SampFTP
    // functions within the library that check for an idle line
    // will NOT work. Fix this by creating temporary buffers and
    // using those instead of the actual private variables.

    new callback[SAMPFTP_MAX_CALLBACK_LEN];
    new uid = _SampFTP_Connections[connid][TRANSFER_UNIQUE_ID];

    memcpy( callback, _SampFTP_Connections[connid][TRANSFER_CALLBACK], 0,
            strlen(_SampFTP_Connections[connid][TRANSFER_CALLBACK])*4 + 1,
            SAMPFTP_MAX_CALLBACK_LEN );  

    _SampFTP_Connections[connid][TRANSFER_CALLBACK][0] = '\0';
    _SampFTP_Connections[connid][TRANSFER_UNIQUE_ID] = 0;
    _SampFTP_Connections[connid][TRANSFER_STATE] = 0;   

    CallLocalFunction(  callback, "iiis", connid, code, uid, data);
}