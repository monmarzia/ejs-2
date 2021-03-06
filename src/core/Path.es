/*
    Path.es --  Path class. Path's represent files in a file system. The file may or may not exist.  

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
    FUTURE
        Path.expand(hash: Object = null)        Expand $VARS or ${vars} in path. Also expand ~user
        Path.split(): Array                     Return dir + base
        Path.splitRoot(): Array                 Return root + rest
        Path.splitExtension(): Array            Return rest + extension
        Path.normalizeCase                      Map to lower case
        Path.canonicalize()                     Resolve symbolic links and return normalized, validated, fully resolved path.
                                                Error if the path does not exist.
        Path.crc(): Number                      Calculate a 32 bit crc of the file contents
        Path.md5(): Number                      Calculate an MD5 digest of the file contents
        Path.touch(date: Date = null): Void     Update modified time. Create if non-existant
        Path.perms = NNN                        Permissions getter / setter
        Path.isReadable, isWritable, isHidden, isSpecial, is Mount
        Path.rmtree()                           Recursive removal
        Path.validate                           Validate a path.
 */

module ejs {

    /**
        Paths are filename objects and may or may not represent physical files in a file system. That is, the 
        corresponding file or directory for the Path  may or may not exist. Once created, Paths are immutable and their
        path value cannot be changed.
        @spec ejs
        @stability evolving
     */
    final class Path {

        use default namespace public

        /**
            Create a new Path object and set the path's location.
            @param path Name of the path to associate with this path object. 
         */
        native function Path(path: String = ".")

        /**
            An equivalent absolute path equivalent for the current path. The path is normalized and uses the native system
            directory separator.
            On Windows, this path with contain a drive specifier and will have back-slash directory separators.
            On Cygwin, this path will be a Cygwin style path without a drive specifier and with forward-slash 
            directory separators. If the path is outside the Cygwin filesystem (outside c:/cygwin), the path 
            will have a /cygdrive/DRIVE prefix. 
            To get a Windows style path, use the $windows property accessor.
         */
        native function get absolute(): Path

        /**
            When the file represented by the path was last accessed. Set to null if the file does not exist.
         */
        native function get accessed(): Date?

        /**
            Append data to a file.
            @param data Data to append to the file
            @param options File open options. Defaults to "atw"
         */
        function append(data, options = "atw"): Void {
            let file = open(options)
            file.write(data)
            file.close()
        }

        /**
            File security permissions.
            @return the file attributes object hash. Fields include: 
            @options user String representing the file user name
            @options group String representing the file group name
            @options uid Number representing the file user id
            @options gid Number representing the file group id
            @options permissions Number File Posix permissions mask
         */
        native function get attributes(): Object

        /**
            Define file security permissions.
            @options user String representing the file user name
            If both user and uid are specified, user takes precedence. 
            @options group String representing the file group name
            If both group and gid are specified, group takes precedence.
            @options uid Number representing the file user id
            @options gid Number representing the file group id
            @options permissions Number File Posix permissions mask
         */
        native function setAttributes(attributes: Object): Void

        /**
            The base of portion of the path. The base portion is the trailing portion without any directory elements.
         */
        native function get basename(): Path
        
        /**
            Path components. The path is normalized and then broken into components for each directory level. 
            The result is an array object with an element for each segment of the path. If the path is an absolute path, 
            the first element represents the file system root and will be the empty string or drive spec 
            on a Windows like systems. Array.join("/") can be used to put components back into a complete path.
         */
        native function get components(): Array
  
        /**
            Test if the path name contains a substring
            @param pattern String pattern to search for
            @return Boolean True if found.
         */
        function contains(pattern: String): Boolean
            name.contains(pattern)

        /**
            Copy a file
            @param target New file location
            @param options Object hash
            @options permissions Set to a numeric Posix permissions mask. Not implemented.
            @options user String representing the file user name
                If both user and uid are specified, user takes precedence. 
            @options group String representing the file group name
                If both group and gid are specified, group takes precedence.
            @options uid Number representing the file user id
            @options gid Number representing the file group id
         */
        native function copy(target: Object, options: Object? = null): Void

