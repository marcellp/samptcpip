/* samptcpip -> samp-ftp
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-ftp/ftp-pi.inc - the samp-ftp protocol interpreter
 *
 */

/**
    <summary>
        The samp-ftp user protocol interpreter (per RFC959).

        It parses and returns to messages returned by the
        FTP server.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect().
    </param>

    <param name="Socket:id">
        The TCP socket for client-server communication.
    </param>

    <param name="data[]">
        The action you want to perform.
    </param>

    <returns>
        This function does not handle returns.
    </returns>
*/
stock _SampFTP_ProtocolInterpreter(connid, Socket:id, data[])
{
    // We got a response, so let's set MSG_SET_AT to 0.
    _SampFTP_Connections[connid][MSG_SENT_AT] = 0;

    // Every FTP message should start with a status code
    // which is 3 digits long. The first digit is at least
    // 1, so we can set this to 0.
    new code = 0;

    // If the message length is less than that, don't even
    // care.
    if( strlen(data) < 3 )
        return;

    // Check if the first 3 characters are numeric. If so,
    // add them to the code. Also, pawn has no native
    // exponentiation operation so this is my hacky alternative.
    for( new i = 0; i < 3; i++ ) {
        if( data[i] < '0' || data[i] > '9' )
            return;

        if( i == 0 )
            code += (data[i] - '0') * 100;
        else if( i == 1 )
            code += (data[i] - '0') * 10;
        else
            code += (data[i] - '0');
    }

    // Print the code for debug purposes.
    printf("code: %d", code);

    _SampFTP_ProcessMessage(connid, id, code, data);
}

/**
    <summary>
        The SampFTP message processor. It takes a message
        and determines what to do with it.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect().
    </param>

    <param name="Socket:id">
        The TCP socket for client-server communication.
    </param>

    <param name="code">
        The FTP server return code.
    </param>

    <param name="data[]">
        The raw data sent by the server.
    </param>

    <returns>
        This function does not handle returns.
    </returns>
*/
_SampFTP_ProcessMessage(connid, Socket:id, code, data[])
{
    #pragma unused data

    // The query to be sent to the server.
    new msg[128];

    switch( code ) {
        case 220: {
            // Service ready for new user.
            _SampFTP_Connections[connid][SERVICE_READY] = true;

            // Check if we have login credentials and try to log in
            // if we do.
            if( _SampFTP_Connections[connid][USERNAME][0] ) {
                format( msg, sizeof(msg), "USER %s\r\n",
                        _SampFTP_Connections[connid][USERNAME]);
                socket_send(id, msg, strlen(msg));
            }
        }
        case 230: {
            // User logged in, proceed.
            _SampFTP_Connections[connid][AUTHENTICATED] = true;
            CallLocalFunction(  "SampFTP_AuthResponse", "ibs",
                                connid, true, data);
        }
        case 226: {
            // Closing data connection.
            if( _SampFTP_Connections[connid][TRANSFER_STATE]
                > _SAMPFTP_PASV_CALLBACK ) {

                // This shouldn't be necessary but do it anyways.
                // The socket should automatically be destroyed by the plugin
                // when the server closes the TCP connection.

                if( is_socket_valid(_SampFTP_Connections[connid][DATA_CONN]) ) {
                    socket_destroy(_SampFTP_Connections[connid][DATA_CONN]);
                }

                // Just to be safe, reset this too.

                _SampFTP_Connections[connid][DATA_CONN] = Socket:-1;

                // CallLocalFunction() will call the callback before
                // MSG_CALLBACK gets set to zero. This means that any SampFTP
                // functions within the library that check for an idle line
                // will NOT work. Fix this by creating temporary buffers and
                // using those instead of the actual private variables.

                new callback[SAMPFTP_MAX_CALLBACK_LEN];
                new uid = _SampFTP_Connections[connid][TRANSFER_UNIQUE_ID];

                memcpy( callback,
                        _SampFTP_Connections[connid][TRANSFER_CALLBACK], 0,
                        strlen(_SampFTP_Connections[connid][TRANSFER_CALLBACK])
                            *4 + 1,
                        SAMPFTP_MAX_CALLBACK_LEN );

                _SampFTP_Connections[connid][TRANSFER_CALLBACK][0] = '\0';
                _SampFTP_Connections[connid][TRANSFER_UNIQUE_ID] = 0;
                _SampFTP_Connections[connid][TRANSFER_STATE] = 0;

                CallLocalFunction( "SampFTP_CloseDatConn", "isi", connid,
                                    callback, uid);
            }
        }
        case 331: {
            // User name OK, need password.
            format( msg, sizeof(msg), "PASS %s\r\n",
                    _SampFTP_Connections[connid][PASSWORD] );

            socket_send(id, msg, strlen(msg));
        }
        case 530: {
            // Not logged in.
            _SampFTP_Connections[connid][AUTHENTICATED] = false;
            CallLocalFunction(  "SampFTP_AuthResponse", "ibs",
                                connid, false, data);
        }
        default: {
            if( _SampFTP_Connections[connid][MSG_CALLBACK][0] != '\0' )
            {
                // CallLocalFunction() will call the callback before
                // MSG_CALLBACK gets set to zero. This means that any SampFTP
                // functions within the library that check for an idle line
                // will NOT work. Fix this by creating temporary buffers and
                // using those instead of the actual private variables.
                new callback[SAMPFTP_MAX_CALLBACK_LEN];
                new uid = _SampFTP_Connections[connid][MSG_UNIQUE_ID];

                memcpy( callback, _SampFTP_Connections[connid][MSG_CALLBACK], 0,
                        strlen(_SampFTP_Connections[connid][MSG_CALLBACK])*4+1,
                        SAMPFTP_MAX_CALLBACK_LEN );

                _SampFTP_Connections[connid][MSG_CALLBACK][0] = '\0';
                _SampFTP_Connections[connid][MSG_UNIQUE_ID] = 0;

                CallLocalFunction(  callback, "iiis", connid, code, uid, data);
            }
        }
    }
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

public onSocketAnswer(Socket:id, data[])
{
    for( new i = 0; i<SAMPFTP_MAX_CONNS; i++ ) {
        if( is_socket_valid(_SampFTP_Connections[i][CONTROL_CONN])
            && _SampFTP_Connections[i][CONTROL_CONN] == id ) {

            _SampFTP_ProtocolInterpreter(i, id, data);
            break;
        }
        else if( is_socket_valid(_SampFTP_Connections[i][DATA_CONN])
            && _SampFTP_Connections[i][DATA_CONN] == id
            && _SampFTP_Connections[i][TRANSFER_STATE]
                != _SAMPFTP_PASV_CALLBACK ) {

            CallLocalFunction(  _SampFTP_Connections[i][TRANSFER_CALLBACK],
                                "iis",
                                i,
                                _SampFTP_Connections[i][TRANSFER_UNIQUE_ID],
                                data);

            break;
        }
    }
}


#if defined _ALS_onSocketAnswer
    #undef onSocketAnswer
#else
    #define _ALS_onSocketAnswer
#endif
#define onSocketAnswer _SampFTP_onSocketAnswer