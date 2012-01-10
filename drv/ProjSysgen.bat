REM Switch on the first parameter to see what stage this is being invoked at
if /i "%1"=="preproc" goto :Preproc
if /i "%1"=="pass1" goto :Pass1
if /i "%1"=="pass2" goto :Pass2
if /i "%1"=="report" goto :Report
echo %0 - Unknown build type parameter: '%1'
goto :EOF

:Preproc
    goto :EOF
:Pass1
	goto :EOF
:Pass2
	goto :EOF
:Report
	goto :EOF