        /**
            When the file represented by the path was created. Set to null if the file does not exist.
         */
        native function get created(): Date?

        /**
            The directory portion of a file. The directory portion is the leading portion including all 
            directory elements and excluding the base name. On some systems, it will include a drive specifier.
            See also $parent which will determine the parent directory of relative paths.
         */
        native function get dirname(): Path

        /**
            Return true if the path ends with the given suffix
            @param suffix String suffix to compare with the path.
            @return true if the path does begin with the suffix
         */
        function endsWith(suffix: String): Boolean
            portable.name.endsWith(Path(suffix).portable)

        /**
            Does a file exist for this path.
         */
        native function get exists(): Boolean 

        /**
            File extension portion of the path. The file extension is the portion after (and not including) the last ".".
         */
        native function get extension(): String 

        /**
            TODO - should do pattern matching
            @hide
         */
        function findAbove(name: String): Path? {
            let dir: Path = this
            do {
                if (dir.join(name).exists) {
                    return dir.join(name)
                }
                dir = dir.parent
            } while (dir != dir.parent)
            return null
        }

        /**
            Do Posix glob style file matching.
            @param patterns Pattern to match files. This can be a String, Path or array of String/Paths. 
            The wildcard '?' matches any single character, '*' matches zero or more characters in a filename or 
                directory, '** /' matches zero or more files or directories and matches recursively in a directory
                tree.  If a pattern terminates with "/" it will only match directories. 
                The pattern '**' is equivalent to '** / *' (ignore spaces). 
                The Posix "[]" and "{a,b}" style expressions are not supported.
            @param options Optional properties to control the matching.
            @option depthFirst Do a depth first traversal. If "dirs" is specified, the directories will be shown after
                the files in the directory. Otherwise, directories will be listed first.
            @option exclude Regular expression pattern of files to exclude from the results. Matches the entire path.
                Only for the purpose of this match, directories will have "/" appended. To exclude directories in the
                results, use {exclude: /\/$/}. The trailing "/" will not be returned in the results.
            @option hidden Include hidden files starting with "."
            @option include Regular expression pattern of files to include in the results. Matches the entire returned path.
                Only for the purpose of this match, directories will have "/" appended. To include only directories in the
                results, use {include: /\/$/}
            @option missing Set to undefined to report patterns that don't resolve into any files or directories 
                by throwing an exception. Set to any non-null value to be used in the results when there are no matching
                files or directories. Set to the empty string to use the patterns in the results and set
                to null to do nothing.
            @option relative Return paths relative to the Path, otherwise result entries include the Path. Defaults to false.
            @return An Array of Path objects for each file in the directory.
         */
        native function files(patterns: Object! = '*', options: Object? = null): Array 

        /**
            The file system containing this Path 
            @return The FileSystem object for this path
         */
        function get fileSystem(): FileSystem
            FileSystem(this)

        /**
            Get iterate over any files contained under this path (assuming it is a directory) "for (v in files)". 
                This operates the same as getValues on a Path object.
            @return An iterator object.
            @example:
                for (f in Path("."))
         */
        override iterator native function get(): Iterator

        /**
            Get an iterator for this file to be used by "for each (v in obj)". Return 
                This operates the same as $get on a Path object.
            @return An iterator object.
            @example
                for each (f in Path("."))
         */
        override iterator native function getValues(): Iterator

        /**
            Does the file path have a drive spec (C:) in it's name. Only relevant on Windows like systems.
            @return True if the file path has a drive spec
         */
        native function get hasDrive(): Boolean 

        /**
            Is the path absolute, i.e. Beginning at the file system's root.
         */
        native function get isAbsolute(): Boolean

        //  MOB - need isExecutable
        /**
            Is the path a directory. If the path is a symbolic link, the target of the link is tested.
            @return true if the file is a directory
         */
        native function get isDir(): Boolean

