;; Configuration

(modify-coding-system-alist 'file "\\.bs\\'" 'utf-8)
(add-to-list 'auto-mode-alist '("\\.h$" . c++-mode))
(add-to-list 'auto-mode-alist '("\\.bs$" . java-mode))

(defvar my-cpp-other-file-alist
  '(("\\.cpp\\'" (".h" ".hpp"))
    ("\\.h\\'" (".cpp" ".c"))
    ("\\.c\\'" (".h"))
    ))

(add-hook 'compilation-mode-hook
	  (lambda ()
	    (setq tab-width 4)))


;; Setup code-style. From the Linux Kernel Coding style.

(require 'whitespace)
(setq whitespace-style '(face trailing lines-tail))
(setq whitespace-line-column 120)

(defun blank-line ()
  (= (point) (line-end-position)))

(defun c-lineup-arglist-tabs-only (ignored)
  "Line up argument lists by tabs, not spaces"
  (let* ((anchor (c-langelem-pos c-syntactic-element))
	 (column (c-langelem-2nd-pos c-syntactic-element))
	 (offset (- (1+ column) anchor))
	 (steps (floor offset c-basic-offset)))
    (* (max steps 1)
       c-basic-offset)))

(add-hook 'c-mode-common-hook
          (lambda ()
            ;; Add kernel style
            (c-add-style
             "linux-tabs-only"
             '("linux" (c-offsets-alist
                        (arglist-cont-nonempty
                         c-lineup-gcc-asm-reg
                         c-lineup-arglist-tabs-only))))
	    (setq tab-width 4)
	    (setq ff-other-file-alist my-cpp-other-file-alist)
	    (setq ff-special-constructs nil)
	    (setq indent-tabs-mode t)
	    (c-set-style "linux-tabs-only")
	    (whitespace-mode t)
	    (setq c-basic-offset 4)

	    ;; Better handling of comment-indentation. Why needed?
	    (c-set-offset 'comment-intro 0)))

(defun storm-insert-comment ()
  "Insert a comment in c-mode"
  (interactive "*")
  (insert "/**")
  (indent-for-tab-command)
  (insert "\n * ")
  (indent-for-tab-command)
  (let ((to (point)))
    (insert "\n */")
    (indent-for-tab-command)
    (insert "\n")
    (if (not (blank-line))
	(indent-for-tab-command))
    (goto-char to)))

(defun storm-cpp-singleline ()
  (interactive "*")
  (if (blank-line)
      (insert "// ")
    (progn
      (storm-open-line)
      (insert "// "))))

(defun storm-return () 
  "Advanced `newline' command for Javadoc multiline comments.   
   Insert a `*' at the beggining of the new line if inside of a comment."
  (interactive "*")
  (let* ((last (point))
         (is-inside
          (if (search-backward "*/" nil t)
              ;; there are some comment endings - search forward
              (search-forward "/*" last t)
            ;; it's the only comment - search backward
            (goto-char last)
            (search-backward "/*" nil t))))

    ;; go to last char position
    (goto-char last)

    ;; the point is inside some comment, insert `* '
    (if is-inside
        (progn
          (newline-and-indent)
          (insert "*")
	  (insert " ")
	  (indent-for-tab-command))
      ;; else insert only new-line
      (newline-and-indent))))

;; Setup keybindings

(global-set-key (kbd "M-g c") 'goto-char)
(global-set-key (kbd "M-g M-c") 'goto-char)


(add-hook 'c-mode-common-hook
	  (lambda ()
	    (local-set-key (kbd "M-o") 'ff-find-other-file)
	    (local-set-key (kbd "C-o") 'storm-open-line)
	    (local-set-key (kbd "RET") 'storm-return)
	    (local-set-key (kbd "C-M-j") 'storm-cpp-singleline)
	    (local-set-key (kbd "C-M-k") 'storm-insert-comment)
	    (local-set-key (kbd "C->") "->")
	    )
	  )

;; Helpers for bindings.

(defun storm-open-line ()
  (interactive "*")
  (open-line 1)
  (let ((last (point)))
    (move-beginning-of-line 2)
    (if (not (blank-line))
	(indent-for-tab-command))
    (goto-char last)))

(add-hook 'find-file-hooks 'correct-win-filename)
(defun correct-win-filename ()
  (interactive)
  (rename-buffer (file-name-nondirectory buffer-file-name) t))

;; Behaviour.

(setq compilation-scroll-output 'first-error)

;; (defun check-buffer ()
;;   "If the buffer has been modified, ask the user to revert it, just like find-file does."
;;   (interactive)
;;   (if (and (not (verify-visited-file-modtime))
;; 	   (not (buffer-modified-p))
;; 	   (yes-or-no-p
;; 	    (format "File %s changed on disk. Reload from disk? " (file-name-nondirectory (buffer-file-name)))))
;;       (revert-buffer t t)))

;; (defadvice switch-to-buffer
;;   (after check-buffer-modified)
;;   (check-buffer))

;;(ad-activate 'switch-to-buffer)

;; Good stuff while using the plugin.
(load "~/Projects/storm/Plugin/emacs.el")
(load "~/Projects/storm/Plugin/emacs-test.el")
(setq storm-mode-root "~/Projects/storm/root")
(setq storm-mode-compiler "~/Projects/storm/debug/Storm")
(setq storm-mode-include '(
			   ("~/Projects/storm/External/final" . "final")
			   ("~/Projects/storm/External/thesis" . "thesis")
			   ))
(setq storm-mode-compile-compiler t)

(add-to-list 'command-switch-alist '("storm-demo" . to-storm-demo))
(defun to-storm-demo (&rest params)
  (interactive)
  (setq mymake-compile-frame nil)
  (setq storm-mode-compile-compiler nil)
  (set-face-attribute 'default nil :height 200)
  (global-storm-mode t))

(global-set-key (kbd "C-c h") 'storm-doc)

;;; For debugging ;;;

(global-set-key (kbd "C-M-u") 'storm-save-restart)
(global-set-key (kbd "C-M-y") 'storm-stop)

;; For debugging mymake when needed...
;;(setq mymake-command "C:/Users/Filip/Projects/mymake/release/mymake.exe")
