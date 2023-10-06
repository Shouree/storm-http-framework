The Language Server
===================

Storm provides a language server to make the information possessed by Storm available to a text
editor or IDE. Among other things, the language server is able to provide syntax highlighting for
the extensible languages in Storm. It also allows a more convenient interface to browse the built-in
documentation compared to the interactive top-loop. There are also plans to extend the protocol to
allow hot-reloading of source code, and evaluating expressions from the language server. The goal is
thus to be able to perform most tasks directly from the language server, so that Storm can be
tightly integrated in your preferred text editor or IDE.

The language server is implemented as a program that is started by the editor whenever it is
required. The editor then communicates with the language server using the [protocol](md:Protocol)
described on this page to inform the language server of changes, and to receive responses. For this
to work, the editor most likely needs an appropriate plugin. Currently, an [Emacs plugin](md:Emacs_Plugin)
is maintained alongside Storm and it is distributed alongside the [binary releases](md:/Downloads)
of Storm. If no plugin is available for your editor, the protocol is
described [here](md:Protocol).

For more details on the language server, more detailed information is available
[here](http://urn.kb.se/resolve?urn=urn:nbn:se:liu:diva-138847).
