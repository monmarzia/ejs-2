#
#   ejs-solaris-debug.sh -- Build It Shell Script to build Embedthis Ejscript
#

ARCH="x86"
ARCH="`uname -m | sed 's/i.86/x86/;s/x86_64/x64/'`"
OS="solaris"
PROFILE="debug"
CONFIG="${OS}-${ARCH}-${PROFILE}"
CC="gcc"
LD="/usr/bin/ld"
CFLAGS="-Wall -fPIC -g"
DFLAGS="-D_REENTRANT -DPIC -DBIT_DEBUG"
IFLAGS="-I${CONFIG}/inc"
LDFLAGS="-g"
LIBPATHS="-L${CONFIG}/bin"
LIBS="-llxnet -lrt -lsocket -lpthread -lm -ldl"

[ ! -x ${CONFIG}/inc ] && mkdir -p ${CONFIG}/inc ${CONFIG}/obj ${CONFIG}/lib ${CONFIG}/bin

[ ! -f ${CONFIG}/inc/bit.h ] && cp projects/ejs-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
if ! diff ${CONFIG}/inc/bit.h projects/ejs-${OS}-${PROFILE}-bit.h >/dev/null ; then
	cp projects/ejs-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h
fi

rm -rf ${CONFIG}/inc/mpr.h
cp -r src/deps/mpr/mpr.h ${CONFIG}/inc/mpr.h

${CC} -c -o ${CONFIG}/obj/mprLib.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/mprLib.c

${CC} -shared -o ${CONFIG}/bin/libmpr.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/mprLib.o ${LIBS}

${CC} -c -o ${CONFIG}/obj/mprSsl.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/mprSsl.c

${CC} -shared -o ${CONFIG}/bin/libmprssl.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/mprSsl.o -lmpr ${LIBS}

${CC} -c -o ${CONFIG}/obj/manager.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/manager.c

${CC} -o ${CONFIG}/bin/ejsman ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/manager.o -lmpr ${LIBS} ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/makerom.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/mpr/makerom.c

${CC} -o ${CONFIG}/bin/makerom ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/makerom.o -lmpr ${LIBS} ${LDFLAGS}

rm -rf ${CONFIG}/inc/pcre.h
cp -r src/deps/pcre/pcre.h ${CONFIG}/inc/pcre.h

${CC} -c -o ${CONFIG}/obj/pcre.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/pcre/pcre.c

${CC} -shared -o ${CONFIG}/bin/libpcre.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/pcre.o ${LIBS}

rm -rf ${CONFIG}/inc/http.h
cp -r src/deps/http/http.h ${CONFIG}/inc/http.h

${CC} -c -o ${CONFIG}/obj/httpLib.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/http/httpLib.c

${CC} -shared -o ${CONFIG}/bin/libhttp.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/httpLib.o -lpcre -lmpr ${LIBS}

${CC} -c -o ${CONFIG}/obj/http.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/http/http.c

${CC} -o ${CONFIG}/bin/http ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/http.o -lhttp ${LIBS} -lpcre -lmpr ${LDFLAGS}

rm -rf ${CONFIG}/inc/sqlite3.h
cp -r src/deps/sqlite/sqlite3.h ${CONFIG}/inc/sqlite3.h

${CC} -c -o ${CONFIG}/obj/sqlite3.o -mtune=generic -fPIC ${LDFLAGS} -w ${DFLAGS} -I${CONFIG}/inc src/deps/sqlite/sqlite3.c

${CC} -shared -o ${CONFIG}/bin/libsqlite3.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/sqlite3.o ${LIBS}

${CC} -c -o ${CONFIG}/obj/sqlite.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/sqlite/sqlite.c

${CC} -o ${CONFIG}/bin/sqlite ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/sqlite.o -lsqlite3 ${LIBS} ${LDFLAGS}

rm -rf ${CONFIG}/inc/zlib.h
cp -r src/deps/zlib/zlib.h ${CONFIG}/inc/zlib.h

