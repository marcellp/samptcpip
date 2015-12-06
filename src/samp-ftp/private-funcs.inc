/* samptcpip -> samp-ftp
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-ftp/private-funcs.inc - private functions
 *
 */

/**
    <summary>
        Checks if a given connection ID corresponds to a valid array index.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect()
    </param>
    
    <returns>
        False if the array index is out of bounds, true otherwise.
    </returns>
*/
stock _SampFTP_CheckConnBounds(connid)
{
    if( connid >= SAMPFTP_MAX_CONNS || connid < 0 ) {
        SampFTP_errno = SAMPFTP_OUT_OF_BOUNDS;
        return false;
    }
    return true;
}

/**
    <summary>
        Checks if a given connection ID is valid.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect()
    </param>
    
    <returns>
        False if the connection ID is invalid, true otherwise.
    </returns>
*/
stock _SampFTP_IsConnValid(connid)
{
    if( !_SampFTP_CheckConnBounds(connid) ) return false;

    if( !_SampFTP_Connections[connid][INITIALIZED] ) {
        SampFTP_errno = SAMPFTP_OBJ_NOT_INITIALIZED;
        return false;
    }

    return true;
}

/**
    <summary>
        Checks if a given connection is busy with requests.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect()
    </param>
    
    <returns>
        True if the connection is occupied, false otherwise.
    </returns>
*/
stock _SampFTP_IsSocketOccupied(connid)
{
    if( ((GetTickCount() - _SampFTP_Connections[connid][MSG_SENT_AT])
        < SAMPFTP_NO_RESPONSE_TIMEOUT)
        || _SampFTP_Connections[connid][MSG_CALLBACK][0] != '\0' ) {
        SampFTP_errno = SAMPFTP_MESSAGE_IS_BEING_SENT;
        return true;       
    }
    return false;
}

/**
    <summary>
        Checks if the connection is authenticated.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect()
    </param>
    
    <returns>
        False if the connection is not authenticated,
        true otherwise.
    </returns>
*/
stock _SampFTP_IsAuthenticated(connid)
{
    if( !_SampFTP_Connections[connid][AUTHENTICATED] ) {
        SampFTP_errno = SAMPFTP_NOT_AUTHENTICATED;
        return false;
    }
    return true;
}

/**
    <summary>
        Clears a given samp-ftp connection.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect()
    </param>
    
    <returns>
        This function does not handle returns.
    </returns>
*/
stock _SampFTP_ClearConn(connid)
{
    if( !_PawnTest_CheckSuiteBounds(connid) ) return;

    if( is_socket_valid(_SampFTP_Connections[connid][CONNSOCKET]) )
        socket_destroy(_SampFTP_Connections[connid][CONNSOCKET]);

    if( is_socket_valid(_SampFTP_Connections[connid][TRANSFERSOCKET]) )
        socket_destroy(_SampFTP_Connections[connid][TRANSFERSOCKET]);

    for(new i = 0; _E_SAMPFTP_CONN:i < _E_SAMPFTP_CONN; i++) {
        _SampFTP_Connections[connid][_E_SAMPFTP_CONN:i] = 0;
    }

    return;
}