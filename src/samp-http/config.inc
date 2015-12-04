/* samptcpip -> samp-http
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-http/config.inc - the samp-http configuration file
 *
 */

// The maximum number of headers that can be added
// to a Request object.
#define MAX_HEADERS 50

// The maximum URL length that samp-http can deal
// with.
#define MAX_URL_LEN 128

// The maximum length of user-suppleid callback
// functions.
#define MAX_FUNCTION_NAME 50