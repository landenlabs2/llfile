@rem
@rem  Test llfile
@rem

@echo off
@if EXIST testDir rmdir /s /q testDir

@mkdir testDir

copy Release\llfile.exe testDir\
@pushd testDir

@rem Install
llfile -xi >nul

if not ERRORLEVEL 26 (
   echo "Install failed, exitcode %ERRORLEVEL%"
   goto END
   )
   

@echo on
@prompt $g$s

@set heading="\n=========================(%%s)=========================\n\n"

@cls
@p "-i=Show various colored text" "-p=%heading%"
p -C=red,whiteBg   "-p= red " -C=green,whiteBg "-p=green " -C=blue,whiteBg  "-p=blue on white\n"
p -C=red,blackBg   "-p= red " -C=green,blackBg "-p=green " -C=blue,blackBg  "-p=blue on black\n"
p -C=green+red,redBg "-p= yellow on red\n"
p -C=black,whiteBg "-p= black on white\n"
@p -p=\n\n
@pause

   
@cls
@p "-i=Show time/date text" "-p=%heading%"
p -C=black,whiteBg "-l=DOS Date and Time %%a %%x %%X " "-z=Gmt= %%x %%X"
(date/t & time/t)  | p -C=black,whiteBg -I=- "-p=DOS Date and Time %%s %%s\n"
@p -p=\n\n
@pause

@cls
@p "-i=Show Path environment variable" "-p=%heading%"
p -e=PATH "-p=\t%%s\n"
@p "-p=\n\nNumbered and limited to those with Program in their name\n"
p -e=PATH -P=Program "-p=\t%%4# %%s\n"
@p -p=\n\n
@pause

cls
@p "-i=Search Path environment variable for p*.exe" "-p=%heading%"
p -e=PATH "-p=%%4# %%s\n" p*.exe
@p -p=\n\n
@pause


@cls
@p "-i=Compare Directory listing" "-p=%heading%"
mkdir txt
ld -Sn > txt\ld1.txt
dir /On > txt\dir1.txt

fc /w txt\ld1.txt txt\dir1.txt
@p "-p=\n--(%ERRORLEVEL%)-- Default Directory Comparison results (mostly equal) "
@pause

@cls
@p "-i=Compare Sorted Directory listing" "-p=%heading%"
ld -n -Sn -1=txt\ld2.txt  
dir /On /b > txt\dir2.txt

fc /w txt\ld2.txt txt\dir2.txt
@p "-p=\n--(%ERRORLEVEL%)-- Directory Name comparison results (100% equal) "
@pause

@cls
@p "-i=Compare Sorted Directory listing" "-p=%heading%"
ld -ct lm.exe
ld -S-m -tm  > txt\ld3.log
dir /tw /o-d /O-n  > txt\dir3.log
fc /w txt\ld3.log txt\dir3.log
@p "-p=\n-- Directory Name comparison results (mostly equal) "
@pause

@cls 
@p "-i=Sorted Directory various ways" "-p=%heading%"
@p "-p=---Sort by Extension:\n"
ld -Se txt
@p "-p=\n\n---Sort by reverse Extension:\n"
ld -S-e txt
@pause

@cls
@p "-p=\n\n---Sort by Name:\n"
ld -Sn txt
@p "-p=\n\n---Sort by reverseName:\n"
ld -S-n txt
@pause

@cls
@p "-p=\n\n---Sort by Size:\n"
ld -Ss txt
@p "-p=\n\n---Sort by reverse Size:\n"
ld -S-s txt
@pause

@cls
@p "-p=\n\n---Sort by Creation Time:\n"
ld -Sc -tc txt
@p "-p=\n\n---Sort by Modification Time:\n"
ld -Sm -tm txt
@pause

@cls
@p "-i=File Copy and Compare" "-p=%heading%"
lc -q * dst1\*
lc -q * dst1\sub1\*

xcopy * dst2\* >nul
xcopy * dst2\sub1\* >nul

cmp dst1\sub1\* dst2\sub1\*
@p   "-p=\n--(%ERRORLEVEL%)-- Copy and Compare results (100% equal) "
@pause

