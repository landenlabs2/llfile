
@rem
@rem     Hardlink llfile.exe to specific command executables
@rem    http://landenlabs.com/

@rem  Assumes llfile.exe is in current directory.

@del /Q ld.exe lc.exe le.exe lr.exe lm.exe lf.exe p.exe cmp.exe
@rem move llfile-v*.exe llfile.exe

@rem Install and uninstall is now built-in to llfile, use -xu to uninstall
llfile -xi
goto END

@rem short names
fsutil hardlink create ld.exe llfile.exe
fsutil hardlink create lc.exe llfile.exe
fsutil hardlink create le.exe llfile.exe
fsutil hardlink create lr.exe llfile.exe
fsutil hardlink create lm.exe llfile.exe
fsutil hardlink create lf.exe llfile.exe
fsutil hardlink create p.exe llfile.exe
fsutil hardlink create cmp.exe llfile.exe

@rem long names
fsutil hardlink create lldir.exe  llfile.exe
fsutil hardlink create llcopy.exe llfile.exe
fsutil hardlink create llexec.exe llfile.exe
fsutil hardlink create lldel.exe  llfile.exe
fsutil hardlink create llmove.exe llfile.exe
fsutil hardlink create where.exe llfile.exe
fsutil hardlink create llfind.exe llfile.exe
fsutil hardlink create printf.exe llfile.exe
fsutil hardlink create llcmp.exe llfile.exe

:END