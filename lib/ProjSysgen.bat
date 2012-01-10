@REM
@REM Copyright (c) Microsoft Corporation.  All rights reserved.
@REM
@REM
@REM Use of this sample source code is subject to the terms of the Microsoft
@REM license agreement under which you licensed this sample source code. If
@REM you did not accept the terms of the license agreement, you are not
@REM authorized to use this sample source code. For the terms of the license,
@REM please see the license agreement between you and Microsoft or, if applicable,
@REM see the LICENSE.RTF on your install media or the root of your tools installation.
@REM THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
@REM
if /i not "%1"=="preproc" goto :Not_Preproc
    goto :EOF
:Not_Preproc
if /i not "%1"=="pass1" goto :Not_Pass1

    REM ==============================================================================================
    REM
    REM Standard SDK features
    REM Post CE 5.0 this no longer supported,
    REM but is included for backwards compatibility.
    REM ==============================================================================================
    if not "%SYSGEN_USDK%"=="1" goto NoUSDK
        set SYSGEN_AYGSHELL=1
        set SYSGEN_AUDIO=1
        set SYSGEN_WININET=1
        set SYSGEN_URLMON=1
        set SYSGEN_CPP_EH_AND_RTTI=1
        set SYSGEN_REDIR=1
        set SYSGEN_MSXML_DOM=1
        set SYSGEN_ATL=1
        set SYSGEN_SOAPTK_CLIENT=1
        set SYSGEN_MSMQ=1
        set SYSGEN_LDAP=1
        set SYSGEN_OBEX_CLIENT=1
        set SYSGEN_AUTH=1
        set SYSGEN_GRADFILL=1
        set SYSGEN_PRINTING=1
        set __SYSGEN_STANSDK=1
        set __SYSGEN_COM_GUIDS=1
        set __SYSGEN_COM_STG=1
        set SYSGEN_COMMDLG=1
        set SYSGEN_STDIOA=1
        set SYSGEN_STANDARDSHELL=1
        set SYSGEN_MSXML_SAX=1
        set SYSGEN_DOTNETV2_SUPPORT=1
    :NoUSDK

    goto :EOF
:Not_Pass1
if /i not "%1"=="pass2" goto :Not_Pass2
    goto :EOF
:Not_Pass2
if /i not "%1"=="report" goto :Not_Report
    goto :EOF
:Not_Report
echo %0 Invalid parameter %1
