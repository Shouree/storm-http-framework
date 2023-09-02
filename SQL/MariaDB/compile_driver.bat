@echo off
cd ..

set output=%1

IF EXIST root\sql\%output% EXIT 0

echo Building the mariadb client...

cd Windows\mariadb

set builddir=build\
set flags=/nologo /EHsc /FC /MD /O2
set defines=/DHAVE_ICONV /D_CRT_SECURE_NO_WARNINGS /DTHREAD /DHAVE_COMPRESS /DLIBMARIADB /DHAVE_SCHANNEL /DHAVE_TLS /DHAVE_WINCRYPT
set linkflags=/nologo /MANIFEST /NXCOMPAT /DLL
set libs=ws2_32.lib shlwapi.lib secur32.lib crypt32.lib advapi32.lib bcrypt.lib

REM check if _64, if so, compile for 64-bit. (condition below removes part of string we're looking for)
IF NOT %output:_64.dll=% == %output% (
  set linkflags=%linkflags% /MACHINE:X64
  set builddir=build64\

  echo EXPORTS >exports.def
  echo   mysql_init >>exports.def
  echo   mysql_close >>exports.def
) else (
  echo EXPORTS >exports.def
  echo   _mysql_init@4 >>exports.def
  echo   _mysql_close@4 >>exports.def
)


set flags=%flags% /Fo%builddir%

IF NOT EXIST %builddir% MKDIR %builddir%

cl /c %flags% %defines% /Ilibmariadb /Iinclude /Izlib libmariadb\*.c libmariadb\secure\ma_schannel.c libmariadb\secure\schannel*.c win-iconv\*.c zlib\*.c

REM crypto plugins and auth.
cl /c %flags% %defines% /DMYSQL_CLIENT=1 /wd4244 /wd4146 /Iinclude /Iplugins\auth libmariadb\secure\win_crypt.c plugins\auth\my_auth.c plugins\auth\old_password.c plugins\auth\dialog.c plugins\auth\ref10\*.c plugins\auth\ed25519.c plugins\auth\caching_sha2_pw.c plugins\auth\sha256_pw.c plugins\auth\auth_gssapi_client.c plugins\auth\sspi_client.c plugins\auth\sspi_errmsg.c plugins\auth\old_password.c plugins\auth\mariadb_cleartext.c
REM remaining plugins
cl /c %flags% %defines% /Iinclude plugins\pvio\pvio_socket.c plugins\pvio\pvio_npipe.c plugins\pvio\pvio_shmem.c


link %linkflags% /OUT:%output% %libs% %builddir%*.obj /DEF:exports.def
mt -nologo -manifest %output%.manifest -outputresource:%output%;#2

move /Y %output% ..\..\root\sql\%output%
del %output:.dll=.*%