${CC} -c -o ${CONFIG}/obj/zlib.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/deps/zlib/zlib.c

${CC} -shared -o ${CONFIG}/bin/libzlib.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/zlib.o ${LIBS}

rm -rf ${CONFIG}/inc/ejs.cache.local.slots.h
cp -r src/slots/ejs.cache.local.slots.h ${CONFIG}/inc/ejs.cache.local.slots.h

rm -rf ${CONFIG}/inc/ejs.db.sqlite.slots.h
cp -r src/slots/ejs.db.sqlite.slots.h ${CONFIG}/inc/ejs.db.sqlite.slots.h

rm -rf ${CONFIG}/inc/ejs.slots.h
cp -r src/slots/ejs.slots.h ${CONFIG}/inc/ejs.slots.h

rm -rf ${CONFIG}/inc/ejs.web.slots.h
cp -r src/slots/ejs.web.slots.h ${CONFIG}/inc/ejs.web.slots.h

rm -rf ${CONFIG}/inc/ejs.zlib.slots.h
cp -r src/slots/ejs.zlib.slots.h ${CONFIG}/inc/ejs.zlib.slots.h

rm -rf ${CONFIG}/inc/ejs.h
cp -r src/ejs.h ${CONFIG}/inc/ejs.h

rm -rf ${CONFIG}/inc/ejsByteCode.h
cp -r src/ejsByteCode.h ${CONFIG}/inc/ejsByteCode.h

rm -rf ${CONFIG}/inc/ejsByteCodeTable.h
cp -r src/ejsByteCodeTable.h ${CONFIG}/inc/ejsByteCodeTable.h

rm -rf ${CONFIG}/inc/ejsCompiler.h
cp -r src/ejsCompiler.h ${CONFIG}/inc/ejsCompiler.h

rm -rf ${CONFIG}/inc/ejsCustomize.h
cp -r src/ejsCustomize.h ${CONFIG}/inc/ejsCustomize.h

${CC} -c -o ${CONFIG}/obj/ecAst.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/compiler/ecAst.c

${CC} -c -o ${CONFIG}/obj/ecCodeGen.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/compiler/ecCodeGen.c

${CC} -c -o ${CONFIG}/obj/ecCompiler.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/compiler/ecCompiler.c

${CC} -c -o ${CONFIG}/obj/ecLex.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/compiler/ecLex.c

${CC} -c -o ${CONFIG}/obj/ecModuleWrite.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/compiler/ecModuleWrite.c

${CC} -c -o ${CONFIG}/obj/ecParser.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/compiler/ecParser.c

${CC} -c -o ${CONFIG}/obj/ecState.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/compiler/ecState.c

${CC} -c -o ${CONFIG}/obj/dtoa.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/dtoa.c

${CC} -c -o ${CONFIG}/obj/ejsApp.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsApp.c

${CC} -c -o ${CONFIG}/obj/ejsArray.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsArray.c

${CC} -c -o ${CONFIG}/obj/ejsBlock.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsBlock.c

${CC} -c -o ${CONFIG}/obj/ejsBoolean.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsBoolean.c

${CC} -c -o ${CONFIG}/obj/ejsByteArray.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsByteArray.c

${CC} -c -o ${CONFIG}/obj/ejsCache.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsCache.c

${CC} -c -o ${CONFIG}/obj/ejsCmd.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsCmd.c

${CC} -c -o ${CONFIG}/obj/ejsConfig.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsConfig.c

${CC} -c -o ${CONFIG}/obj/ejsDate.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsDate.c

${CC} -c -o ${CONFIG}/obj/ejsDebug.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsDebug.c

${CC} -c -o ${CONFIG}/obj/ejsError.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsError.c

${CC} -c -o ${CONFIG}/obj/ejsFile.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsFile.c

${CC} -c -o ${CONFIG}/obj/ejsFileSystem.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsFileSystem.c

