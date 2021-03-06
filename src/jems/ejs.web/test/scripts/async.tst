/*
    Test the async.es - This is an async script. Use an async client as well for good measure.
 */

const HTTP = App.config.uris.http
const TIMEOUT = 10000

var http: Http = new Http
http.async = true

var complete

var buf = new ByteArray
http.on("readable", function (event, http) {
    http.read(buf, -1)
})
http.on("close", function (event, http) {
    complete = true
})

http.get(HTTP + "/async.es")
let mark = new Date
while (!complete && mark.elapsed < TIMEOUT) {
    App.run(TIMEOUT - mark.elapsed, 1)
}

assert(http.status == 200)
assert(buf == "Now done\n")
assert(complete)
http.close()
