;;; file-log.el --- hook functions for simple file logging

;; This file is in public domain.

;; Author: Andrew Makousky
;; Created: 05 Jan 2012
;; Version: 0.1
;; Keywords: files log logging

;;; Commentary:

;; Sometimes I find it is useful to be able to track what files I
;; modified since a certain date without having to search for these
;; files or use a full-blown version cotrol system.  As long as you
;; only edit files in Emacs, you can keep a list of every file that
;; you have changed since the log was initially created by binding the
;; function `file-log-save' to either `before-save-hook' or
;; `after-save-hook'.

;;; Code:

(provide 'file-log)

(defgroup file-log nil
  "Simple logging of file changes."
  :prefix 'file-log
  :group 'files)

(defcustom file-log-log-file "~/mod/emacs-rec.txt"
  "Name of the file to write log entries to."
  :group 'file-log
  :type 'file)

(defcustom file-log-cmd-syntax nil
  "If `t', specifies that the file log should take the form of a
series of touch commands.  If `nil', only file names will be
written."
  :group 'file-log
  :type 'boolean)

;;;###autoload
(defun file-log-write ()
  "Writes the current buffer's file name to a designated log file.
If this is bound to `before-save-hook' or `after-save-hook', then
the hooks are temporarily disabled when saving the log file."
  (interactive)
  (save-excursion
    (let (log-buffer changed-buffer prev-before-hooks prev-after-hooks)
      (setq changed-buffer (current-buffer))
      (setq log-buffer (find-file-noselect file-log-log-file))
      (set-buffer log-buffer)
      (goto-char (point-max))
      (if file-log-cmd-syntax
	  (insert "sh-cmd: touch \""))
      (insert (file-log-escape-specials (buffer-file-name changed-buffer)))
      (if file-log-cmd-syntax
	  (insert "\""))
      (insert "\n")
      ;; Before we save this buffer, we must make sure that this
      ;; log-update hook is disabled.  Otherwise, we will get a problem of
      ;; infinite recursion.  Either that, or we might be able to define a
      ;; special mode in which we change the save hooks.
      (setq prev-before-hooks before-save-hook)
      (setq prev-after-hooks after-save-hook)
      (customize-set-variable 'before-save-hook nil)
      (customize-set-variable 'after-save-hook nil)
      (save-buffer)
      (customize-set-variable 'before-save-hook prev-before-hooks)
      (customize-set-variable 'after-save-hook prev-after-hooks))))

(defun file-log-disable ()
  "Disables file logging while editing a file log.  You should
always disable file logging before editing the file log in Emacs,
otherwise annoying or bad things could happen.  This is merely a
convenience function that assumes you do not have any other save
hooks that you want to keep set while editing the file log."
  (interactive)
  (setq file-log-prev-before-hooks before-save-hook)
  (setq file-log-prev-after-hooks after-save-hook)
  (customize-set-variable 'before-save-hook nil)
  (customize-set-variable 'after-save-hook nil))

(defun file-log-enable ()
  "Convenience function to enable the file log if you disabled it
with `file-log-disable'."
  (interactive)
  (customize-set-variable 'before-save-hook file-log-prev-before-hooks)
  (customize-set-variable 'after-save-hook file-log-prev-after-hooks))

(defun file-log-escape-specials (file-name)
  "Replaces each character that would be considered special
inside of a shell double quote with an escape sequence."
  (setq file-name (file-log-escape-char file-name "\\\\" "\\\\"))
  (setq file-name (file-log-escape-char file-name "\"" "\\\""))
  (setq file-name (file-log-escape-char file-name "\\$" "\\$"))
  (setq file-name (file-log-escape-char file-name "`" "\\`"))
  (setq file-name (file-log-escape-char file-name "\n" "${NL}")))

(defun file-log-escape-char (string char escape)
  "Replaces CHAR character with an escape sequence ESCAPE so that
the STRING can be used in shell scripts.  Because CHAR will be
used in `string-match', it must be escaped if it is special when
used in a regular expression."
  (let ((lastmatch 0))
    (while (setq lastmatch (string-match char string lastmatch))
      (setq string (replace-match escape nil t string))
      (setq lastmatch (+ lastmatch (length escape)))))
  string)

;; (defun test-fmt-modtime ()
;;   "Try displaying a buffer's modified time."
;;   (format-time-string "%Y-%m-%d %H:%M:%S %z" (visited-file-modtime) nil))