${CC} -c -o ${CONFIG}/obj/ejsFrame.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsFrame.c

${CC} -c -o ${CONFIG}/obj/ejsFunction.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsFunction.c

${CC} -c -o ${CONFIG}/obj/ejsGC.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsGC.c

${CC} -c -o ${CONFIG}/obj/ejsGlobal.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsGlobal.c

${CC} -c -o ${CONFIG}/obj/ejsHttp.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsHttp.c

${CC} -c -o ${CONFIG}/obj/ejsIterator.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsIterator.c

${CC} -c -o ${CONFIG}/obj/ejsJSON.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsJSON.c

${CC} -c -o ${CONFIG}/obj/ejsLocalCache.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsLocalCache.c

${CC} -c -o ${CONFIG}/obj/ejsMath.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsMath.c

${CC} -c -o ${CONFIG}/obj/ejsMemory.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsMemory.c

${CC} -c -o ${CONFIG}/obj/ejsMprLog.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsMprLog.c

${CC} -c -o ${CONFIG}/obj/ejsNamespace.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsNamespace.c

${CC} -c -o ${CONFIG}/obj/ejsNull.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsNull.c

${CC} -c -o ${CONFIG}/obj/ejsNumber.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsNumber.c

${CC} -c -o ${CONFIG}/obj/ejsObject.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsObject.c

${CC} -c -o ${CONFIG}/obj/ejsPath.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsPath.c

${CC} -c -o ${CONFIG}/obj/ejsPot.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsPot.c

${CC} -c -o ${CONFIG}/obj/ejsRegExp.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsRegExp.c

${CC} -c -o ${CONFIG}/obj/ejsSocket.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsSocket.c

${CC} -c -o ${CONFIG}/obj/ejsString.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsString.c

${CC} -c -o ${CONFIG}/obj/ejsSystem.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsSystem.c

${CC} -c -o ${CONFIG}/obj/ejsTimer.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsTimer.c

${CC} -c -o ${CONFIG}/obj/ejsType.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsType.c

${CC} -c -o ${CONFIG}/obj/ejsUri.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsUri.c

${CC} -c -o ${CONFIG}/obj/ejsVoid.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsVoid.c

${CC} -c -o ${CONFIG}/obj/ejsWebSocket.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsWebSocket.c

${CC} -c -o ${CONFIG}/obj/ejsWorker.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsWorker.c

${CC} -c -o ${CONFIG}/obj/ejsXML.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsXML.c

${CC} -c -o ${CONFIG}/obj/ejsXMLList.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsXMLList.c

${CC} -c -o ${CONFIG}/obj/ejsXMLLoader.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/core/src/ejsXMLLoader.c

${CC} -c -o ${CONFIG}/obj/ejsByteCode.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vm/ejsByteCode.c

${CC} -c -o ${CONFIG}/obj/ejsException.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vm/ejsException.c

${CC} -c -o ${CONFIG}/obj/ejsHelper.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vm/ejsHelper.c

${CC} -c -o ${CONFIG}/obj/ejsInterp.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vm/ejsInterp.c

${CC} -c -o ${CONFIG}/obj/ejsLoader.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vm/ejsLoader.c

${CC} -c -o ${CONFIG}/obj/ejsModule.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vm/ejsModule.c

${CC} -c -o ${CONFIG}/obj/ejsScope.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vm/ejsScope.c

${CC} -c -o ${CONFIG}/obj/ejsService.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/vm/ejsService.c

