/* samptcpip -> samp-http
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-http/main.inc - enumerations and declarations
 *
 */

enum {
    PAWNHTTP_HEAD,
    PAWNHTTP_GET,
    PAWNHTTP_POST,
    PAWNHTTP_PUT,
    PAWNHTTP_PATCH,
    PAWNHTTP_DELETE
}

enum E_PAWNHTTP_REQUEST {
    bool:_INITIALIZED,
    Socket:_SOCKET,

    METHOD,
    URL[MAX_URL_LEN],
    HEADERS[MAX_HEADERS],
    PARAMS,
    CALLBACK_FUNCTION[MAX_FUNCTION_NAME]
}

stock PawnHttp_CreateRequest(method, url[], PawnHttp_Request:req[E_PAWNHTTP_REQUEST])
{
    req[_INITIALIZED] = true;
    req[METHOD] = method;
    memcpy(req[URL], url, 0, strlen(url)*4, MAX_URL_LEN);
    return;
}

stock PawnHttp_SendRequest(PawnHttp_Request:req[E_PAWNHTTP_REQUEST])
{
    new Socket:sock = req[_SOCKET] = socket_create(TCP);
    new shitfuck[255] = "GET /get HTTP/1.1\r\nHost: httpbin.org\r\nUser-Agent: Faszkliens\r\nReferer: http://httpbin.org/\r\n\r\n";

    if( is_socket_valid(sock) ) {
        socket_connect(sock, req[URL], 80);
        socket_send(sock, shitfuck, strlen(shitfuck));

        printf(shitfuck);
    }

    return;
}

public onSocketAnswer(Socket:id, data[])
{
    printf("lol data: %s", data);
}