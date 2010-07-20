/*
    Header tests
 */

const HTTP = "http://127.0.0.1:" + ((global.test && test.config.http_port) || 6700)

var http: Http = new Http

http.addHeader("key", "value")
http.get(HTTP + "/index.html")
assert(http.status == 200)

http.get(HTTP + "/index.html")
connection = http.header("connection")
assert(connection == "keep-alive" || connection == "close")
connection = http.header("Connection")
assert(connection == "keep-alive" || connection == "close")

http.get(HTTP + "/index.html?mob=1")
assert(http.contentType == "text/html")
assert(http.headers.date.contains("GMT"))
http.close()
