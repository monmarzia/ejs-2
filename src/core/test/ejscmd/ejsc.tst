/*
    ejsc.tst - Test compiler
 */
require ejs.unix

let compileFlags = [
    "",
    "--parse",
    "--debug",
    "--optimize 1",
    "--nobind",
]

/*
    Compile a test stub so we can use the same tests in commands
 */
Cmd.run(Cmd.locate("ejsc") + " --out ejs.test.mod test.stub.es")

if (test.depth >= 2) {
    for (i = 0; i < test.depth && i < compileFlags.length; i++) {
        let flags = compileFlags[i]
        let command = Cmd.locate("ejsc") + " " + flags

        for each (f in find(["lang", "db", "lib", "regress"], "*.tst")) {
            Cmd.sh(command + " " + f)
        }
    }
    rm("ejs.test.mod")

} else {
    test.skip("Runs at depth 2")
}
