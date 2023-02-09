;; Implementation of the Storm mode for Emacs.
;;
;; This mode is a bit special compared to other modes. Once enabled, it spawns a Storm process. This
;; process is then used to provide syntax highlighting for all supported file types, even if
;; storm-mode is not configured to use those file types.
;;
;; Use 'global-storm-mode' to start storm-mode globally, or 'storm-mode' to use storm-mode for a
;; single buffer.

;; Disable lockfiles as they confuse Storm...
(setq create-lockfiles nil)

;; Decrease the delay on w32 as the default introduces a noticeable delay for tab completion otherwise.
;; Note: Use emacs 25 or later, otherwise sending messages is very slow on windows.
(setq w32-pipe-read-delay 0)
(setq w32-pipe-buffer-size (* 128 1024))

;; Configuration.
(defvar storm-mode-root nil "Root of the Storm source tree.")
(defvar storm-mode-compiler nil "Path to the Storm compiler.")
(defvar storm-mode-include nil "Additional paths to include. List of cons-cells: (path . pkg).")
(defvar storm-mode-stdlib nil "Root of the Storm standard library (currently not used).")
(defvar storm-mode-compile-compiler nil "Try to compile the compiler before starting it.")
(defvar storm-mode-max-edits 20 "Maximum number of entries saved in the edit history.")

