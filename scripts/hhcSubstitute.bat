@ECHO OFF

ECHO Running LA HHC substitute script

"C:/Program Files (x86)/HTML Help Workshop/hhc.exe" %1 %2 %3

REM Force exit with a 0 error code (hhc often returns an error code of 1, even though it successfully generated the output file)
EXIT /B 0
