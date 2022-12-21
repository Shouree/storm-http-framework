# Place the following line in the file ~/.gdbinit to load this file.
# add-auto-load-safe-path ~/Projects/storm/.gdbinit
handle SIGSEGV nostop noprint
# Old signals:
handle SIGXFSZ nostop noprint
handle SIGXCPU nostop noprint
# New signals
handle SIG34 nostop noprint
handle SIG35 nostop noprint