        /**
            Is the path is a symbolic link file. Not available on some platforms such as Windows and VxWorks.
            @return true if the file is a symbolic link
         */
        native function get isLink(): Boolean

        /**
            Is the path a regular file. If the path is a symbolic link, the target of the link is tested.
            @return true if the file is a regular file and not a directory
         */
        native function get isRegular(): Boolean

        /**
            Is the path is relative and not absolute.
         */
        native function get isRelative(): Boolean

        /**
            Join paths. Joins one more more paths together. If the other path is absolute, return it. Otherwise
            add a separator, and continue joining. The separator is chosen to match the first separator found in 
            either path. If none found, the default file system path separator is used.
            @return A new joined, normalized Path object.
         */
        native function join(...other): Path

        /**
            Join an extension to a path. If the base name of the path already has an extension, this call does nothing.
            @param ext Extension to add. The extension may or may not contain a leading period. Best practice is to 
                not have leading periods on extensions.
            @param force Set to true to always add the extension.
            @return A new Path object with the given extension if the path did not already have one.
         */
        native function joinExt(ext: String, force: Boolean = false): Path

        /**
            The length of the path name in bytes. Note: this is not the file size. For that, use Path.size
         */
        native function get length(): Number 

        /**
            The target pointed to if this path is a symbolic link. Not available on some platforms such as Windows and 
            VxWorks. If the path is not a symbolic link, it is set to null.
         */
        native function get linkTarget(): Path?

        /**
            Make a new directory and all required intervening directories. If the directory already exists, 
                the function returns without any further action. 
            @param options
            @options permissions Directory permissions to use for all created directories. Set to a numeric 
                Posix permissions mask.
            @options user String representing the file user for all created directories.
                If both user and uid are specified, user takes precedence. 
            @options group String representing the file group for all created directories.
                If both group and gid are specified, group takes precedence.
            @options uid Number representing the file user id
            @options gid Number representing the file group id
            @return True if the directory can be made or already exists
         */
        native function makeDir(options: Object? = null): Boolean

        /**
            Create a link to a file. Not available on some platforms such as Windows and VxWorks.
            @param target Path to an existing file to link to.
            @param hard Set to true to create a hard link. Otherwise the default is to create a symbolic link.
            @returns this path
         */
        native function makeLink(target: Path, hard: Boolean = false): Void

        /**
            Create a new temporary file. The temp file is located in the directory specified by the Path object. 
            If the path object is created with an empty path value, the temporary file will be created in the standard
            system location for temp files. This is typically /tmp or /temp.
            @returns a new Path object for the temp file.
         */
        native function temp(): Path

        /**
            Get a path after mapping the path directory separator
            @param sep Path directory separator to use. Defaults to the separator for this path.
            @return a new Path after mapping separators
         */
        native function map(sep: String = separator): Path

        /**
            Match the path to a regular expression pattern
            @param pattern The regular expression pattern to search for
            @return Returns an array of matching substrings in the path
         */
        function match(pattern: RegExp): Array?
            this.toString().match(pattern)

        /** 
            Get the mime type for a path or extension string. The path's extension is used to lookup the corresponding
            mime type.
            @returns The mime type string
         */
        native function get mimeType(): String

        /**
            When the file represented by the path was last modified. Set to null if the file does not exist.
         */
        native function get modified(): Date?

        /**
            Name of the Path as a string. This is the same as $toString().
         */
        native function get name(): String 

        /**
            Natural (native) respresentation of the path for the platform. This uses the platform file system path 
            separator, this is "\" on Windows and "/" on unix and Cygwin. The returned path is normalized. 
            See also $portable for a portable representation.
         */
        native function get natural(): Path 

        /**
            Normalized representation of the path. This removes "segment/.." and "./" components. Separators are made 
            uniform by converting all separators to be the same as the first separator found in the path. Note: the 
            result is not converted to an absolute path.
         */
        native function get normalize(): Path

