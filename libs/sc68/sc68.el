;;;; Emacs Lisp help for writing sc68 code. ;;;;
;;;; $Id$

;;; The sc68 hacker's C conventions.
;;; Based on curl.el from cURL package.

(defconst sc68-c-style
  '((c-basic-offset . 2)
    (c-comment-only-line-offset . 0)
    (c-hanging-braces-alist     . ((substatement-open before after)))
    (c-offsets-alist . ((topmost-intro        . 0)
			(topmost-intro-cont   . 0)
			(substatement         . +)
			(substatement-open    . 0)
			(statement-case-intro . +)
			(statement-case-open  . 0)
			(case-label           . 0)
			(label                . /)
			))
    ;; (c-doc-comment-style . ((c-mode . (gtkdoc javadoc))))
    )
  "sc68 C Programming Style")

(defun sc68-code-cleanup ()
  "no docs"
  (interactive)
  (untabify (point-min) (point-max))
  (delete-trailing-whitespace)
)

;; Customizations for all of c-mode, c++-mode, and objc-mode
(defun sc68-c-mode-common-hook ()
  "sc68 C mode hook"
  ;; add sc68 style and set it for the current buffer
  (c-add-style "sc68" sc68-c-style t)
  (setq tab-width 8
	indent-tabs-mode nil		; Use spaces. Not tabs.
	comment-column 40
	c-font-lock-extra-types (append '("u8" "s8" "u16" "s16" "u32" "s32"))
	)
  ;; keybindings for C, C++, and Objective-C.  We can put these in
  ;; c-mode-base-map because of inheritance ...
  (define-key c-mode-base-map "\M-q" 'c-fill-paragraph)
  (define-key c-mode-base-map "\M-m" 'sc68-code-cleanup)
  (setq c-recognize-knr-p nil)
  ;;; (add-hook 'write-file-hooks 'delete-trailing-whitespace t)
  (setq show-trailing-whitespace t)
  )

;; Set this is in your .emacs if you want to use the c-mode-hook as
;; defined here right out of the box.
;(add-hook 'c-mode-common-hook 'sc68-c-mode-common-hook)
