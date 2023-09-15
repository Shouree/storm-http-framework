Language Server
==================

In order to make interaction with new languages and language extensions easier, Storm provides a
language server. This means that it is possible to run Storm in the background while developing
programs so that the editor is able to ask Storm about tasks such as syntax highlighting and
indentation. This, in turn allows the editor to provide good feedback regardless of whether the
language is supported by the editor or not.

In order to use the language server, a plugin for your editor is required. At the moment, a
[plugin for Emacs](md://Storm/Language_server/Emacs_plugin) is maintained as a reference implementation.
It is included when downloading Storm as `storm-mode.el`. If no plugin is yet available for your editor
of choice, the protocol is described [here](md://Storm/Language_server/Protocol), which makes it
possible to build one without too much trouble.

The goal is to extend the protocol to allow sending commands to the top-loop, compiling parts of the
system etc. through the language server. We're not there yet, but we're getting there!

For a more detailed description of the language server, look at the master thesis that formed the
foundation of the language server:
[http://urn.kb.se/resolve?urn=urn:nbn:se:liu:diva-138847](http://urn.kb.se/resolve?urn=urn:nbn:se:liu:diva-138847)

