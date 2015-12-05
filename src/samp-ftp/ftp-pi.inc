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

    // Call a callback function.
    CallLocalFunction("SampFTP_Response", "iis", connid, code, data);

    // Print the code for debug purposes.
    printf("code: %d", code);

    _PawnFtp_ProcessMessage(connid, id, code, data);
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
_PawnFtp_ProcessMessage(connid, Socket:id, code, data[])
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
            CallLocalFunction(  _SampFTP_Connections[connid][MSG_CALLBACK],
                                "is",
                                _SampFTP_Connections[connid][MSG_UNIQUE_ID],
                                data);
        } 
    } 
}

public onSocketAnswer(Socket:id, data[])
{
    for( new i = 0; i<SAMPFTP_MAX_CONNS; i++ ) {
        if( is_socket_valid(_SampFTP_Connections[i][CONTROL_CONN])
            && _SampFTP_Connections[i][CONTROL_CONN] == id ) {
            _SampFTP_ProtocolInterpreter(i, id, data);
            break;
        }
    }
}


#if defined _ALS_onSocketAnswer
    #undef onSocketAnswer
#else
    #define _ALS_onSocketAnswer
#endif
#define onSocketAnswer _PawnFtp_onSocketAnswer