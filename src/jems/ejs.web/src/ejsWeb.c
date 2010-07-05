/*
    ejsWeb.c -- Ejscript web framework.

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */
/********************************** Includes **********************************/

#include    "ejs.h"
#include    "ejsWeb.h"
#include    "ecCompiler.h"
#include    "ejs.web.slots.h"

/***************************** Forward Declarations ***************************/

static int configureWebTypes(Ejs *ejs);

/************************************ Code ************************************/

static int requestWorker(EjsRequest *req)
{
    Ejs         *ejs;
    EjsObj      *argv[1];

    ejs = req->ejs;
    argv[0] = (EjsObj*) req;
    ejsRunFunctionBySlot(ejs, ejs->global, ES_ejs_web_Web_workerHelper, 1, argv);
    return 0; 
}


static EjsObj *req_worker(Ejs *ejs, EjsRequest *req, int argc, EjsObj **argv)
{
    Ejs         *nejs;
    EjsRequest  *nreq;
    HttpConn    *conn;

#if TRY
    if ((nejs = ejsCreateVm(ejsService, ejs, NULL, NULL, 0)) == 0) {
        //  THROW
        return 0;
    }
#else
    //  Cloning ejs would be faster
    //  MOB -- need to pick up service
    if ((nejs = ejsCreateVm(ejsGetService(req), NULL, NULL, NULL, EJS_FLAG_MASTER)) == 0) {
        //  THROW
        return 0;
    }
    nejs->location = ejs->location;
    if (ejsLoadModule(ejs, "ejs.web", -1, -1, 0) < 0) {
        mprError(ejs, "Can't load ejs.web.mod: %s", ejsGetErrorMsg(ejs, 1));
        return 0;
    }
#endif
    conn = req->conn;
    nreq = ejsCloneRequest(ejs, req, 1);
    httpSetConnContext(req->conn, nreq);
    conn->dispatcher = nejs->dispatcher;

    //  MOB -- need to schedule event to pick up the request
    //  MOB -- missing server object **** Remove from Request.server
    //      may need documentRoot and serverRoot in Request then
    if (mprCreateEvent(nejs->dispatcher, "RequestWorker", 0, (MprEventProc) requestWorker, nreq, 0) < 0) {
        ejsThrowStateError(ejs, "Can't create worker event");
    }
    return 0;
}


/*  
    HTML escape a string
    function escapeHtml(str: String): String
 */
static EjsObj *web_escapeHtml(Ejs *ejs, EjsObj *unused, int argc, EjsObj **argv)
{
    EjsString   *str;

    str = (EjsString*) argv[0];
    return (EjsObj*) ejsCreateStringAndFree(ejs, mprEscapeHtml(ejs, str->value));
}


/******************************************************************************/

static int configureWebTypes(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;
    int         slotNum;

    type = ejsGetTypeByName(ejs, "ejs.web", "Web");
    if (type == 0) {
        mprError(ejs, "Can't find Web class");
        ejs->hasError = 1;
        return MPR_ERR_CANT_INITIALIZE;
    }
    ejs->webType = type;

    ejsBindMethod(ejs, type, ES_ejs_web_Web_worker, (EjsProc) req_worker);

    if ((slotNum = ejsLookupProperty(ejs, ejs->global, ejsName(&qname, "ejs.web", "escapeHtml"))) != 0) {
        ejsBindFunction(ejs, ejs->global, slotNum, web_escapeHtml);
    }
    ejsConfigureHttpServerType(ejs);
    ejsConfigureRequestType(ejs);
    ejsConfigureSessionType(ejs);
    return 0;
}


/*  
    Module load entry point
 */
int ejs_web_Init(MprCtx ctx)
{
    return ejsAddNativeModule(ctx, "ejs.web", configureWebTypes, _ES_CHECKSUM_ejs_web, EJS_LOADER_ETERNAL);
}


/*
    @copy   default
 
    Copyright (c) Embedthis Software LLC, 2003-2010. All Rights Reserved.
    Copyright (c) Michael O'Brien, 1993-2010. All Rights Reserved.
 
    This software is distributed under commercial and open source licenses.
    You may use the GPL open source license described below or you may acquire
    a commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.TXT distributed with
    this software for full details.
 
    This software is open source; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version. See the GNU General Public License for more
    details at: http://www.embedthis.com/downloads/gplLicense.html
 
    This program is distributed WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 
    This GPL license does NOT permit incorporating this software into
    proprietary programs. If you are unable to comply with the GPL, you must
    acquire a commercial license to use this software. Commercial licenses
    for this software and support services are available from Embedthis
    Software at http://www.embedthis.com
 
    @end
 */