${CC} -shared -o ${CONFIG}/bin/libejs.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ecAst.o ${CONFIG}/obj/ecCodeGen.o ${CONFIG}/obj/ecCompiler.o ${CONFIG}/obj/ecLex.o ${CONFIG}/obj/ecModuleWrite.o ${CONFIG}/obj/ecParser.o ${CONFIG}/obj/ecState.o ${CONFIG}/obj/dtoa.o ${CONFIG}/obj/ejsApp.o ${CONFIG}/obj/ejsArray.o ${CONFIG}/obj/ejsBlock.o ${CONFIG}/obj/ejsBoolean.o ${CONFIG}/obj/ejsByteArray.o ${CONFIG}/obj/ejsCache.o ${CONFIG}/obj/ejsCmd.o ${CONFIG}/obj/ejsConfig.o ${CONFIG}/obj/ejsDate.o ${CONFIG}/obj/ejsDebug.o ${CONFIG}/obj/ejsError.o ${CONFIG}/obj/ejsFile.o ${CONFIG}/obj/ejsFileSystem.o ${CONFIG}/obj/ejsFrame.o ${CONFIG}/obj/ejsFunction.o ${CONFIG}/obj/ejsGC.o ${CONFIG}/obj/ejsGlobal.o ${CONFIG}/obj/ejsHttp.o ${CONFIG}/obj/ejsIterator.o ${CONFIG}/obj/ejsJSON.o ${CONFIG}/obj/ejsLocalCache.o ${CONFIG}/obj/ejsMath.o ${CONFIG}/obj/ejsMemory.o ${CONFIG}/obj/ejsMprLog.o ${CONFIG}/obj/ejsNamespace.o ${CONFIG}/obj/ejsNull.o ${CONFIG}/obj/ejsNumber.o ${CONFIG}/obj/ejsObject.o ${CONFIG}/obj/ejsPath.o ${CONFIG}/obj/ejsPot.o ${CONFIG}/obj/ejsRegExp.o ${CONFIG}/obj/ejsSocket.o ${CONFIG}/obj/ejsString.o ${CONFIG}/obj/ejsSystem.o ${CONFIG}/obj/ejsTimer.o ${CONFIG}/obj/ejsType.o ${CONFIG}/obj/ejsUri.o ${CONFIG}/obj/ejsVoid.o ${CONFIG}/obj/ejsWebSocket.o ${CONFIG}/obj/ejsWorker.o ${CONFIG}/obj/ejsXML.o ${CONFIG}/obj/ejsXMLList.o ${CONFIG}/obj/ejsXMLLoader.o ${CONFIG}/obj/ejsByteCode.o ${CONFIG}/obj/ejsException.o ${CONFIG}/obj/ejsHelper.o ${CONFIG}/obj/ejsInterp.o ${CONFIG}/obj/ejsLoader.o ${CONFIG}/obj/ejsModule.o ${CONFIG}/obj/ejsScope.o ${CONFIG}/obj/ejsService.o -lhttp ${LIBS} -lpcre -lmpr

${CC} -c -o ${CONFIG}/obj/ejs.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cmd/ejs.c

${CC} -o ${CONFIG}/bin/ejs ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejs.o -lejs ${LIBS} -lhttp -lpcre -lmpr ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/ejsc.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cmd/ejsc.c

${CC} -o ${CONFIG}/bin/ejsc ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsc.o -lejs ${LIBS} -lhttp -lpcre -lmpr ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/ejsmod.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/cmd src/cmd/ejsmod.c

${CC} -c -o ${CONFIG}/obj/doc.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/cmd src/cmd/doc.c

${CC} -c -o ${CONFIG}/obj/docFiles.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/cmd src/cmd/docFiles.c

${CC} -c -o ${CONFIG}/obj/listing.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/cmd src/cmd/listing.c

${CC} -c -o ${CONFIG}/obj/slotGen.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/cmd src/cmd/slotGen.c

${CC} -o ${CONFIG}/bin/ejsmod ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsmod.o ${CONFIG}/obj/doc.o ${CONFIG}/obj/docFiles.o ${CONFIG}/obj/listing.o ${CONFIG}/obj/slotGen.o -lejs ${LIBS} -lhttp -lpcre -lmpr ${LDFLAGS}

${CC} -c -o ${CONFIG}/obj/ejsrun.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/cmd/ejsrun.c