;; State for all buffers using storm-mode.
(defvar global-storm-mode nil "Is 'global-storm-mode' enabled? Use the function 'global-storm-mode' to enable/disable.")
(defvar storm-mode-buffers (make-hash-table) "Map int->buffer for all buffers and their associated id.")
(defvar storm-mode-types (make-hash-table :test 'equal) "Map str->t/nil of supported file types.")
(defvar storm-mode-next-id 0   "Next usable buffer ID in storm-mode.")
(defvar storm-started-hook nil "Hook executed when the storm process has started.")
(defvar-local storm-buffer-id nil "The ID of the current buffer in storm-mode.")
(defvar-local storm-buffer-edit-id 0 "The ID of the next edit operation for the current buffer.")
(defvar-local storm-buffer-edits nil
  "List of the last few edits to this buffer. This is so we can handle 'late' coloring messages from Storm.
   The entries of this list has the form (offset skip marker) where offset is the character offset where the edit started,
   skip is the number of characters erased (if any) and marker is a marker placed at offset + skip.")
(defvar-local storm-buffer-no-changes nil "Inhibit change notifications for the current buffer.")
(defvar-local storm-buffer-last-point 0 "Remembers where Storm thinks the point is located in this buffer.")

;; Utility for simple benchmarking.
(defmacro log-time (name &rest body)
  "Measure the time taken to execute 'body', output as a message."
  `(let* ((start-time (current-time))
	  (body-result (progn ,@body)))
     (message "%s %.02f ms" ,name (* 1000 (float-time (time-since start-time))))
     body-result))

;; Mark errors in compilation-mode properly.
(require 'compile)

;; Compilation mode assumes that error messages are in screen columns. This is typically not the
;; case (for C++ or Storm).
(setq-default compilation-error-screen-columns nil)

(defun storm-find-error-buffer (full-file)
  (let ((buffer (find-buffer-visiting full-file)))
    (if buffer
	buffer
      (find-file-noselect full-file))))

(setq storm-error-regexp "^ *\\(p?[0-9]+[>:] ?\\)?@\\([^(\n\t]+\\)(\\([0-9]+\\)\\(-[0-9]+\\)?): [A-Za-z ]+ error:")

;; compile.el was updated in emacs 27. In the new version, the old way of adding custom hooks no
;; longer work properly. Instead, it provides a documented way of adding hooks.
(cl-pushnew (if (> emacs-major-version 26)
		;; For new emacs
		`(storm ,storm-error-regexp
			2 ;; File
			(storm-compute-line-begin . storm-compute-line-end) ;; Lines
			(storm-compute-col-begin . storm-compute-col-end) ;; Columns
			2 ;; Error type
			)
	      ;; For old emacs
	      `(storm ,storm-error-regexp
		      2 ;; File
		      storm-old-compute-line ;; Function to call (line)
		      3 ;; Column (likely not used due to hook)
		      nil ;; Type (likely not used due to hook)
		      )
	      )
	    compilation-error-regexp-alist-alist)
(add-to-list 'compilation-error-regexp-alist 'storm)

;; Old-style lookup function. Only works for emacs <= 25.
;; Does not put a end-marker anywhere, as the undocumented hook does not support it.
(defun storm-old-compute-line (data col)
  ;; Only works on the old compile.el.
  (let* ((file (nth 0 data))
	 (dir (nth 1 data))
	 (args (nth 2 data))
	 (char (1+ (string-to-number col)))
	 (full-file (expand-file-name file dir))
	 (buffer (storm-find-error-buffer full-file)))

    (with-current-buffer buffer
      (save-excursion
	(goto-char char)
	(cons nil ; Should be a marker, but is ignored by compile.el
	      (point-marker))))))


;; New-style (documented) lookup functions. Works for emacs >= 26.
;; It is a bit stupid that we have to convert from offset -> line + col so that compile.el
;; can convert it back to a marker later on.

;; An interesting side-effect of this (no idea why) is that compile.el puts the mark on the first
;; Storm error, even if there are earlier errors from e.g. C++. It is unclear why this happens.

;; Minimal cache for the buffer lookup. The functions are called alongside each other, so this will
;; be a hit most of the times.
(defvar storm-buffer-lookup-cache '("" . nil) "Cache for buffer lookups in the Storm compilation mode.")

;; Find an element in the cace.
(defun storm-cached-find-error-buffer (short-file)
  (if (string= (car storm-buffer-lookup-cache) short-file)
      (cdr storm-buffer-lookup-cache)
    (let ((buffer (storm-find-error-buffer (expand-file-name short-file))))
      (setq storm-buffer-lookup-cache (cons short-file buffer))
      buffer)))

;; Find line numbers.
(defun storm-compute-line (filename offset)
  (save-match-data
    (with-current-buffer (storm-cached-find-error-buffer filename)
      (line-number-at-pos (1+ (string-to-number offset)) t))))

(defun storm-compute-line-begin ()
  (storm-compute-line (match-string-no-properties 2) (match-string-no-properties 3)))

(defun storm-compute-line-end ()
  (let ((end (match-string-no-properties 4)))
    (if end
	(storm-compute-line (match-string-no-properties 2) (substring end 1 nil))
      (storm-compute-line (match-string-no-properties 2) (match-string-no-properties 3)))))

;; Find column numbers.
(defun storm-compute-col (filename offset)
  (save-match-data
    (with-current-buffer (storm-cached-find-error-buffer filename)
      (save-excursion
	(setq offset (1+ (string-to-number offset)))
	(goto-char offset)
	(+ (- offset (line-beginning-position)) compilation-first-column)))))

(defun storm-compute-col-begin ()
  (storm-compute-col (match-string-no-properties 2) (match-string-no-properties 3)))

(defun storm-compute-col-end ()
  (let ((end (match-string-no-properties 4)))
    (if end
	(storm-compute-col (match-string-no-properties 2) (substring end 1 nil))
      nil))) ;; nil means end of the line



;; Storm-mode for buffers.
(defun global-storm-mode (enable)
  "Use storm-mode for all applicable buffers. Pass 't or 'nil to set explicitly. Call with 'toggle or 
   call interactively to toggle."
  (interactive '(toggle))
  (when (eq enable 'toggle)
    (setq enable (not global-storm-mode))
    (message "Global Storm mode is %s" (if enable "on" "off")))
  (setq global-storm-mode enable)
  (when enable
    ;; Get notified about newly opened buffers. This is a no-op if the advice has already been added.
    (advice-add 'set-auto-mode :around #'storm-set-auto-mode)
    ;; Get notified when Storm is started so we may enable Storm-mode on all buffers.
    (add-hook 'storm-started-hook #'global-storm-started)
    ;; Start the process, if that is not done already.
    (if (not (storm-running-p))
	(storm-start)
      (global-storm-started))))

;; Hook into the 'set-auto-mode' function, so that we may get notified about newly opened files.
(defun storm-set-auto-mode (original &rest args)
  "Storm hook for 'set-auto-mode'."
  (unless (auto-storm-mode)
    (apply original args)))

(defun auto-storm-mode ()
  "Enable Storm mode for the current buffer if applicable."
  (when (and buffer-file-name global-storm-mode)
    (unless storm-buffer-id
      (let ((ext (file-name-extension buffer-file-name)))
	(when (storm-supports ext)
	  (storm-mode)
	  't)))))

(defun global-storm-started ()
  "Called when Storm has been started. If 'global-storm-mode' is
  enabled, checks all open buffers to see which shall use Storm-mode."
  (when global-storm-mode
    (let ((buffers (buffer-list)))
      (while buffers
	(with-current-buffer (car buffers)
	  (auto-storm-mode))
	(setq buffers (cdr buffers))))))

(defun storm-mode ()
  "Use storm-mode for the current buffer."
  (interactive)

  (kill-all-local-variables)
  ;; For now, we use the default syntax table.
  (use-local-map storm-mode-map)
  (set (make-local-variable 'indent-line-function) 'storm-indent-line)
  (setq major-mode 'storm-mode)
  (setq mode-name "Storm")
  (setq tab-width 4)
  (storm-register-buffer (current-buffer))
  (add-hook 'kill-buffer-hook #'storm-buffer-killed)
  (add-hook 'change-major-mode-hook #'storm-buffer-killed)
  (add-hook 'after-change-functions #'storm-buffer-changed)
  (add-hook 'post-command-hook #'storm-buffer-action)

  (run-mode-hooks 'storm-mode-hook))

(provide 'storm-mode)

(defun storm-indent-line ()
  "Indent the current line in storm-mode."
  (interactive)
  (when storm-buffer-id
    (let ((pos (point))
	  (goto-pos (- (point-max) (point)))
	  (beg (progn (beginning-of-line) (point)))
	  indent)
      (skip-chars-forward " \t")
      (if (< pos (point))
	  (setq goto-pos (- (point-max) (point))))
      (setq indent (storm-line-indentation))
      (when (integerp indent)
	;; Note: we can not use combine-after-change-calls here as it
	;; destroys the callbacks for buffer changes we get from Emacs.
	(delete-region beg (point))
	(indent-to indent))

      (goto-char (- (point-max) goto-pos))
      indent)))

(defun storm-line-indentation ()
  "Compute the indentation of the current line by querying the language server."
  (let ((response (storm-query (list 'indent storm-buffer-id (1- (point))))))
    (when (listp response)
      (setq storm-buffer-last-point (point))
      (when (>= (length response) 3)
	(let ((file-id (nth 0 response))
	      (kind    (nth 1 response))
	      (value   (nth 2 response)))
	  (cond ((not (eq storm-buffer-id file-id))
		 'noindent)
		((eq kind 'level)
		 (* value tab-width))
		((eq kind 'as)
		 (save-excursion
		   (goto-char (1+ value))
		   (current-column)))
		(t 'noindent)))))))

(defvar storm-mode-hook nil "Hook run when storm-mode is initialized.")

(defvar storm-mode-map
  (let ((map (make-keymap)))
    ;; Shortcuts for debugging the internal representation in Storm.
    (define-key map "\C-cd" 'storm-debug-tree)
    (define-key map "\C-cc" 'storm-debug-content)
    (define-key map "\C-cu" 'storm-debug-re-color)
    (define-key map "\C-cr" 'storm-debug-re-open)
    (define-key map "\C-cx" 'storm-debug-un-color)
    (define-key map "\C-ce" 'storm-debug-find-error)
    (define-key map "}"     'storm-insert-indent)
    (define-key map "\C-ch" 'storm-doc)
    map)
  "Keymap for storm-mode")


;; Commands.

(defun storm-insert-indent (arg)
  "Insert character and update indentation after doing so. Much like electric-indent in Emacs."
  (interactive "*P")
  ;; Note: If paren highlighting is active, the self-insert-command will block for some
  ;; time. Because of this, we will temporarily unset the blink function and then call it manually
  ;; later on.
  (let ((old-blink-paren blink-paren-function)
	(blink-paren-function nil))
    (self-insert-command (prefix-numeric-value arg))
    (storm-indent-line)
    ;; Blink paren if applicable.
    (and (eq (char-syntax (c-last-command-char)) ?\)) ;; is it a close paren character?
	 old-blink-paren ;; something to call?
	 (save-excursion
	  (funcall old-blink-paren)))))

(defun storm-debug-tree ()
  "Output debug information containing the syntax tree for the current buffer."
  (interactive)
  (when storm-buffer-id
    (storm-send (list 'debug storm-buffer-id t))))

(defun storm-debug-content ()
  "Output debug information of the contents of the current buffer."
  (interactive)
  (when storm-buffer-id
    (storm-send (list 'debug storm-buffer-id nil))))

(defun storm-debug-re-color ()
  "Ask Storm to re-color the current buffer."
  (interactive)
  (when storm-buffer-id
    (storm-send (list 'color storm-buffer-id))))

(defun storm-debug-un-color ()
  "Remove all syntax coloring from the current buffer."
  (interactive)
  (when storm-buffer-id
    (storm-set-color (point-min) (point-max) nil)))

(defun storm-debug-re-open ()
  "Quickly re-open the current buffer in case it has gotten out of sync with the Storm 
   process for some reason. This should not be neccessary, but is provided as it is convenient
   during debugging."
  (interactive)
  (when storm-buffer-id
    (storm-register-buffer (current-buffer))))

(defun storm-debug-find-error ()
  "Find and output the first syntax error in the current buffer."
  (interactive)
  (when storm-buffer-id
    (storm-send (list 'error storm-buffer-id))))

;; Convenience for highlighting.

(defun storm-set-color (from to face)
  "Convenience for highlighting parts of the text."
  (when (< from to)
    (with-silent-modifications
      ;; Maybe not the correct way of doing this, but makes sure old properties from copy+paste
      ;; operations are properly overwritten.
      (set-text-properties from to (list 'font-lock-face face)))))


(defvar storm-colors
  (let ((map (make-hash-table)))
    (puthash 'comment 'font-lock-comment-face map)
    (puthash 'delimiter 'font-lock-delimiter-face map)
    (puthash 'string 'font-lock-string-face map)
    (puthash 'constant 'font-lock-constant-face map)
    (puthash 'keyword 'font-lock-keyword-face map)
    (puthash 'fn-name 'font-lock-function-name-face map)
    (puthash 'var-name 'font-lock-variable-name-face map)
    (puthash 'type-name 'font-lock-type-face map)
    map)
  "Lookup color names in emacs lisp")

(defun storm-find-color (color)
  "Find the proper symbol representing 'color'."
  (gethash color storm-colors nil))

(defface storm-server-killed
  '((t :foreground "dark red"))
  "Face used indicating the server died for some reason.")

(defface storm-msg-error
  '((t :foreground "red"))
  "Face used indicating communication errors.")

;; Buffer management.

(defun storm-register-buffer (buffer)
  "Register a new buffer with the running storm process."
  (with-current-buffer buffer
    (unless storm-buffer-id
      ;; Not in the list of buffers, add it!
      (setq storm-buffer-id storm-mode-next-id)
      (puthash storm-mode-next-id buffer storm-mode-buffers)
      (setq storm-mode-next-id (1+ storm-buffer-id)))

    ;; Clear the edit history.
    (setq storm-buffer-edit-id 0)
    (setq storm-buffer-edits nil)
    (setq storm-buffer-last-point (point))

    ;; Tell Storm what is happening.
    (storm-send (list 'open
		      storm-buffer-id
		      (buffer-file-name)
		      (buffer-substring-no-properties (point-min) (point-max))
		      (point)))))

(defun storm-buffer-killed ()
  "Called when a buffer is going to be killed."
  (when storm-buffer-id
    (let ((buf-id storm-buffer-id))
      ;; Remove the id early in case we accidentally enter a recursive call...
      (setq storm-buffer-id nil)
      (remhash buf-id storm-mode-buffers)
      (when (storm-running-p)
	(storm-send (list 'close buf-id))))))


(defun storm-buffer-changed (edit-begin edit-end old-length)
  "Called when the contents of a buffer has been changed.
  NOTE: This is called when we set text color of the buffer as well... Inhibit that!"
  (when (and storm-buffer-id (not storm-buffer-no-changes))
    ;; Remember the edit.
    (let ((marker (make-marker)))
      (setq storm-buffer-edit-id (1+ storm-buffer-edit-id))
      (setq storm-buffer-edits
	    (storm-limit-edit-length
	     storm-mode-max-edits
	     (cons (list edit-begin
			 old-length
			 (set-marker marker edit-end))
		   storm-buffer-edits))))

    ;; Tell Storm.
    (setq storm-buffer-last-point edit-begin)
    (storm-send (list 'edit
		      storm-buffer-id
		      storm-buffer-edit-id
		      (1- edit-begin)
		      (+ edit-begin old-length -1)
		      (buffer-substring-no-properties edit-begin edit-end)))))

(defun storm-buffer-action ()
  "Called after each user interaction. We use this to tell Storm approximatly where the cursor is."
  (when storm-buffer-id
    (let ((np (point)))
      (when (< 500 (abs (- np storm-buffer-last-point)))
	(setq storm-buffer-last-point np)
	(storm-send (list 'point storm-buffer-id (1- np)))))))

(defun storm-limit-edit-length (max-length list)
  "Limit the length of the list 'list' containing edits."
  (let ((at list)
	(pos 1)
	(free nil))
    (while (consp at)
      (when (= pos max-length)
	(setq free (cdr at))
	(setcdr at nil))
      (setq at (cdr at))
      (setq pos (1+ pos)))

    ;; Remove any markers so that they do not slow things down...
    (while (consp free)
      (set-marker (caddr (car free)) nil)
      (setq free (cdr free)))
    list))


;; Handle messages

(defun first-n (list n)
  "Extract the first 'n' elements of a list"
  (if (and (> n 0) (consp list))
      (cons (car list)
	    (first-n (cdr list) (1- n)))
    nil))

(defun storm-active-edits (edit-id)
  "Find the part of the edit history relevant to consider when coloring text."
  (let ((entries (first-n storm-buffer-edits
			  (- storm-buffer-edit-id edit-id))))
    (when (consp entries)
      (sort entries (lambda (a b) (< (car a) (car b)))))))

(defmacro storm-check-edits (pos min-pos max-pos edits)
  "Update 'pos' according to the history in 'edits'. Consumes entries from 'edits'."
  `(while (and (consp ,edits)
	       (<= (car (car ,edits)) ,pos))
     (let* ((entry (car ,edits))
	    (offset (nth 0 entry))
	    (skip   (nth 1 entry))
	    (marker (nth 2 entry)))
       (when (> skip 0)
	 (setq ,min-pos (min (car entry) ,max-pos)))
       (setq ,pos (+ (marker-position marker) (- ,pos offset skip)))
       (setq ,edits (cdr ,edits)))))

(defun storm-on-color (params)
  "Handle the color-message."
  (if (>= (length params) 3)
      (let* ((buffer-id (nth 0 params))
	     (edit-id   (nth 1 params))
	     (start-pos (1+ (nth 2 params)))
	     (at        (nthcdr 3 params))
	     (buffer    (gethash buffer-id storm-mode-buffers nil)))
	(when buffer
	  (with-current-buffer buffer
	    ;; Make sure we do not act on our own color notifications!
	    (let ((storm-buffer-no-changes t)
		  (edits   (storm-active-edits edit-id))
		  (lowest  (point-min))
		  (highest (point-max))
		  (end-pos 0))

	      (storm-check-edits start-pos lowest highest edits)
	      (while (consp at)
		(setq end-pos (+ start-pos (car at)))
		(storm-check-edits end-pos lowest highest edits)

		(storm-set-color
		 (max lowest start-pos)
		 (min highest end-pos)
		 (storm-find-color (cadr at)))

		(setq start-pos end-pos)
		(setq at (nthcdr 2 at)))))))
    "Too few parameters."))

(defun storm-supports (ext)
  "Is the file type 'ext' supported by Storm? Returns false if no Storm process is running."
  (when (and (storm-running-p) (stringp ext))
    (let ((old (gethash ext storm-mode-types 'unknown)))
      (if (eq old 'unknown)
	  (let ((r (storm-query (list 'supported ext))))
	    (when (listp r)
	      (puthash ext (cadr r) storm-mode-types)
	      (cadr r)))
	old))))


;; Communication with the Storm process.

(defvar storm-process nil "The currently running Storm process.")
(defvar storm-process-output nil "The buffer currently used for output from Storm.")
(defvar storm-process-buffer "" "Buffer used when reading input from the Storm process.")
(defvar storm-process-sym-to-id nil "List of symbols shared with the Storm process.")
(defvar storm-process-id-to-sym nil "List of symbols shared with the Storm process.")
(defvar storm-process-next-id 0 "Next symbol id.")
(defvar storm-process-stop-timer nil
  "Timer used for timeout when killing the storm process along with the function to run afterwards.")
(defvar storm-process-message-queue nil
  "Messages queued for when the storm process was started.")
(defvar storm-process-query nil "If symbol: waiting for a message with the given header. If list: result from a query.")
(defvar storm-process-wait nil "If symbol: waiting for a message with the given header.")
(defvar storm-messages
  (let ((map (make-hash-table)))
    (puthash 'color 'storm-on-color map)
    map)
  "Messages handled by this plugin.")

(defun storm-start ()
  "Make sure that the background Storm process is up and running. Start it if neccessary."
  (interactive)
  (unless (storm-running-p)
    ;; TODO: Ask for missing parameters!
    (if storm-mode-compile-compiler
	(storm-start-compile)
      (storm-start-compiler))))

(defun storm-stop (&optional when-done)
  "Stop the current Storm process (if any)."
  (interactive)
  (if storm-process
      (progn
	;; Give Storm 2 seconds to terminate gracefully...
	(setq storm-process-stop-timer
	      (cons
	       (run-at-time 2 nil 'storm-kill)
	       when-done))
	;; Send a quit message.
	(storm-output-string "\n\nTerminating...\n" 'storm-server-killed)
	(storm-send '(quit)))
    (when when-done
      (funcall when-done))))

(defun storm-kill ()
  "Kill the storm process."
  (storm-output-string "\n\nKilling storm process...\n" 'storm-server-killed)
  (storm-terminated))

(defun storm-terminated ()
  "Called when the storm process has been terminated (or should be forcefully terminated)."
  (delete-process storm-process)
  (setq storm-process-output nil)
  (setq storm-process nil)
  (when storm-process-stop-timer
    (let ((to-call (cdr storm-process-stop-timer)))
      (cancel-timer (car storm-process-stop-timer))
      (setq storm-process-stop-timer nil)
      (when to-call
	(funcall to-call)))))

(defun storm-save-restart ()
  "Restart the storm process after saving buffers."
  (interactive)
  (save-some-buffers)
  (storm-restart))

(defun storm-restart ()
  "Restart the current Storm process."
  (interactive)
  (storm-stop 'storm-start))

(defun storm-send (message &rest force)
  "Send a message to the Storm process (launching it if it is not running)."
  (if (storm-running-p)
      ;; Keep message ordering if we try to send messages during startup.
      (if (and (eq force 'nil) (consp storm-process-message-queue))
	  (setq storm-process-message-queue
		(cons message storm-process-message-queue))
	(process-send-string storm-process (storm-encode message)))
    (progn
      (setq storm-process-message-queue
	    (cons message storm-process-message-queue))
      (storm-start))))

(defun storm-query (message &optional timeout)
  "Send a message to the Storm process and await a result for a maximum of 1 second. The 
   response is expected to start with the same symbol as the sent message."

  (setq storm-process-query (car message))
  (storm-send message)

  (unless timeout
    (setq timeout 1))

  ;; Wait until we receive some data.
  (let ((start-time (current-time)))
    (while (and (not (listp storm-process-query))
		(< (float-time (time-since start-time)) timeout))
      (accept-process-output storm-process timeout)))

  (prog1
      storm-process-query
    (setq storm-process-query nil)))

(defun storm-wait-for (type &optional timeout)
  "Wait for a message of 'type' to arrive. 'timeout' is measured in seconds."
  (unless timeout
    (setq timeout 1.0))

  (setq storm-process-wait type)

  ;; Wait until we receive some data.
  (let ((start-time (current-time)))
    (while (and storm-process-wait
		(< (float-time (time-since start-time)) timeout))
      (accept-process-output storm-process timeout)))

  (prog1
      (eq storm-process-wait nil)
    (setq storm-process-wait nil)))

(defun storm-running-p ()
  "Get the current status of the Storm process."
  (if storm-process
      (process-live-p storm-process)
    nil))

(defun storm-parent-directory (dir)
  (unless (equal "/" dir)
    (file-name-directory (directory-file-name dir))))


(defvar storm-running-compile nil "The buffer currently used for compiling Storm.")

(require 'compile)
(defun storm-start-compile ()
  "Start the Storm compiler by first compiling it."
  (add-to-list 'compilation-finish-functions 'storm-compilation-finished)
  ;; Make sure not to run multiple compilations.
  (unless storm-running-compile
    (let ((default-directory (storm-parent-directory (file-name-directory storm-mode-compiler))))
      (setq storm-running-compile
	    (compilation-start "mm Main -ne")))))

(defun storm-compilation-finished (buffer exit-status)
  "Called whenever a compilation buffer has finished."
  (when (eq buffer storm-running-compile)
    (setq storm-running-compile nil)
    (if (string-prefix-p "finished" exit-status)
	(progn
	  (message "Compilation successful, starting Storm!")
	  (storm-start-compiler))
      (progn
	(message "Compilation failed.")
	(setq storm-process-message-queue nil)))))

(defun storm-include-params ()
  (storm-include-params-i storm-mode-include))

(defun storm-include-params-i (list)
  (if (null list)
      'nil
    (let* ((first (car list))
	   (path  (car first))
	   (pkg   (cdr first)))
      (append
       (list "-I" pkg (expand-file-name path))
       (storm-include-params-i (cdr list))))))

(defun storm-start-compiler ()
  "Start the Storm compiler."
  ;; Don't use a PTY. Otherwise (=4) breaks everything.
  (let ((process-connection-type nil))
    (setq storm-process
	  (apply #'start-process
		 "*storm-interactive*"
		 nil
		 (append
		  (list storm-mode-compiler
			"-r" (expand-file-name storm-mode-root))
		  (storm-include-params)
		  (list "--server")))))
  (set-process-coding-system storm-process 'binary 'binary)
  (set-process-filter storm-process 'storm-on-message)
  (set-process-sentinel storm-process 'storm-on-status)
  (set-process-query-on-exit-flag storm-process nil)

  ;; Set up the compilation buffer.
  (setq storm-process-output (get-buffer-create "*compilation*"))
  (setq storm-process-buffer "")
  (setq storm-process-sym-to-id (make-hash-table))
  (setq storm-process-id-to-sym (make-hash-table))
  (setq storm-process-next-id 0)
  (setq storm-mode-types (make-hash-table :test 'equal))
  (with-current-buffer storm-process-output
    (buffer-disable-undo)
    (setq buffer-read-only nil)
    (erase-buffer)
    (setq buffer-read-only t)
    (set-buffer-multibyte t)
    (compilation-mode))

  ;; Re-create any open buffers inside the Storm process.
  (maphash
   (lambda (id buffer)
     (storm-register-buffer buffer))
   storm-mode-buffers)

  ;; Send any queued messages.
  (storm-send-messages storm-process-message-queue)
  (setq storm-process-message-queue nil)

  (run-hooks 'storm-started-hook))


(defun storm-send-messages (queue)
  "Send all messages in the queue, in reverse order (as that is how they are added)."
  (when queue
    (storm-send-messages (cdr queue))
    (storm-send (car queue) t)))

(defun storm-on-message (process input)
  "Receives input from a Storm process and acts accordingly."
  (setq storm-process-buffer
	(storm-decode-message-string (concat storm-process-buffer input))))

(defun storm-decode-message-string (string)
  "Decodes 'input', returns anything that was not processed."
  (let* ((r (storm-decode-messages string))
	 (complete (car r))
	 (remaining (cdr r)))
    (when complete
      ;; Try to output as much as possible.
      (let* ((decoded (decode-coding-string remaining 'utf-8))
	     (remaining-at (1- (length remaining)))
	     (decoded-at (1- (length decoded))))

	;; Make sure we do not have any partial codepoints in the string.
	(while (and (>= decoded-at 0)
		    (>= (aref decoded decoded-at) #x110000))
	  (setq remaining-at (1- remaining-at))
	  (setq decoded-at (1- decoded-at)))

	(storm-output-string (substring decoded 0 (1+ decoded-at)) nil)
	(setq remaining (substring remaining (1+ remaining-at)))))

    remaining))


(defun storm-decode-messages (string)
  "Decode all messages in 'string'. Returns (t remaining) if we managed to parse something,
   or (nil remaining) if we failed parsing things."
  (let ((null-char (cl-position #x00 string))
	(failed 'nil))
    (while (and (not failed)
		(numberp null-char))
      ;; Output the string found in the beginning.
      (storm-output-string (substring string 0 null-char) 'nil)
      (setq string (substring string null-char))

      ;; Try to parse the remaining string.
      (let ((result (storm-decode string)))
	(if (not result)
	    (setq failed 't)
	  (let ((consumed (car result))
		(message (cdr result)))
	    (setq string (substring string consumed))
	    (storm-handle-message message)
	    (setq null-char (cl-position #x00 string))))))

    ;; Figure out the result.
    (if failed
	(cons nil string)
      (cons t string))))

(defun storm-handle-message (msg)
  "Handle a message"
  (let ((err (storm-handle-message-i msg)))
    (when (stringp err)
      (storm-output-string
       (format "\nError when processing message: %s\n  message: %S\n" err msg) 'storm-msg-error))))

(defun storm-handle-message-i (msg)
  "Handle a message"
  (if (consp msg)
      (let* ((header (car msg))
	     (found (gethash header storm-messages)))
	(when (eq header storm-process-wait)
	  (setq storm-process-wait nil))
	(if (eq header storm-process-query)
	    (progn
	      (setq storm-process-query (cdr msg))
	      t)
	  (if found
	      (funcall found (cdr msg))
	    (format "The header %S is not supported." header))))
    "Invalid message format. Expected a list."))

(defvar storm-cr-lf "
")
(defvar storm-lf "
")

(defun storm-output-string (text &optional face)
  (when storm-process-output
    (with-current-buffer storm-process-output
      (let* (deactivate-mark ;; Don't mess with the mark in other buffers!
	     (buffer-read-only nil)
	     (old-point (point))
	     (start (point-max)))
	(goto-char (point-max))
	(insert text)
	;; Replace CR-LF with LF
	(goto-char (max (point-min) (1- start)))
	(while (search-forward storm-cr-lf nil t)
	  (replace-match storm-lf nil t))
	(when face
	  (storm-set-color start (point-max) face))
	(goto-char old-point)))))

(defun storm-on-status (process change)
  "Receives status notifications from the Storm process."
  (when (eq process storm-process)
    (unless (process-live-p storm-process)
      (message "Storm process terminated.")
      (storm-output-string "\n\nStorm process terminated.\n" 'storm-server-killed)
      (storm-terminated))))


;; Decoding messages in a raw string.

(defmacro storm-state (src)
  "Create a state for decoding messages. car is the current position, cdr is the length."
  `(cons 1 (length ,src)))

(defmacro storm-state-error (state)
  "Decide if the state is in an error condition. (ie. pos > length)"
  `(> (car ,state) (cdr ,state)))

(defmacro storm-state-set-error (state)
  "Make 'state' into an error state."
  `(progn
     (setcar ,state (1+ (cdr ,state)))
     'nil))

(defmacro storm-state-more-p (state more)
  "Decide if there is at least 'more' additional characters."
  `(if (eq 'nil ,more)
       nil
     (<= (+ (car ,state) ,more) (cdr ,state))))

(defun storm-decode (src)
  "Decode as much as possible of a message. Discards the first byte (assumes it is zero). Returns either
   (number result), which means that 'number' characters were consumed resulting in 'result',
   or nil, which means that nothing was consumed."
  (let* ((state (storm-state src))
	 (result (storm-decode-first-entry src state)))
    (if (storm-state-error state)
	'nil
      (progn
	(cons (car state) result)))))

(defun storm-decode-first-entry (src state)
  "Decode the first entry in 'src', starting by making sure the total number of bytes are available."
  (let ((msg-len (storm-decode-number src state)))
    (if (storm-state-more-p state msg-len)
	(storm-decode-entry src state)
      (storm-state-set-error state))))

(defun storm-decode-entry (src state)
  "Decode an entry in src."
  (if (storm-state-more-p state 1)
      (let* ((pos (car state))
	     (ch (aref src pos)))
	(setcar state (1+ pos))
	(cond ((= ch #x00) 'nil)
	      ((= ch #x01) (storm-decode-cons src state))
	      ((= ch #x02) (storm-decode-number src state))
	      ((= ch #x03) (storm-decode-string src state))
	      ((= ch #x04) (storm-decode-new-symbol src state))
	      ((= ch #x05) (storm-decode-old-symbol src state))
	      (t 'nil)))
    (storm-state-set-error state)))

(defun storm-decode-cons (src state)
  "Decode a sequence of cons-cells. If this function would be purely recursive, we quickly run into problems."
  (if (storm-state-error state)
      'nil
    (let* ((first (cons (storm-decode-entry src state) nil))
	   (current first))
      (while (and (storm-state-more-p state 1)
		  (= (aref src (car state)) #x01))
	(setcar state (1+ (car state)))
	(setcdr current (cons
			 (storm-decode-entry src state)
			 nil))
	(setq current (cdr current)))

      ;; Set 'cdr' of the last element as well!
      (if (storm-state-error state)
	  'nil
	(progn
	  (setcdr current (storm-decode-entry src state))
	  first)))))

(defun storm-decode-number (src state)
  "Decode a number"
  (let ((start (car state)))
    (if (storm-state-more-p state 4)
	(progn
	  (setcar state (+ 4 start))
	  (logior (lsh (aref src start) 24)
		  (lsh (aref src (+ start 1)) 16)
		  (lsh (aref src (+ start 2)) 8)
		  (aref src (+ start 3))))
      (storm-state-set-error state))))

(defun storm-decode-string (src state)
  "Decode a string"
  (let ((count (storm-decode-number src state)))
    (if (storm-state-more-p state count)
	(let ((pos (car state)))
	  (setcar state (+ count pos))
	  (decode-coding-string (substring src pos (+ pos count)) 'utf-8))
      (storm-state-set-error state))))

(defun storm-decode-new-symbol (src state)
  "Decode a new symbol"
  (let* ((id (storm-decode-number src state))
	 (str (storm-decode-string src state)))
    (if (storm-state-error state)
	'nil
      (let ((sym (intern str)))
	(puthash sym id storm-process-sym-to-id)
	(puthash id sym storm-process-id-to-sym)
	sym))))

(defun storm-decode-old-symbol (src state)
  "Decode an already existing symbol"
  (let ((id (storm-decode-number src state)))
    (if (storm-state-error state)
	'nil
      (gethash id storm-process-id-to-sym))))

;; Encode messages.

(defun storm-encode (msg)
  "Encode a s-expression."
  ;; We make sure not to clobber 'deactivate-mark' by saving it inside a let statement.
  ;; Writing into a buffer sets 'deactivate-mark', which causes the mark to be deactivated
  ;; during editing at unexpected times.
  (let (deactivate-mark)
    (with-temp-buffer
      (set-buffer-multibyte nil)
      (storm-encode-buffer msg)
      (let ((msg-len (- (point-max) (point-min))))
	(goto-char (point-min))
	;; Start of message marker.
	(insert-char #x00)
	;; Message length.
	(storm-encode-number msg-len))
      (buffer-string))))

(defun storm-encode-buffer (msg)
  "Encode a s-expression. Output to the current buffer."
  (cond ((eq msg 'nil)
	 (insert-char #x00))
	((consp msg)
	 (insert-char #x01)
	 (storm-encode-cons msg))
	((numberp msg)
	 (insert-char #x02)
	 (storm-encode-number msg))
	((stringp msg)
	 (insert-char #x03)
	 (storm-encode-string msg))
	((symbolp msg)
	 (storm-encode-sym msg))
	(t 'nil)))

(defun storm-encode-cons (c)
  (storm-encode-buffer (car c))
  (let ((at (cdr c)))
    (while (consp at)
      (insert-char #x01)
      (storm-encode-buffer (car at))
      (setq at (cdr at)))
    (storm-encode-buffer at)))

(defun storm-encode-number (num)
  (insert-char (lsh num -24))
  (insert-char (logand #xFF (lsh num -16)))
  (insert-char (logand #xFF (lsh num -8)))
  (insert-char (logand #xFF num)))

(defun storm-encode-string (str)
  (let ((coded (encode-coding-string str 'utf-8)))
    (storm-encode-number (length coded))
    (insert coded)))

(defun storm-encode-sym (sym)
  (let ((id (gethash sym storm-process-sym-to-id nil)))
    (if id
	(progn
	  (insert #x05)
	  (storm-encode-number id))
      (progn
	(setq id storm-process-next-id)
	(setq storm-process-next-id (1+ id))
	(puthash sym id storm-process-sym-to-id)
	(puthash id sym storm-process-id-to-sym)
	(insert #x04)
	(storm-encode-number id)
	(storm-encode-string (symbol-name sym))))))

;; Send some messages to Storm to test the implementation.
;; (let ((i 0))
;;   (while (< i 2)
;;     (mapcar
;;      (lambda (x)
;;        (send-string storm-process (char-to-string x))
;;        (sleep-for 0 10)
;;        (redisplay))
;;      (storm-encode '(test 1 2 3 4 5 6 7 "Hej")))
;;     (send-string storm-process "garbage")
;;     (setq i (1+ i))))


;; For debugging
;; (setq storm-process-sym-to-id (make-hash-table))
;; (setq storm-process-id-to-sym (make-hash-table))
;; (let ((msg (concat (storm-encode '(color 0 0 1 2 string 2 nil 2 string 2 nil)) "abc")))
;;  (storm-decode-message-string msg))

;; (defun storm-output-string (s z)
;;  (message "%s" s))
;; (recursion-depth)

;; Timing messages with large strings.
;; (with-current-buffer "eval.bs"
;;   (let ((str (log-time "Extracted text" (buffer-string))))
;;     (log-time "Send message " (storm-send (list 'dummy str)))))


(defvar storm-name-history nil "History for entered names.")
(defvar storm-complete-cache '(nil nil) "Last queried completions.")

(defun storm-complete-try (string predicate)
  "Try to complete 'string'. Returns either 't if it is a unique match, 'nil if no match and a string if multiple entries match."
  (try-completion string (storm-complete-all string predicate) predicate))


(defun storm-complete-all (string predicate)
  "Get all completions for 'string'."
  (unless (string= (car storm-complete-cache) string)
    ;; Update the cache.
    (let ((result (storm-query (list 'complete-name string default-directory) 2))) ;; Allow more timeout if compilation is needed.
      ;; When storm-query fails due to timeout, it returns the symbol "complete-name". If so, just pretend we have no completions.
      (when (symbolp result)
	(setq result '()))

      (setcar storm-complete-cache string)
      (setcdr storm-complete-cache result)))

  ;; TODO: respect predicate?
  (cdr storm-complete-cache))

(defun storm-complete-test (string predicate)
  "Is 'string' an exact match?"
  (test-completion string (storm-complete-all string predicate) predicate))

(defun storm-sort-names (names)
  "Sort function for names during autocompletion."
  (sort names #'string<))

(defun storm-complete-meta (string predicate)
  "Return metadata."
  '(metadata
    (category . storm-name)
    (display-sort-function . storm-sort-names)
    (cycle-sort-function . storm-sort-names)))

(defun storm-find-dot (string dir)
  "Find the last dot (.), looking either from the beginning or from the end."
  "Parens are respected, and dots inside them are ignored."
  (let ((parens 0)
	(pos 0)
	(result 0)
	(len (length string)))
    (when (< dir 0)
      (setq pos (1- len))
      (setq result len))

    (while (and (< pos len) (>= pos 0))
      (let ((ch (string-to-char (substring string pos (1+ pos)))))
	(cond ((and (= parens 0) (= ch ?.)) (setq result (1+ pos)))
	      ((= ch ?\() (setq parens (1+ parens)))
	      ((= ch ?\)) (setq parens (1- parens))))
	)
      (setq pos (+ pos dir))
      )
    result
    )
  )

(defun storm-complete-boundaries (string predicate suffix)
  (cons
   'boundaries
   (cons
    ;; (storm-find-dot string 1)
    ;; (storm-find-dot suffix -1))))
    0 (length suffix))))

(defun storm-complete (string predicate mode)
  (cond ((eq mode 'nil)      (storm-complete-try  string predicate))
	((eq mode 't)        (storm-complete-all  string predicate))
	((eq mode 'lambda)   (storm-complete-test string predicate))
	((eq mode 'metadata) (storm-complete-meta string predicate))
	((and (consp mode) (eq (car mode) 'boundaries))
	 (storm-complete-boundaries string predicate (cdr mode)))
	(t 'nil)))

(defun storm-read-name (prompt)
  "Read a name of an entity in Storm, giving the ability for auto-completion if available."
  (storm-start)
  (completing-read prompt 'storm-complete nil 'confirm nil 'storm-name-history))


(defun storm-doc (name &optional no-history)
  (interactive (list (storm-read-name "Show documentation for: ")))

  (let* ((msg (if (stringp name)
		  (list 'documentation name default-directory)
		name))
	 (doc (storm-query msg 3))) ;; Allow 3 s timeout for compilation.
    (if (or (null doc) (null (car doc)))
	(message "\"%s\" does not exist." name)
      (storm-show-doc msg (car doc) no-history))))


(defun storm-show-doc (msg doc no-history)
  (with-current-buffer-window
   "*storm-documentation*" nil nil

   (unless no-history
     ;; Add to history.
     (setq storm-doc-next nil)
     (setq storm-doc-prev (cons msg (limit storm-doc-prev storm-doc-history))))

   (mapc #'storm-insert-doc doc)
   (storm-doc-mode)

   ))

(defun limit (seq max)
  "Limit the length of 'seq' to 'max' elements."
  (if (<= max 0)
      nil
    (if (null seq)
	nil
      (cons (car seq)
	    (limit (cdr seq) (1- max))))))

(defun storm-insert-doc (doc)
  (if (< (length doc) 6)
      (insert "Invalid documentation format returned from Storm.\n")
    (let ((name   (nth 0 doc))
	  (params (nth 1 doc))
	  (notes  (nth 2 doc))
	  (vis    (nth 3 doc))
	  (body   (nth 4 doc))
	  (pos    (nth 5 doc))
	  (refs   (nth 6 doc)))
      (when vis
	(storm-insert-link (cdr vis) (car vis) nil)
	(insert " "))
      (storm-insert-face name 'bold)
      (unless (null params)
	(insert "(")
	(storm-insert-param (car params))
	(mapc (lambda (x) (insert ", ") (storm-insert-param x)) (cdr params))
	(insert ")"))
      (unless (null notes)
	(insert " ")
	(storm-insert-note (car notes))
	(mapc (lambda (x) (insert ", ") (storm-insert-note x)) (cdr notes)))
      (insert ":\n\n")
      (when pos
	(insert "At: ")
	(storm-insert-pos-link (format "%s(%d-%d)" (nth 0 pos) (nth 1 pos) (nth 2 pos)) pos t)
	(insert "\n\n"))
      (insert body "\n")
      (dolist (ref refs)
	(storm-insert-face (format "\n%s (mouse-1 or RET to visit):\n" (car ref)) 'bold)
	(dolist (entry (cdr ref))
	  (storm-insert-link (cdr entry) (car entry) nil)
	  (insert "\n")))))
  (insert "\n\n"))

(defun storm-insert-face (text face)
  (let* ((from (point))
	 (to   (progn (insert text) (point))))
    (add-text-properties from to (list 'face face))))

(defun storm-insert-link (text target mark)
  (let* ((from (point))
	 (to   (progn (insert text) (point))))
    (make-button from to
		 'target target
		 'follow-link t
		 'face (if mark 'button nil)
		 'help-echo (format "Visit the documentation for '%s'." target)
		 'action #'storm-link-pressed)))

(defun storm-link-pressed (button)
  (let ((target (button-get button 'target)))
    (cond ((eq target 'forward) (storm-doc-fwd))
	  ((eq target 'back) (storm-doc-back))
	  (t (storm-doc target)))))

(defun storm-insert-pos-link (text target mark)
  (let* ((from (point))
	 (to (progn (insert text) (point))))
    (make-button from to
		 'target target
		 'follow-link t
		 'face (if mark 'button nil)
		 'help-echo (format "Visit the file '%s'." (nth 0 target))
		 'action #'storm-pos-link-pressed)))

(defun storm-pos-link-pressed (button)
  (let* ((target (button-get button 'target))
	 (file   (nth 0 target))
	 (start  (nth 1 target))
	 (end    (nth 1 target)))
    (find-file file)
    (goto-char (1+ start))))

(defun storm-doc-fwd ()
  (interactive)
  (when storm-doc-next
    (setq storm-doc-prev (cons (car storm-doc-next) (limit storm-doc-prev storm-doc-history)))
    (setq storm-doc-next (cdr storm-doc-next))
    (storm-doc (car storm-doc-prev) t)))

(defun storm-doc-back ()
  (interactive)
  (when (and storm-doc-prev (> (length storm-doc-prev) 1))
    (setq storm-doc-next (cons (car storm-doc-prev) (limit storm-doc-next storm-doc-history)))
    (setq storm-doc-prev (cdr storm-doc-prev))
    (storm-doc (car storm-doc-prev) t)))


(defun storm-insert-param (param)
  (let ((name      (nth 0 param))
	(type-name (nth 1 param))
	(type-ref  (nth 2 param))
	(ref       (nth 3 param)))

    (if (null type-name)
	(setq type-name "void"))
    (if (null type-ref)
	(insert "void")
      (storm-insert-link type-name type-ref t))
    (when ref
      (insert "&"))
    (when (and (not (null name)) (< 0 (length name)))
      (insert " " name))))

(defun storm-insert-note (param)
  ;; Note: Either 1 or 4 elements.
  (if (> (length param) 2)
      (let ((note      (nth 0 param))
	    (type-name (nth 1 param))
	    (type-ref  (nth 2 param))
	    (ref       (nth 3 param)))
	(when (and (not (null note)) (< 0 (length note)))
	  (insert note " "))
	(if (null type-name)
	    (setq type-name "void"))
	(if (null type-ref)
	    (insert "void")
	  (storm-insert-link type-name type-ref t))
	(when ref
	  (insert "&")))
    (insert (car param))))

(defvar storm-doc-history 20 "Number of history entries for documentation in Storm.")
(defvar storm-doc-prev nil "History for the documentation from Storm.")
(defvar storm-doc-next nil "History for the documentation from Storm.")

(defun storm-doc-mode ()
  "Activate documentation mode in the current buffer."
  (setq major-mode 'storm-doc-mode)
  (setq mode-name "Storm documentation")

  ;; Add buttons indicating 'forward' and 'back' at the end.
  (save-excursion
    (goto-char (point-max))
    (when (and storm-doc-prev (> (length storm-doc-prev) 1))
      (storm-insert-link "[back]" 'back t)
      (when storm-doc-next
	(insert "  ")))
    (when storm-doc-next
      (storm-insert-link "[forward]" 'forward t))
    (insert "\n"))

  ;; Keybindings.
  (local-set-key (kbd "C-c C-b") #'storm-doc-back)
  (local-set-key (kbd "C-c C-f") #'storm-doc-fwd)
  )
