/*
    Script.es -- Ejscript templated web content handler
 */

module ejs.web {

    /**
        Run a script app. The script at request.filename will be run
        @param request Request object
        @return A response object
        @spec ejs
        @stability prototype
     */
    function ScriptApp(request: Request) {
        let app = ScriptBuilder(request)
        return app(request)
    }


    /** 
        Script builder for use in routing tables to load pure script files (*.es).
        @param request Request object. 
        @return A web script function that services a web request.
        @example:
          { name: "index", builder: ScriptBuilder, match: "\.es$" }
        @spec ejs
        @stability prototype
     */
    function ScriptBuilder(request: Request): Function {
        if (!request.filename.exists) {
            request.writeError(Http.NotFound, "Cannot find " + request.pathInfo) 
            /* Simple abort request */
            throw true
        }
        try {
            return Loader.require(request.filename, request.config).app
        } catch (e) {
            request.writeError(Http.ServerError, e)
        }
    }
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