        /**
            Open a path and return a File object.
            @param options
            @options mode optional file access mode string. Use "r" for read, "w" for write, "a" for append to existing
                content, "+" never truncate.
            @options permissions optional Posix permissions number mask. Defaults to 0664.
            @options user String representing the file user
            @options group String representing the file group
            @return a File object which implements the Stream interface.
            @throws IOError if the path or file cannot be created.
         */
        function open(options: Object? = null): File
            new File(this, options)

        /**
            Open a file and return a TextStream object.
            @param options Optional options object
            @options mode optional file access mode string. Use "r" for read, "w" for write, "a" for append to existing
                content, "+" never truncate.
            @options encoding Text encoding format
            @options permissions optional Posix permissions number mask. Defaults to 0664.
            @options user String representing the file user
            @options group String representing the file group
            @return a TextStream object which implements the Stream interface.
            @throws IOError if the path or file cannot be opened or created.
         */
        function openTextStream(options: Object? = null): TextStream
            new TextStream(open(options))

        /**
            Open a file and return a BinaryStream object.
            @param options Optional options object
            @options mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate. 
                Defaults to Read.
            @options permissions optional Posix permissions number mask. Defaults to 0664.
            @options user String representing the file user
            @options group String representing the file group
            @return a BinaryStream object which implements the Stream interface.
            @throws IOError if the path or file cannot be opened or created.
         */
        function openBinaryStream(options: Object? = null): BinaryStream
            new BinaryStream(open(options))

        /**
            The parent directory of path. This may be an absolute path if there are no parent directories in the path.
         */
        native function get parent(): Path

        /*  MOB -- different to File.permissions. Should have something that returns an object with full path/file
            attributes including group/user. The perms should be broken down into world:group:user */
        //  MOB - should not use a setter should be functions
        /**
            The file permissions of a path. This number contains the Posix style permissions value or null if the file 
            does not exist. NOTE: this is not a string representation of an octal posix mask. 
         */
        native function get perms(): Number?
        native function set perms(perms: Number): Void

        /**
            The path in a portable (like Unix) representation. This uses "/" separators. The value is is normalized and 
            the separators are mapped to "/". See also $natural for convertion to the O/S native path representation.
         */
        native function get portable(): Path 

        /**
            Read the file contents as a ByteArray. This method opens the file, reads the contents, closes the file and
                returns a new ByteArray with the contents.
            @return A ByteArray
            @throws IOError if the file cannot be read
            @example:
                var b: ByteArray = Path("/tmp/a.txt").readBytes()
         */
        function readBytes(): ByteArray? {
            let file: File = File(this).open()
            result = file.readBytes()
            file.close()
            return result
        }

        /**
            Read the file containing a JSON string and return a deserialized object. 
            This method opens the file, reads the contents, deserializes the object and closes the file.
            @return An object.
            @throws IOError if the file cannot be read
            @example:
                data = Path("/tmp/a.json").readJson()
         */
        function readJSON(): Object? {
            let file: File = open(this)
            result = file.readString()
            file.close()
            return deserialize(result)
        }

        /**
            Read the file contents as an array of lines. Each line is a string. This method opens the file, reads the 
                contents and closes the file.
            @return An array of strings.
            @throws IOError if the file cannot be read
            @example:
                for each (line in Path("/tmp/a.txt").readLines())
         */
        function readLines(): Array? {
            let stream: TextStream = TextStream(open(this))
            result = stream.readLines()
            stream.close()
            return result
        }

        /**
            Read the file contents as a string. This method opens the file, reads the contents and closes the file.
            @return A string.
            @throws IOError if the file cannot be read
            @example:
                data = Path("/tmp/a.txt").readString()
         */
        function readString(): String? {
            let file: File = open(this)
            result = file.readString()
            file.close()
            return result
        }

