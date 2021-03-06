/*
    Static.es - Static content handler
 */

module ejs.web {

    /** 
        Static content handler. This supports DELETE, GET, POST and PUT methods. It handles directory redirection
        and will use X-SendFile for efficient transmission of static content. The If-Match, If-None-Match,
        If-Modified-Since and If-Unmodified-Since headers are supported.
        @param request Request objects
        @returns A response hash object
        @example:
          { name: "index", builder: StaticApp }
        @spec ejs
        @stability prototype
     */
    function StaticApp(request: Request): Object {
        let filename = request.filename
        let status = Http.Ok, body
        let hdr

        let headers = {
            "Content-Type": Uri(request.uri).mimeType,
        }
        if (request.method != "PUT") {
            if ((encoding = request.header("Accept-Encoding")) && encoding.contains("gzip")) {
                let compressed = Path(filename + ".gz")
                if (compressed.exists) {
                    filename = request.filename = compressed
                    headers["Content-Encoding"] = "gzip"
                }
            }
            if (!filename.exists) {
                return {
                    status: Http.NotFound, 
                    body: errorBody("Not Found", "Cannot find " + escapeHtml(request.pathInfo))
                }
            }
        }
        let etag
        if (filename.exists) {
            etag = "%x-%x".format(filename.size, filename.modified)
            headers["ETag"] = etag
            headers["Last-Modified"] = filename.modified.toUTCString()
        }

        /*
            If a specified tag matches, then return the full resource
         */
        let match = false
        if (hdr = request.header("If-Match")) {
            for each (rtag in hdr.split(",")) {
                if (rtag == etag || (rtag == "*" && filename.exists)) {
                    match = true
                    break
                }
            }
            if (!match) {
                return { status: Http.PrecondFailed }
            }
        }
        /*
            If a specified etag matches, then no need to process the request. Respond with Not-Modified.
         */
        match = true
        if (hdr = request.header("If-None-Match")) {
            for each (rtag in hdr.split(",")) {
                if (rtag != etag || (rtag == "*" && !filename.exists)) {
                    match = false
                }
            }
            if (!match) {
                status = Http.PrecondFailed
                /* Keep going to allow If-Modified-Since to be analysed */
            }
        }

        /*
            If the resource has not been modified since, return Not-Modified
         */
        if (match && (when = request.header("If-Modified-Since"))) {
            if (request.method == "GET" || request.method == "HEAD") {
                if (filename.exists && filename.modified <= Date.parse(when) && !request.header("Range")) {
                    status = Http.NotModified
                }
            }
        }
        if (when = request.header("If-Unmodified-Since")) {
            if (!filename.exists || Date.parse(when) < filename.modified) {
                status = Http.PrecondFailed
            }
        }
        if (status == Http.NotModified &&
                (hdr = request.header("Cache-Control")) && (hdr.contains("max-age=0") || hdr.contains("no-cache"))) {
            status = Http.Ok
        }
        if (status != Http.Ok) {
            return { headers: headers, status: status }
        }
        let expires = request.config.web.expires
        if (expires) {
            let lifetime = expires[request.extension] || expires[""]
            if (lifetime) {
                headers["Cache-Control"] = "max-age=" + lifetime
                /* OPT - don't really need to do expires. Cache-Control should be sufficient */
                let when = new Date
                when.time += (lifetime * 1000)
                headers["Expires"] = when.toUTCString()
            }
        }
        if (request.method == "GET" || request.method == "POST") {
            headers["Content-Length"] = filename.size
            if (request.config.web.nosend) {
                body = File(filename, "r")
            } else {
                body = filename
            }

        } else if (request.method == "DELETE") {
            status = Http.NoContent
            try {
                if (!filename.remove()) {
                    status = Http.NotFound
                }
            } catch {
                status = Http.NotFound
            }

        } else if (request.method == "PUT") {
            request.dontAutoFinalize()
            return { body: put }

        } else if (request.method == "HEAD") {
            /* Just need the content length */
            headers["Content-Length"] = filename.size

        } else if (request.method == "OPTIONS") {
            headers["Allow"] = "OPTIONS,GET,HEAD,POST,PUT,DELETE"

        } else if (request.method == "TRACE") {
            // body = "TRACE " + request.pathInfo + " " + request.protocol + "\r\n"
            body = "<!DOCTYPE html>\r\n" +
                "<html><head><title>Trace Request Denied</title></head>\r\n" +
                "<body>The TRACE method is disabled on this server.</body>\r\n" +
                "</html>\r\n"
            status = Http.NotAcceptable

        } else {
            status = Http.BadMethod
            body = errorBody("Unsupported method ", "Method " + escapeHtml(request.method) + " is not supported")
        }
        return {
            status: status,
            headers: headers,
            body: body
        }

        /* Inline function to handle put requests */
        function put(request: Request) {
            let path = request.dir.join(request.pathInfo.trimStart('/'))
            request.status = path.exists ? Http.NoContent : Http.Created

            let file = new File(path, "w")
            file.position = 0;

            request.input.on("readable", function () {
                buf = new ByteArray
                if (request.read(buf)) {
                    file.write(buf)
                } else {
                    file.close()
                    request.finalize()
                }
            })
            request.input.on(["close", "complete", "error"], function (event, request) {
                if (event == "error") {
                    file.close()
                    file.remove()
                }
            })
        }
    }

    //  MOB -- rename to scriptBuilder
    /** 
        Static builder for use in routing tables to serve static file content.
        @param request Request object. 
        @return A web script function that services a web request for static content
        @spec ejs
        @stability prototype
     */
    function StaticBuilder(request: Request): Function {
        //  MOB - should not need "ejs.web"
        return "ejs.web"::StaticApp
    }
}

/*
    @copy   default
    
    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.
    Copyright (c) Michael O'Brien, 1993-2012. All Rights Reserved.
    
    This software is distributed under commercial and open source licenses.
    You may use the GPL open source license described below or you may acquire 
    a commercial license from Embedthis Software. You agree to be fully bound 
    by the terms of either license. Consult the LICENSE.md distributed with 
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
    
    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
