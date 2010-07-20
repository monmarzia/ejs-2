/*
    Test escaping html
 */

require ejs.web

var s = "&head;"
s = escapeHtml(s)
assert(s == "&amp;head;")

s = unescapeHtml(s)
assert(s == "&head;")

s = "\"'"
s = escapeHtml(s)
assert(s == "&quot;&#39;")

