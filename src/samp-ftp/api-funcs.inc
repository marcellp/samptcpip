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
stock bool:SampFTP_SendCommand(connid, cmd[], callback[], id = 0)
{
    if( !_SampFTP_IsConnValid(connid) ) return false;
    if( !_SampFTP_IsAuthenticated(connid) ) return false;
    if( _SampFTP_IsSocketOccupied(connid) ) return false;

    if( strlen(callback) == 0 ) {
        SampFTP_errno = SAMPFTP_STR_OUT_OF_BOUNDS;
        return false;
    }

    socket_send(_SampFTP_Connections[connid][CONTROL_CONN],
                cmd, strlen(cmd));
    socket_send(_SampFTP_Connections[connid][CONTROL_CONN],
                "\r\n", 2);

    _SampFTP_Connections[connid][MSG_SENT_AT] = GetTickCount();
    _SampFTP_Connections[connid][MSG_UNIQUE_ID] = id;
    
    memcpy( _SampFTP_Connections[connid][MSG_CALLBACK], callback, 0,
            strlen(callback)*4, SAMPFTP_MAX_CALLBACK_LEN );

    printf("sendcommand");

    return true;
}

stock bool:SampFTP_EnablePassiveMode(connid, callback[], id = 0)
{
    if( !_SampFTP_IsConnValid(connid) ) return false;
    if( !_SampFTP_IsAuthenticated(connid) ) return false;
    if( _SampFTP_IsSocketOccupied(connid) ) return false;    

    memcpy( _SampFTP_Connections[connid][TRANSFER_CALLBACK], callback, 0,
            strlen(callback)*4, SAMPFTP_MAX_CALLBACK_LEN );
    _SampFTP_Connections[connid][TRANSFER_STATE] = _SAMPFTP_PASV_CALLBACK;
    _SampFTP_Connections[connid][TRANSFER_UNIQUE_ID] = id;

    return SampFTP_SendCommand(connid, "PASV", "_SampFTP_Passive_Call");
}

/**
    <summary>
        Analyzes a given piece of data for PASV information.
    </summary>

    <param name="&ip_ref">
        A 32-bit number for the given IP address, passed by reference.
    </param>

    <param name="&port_ref">
        A number for the parsed port, passed by reference.
    </param>

    <returns>
        True if the numbers parsed by the argument within one "block"
        are within the byte range (0-255), false otherwise.
    </returns>

    <remarks>
        This function aims to support as many different PASV implementations
        as possible, but it will generate a lot of false positives if the
        server deliberately tries to send malformed input such as:

        11bar2, foo55,,,111,0.

        Malformed input shouldn't be able to generate invalid IP addresses
        or port numbers, though.

        For this function to work, the data[] variable should contain the
        COMPLETE 227 response sent by the server.
    </remarks>
*/
stock bool:_SampFTP_ParsePASV(data[], &ip_ref, &port_ref)
{
    /* 0: first byte of ip
       1: second byte of ip
       2: third byte of ip
       3: fourth byte of ip
       4: first byte of port
       5: second byte of port
    */
    new parse_state = 0;

    // The accumulator that stores the value of the specific byte.
    new acc = 0;

    // The variable for storing the IP address.
    new ip = 0;

    // The variable for storing the port.
    new port = 0;

    // The first 3 bits represent the status code.
    for( new i = 3; i<strlen(data); i++ ) {
        if( parse_state > 5 )
            break;

        if( data[i] >= '0' && data[i] <= '9' ) {
            acc = (acc * 10) + (data[i] - '0');
        } else if( (parse_state < 5 && data[i] == ',')
                    || (parse_state == 5
                        && ( data[i] == ',' || data[i] == '\r'
                            || data[i] == '\n' || data[i] == ')'
                            || data[i] == '\0'
                            )
                        )
                    ) 
        {
            if( acc > 255 )
                return false;

            if( parse_state == 0 )
                ip += acc << 24;
            else if( parse_state == 1 )
                ip += acc << 16;
            else if( parse_state == 2 )
                ip += acc << 8;
            else if( parse_state == 3 )
                ip += acc;
            else if( parse_state == 4 )
                port += acc << 8;
            else if( parse_state == 5 )
                port += acc;

            acc = 0;
            parse_state++;
        }
    }

    ip_ref = ip;
    port_ref = port;
    return true;
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
            strlen(_SampFTP_Connections[connid][TRANSFER_CALLBACK])*4,
            SAMPFTP_MAX_CALLBACK_LEN );  

    _SampFTP_Connections[connid][TRANSFER_CALLBACK][0] = '\0';
    _SampFTP_Connections[connid][TRANSFER_UNIQUE_ID] = 0;
    _SampFTP_Connections[connid][TRANSFER_STATE] = 0;   

    CallLocalFunction(  callback, "iiis", connid, code, uid, data);
}