${CC} -o ${CONFIG}/bin/ejsrun ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsrun.o -lejs ${LIBS} -lhttp -lpcre -lmpr ${LDFLAGS}

cd src/core >/dev/null ;\
../../${CONFIG}/bin/ejsc --out ../../${CONFIG}/bin/ejs.mod --debug --optimize 9 --bind --require null *.es  ;\
../../${CONFIG}/bin/ejsmod --require null --cslots ../../${CONFIG}/bin/ejs.mod ;\
if ! diff ejs.slots.h ../../${CONFIG}/inc/ejs.slots.h >/dev/null; then cp ejs.slots.h ../../${CONFIG}/inc; fi ;\
rm -f ejs.slots.h ;\
cd - >/dev/null 

rm -rf ${CONFIG}/bin/bit.es
cp -r src/jems/ejs.bit/bit.es ${CONFIG}/bin/bit.es

cd src/jems/ejs.bit >/dev/null ;\
rm -fr ../../../${CONFIG}/bin/bits ;\
cp -r bits ../../../${CONFIG}/bin ;\
cd - >/dev/null 

#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/ejs.zlib.mod
${CC} -c -o ${CONFIG}/obj/ejsZlib.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/jems/ejs.zlib/ejsZlib.c

${CC} -shared -o ${CONFIG}/bin/libejs.zlib.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsZlib.o -lzlib -lejs ${LIBS} -lhttp -lpcre -lmpr

${CC} -o ${CONFIG}/bin/bit ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsrun.o ${CONFIG}/obj/mprLib.o ${CONFIG}/obj/pcre.o ${CONFIG}/obj/httpLib.o ${CONFIG}/obj/ecAst.o ${CONFIG}/obj/ecCodeGen.o ${CONFIG}/obj/ecCompiler.o ${CONFIG}/obj/ecLex.o ${CONFIG}/obj/ecModuleWrite.o ${CONFIG}/obj/ecParser.o ${CONFIG}/obj/ecState.o ${CONFIG}/obj/dtoa.o ${CONFIG}/obj/ejsApp.o ${CONFIG}/obj/ejsArray.o ${CONFIG}/obj/ejsBlock.o ${CONFIG}/obj/ejsBoolean.o ${CONFIG}/obj/ejsByteArray.o ${CONFIG}/obj/ejsCache.o ${CONFIG}/obj/ejsCmd.o ${CONFIG}/obj/ejsConfig.o ${CONFIG}/obj/ejsDate.o ${CONFIG}/obj/ejsDebug.o ${CONFIG}/obj/ejsError.o ${CONFIG}/obj/ejsFile.o ${CONFIG}/obj/ejsFileSystem.o ${CONFIG}/obj/ejsFrame.o ${CONFIG}/obj/ejsFunction.o ${CONFIG}/obj/ejsGC.o ${CONFIG}/obj/ejsGlobal.o ${CONFIG}/obj/ejsHttp.o ${CONFIG}/obj/ejsIterator.o ${CONFIG}/obj/ejsJSON.o ${CONFIG}/obj/ejsLocalCache.o ${CONFIG}/obj/ejsMath.o ${CONFIG}/obj/ejsMemory.o ${CONFIG}/obj/ejsMprLog.o ${CONFIG}/obj/ejsNamespace.o ${CONFIG}/obj/ejsNull.o ${CONFIG}/obj/ejsNumber.o ${CONFIG}/obj/ejsObject.o ${CONFIG}/obj/ejsPath.o ${CONFIG}/obj/ejsPot.o ${CONFIG}/obj/ejsRegExp.o ${CONFIG}/obj/ejsSocket.o ${CONFIG}/obj/ejsString.o ${CONFIG}/obj/ejsSystem.o ${CONFIG}/obj/ejsTimer.o ${CONFIG}/obj/ejsType.o ${CONFIG}/obj/ejsUri.o ${CONFIG}/obj/ejsVoid.o ${CONFIG}/obj/ejsWebSocket.o ${CONFIG}/obj/ejsWorker.o ${CONFIG}/obj/ejsXML.o ${CONFIG}/obj/ejsXMLList.o ${CONFIG}/obj/ejsXMLLoader.o ${CONFIG}/obj/ejsByteCode.o ${CONFIG}/obj/ejsException.o ${CONFIG}/obj/ejsHelper.o ${CONFIG}/obj/ejsInterp.o ${CONFIG}/obj/ejsLoader.o ${CONFIG}/obj/ejsModule.o ${CONFIG}/obj/ejsScope.o ${CONFIG}/obj/ejsService.o ${CONFIG}/obj/zlib.o ${CONFIG}/obj/ejsZlib.o ${LIBS} ${LDFLAGS}