        /**
            Read the file contents as an XML object.  This method opens the file, reads the contents and closes the file.
            @return An XML object
            @throws IOError if the file cannot be read
         */
        function readXML(): XML? {
            let file: File = open(this)
            let data = file.readString()
            file.close()
            if (data == null) {
                return null
            }
            return new XML(data)
        }

        /**
            That path in a form relative to the application's current working directory. The value is normalized.
         */
        native function get relative(): Path

        /**
            Get a relative path to an origin
            @param origin The origin path to use when calculating a relative path to the destination (this path).
                If origin is null, the current working directory is used as the origin.
            @return A new relative path
         */
        native function relativeTo(origin: Path? = null): Path

        /**
            Delete the file associated with the Path object. If this is a directory without contents it will be removed.
            @return True if the file is sucessfully deleted or does not exist.
         */
        native function remove(): Boolean 

        //  MOB - consider remove({contents: true})
        /**
            Removes a directory and its contents
            If the path is a directory, this call will remove all subdirectories and their contents and finally the
            directory itself. If the directory does not exist, this call does not error and does nothing.
            @return True if all the contents are sucessfully deleted or if the directory does not exist. 
                Otherwise return false.
         */
        function removeAll(): Boolean {
            let passed = true
            if (name == "" || name == "/") {
                throw new ArgError("Bad path for removeAll")
            }
            for each (f in files('**', {depthFirst: true, hidden: true})) {
                if (!f.remove()) {
                    passed = false
                }
            }
            if (!remove()) {
                passed = false
            }
            return passed
        }

        /**
            Remove the drive specifier from a path
            @hide
         */
        function removeDrive(): Path {
            let sep = separator
            return hasDrive ? (sep + components.slice(1).join(sep)) : this
        }

        /**
            Rename a file. If the new path exists it is removed before the rename.
            @param target New name of the path
            @return True if the file is renamed.
         */
        native function rename(target: Path): Boolean
        
        /**
            Replace the path extension and return a new path.
            @return A new path with the given extension.
         */
        function replaceExt(ext: String): Path {
            if (ext && ext[0] != '.') {
                ext = "." + ext
            }
            return this.trimExt() + ext
        }

        /**
            Search and replace. Search for the given pattern which can be either a string or a regular expression 
            and replace it with the replace text. If the pattern is a string, only the first occurrence is replaced.
            @param pattern The regular expression or string pattern to search for.
            @param replacement The string to replace the match with or a function to generate the replacement text. The
                replacement string can contain special replacement patterns: "$$" inserts a "\$", "\$&" inserts the
                matched substring, "\$`" inserts the portion that preceeds the matched substring, "\$'" inserts the
                portion that follows the matched substring, and "\$N" inserts the Nth parenthesized substring.
                The replacement parameter can also be a function which will be invoked and the function return value 
                will be used as the resplacement text. The function will be invoked multiple times for each match to be 
                replaced if the regular expression is global. The function will be invoked with the signature:

                function (matched, submatch_1, submatch_2, ..., matched_offset, original_string)
            @return Returns a new string.
            @spec ejs
         */
        function replace(pattern, replacement): Path
            Path(name.replace(pattern, replacement))

        /**
            Resolve paths in the neighborhood of this path. Resolve operates like join, except that it joins the 
            given paths to the directory portion of the current ("this") path. For example: 
            Path("/usr/bin/ejs/bin").resolve("lib") will return "/usr/lib/ejs/lib". i.e. it will return the
            sibling directory "lib".

            Resolve operates by determining a virtual current directory for this Path object. It then successively 
            joins the given paths to the directory portion of the current result. If the next path is an absolute path, 
            it is used unmodified.  The effect is to find the given paths with a virtual current directory set to the 
            directory containing the prior path.

            Resolve is useful for creating paths in the region of the current path and gracefully handles both 
            absolute and relative path segments.
            @param otherPaths Paths to resolve in the region of this path.
            @return A new Path object that is resolved against the prior path. 
         */
        native function resolve(...otherPaths): Path

