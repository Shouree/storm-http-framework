;; Tests for the emacs plugin (mainly the protocol).
;(load "emacs.el")

;; Run tests using C-c t
(global-set-key (kbd "C-c t") 'storm-run-tests)

(defun storm-run-tests ()
  (interactive)
  (puthash 'test 'storm-on-test storm-messages)
  (add-hook 'storm-started-hook 'storm-start-tests)
  (setq storm-run-tests t)
  (storm-restart))

;; All tests to run.
(defvar storm-tests
  '(
    storm-short-messages
    storm-short-spaced-messages
    storm-echo-short-messages
    storm-echo-short-spaced-messages
    )
  "All tests to execute.")
(defvar storm-test-msg nil "Store any test messages arrived.")
(defvar storm-run-tests nil "Run tests when the compiler has started.")

(defface storm-test-msg
  '((t :foreground "dark green"))
  "Face used indicating test status.")

(defface storm-test-fail
  '((t :foreground "red"))
  "Face used indicating test failures.")

(defun storm-start-tests ()
  "Called when a new instance has been started."
  (when storm-run-tests
    (setq storm-run-tests nil)
    (let ((at storm-tests)
	  (ok t))
      (while (consp at)
	(setq storm-test-msg nil)
	(storm-output-string (format "Running %S...\n" (car at)) 'storm-test-msg)
	(if (catch 'storm-test-failed (funcall (car at)))
	    (setq at (cdr at))
	  (progn
	    (storm-output-string (format "\nFailed %S. Terminating.\n" (car at)) 'storm-test-fail)
	    (setq ok nil)
	    (setq at nil))))

      (when ok
	(storm-output-string "\nAll tests passed!\n" 'storm-test-msg)))))

(defun storm-wait-message ()
  "Wait for the storm process to send us a message. Times out after about a second."
  (let ((result t))
    (while (and result
		(null storm-test-msg))
      (redisplay)
      (setq result (accept-process-output storm-process 1)))

    (prog1
	(car (last storm-test-msg))
      (setq storm-test-msg (butlast storm-test-msg)))))

(defun storm-on-test (msg)
  "Called when a test-message has arrived."
  (if (consp msg)
      (progn
	(setq storm-test-msg
	      (cons msg storm-test-msg))
	t)
    "Test messages should be lists."))

(defun storm-check-equal (a b)
  (unless (equal a b)
    (storm-output-string (format "%S is not equal to %S\n" a b) 'storm-test-fail)
    (throw 'storm-test-failed nil)))

(defun storm-short-messages ()
  "Send lots of short messages to Storm."
  (storm-send '(test start))
  (let ((msg '(test sum 1 2 3))
	(times 100))
    (dotimes (i times)
      (storm-send msg))

    (storm-send '(test stop))
    (storm-check-equal (storm-wait-message)
		       (list 'sum (* (+ 1 2 3) times)))
    t))

(defun storm-short-spaced-messages ()
  "Send lots of short messages with some data in between."
  (storm-send '(test start))
  (let ((msg '(test sum 2 3 4))
	(times 100))
    (dotimes (i times)
      (storm-send msg)
      (process-send-string storm-process "<>"))

    (storm-send '(test stop))
    (storm-check-equal (storm-wait-message)
		       (list 'sum (* (+ 2 3 4) times)))
    t))

(defun storm-echo-short-messages ()
  "Send lots of short messages to Emacs."
  (storm-send '(test start))
  (let ((times 100))
    (storm-send (list 'test 'send times '(test msg) 'nil))
    (dotimes (i times)
      (storm-check-equal (storm-wait-message)
			 '(msg))))

  (storm-send '(test stop))
  t)
    
(defun storm-echo-short-spaced-messages ()
  "Send lots of short messages to Emacs."
  (storm-send '(test start))
  (let ((times 100))
    (storm-send (list 'test 'send times '(test msg) "<>"))
    (dotimes (i times)
      (storm-check-equal (storm-wait-message)
			 '(msg))))

  (storm-send '(test stop))
  t)
