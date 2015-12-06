/* samptcpip -> samp-ftp
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-ftp/config.inc - the samp-http configuration file
 *
 */

// The maximum number of concurrent connections
#define SAMPFTP_MAX_CONNS 5

// The maximum length of a callback function
#define SAMPFTP_MAX_CALLBACK_LEN 50

// The maximum length of user credentials
#define SAMPFTP_MAX_CREDENTIAL_LEN 50

// The maximum length of a path
#define SAMPFTP_MAX_PATH_NAME 50

// The time after which a new mesage can be sent if the
// previous one hasn't been processed by the server yet.
#define SAMPFTP_NO_RESPONSE_TIMEOUT 25