        /**
            That root directory component of the path. On windows, this includes the drive spec if present in the path.
          */
        native function get root(): Path

        /**
            Compare two paths test if they represent the same file
            @param other Other path to compare with
            @return True if the paths represent the same underlying filename
         */
        native function same(other: Object): Boolean

        /**
            The path separator for this path. This will return the first valid path separator used by the path
            or the default file system path separator if the path has no separators. On Windows, a path may contain
            "/" and "\" separators.  This will be set to a string containing the first separator found in the path.
            Will typically be either "/" or "/\\" depending on the path, platform and file system.
            Use $natural, $portable or $windows to create a new path with different path separators.
         */
        native function get separator(): String

        /**
            Size of the file associated with this Path object. Set to the number of bytes in the file or -1 if the
            size determination failed.
         */
        native function get size(): Number 

        /**
            Return true if the path starts with the given prefix
            @param prefix String prefix to compare with the path.
            @return true if the path does begin with the prefix
         */
        function startsWith(prefix: String): Boolean
            portable.name.startsWith(Path(prefix).portable)

        /**
            Create the path as a symbolic link.
            This will remove any pre-existing path and then create a symbolic link to refer to the target.
            @param target Target the path will refer to.
          */
        native function symlink(target: Path): Void

        /**
            Convert the path to a JSON string. 
            @return a JSON string representing the path.
         */
        native override function toJSON(): String

        /**
            Convert the path to lower case
            @return a new Path mapped to lower case
         */
        function toLowerCase(): Path
            new Path(name.toString().toLowerCase())

        /**
            Return the path as a string. The path is not translated.
            @return a string representing the path.
         */
        native override function toString(): String

        //  TODO - should support reg expressions
        /**
            Trim a pattern from the end of the path
            NOTE: this does a case-sensitive match
            @return a Path containing the trimmed path name
         */
        function trimEnd(pat: String): Path {
            pat = Path(pat).portable
            if (name.endsWith(pat)) {
                loc = name.lastIndexOf(pat)
                if (loc >= 0) {
                    return new Path(name.slice(0, loc))
                }
            }
            return this
        }

        /**
            Trim a file extension from a path
            @return a Path with no extension
         */
        native function trimExt(): Path

        //  TODO - should support reg expressions
        /**
            Trim a pattern from the start of the path
            NOTE: this does a case-sensitive match
            @return a Path containing the trimmed path name
         */
        function trimStart(pat: String): Path {
            if (name.startsWith(pat)) {
                return new Path(name.slice(pat.length))
            }
            return this
        }

        /**
            Reduce the size of the file by truncating it. 
            @param size The new size of the file. If the truncate argument is greater than or equal to the 
                current file size nothing happens.
            @throws IOError if the truncate failed.
         */
        native function truncate(size: Number): Void

        /**
            An equivalent normalized Windows path equivalent for the current path.
         */
        native function get windows(): Path

        //  MOB rename? - bit confusing "write". This really does a "save"
        //  MOB - rename to writecContents or save
        //  MOB - last arg should be permissions?
        /**
            Write the file contents. This method opens the file, writes the contents and closes the file.
            @param args The data to write to the file. Data is serialized in before writing. Note that numbers will not 
                be written in a cross platform manner. If that is required, use the BinaryStream class to write Numbers.
            @throws IOError if the data cannot be written
         */
        function write(...args): Void {
            var file: File = new File(this, { mode: "w", permissions: 0644 })
            try {
                for each (item in args) {
                    if (item is String) {
                        file.write(item)
                    } else {
                        file.write(serialize(item))
                    }
                }
            } 
            catch (es) {
                throw new IOError("Can't write to file")
            }
            finally {
                file.close()
            }
        }

        /** 
            Create a new temporary file. The temp file is located in the directory specified by the Path object. 
            @returns a new Path object for the temp file.
            @hide
            @deprecated 1.0.0
         */
        function makeTemp(): Path
            temp()

    }
}

/*
    @copy   default

    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