@cls
@p "-i=File Copy l* and Compare" "-p=%heading%"
lc l* dst1\sub2\*1
@p "-p=\n\n---(%ERRORLEVEL%) copy l*
cmp -aF dst1\sub2\* dst1\sub1\*
@p "-p=\n--(%ERRORLEVEL%)-- Copy subset and Compare results "
@pause
   
@cls 
@p "-i=Directory usage report" "-p=%heading%"
ld -u dst1\sub1 dst2\sub1
@p -p=\n\n
@pause


:TIMEOP
@cls
@p "-i=Set file times" "-p=%heading%"
@mkdir timeMod

@echo > timeMod\now
@echo > timeMod\yesterday
@echo > timeMod\prevHour
@echo > timeMod\nextHour
@echo > timeMod\tomorrow
ld -ct=now timeMod\now
ld -ct=-1d timeMod\yesterday
ld -ct=-1h timeMod\prevhour
ld -ct=+1h timeMod\nexthour
ld -ct=+1d timeMod\tomorrow

ld -tm timeMod\*
@p -p=\n\n
@pause

@cls
@p "-i=Test file times" "-p=%heading%"
@p "-p=\n----Show Modified time of now (may miss if minute just rolled over) ----\n"
ld -tm -T=mEnow timeMod\*
@p "-p=\n----Show Greater than now, two files nexthour and tomorrow and maybe now----\n"
ld -tm -T=mGnow timeMod\*
@p "-p=\n----Show Less than now, two files prevHour, yesterday ----\n"
ld -tm -T=mLnow timeMod\*
@p "-p=\n----Show Less than -30 minutes, two files prevHour, yesterday ----\n"
ld -tm -T=mL-30m timeMod\*
@p "-p=\n----Show Less than -2 hours, one file  yesterday ----\n"
ld -tm -T=mL-2h timeMod\*
@p "-p=\n----Show Greater than -2 hours, four files  prevHour, nextHour, now, tomorrow ----\n"
ld -tm -T=mG-2h timeMod\*
@p "-p=\n----Show Greater than Jan 1st, 2013, five files ---\n"
ld -tm -T=mG2013:01:01:00:00:00 timeMod\*
@p "-p=\n----Show Less than Dec 31st, 2013, five files ---\n"
ld -tm -T=mL2013:12:31:12:30:59 timeMod\*
@p -p=\n\n
@pause

@cls
lf -F=now,prevHour,nextHour -tm -s
@p   "-p=\n--(%ERRORLEVEL%)-- Find files, default return dir+file count:3 "
lf -E=s -F=now,prevHour,nextHour -s
@p   "-p=\n--(%ERRORLEVEL%)-- Find files, return total found file sizes:39  "
lf -E=f -F=now,prevHour,nextHour -tm
@p   "-p=\n--(%ERRORLEVEL%)-- Find files, return total #files:3  "
@w ld.exe

@cls
@p "-i=Execute attrib command" "-p=%heading%"
ld -n p* | lf -I=- | le -I=- -c="attrib +R "
ld -n p* | lf -I=- | ld -I=-
@p -p=\n\n
@pause


@cls
@p "-i=Move  command" "-p=%heading%"
lm dst1\* new1\*
@p -p=\n\n
@pause

@cls
@p "-i=Move  command" "-p=%heading%"
lm -r dst2\* new1\*
@p -p=\n\n
@pause

@cls
@p "-i=Move  command" "-p=%heading%"
lm -rO dst2\* new1\* 
@p -p=\n\n
@pause

goto END

@cls
@p "-i=Delete Duplicate files" "-p=%heading%"
cmp -d=e dst1\* dst2\*
@p   "-p=\n--(%ERRORLEVEL%)-- compare and delete equal "
@pause

@cls 
@p "-i=Delete Dir (fail on subdirs)" "-p=%heading%"
lr dst1\
@p   "-p=\n--(%ERRORLEVEL%)-- Delete files, fail on subdirectories "
@pause

@cls
@p "-i=Delete Dir and subdirs" "-p=%heading%"
lr -r dst1\
@p   "-p=\n--(%ERRORLEVEL%)-- Delete files and subdirectories "
@pause

:END
@popd
@prompt $p$g
   