require ejs.web

exports.app = function() {
    sendfile("web/sendfile.txt")
}