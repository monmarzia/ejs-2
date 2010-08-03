/*
    Cascade slices and return the response from the first non-404 slice
    Usage:
        require ejs.web
        exports.app = Cascade(app)
 */

module ejs.web {
    /** 
        Cascade middleware script for web apps.
        @param apps Array of applications to try in turn. The first app to return a valid status (!= Http.NotFound) 
        will conclude the request.
        @return A web application function that services a web request and when invoked with the request object will 
            yield a response object.
        @example:
            export.app = Cascade(app1, app2, app3)
     */
    function Cascade(...apps): Object {
        return function(request) {
            for each (app in apps) {
                let response = app(request)
                if (response.status != Http.NotFound) {
                    return response
                }
            }
            return null
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