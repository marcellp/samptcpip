/* samptcpip -> samp-ftp
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-ftp/callbacks.inc - callback declarations
 *
 */

/**
    <summary>
        This callback is called when the server responds
        to a login request.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect().
    </param>

    <param name="bool:success">
        True if the authenatication was successful, false otherwise.
    </param>

    <param name="data[]">
        The message returned by the server. This is either an
        error message (if success == false) or the welcome message
        of the server (may contain useful information).
    </param>

    <returns>
        This callback does not handle returns.
    </returns>
*/
forward SampFTP_AuthResponse(connid, bool:success, data[]);

/**
    <summary>
        This callback is called when the server closes a data connection.
    </summary>

    <param name="connid">
        The ID of the connection returned by SampFTP_Connect().
    </param>

    <param name="callback[]">
        The name of the callback function that got all incoming data
        from the data connection.
    </param>

    <param name="uid">
        The unique ID used by the user to distinguish different types of
        data sent to the same callback.
    </param>

    <returns>
        This callback does not handle returns.
    </returns>
*/
forward SampFTP_CloseDatConn(connid, callback[], uid);