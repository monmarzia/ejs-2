/*
    String.es -- String class
 *
    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

    /**
        Each String object represents a single immutable linear sequence of characters. Strings have operators 
        for: comparison, concatenation, copying, searching, conversion, matching, replacement, and, subsetting.
        @spec evolving
     */
    final class String {

        use default namespace public

        /**
            String constructor. This can take two forms:
            <ul>
                <li>String()</li>
                <li>String(str: String)</li>
            </ul>
            @param str The args can be either empty or a string. If a non-string arg is supplied, the VM will automatically
                cast to a string.
         */
        native function String(...str)

        /**
            Do a case sensitive comparison between this string and another.
            @param compare The string to compare against
            @return -1 if this string is less than the compare string, zero if equal and 1 if greater than.
            @spec ejs
         */
        native function caseCompare(compare: String): Number

        /**
            Do a case insensitive comparison between this string and another.
            @param compare The string to compare against
            @return -1 if this string is less than the compare string, zero if equal and 1 if greater than.
            @spec ejs
         */
        native function caselessCompare(compare: String): Number

        /**
            Return the character at a given position in the string
            @returns a new string containing the selected character. If the index is out of range, returns the empty string.
         */
        native function charAt(index: Number): String

        /**
            Get a character code. 
            @param index The index of the character to retrieve
            @return Return the character code at the specified index. If the index is -1, get the last character.
                Return NaN if the index is out of range.
         */
        native function charCodeAt(index: Number = 0): Number

        /**
            Concatenate strings and returns a new string. 
            @param args Strings to append to this string
            @return Return a new string.
         */
        native function concat(...args): String

        /**
            Check if a string contains a pattern.
            @param pattern The pattern can be either a string or regular expression.
            @return Returns true if the pattern is found.
            @spec ejs
         */
        native function contains(pattern: Object): Boolean

        /**
            Determine if this string ends with a given string
            @param test The string to test with
            @return True if the string matches.
            @spec ejs
         */
        native function endsWith(test: String): Boolean

        /**
            Replace tokens in the string and return an expanded string.
            Tokens are represented by '${field}' where field may contain '.'. For example ${user.name}.
            To preserve an ${token} unmodified, preceed the token with an extra '$'. For example: $${token}.
            @param obj containing tokens to expand
            @param options Options hash
            @option fill Set to a string to use for missing properties. Set to undefined or omit options to 
                throw an exception for missing properties. Set fill to '${}' to preserve undefined tokens as-is. 
                This permits multi-pass expansions.
            @option join Character to use to join array elements. Defaults to space.
            @return Expanded string
         */ 
        native function expand(obj: Object, options: Object? = null): String

        /**
            Format arguments as a string. Use the string as a format specifier. The format specifier has the form:
            %[-+ #,][width][precision][type]. See printf(1) for the meaning of the various fields.
            @param args Array containing the data to format. 
            @return -1 if less than, zero if equal and 1 if greater than.
            @example
                "%5.3f".format(num)
            \n\n
                "%s Arg1 %d, arg2 %d".format("Hello World", 1, 2)
            @spec ejs
         */
        native function format(...args): String

        /**
            Create a string from the character code arguments
            @param codes Character codes from which to create the string
            @returns a new string
         */
        native static function fromCharCode(...codes): String

        /**
            Get an iterator for this array to be used by "for (v in string)"
            @return An iterator object.
         */
        override iterator native function get(): Iterator

        /**
            Get an iterator for this array to be used by "for each (v in string)"
            @return An iterator object.
         */
        override iterator native function getValues(): Iterator

        /**
            Search for an item using strict equality "===". This call searches from the start of the string for 
            the specified element. 
            @param pattern The string to search for.
            @param startIndex Where in the array (zero based) to start searching for the object.
            @return The items index into the array if found, otherwise -1.
         */
        native function indexOf(pattern: String, startIndex: Number = 0): Number

        //  MOB - should be isNumber
        /**
            Is there is at least one character in the string and all characters are digits.
            @spec ejs
         */
        native function get isDigit(): Boolean

        /**
            Is there is at least one character in the string and all characters are alphabetic.
            Uses latin-1 for comparisons.
            @spec ejs
         */
        native function get isAlpha(): Boolean

        /**
            Is there is at least one character in the string and all characters are alphabetic or numeric.
            Uses latin-1 for comparisons.
            @spec ejs
         */
        native function get isAlphaNum(): Boolean

        /**
            Is there is at least one character in the string and there are no upper case characters.
            @return False otherwise
            @spec ejs
         */
        native function get isLower(): Boolean

        /**
            Is there is at least one character in the string and all characters are white space.
            @spec ejs
         */
        native function get isSpace(): Boolean

        /**
            Is there is at least one character in the string and there are no lower case characters.
            @spec ejs
         */
        native function get isUpper(): Boolean

        /**
            Search right to left for a substring starting at a given index.
            @param pattern The string to search for
            @param location The character index at which to start the search.
            @return Return the starting index of the last match found.
         */
        native function lastIndexOf(pattern: String, location: Number = -1): Number

//  TODO - change this to characters when supporting UNICODE.
        /**
            The length of the string in bytes.
         */
        native function get length(): Number

        /**
            Compare another string with the this string object
            @returns zero if the strings are equal, -1 if this string is lexically before @str and 1 if after.
         */
        # UNUSED
        function localeCompare(str: String): Number {
            if (this < string) {
                return -1
            } else if (string == this) {
                return 0
            } else {
                return 1
            }
        }

        //  TODO - should this allow a string?
        /**
            Match a pattern using a regular expression
            @param pattern The regular expression pattern to search for
            @return Returns an array of matching substrings.
         */
        native function match(pattern: RegExp): Array

        /**
            Parse the current string object as a JSON string object. The @filter is an optional filter with the 
            following signature:
                function filter(key: String, value: String): Boolean
            @param filter Optional function to call for each element of objects and arrays. Not yet supported.
            @returns an object representing the JSON string.
         */
        function parseJSON(filter: Function? = null): Object
            deserialize(this)

        /**
            Copy the string into a new string and lower case the first character if there is one. If the first non-white 
            character is not a character or if it is already lower there is no change.
            @return A new String
            @spec ejs
         */
        native function toCamel(): String

        /**
            Copy the string into a new string and capitalize the first character if there is one. If the first non-white 
            character is not a character or if it is already capitalized there is no change.
            @return A new String
            @spec ejs
         */
        native function toPascal(): String

        /**
            Create a new string with all nonprintable characters replaced with unicode hexadecimal equivalents (e.g. \uNNNN).
            @return The new string
            @spec ejs
         */
        native function printable(): String

        /**
            Wrap a string in double quotes.
            @return The new string
         */
        native function quote(): String

        /**
            Remove characters from a string. Remove the elements from @start to @end inclusive. 
            @param start Numeric index of the first element to remove. Negative indicies measure from the end of 
            the string.  -1 is the last character element.
            @param end Numeric index of one past the last element to remove
            @return A new string with the characters removed
            @spec ejs
         */
        native function remove(start: Number, end: Number = -1): String

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
        native function replace(pattern: Object, replacement: Object): String

        /**
            Reverse a string. 
            @return Returns a new string with the order of all characters reversed.
            @spec ejs
         */
        native function reverse(): String

        /**
            Search for a pattern.
            @param pattern Regular expression or string pattern to search for in the string.
            @return Return the starting index of the pattern in the string. Return -1 if not found.
         */
        native function search(pattern: Object): Number

        /**
            Extract a substring.
            @param start The position of the first character to slice.
            @param end The position one after the last character. Negative indicies are measured from the end of the 
                string.  The -1 index means the last character, so slice(0, -1) will return all characters except the 
                last.
            @param step Extract every "step" character.
         */ 
        native function slice(start: Number, end: Number = -1, step: Number = 1): String

        /**
            Split a string into an array of substrings. Tokenizes a string using a specified delimiter.
            @param delimiter String or regular expression object separating the tokens.
            @param limit At most limit strings are extracted. If limit is set to -1, then unlimited strings are extracted.
            @return Returns an array of strings.
         */
        native function split(delimiter: Object, limit: Number = -1): Array

        /**
            Tests if this string starts with the string specified in the argument.
            @param test String to compare against
            @return True if it does, false if it doesn't
            @spec ejs
         */
        native function startsWith(test: String): Boolean

        /**
            Extract a substring. Similar to slice but only allows positive indicies.
            If the end index is larger than start, then the effect of substring is as if the two arguments were swapped.
            @param startIndex Integer location to start copying
            @param end Postitive index of one past the last character to extract.
            @return Returns a new string
         */
        native function substring(startIndex: Number, end: Number = -1): String

        /**
            Replication. Replicate the string N times.
            @param times The number of times to copy the string
            @return A new String with the copies concatenated together. Returns empty string if times is <= zero.
            @spec ejs
         */
        function times(times: Number): String {
            var s: String = ""
            for (i in times) {
                s += this
            }
            return s
        }

        //  TODO. Should this be the reverse?   for (s in "%s %s %s".tokenize(value))
        //  that would be more consistent with format()
        /**
            Scan the input and tokenize according to a string format specifier.
            @param format Tokenizing format specifier
            @returns array containing the tokenized elements
            @example
                for (s in string.tokenize("%s %s %s")) {
                    print(s)
                }
            @spec ejs
         */
        native function tokenize(format: String): Array

        /**
            Convert the string to an equivalent JSON encoding.
            @return A quoted string.
            @throws TypeError If the object could not be converted to a string.
         */ 
        override native function toJSON(): String

        /**
            Convert the string to lower case.
            @return Returns a new lower case version of the string.
            @spec ejs
         */
        native function toLowerCase(): String


        /**
            This function converts the string to a localized lower case string representation. 
            @returns a lower case localized string representation of the string. 
         */ 
        # FUTURE
        function toLocaleLower(): String
            //  TODO should be getting from App.Locale not from date
            toLowerCase(Date.LOCAL)

        /**
            This function converts the string to a localized string representation. 
            @returns a localized string representation of the string. 
         */ 
        # FUTURE
        override function toLocaleString(): String
            toString()

        /**
            This function converts an object to a string representation. Types typically override this to provide 
            the best string representation.
            @returns a string representation of the object. For Objects "[object className]" will be returned, 
            where className is set to the name of the class on which the object was based.
         */ 
        override native function toString(): String

        /**
            Convert the string to a path
            @return Returns an equivalent Path object
            @spec ejs
         */
        function toPath(): Path
            this cast Path

        /**
            Convert the string to upper case.
            @return Returns a new upper case version of the string.
            @spec ejs
         */
        native function toUpperCase(): String

        /**
            Convert the string to localized upper case string
            @return Returns a new localized upper case version of the string.
         */
        # FUTURE
        function toLocaleUpperCase(): String
            //  TODO should be getting from App.Locale not from date
            toUpperCase(Date.LOCAL)


        //  TODO - great if this could take a regexp
        /**
            Returns a trimmed copy of the string. Normally used to trim white space, but can be used to trim any 
            substring from the start and end of the string.
            @param str May be set to a substring to trim from the string. If not set, it defaults to any white space.
            @return Returns a (possibly) modified copy of the string
         */
        native function trim(str: String? = null): String

        /**
            Trim text from the start of a string. 
            @param str May be set to a substring to trim from the string. If not set, it defaults to any white space.
            @return Returns a (possibly) modified copy of the string
         */
        native function trimStart(str: String? = null): String

        /**
            Trim text from the end of a string. 
            @param str May be set to a substring to trim from the string. If not set, it defaults to any white space.
            @return Returns a (possibly) modified copy of the string
         */
        native function trimEnd(str: String? = null): String

        /**
            Return the value of the object
            @returns this object.
         */ 
        # FUTURE
        override function valueOf(): String {
            return this
        }

        /*
            Overloaded operators
         */

        /**
            String subtraction. Remove the first occurance of str.
            @param str The string to remove from this string
            @return Return a new string.
            @spec ejs
         */
        function - (str: String): String {
            var i: Number = indexOf(str)
            if (i < 0) {
                return this
            }
            return remove(i, i + str.length)
        }
        
        /**
            Compare strings. 
            @param str The string to compare to this string
            @return -1 if less than, zero if equal and 1 if greater than.
            @spec ejs
         */
        # DOC_ONLY
        function < (str: String): Number {
            return localeCompare(str) < 0
        }

        /**
            Compare strings.
            @param str The string to compare to this string
            @return -1 if less than, zero if equal and 1 if greater than.
            @spec ejs
         */
        # DOC_ONLY
        function > (str: String): Number {
            return localeCompare(str) > 0
        }

        /**
            Compare strings.
            @param str The string to compare to this string
            @return -1 if less than, zero if equal and 1 if greater than.
            @spec ejs
         */
        # DOC_ONLY
        function == (str: String): Number {
            return localeCompare(str) == 0
        }

        /**
            Format arguments as a string. Use the string as a format specifier.
            @param arg The argument to format. Pass an array if multiple arguments are required.
            @return -1 if less than, zero if equal and 1 if greater than.
            @example
                "%5.3f" % num
            <br/>
                "Arg1 %d, arg2 %d" % [1, 2]
            @spec ejs
         */
        function % (arg: Object): String
            format(arg)

//  MOB - Cleanup
        /** 
            @hide 
            @deprecated 2.0.0
         */
        # Config.Legacy || 1
        function toLower(): String
            toLowerCase()

        /** 
            @hide 
            @deprecated 2.0.0
         */
        # Config.Legacy || 1
        function toUpper(): String
            toUpperCase()
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
