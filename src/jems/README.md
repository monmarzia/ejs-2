Ejscript Jems
===

This directory contains the standard Jem extensions for Ejscript. This includes
the core language libraries, web framework and database interfaces.

### Inline Documentation

The ejsc compiler will parse documentation comments within the /** comment */ comment
delimiters. The first line of text is assumed to be the brief overview. The rest of
the text is the documentation description.

Ejs documentation also supports the following documentation directives:

    @param  argName Description         (Up to next @, case matters on argName)
    @return Sentance                    (If sentance starts with lower case, then start sentance with "Call returns")
    @throws ExceptionType Explanation   (Explanation is the text up to the next @)
    @see keyword keyword ...            (Hot linked see-also keywords)
    @example Description                (Description is up to next @)
    @stability kind                     (Kinds: prototype | evolving | stable | mature | deprecated]
    @requires ECMA                      (Will emit: configuration requires --ejs-ecma)
    @spec                               (ecma-262, ecma-357, ejs-11)
    @hide                               (Hides this entry. API exists but is not documented)

