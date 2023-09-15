Protocol
=========

The language server expects to communicate with a client using standard input and standard
output. Because of this, it is the client's responsibility to start the language server as it
desires. To allow regular use of standard input and standard output for debugging purposes, all data
in the protocol starts with the null-byte. Everything else is considered UTF-8-encoded text which is
to be shown to the user.

The protocol used in the language server is based on s-expressions in LISP. Therefore, it can
represent four basic data types: integers, strings, symbols and cons-cells. A cons-cell is an object
containing two members, usually referred to as `car` and `cdr` or `first` and `rest`. These members
may refer to any instance of the data types in the protocol, including other cons-cells. They can
therefore be used to form more complex data structures, such as singly linked lists. A singly linked
list is implemented by creating one cons-cell for each element in the list. The list content is
stored in the `car` or `first` member of each cons cell. `cdr` or `rest` is used to point to the
next cons-cell in the list. The last element contains the symbol `nil`, which is the LISP equivalent
of `null` in C++ or Java.

S-expressions use the same textual representations as in many LISP dialects, such as Emacs
LISP. *Strings* and *numbers* are represented in the same way string- and numeric literals are
represented in most programming languages. *Symbols* are similar to identifiers in programming
languages, they are a sequence of non-whitespace characters that do not start with digits or a dash
(since that is used for unary negation). *Cons-cells* are represented by two primitives enclosed in
parenthesis and separated by a dot. `(10 . 11)` is a cons-cell containing the numbers 10 and 11. In
this notation, a linked list containing two elements is represented as follows: `(10 . (11 . nil))`.
This notation quickly becomes impractical for large lists. To remedy this, linked lists can also be
written as a sequence of s-expressions separated by whitespace enclosed in parenthesis. The list
above can then be written as `(10 11)`, which is equivalent to `(10 . (11. nil))`.

The protocol is encoded in binary and based around messages. A message is a complete s-expression,
usually a list of some sort. As mentioned earlier, messages always start with a null-byte. The
null-byte is followed by the length of the message, in bytes, encoded as a 32-bit (= 4-bytes)
integer in big-endian format. The *length* bytes following the size constitutes the message body,
which contains a single s-expression. S-expressions always start with a single byte indicating the
type of the stored element, followed by the actual data. The supported types are as follows:

* `0x00`: the symbol `nil`. No additional data is required. `nil` is a commonly used symbol,
  and as such it has a special encoding.
* `0x01`: a cons-cell. Following this byte are two additional s-expressions. The first one
  is the contents of the `car` or `first` member, the second is the contents of the `cdr`
  or `rest` member.
* `0x02`: a number. The following four bytes contain the actual value of a 32-bit number stored
  in big-endian format, just like the size.
* `0x03`: a string. The first four bytes contain the length of the string (in bytes) as a 32-bit
  number in big-endian format. Following the size is the bytes constituting the string. These are
  encoded in UTF-8.
* `0x04`: a symbol. This encoding is used for a symbol that has not previously been sent. The first
  4 bytes is an integer containing an id to use for this symbol in future communication. Following
  the id is a string (without the type-mark) containing the name of the symbol.
* `0x05`: a symbol. This encoding is used for symbols that have previously been sent encoded as `0x04`.
  This representation only contains 4 bytes containing the identifier of the symbol. This identifier
  should be the same as a symbol previously sent using `0x04` in either direction.

For example, the message `(a 10 a "b")`, or equivalently `(a . (10 . (a . ("b" . nil))))`
would be encoded as follows:
```
0x00                     //       // Start of message.
0x00 0x00 0x00 0x1F      //       // Length of the message, 31 bytes.
0x01                     // (     // Start of a cons-cell.
 0x04                    // a     // A new symbol...
  0x00 0x00 0x00 0x01    //       //  with index 1...
  0x00 0x00 0x00 0x01    //       //  identifier length 1...
  0x61                   //       //  identifier content "a"
 0x01                    // . (   // Start of a cons-cell.
  0x02                   // 10    // A number...
   0x00 0x00 0x00 0x0A   //       // value 10
  0x01                   // . (   // Start of a cons-cell.
   0x05                  // a     // A previously seen symbol...
    0x00 0x00 0x00 0x01  //       //  with index 1
   0x01                  // . (   // Start of a cons-cell.
    0x03                 // "b"   // A string...
     0x00 0x00 0x00 0x01 //       //  with length 1...
     0x62                //       //  and content "b".
   0x00                  // . nil // The symbol nil.
                         // ))))  //
```

Messages
---------

Using the protocol described above, it is possible to send s-expressions as messages from the
language server to a text editor and vice versa. Both the language server and the editor may send
messages at any time, but communication is usually initiated by the client. In the language server,
messages are always lists where the first element is a symbol describing the message type.

The language server associates each open file with an integer identifier. This is decided by the
client when a new file is opened and used to refer to that file in all further communication. If an
identifier is reused, the previous file is closed. Each open file contains the following state: the
complete contents of the file, the index of the last edit operation and an approximate location of
the user's cursor. The index of the last edit operation is passed along in `color` messages, so that
the client can determine which version of the file the language server is referring to. The edit
number is initially set to zero, and is changed every time the client sends an `edit` message. The
cursor location is maintained so that the language server is able to prioritize sending syntax
coloring information for text visible to the user (i.e. close to the cursor).

The following messages can be sent from the text editor to the language server:

* `(quit)`: Asks the language server to terminate.
* `(supported type)`: Ask the language server if the file extension *type* is supported. *type* is
  a string containing the file extension without a dot. The language server sends a `(supported type result)`
  message containing the result.
