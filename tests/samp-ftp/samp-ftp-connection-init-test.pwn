/* samptcpip -> samp-ftp
 * A TCP/IP framework for people who would like to keep their sanity.
 *
 * Copyright (c) 2015 Pék Marcell
 * This project is covered by the MIT License, see the attached LICENSE
 * file for more details.
 *
 * samp-ftp-connection-init-test.pwn - the connection init/destroy unit test
 *
 */

#include <a_samp>
#include <pawn-test/pawn-test>
#include <samptcpip/samp-ftp>

forward Setup();
forward Tear();
forward InvalidConnID();

new Connection = -1;

main()
{
    new TestSuite = PawnTest_InitSuite("samp-ftp conn init unit test");

    PawnTest_SetSetup(TestSuite, "Setup");
    PawnTest_SetTeardown(TestSuite, "Tear");
    PawnTest_AddCase(TestSuite, "InvalidConnID");

    PawnTest_Run(TestSuite);
    PawnTest_DestroySuite(TestSuite);    
}

public Setup()
{
    SampFTP_errno = SAMPFTP_NOERROR;
    Connection = SampFTP_InitConn();

    return  PawnTest_AssertEx(  SampFTP_errno == SAMPFTP_NOERROR,
                                "An FTP connection could not be established.")
            ? PAWNTEST_SETUP_SUCCESSFUL
            : PAWNTEST_SETUP_FAILED;
}

public Tear()
{
    SampFTP_errno = SAMPFTP_NOERROR;
    SampFTP_DestroyConn(Connection);

    return  PawnTest_AssertEx(  SampFTP_errno == SAMPFTP_NOERROR,
                                "The FTP connection could not be destroyed.")
            ? PAWNTEST_TEARDOWN_SUCCESSFUL
            : PAWNTEST_TEARDOWN_FAILED;
}

public InvalidConnID()
{
    new bool:ok = true;

    ok &= PawnTest_AssertEx(!SampFTP_DestroyConn(-1),
                            "Negative connection id" );

    ok &= PawnTest_AssertEx(!SampFTP_DestroyConn(SAMPFTP_MAX_CONNS),
                            "Conn id == MAX_CONNS (off-by-one)" );
    ok &= PawnTest_AssertEx(!SampFTP_DestroyConn(SAMPFTP_MAX_CONNS+1),
                            "Conn id == MAX_CONNS+1" );

    return (ok) ? PAWNTEST_CASE_PASSED : PAWNTEST_CASE_FAILED;
}