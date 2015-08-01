# TeX Macro

A C library that parses and executes TeX style macros. This is useful for converting TeX documents in to something like HTML or Markdown. Extend built in functionality with your own callback functions to do things like read from your database or gather data from external sources.

## Usage

Basic usage. Execute two source files in the same content:

```
    int main(){
        // Create tex context
        tex_parser parser = tex_init();

        // Parse and "execute" TeX source, printing results to stdout
        tex_source(parser, "preamble.tex");
        tex_source(parser, "example.tex");
    }

Create a programmatic macro:

```C
    char *my_macro(int argc, char **argv) {
        assert(argc == 3); // First argv is macro name

        // Evaluate macro arguments
        if(strcmp(argv[1], argv[2]) < 0)
            printf("True");
        else
            printf("False");
	    
        // None NULL return must be an error message, halts parsing
        return NULL;
    }

    int main(){
        tex_parser parser = tex_init();

        // Supply a function to handle macro substition
        tex_define_macro(parser, "mymacro", "#1 is less than #2", my_macro);

        // Prints "True" to stdout
        tex_source_str(parser, "\mymacro 1 is less than 2");
    }
```

## Why TeX Macro

Although numerous other text markup languages exists, most of those intended for use on the web are simply thin veils over common HTML functionality (like Markdown). General purpose macro systems (eg. M4) are more powerful, but are often not extensible and tend to be oriented toward the production of computer source code.

TeX, although originally a domain specific language, is quite powerful and has many idioms already established related precise production of natural language documents. The language can be arbitrarily extended on a per-document bases, allowing for concise expression of it's structure. Later on, the sets of macros used by a document may be redefined, to produce the same content in any number of output representations.

Although TeX provides for arbitrarily complex text manipulation, the user is also permitted to define macros that rely on C code to operate. This can be used quite simply to provide powerful constructs. For instance, the user could define a `\shell` macro, which runs it's argument through bash and echos `stdout` into the document, or `\env` which inserts the contents of the given environment variable. Such usage, of course, must be tempered by security concerns. If the user intends to process potentially unsafe source documents, they must take care to restrict potential misuse and probably should provide the software a sanitized working space, or limit the functionality of available macros to known safe operations. For instance, one might replace `\shell` with `\date`, `\wc`, etc.  

## TeX Macro Variation from TeX

Tex Macro is closely based on the original TeX input language, but some changes have been made in the interest of better meeting the need at hand:

		  |TeX			   | TeX Macro
------------------|------------------------|-----------------
Character encoding| 8-bit, system dependant|UTF-8
Character set	  | 8-bit, system dependant|Unicode
Macros		  | All			   |Some
Char Classes      | 12			   |5 default; up to 256

One of the most obvious changes to make was to define the default input encoding as UTF-8.Internally, all characters are conceptually Unicode code points. This a break from the single byte mentality of original TeX, which allowed for only 256 symbols per font, and was remapped based on the user system's keyboard and language. English users should notice no difference, while support for other languages is now much more complete.

Original TeX defined quite a number of default macros. Those related specifically to typesetting are omitted. Such domain specific functionality can, and should, be implemented directly by the user as his need dictates. Obtuse macros like `\write16` are avoided in favor of clearer alternatives such as `\shell` as described above. Although this example is not included in the base functionality due to security concerns.  Otherwise, the naming of TeX macros remain unchanged.

TeX originally called for characters to be classified in to one of 12 classes during the parsing phase. Several of these classes have domain specific definitions, such as & to align text vertically. Such classes are ignored. The functionality of classes used by TeX Macro is described below, and the remaining classes of the 256 possible are left available for the reader's purposes.