* `(open id path content pos)`: Open a new file, *path* (a string), and give it the identifier *id*
  (a number). The current contents of the file are sent as a string in *content*. *pos* is
  optional. If present, it is the current position of the cursor within this file.
* `(close id)`: Closes the file with identifier *id* (a number).
* `(edit id edit from to text)`: An edit has been made to the file with identifier *id* (a
  number). The text between *from* and *to* (numbers, zero based character indexes) has been
  replaced with *text* (a string). *edit* is the edit number of this edit. Initially opening a file
  is considered to be edit number zero.
* `(point id pos)`: The user has moved their cursor in the file with identifier *id* to *pos* (a
  number, zero based character index). This message does not need to be sent every time the user
  moves the cursor. As long as the language server's knowledge of the cursor is close enough, no
  updates need to be sent.
* `(indent id pos)`: Request indentation information for character *pos* (a number) in the file with
  identifier *id* (a number). The language server replies to this message with an `(indent file value)` message.
* `(color id)`: Send all coloring information for file *id* once more.
* `(complete-name string context)`: Reply with a list of names matching *string*. *string* is parsed using the
  default syntax for names in Storm. Names are resolved using a scope that looks for names inside `core`
  and optionally in the package corresponding *context*, if provided.
* `(documentation name context)`: Reply with the documentation for *name*. *name* is parsed just like
  in the `complete-name` message.

The following messages can be sent from the language server to the text editor:

* `(supported type result)`: Sent as a reply to `(supported type)`. Indicates whether the previously
  queried file extension *type* is supported or not. *result* is either `t` or `nil`, indicating
  *supported* and *not supported* respectively.
* `(color id edit start colors...)`: Color a part of the file with identifier *id* as denoted by
  colors. The character indexes in this message refers the text as it was after edit number *edit*.
  If additional edit operations have been performed since, indexes may need to be adjusted by the
  editor. In the Emacs plugin, this is done by remembering the last 20 or so edit
  operations. *start* (a number) is the zero based index of the first character to highlight. The
  remaining elements in the message (labeled *colors...*) describe the colors. The first element in
  *colors* contain the number of characters to color. They shall be colored in the color indicated
  by the next element in *colors*. If there are more elements in *colors*, they are interpreted in
  the same way. For example: `(color 1 1 5 10 comment 5 keyword)` means: start at character number
  5, color the next 10 characters as a comment. The 5 characters following those of the comment
  shall be colored as a keyword.
* `(indent id type value)`: Sent as a reply to `(indent id pos)`. Tells the indentation for the last
  `indentation` request sent for file *id*. *type* is either `level` or `as`, and describes the
  meaning of the number *value*. If *type* is `level`, the line shall be indented *value* levels (a
  level is defined by the client). If *type* is `as`, the line shall be indented as the line
  containing the character at offset *value* (zero based index).
* `(complete-name name...)`: Sent as a reply to `(complete-name string context)` to provide a list
  of possible matches. The matches provided are generally only one level deep, meaning that
  the language server will not traverse the entire name tree to provide all possible completions since
  that would be very expensive and most likely not what is actually desired.
* `(documentation name data)`: Sent as a reply to `(documentation name context)`. If *name* did not exist,
  then *data* is `nil`. Otherwise, *data* is a list with elements of the form described below.

The following colors are available to the language server:

* `comment`: Used for comments. Emacs uses `font-lock-comment-face`.
* `delimiter`: Used for delimiters. Emacs uses `font-lock-delimiter-face`.
* `string`: Used for string literals. Emacs uses `font-lock-string-face`.
* `constant`: Used for constants, such as integer literals. Emacs uses `font-lock-constant-face`.
* `keyword`: Used for keywords in a language. Emacs uses `font-lock-keyword-face`.
* `fn-name`: Used for names of functions in a language. Emacs uses `font-lock-function-face`.
* `var-name`: Used for names of variables in a language. Emacs uses `font-lock-variable-face`.
* `type-name`: Used for names of types in a language. Emacs uses `font-lock-type-face`.

The data inside the `documentation` is a list containing the following data:

* `name`: A string containing the name of the entity.
* `params`: A list of parameters to this entity. Each of the parameters are a list, `(name type-name type-ref ref)`,
  where *name* is the name of the parameter, *type-name* is a string that contains the name of the type
  shown to the user, *type-ref* is a string containing the fully-qualified name of the type, or `nil` if
  it refers to `void`. *ref* is `t` if the parameter is a reference, otherwise `nil`.
* `notes`: A list of notes for this entity. Takes the same form as `params` above, except that *type-name* does
  not necessarily refer to a type (and thus has a *type-ref* that is `nil`). Any entity is valid as a
  note. The `type` and `ref` may be left out for notes, which means that no type shall be shown for this
  note at all. Note: The return value for functions is usually returned as a note.
* `visibility`: Visibility of this entity as a cons cell `(type . title)`, or `nil` if none was provided.
  *title* is a string suitable to show the user and *type* is the type implementing this visibility.
* `body`: The body text of the documentation as a string.
* `pos`: The location in source code of this element, as a list `(filename start-offset end-offset)`, where
  `filename` is the filename, and `start-offset` and `end-offset` is the start and end character offsets the
  file, or `nil` if no information is available.
* `refs`: A list of references to other things in the system. Each element has the form `(name . contents)`,
  where *name* is the name of this category and *contents* is a list with the actual contents. Each element
  there has the form `(name . title)` where *name* is the full name of the member and *title* is a short
  summary that is suitable to show the user.

