/* samptcpip -> samp-ftp
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-ftp/main.inc - enumerations and declarations
 *
 */

enum {
    SAMPFTP_NOERROR,
    SAMPFTP_OUT_OF_BOUNDS,
    SAMPFTP_OBJ_NOT_INITIALIZED,
    SAMPFTP_MAX_CONNS_REACHED,
    SAMPFTP_NO_HOSTNAME_SET,
    SAMPFTP_STR_OUT_OF_BOUNDS,
    SAMPFTP_INVALID_PORT,
    SAMPFTP_INVALID_ACTION,
    SAMPFTP_SOCKET_CREATION_FAILED,
    SAMPFTP_CONNECTION_FAILED,
    SAMPFTP_SERVICE_NOT_READY,
    SAMPFTP_ALREADY_AUTHENTICATED,
    SAMPFTP_NOT_AUTHENTICATED,
    SAMPFTP_MESSAGE_IS_BEING_SENT
}

enum {
    SAMPFTP_DOWNLOAD,
    SAMPFTP_UPLOAD
}

enum _E_SAMPFTP_CONN {
    bool:INITIALIZED,
    bool:SSL,
    bool:SERVICE_READY,
    bool:AUTHENTICATED,
    bool:RELOGIN,
    MSG_SENT_AT,
    MSG_CALLBACK[SAMPFTP_MAX_CALLBACK_LEN],
    MSG_UNIQUE_ID,
    USERNAME[SAMPFTP_MAX_CREDENTIAL_LEN],
    PASSWORD[SAMPFTP_MAX_CREDENTIAL_LEN],
    Socket:CONTROL_CONN,
    Socket:DATA_CONN
}
new _SampFTP_Connections[SAMPFTP_MAX_CONNS][_E_SAMPFTP_CONN];

new SampFTP_errno;