#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/ejs.unix.mod
#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/jem.es
${CC} -o ${CONFIG}/bin/jem ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsrun.o -lejs ${LIBS} -lhttp -lpcre -lmpr ${LDFLAGS}

#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/ejs.db.mod
#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/ejs.db.mapper.mod
#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/ejs.db.sqlite.mod
${CC} -c -o ${CONFIG}/obj/ejsSqlite.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc src/jems/ejs.db.sqlite/ejsSqlite.c

${CC} -shared -o ${CONFIG}/bin/libejs.db.sqlite.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsSqlite.o -lsqlite3 -lejs -lmpr ${LIBS} -lhttp -lpcre

cd src/jems/ejs.web >/dev/null ;\
../../../${CONFIG}/bin/ejsc --out ../../../${CONFIG}/bin/ejs.web.mod --debug --optimize 9 *.es ;\
../../../${CONFIG}/bin/ejsmod --cslots ../../../${CONFIG}/bin/ejs.web.mod ;\
if ! diff ejs.web.slots.h ../../../${CONFIG}/inc/ejs.web.slots.h >/dev/null; then cp ejs.web.slots.h ../../../${CONFIG}/inc; fi ;\
rm -f ejs.web.slots.h ;\
cd - >/dev/null 

${CC} -c -o ${CONFIG}/obj/ejsHttpServer.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/jems/ejs.web/src src/jems/ejs.web/ejsHttpServer.c

${CC} -c -o ${CONFIG}/obj/ejsRequest.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/jems/ejs.web/src src/jems/ejs.web/ejsRequest.c

${CC} -c -o ${CONFIG}/obj/ejsSession.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/jems/ejs.web/src src/jems/ejs.web/ejsSession.c

${CC} -c -o ${CONFIG}/obj/ejsWeb.o -mtune=generic -Wall -fPIC ${LDFLAGS} ${DFLAGS} -I${CONFIG}/inc -Isrc/jems/ejs.web/src src/jems/ejs.web/ejsWeb.c

${CC} -shared -o ${CONFIG}/bin/libejs.web.so ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsHttpServer.o ${CONFIG}/obj/ejsRequest.o ${CONFIG}/obj/ejsSession.o ${CONFIG}/obj/ejsWeb.o -lejs ${LIBS} -lhttp -lpcre -lmpr

cd src/jems/ejs.web >/dev/null ;\
rm -fr ../../../${CONFIG}/bin/www ;\
cp -r www ../../../${CONFIG}/bin ;\
cd - >/dev/null 

#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/ejs.template.mod
#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/ejs.tar.mod
#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/mvc.es
${CC} -o ${CONFIG}/bin/mvc ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsrun.o -lejs ${LIBS} -lhttp -lpcre -lmpr ${LDFLAGS}

#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/ejs.mvc.mod
#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/utest.es
#  Omit build script /Users/mob/git/ejs/solaris-x86-debug/bin/utest.worker
${CC} -o ${CONFIG}/bin/utest ${LDFLAGS} ${LIBPATHS} ${CONFIG}/obj/ejsrun.o -lejs ${LIBS} -lhttp -lpcre -lmpr ${LDFLAGS}

