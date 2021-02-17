;;; GnuTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007-2012 Free Software Foundation, Inc.
;;;
;;; GnuTLS is free software; you can redistribute it and/or
;;; modify it under the terms of the GNU Lesser General Public
;;; License as published by the Free Software Foundation; either
;;; version 2.1 of the License, or (at your option) any later version.
;;;
;;; GnuTLS is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;; Lesser General Public License for more details.
;;;
;;; You should have received a copy of the GNU Lesser General Public
;;; License along with GnuTLS; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

;;; Written by Ludovic Court�s <ludo@chbouib.org>

(define-module (gnutls build utils)
  :use-module (srfi srfi-13)
  :export (scheme-symbol->c-name))

;;;
;;; Common utilities for the binding generation code.
;;;


;;;
;;; Utilities.
;;;

(define (scheme-symbol->c-name sym)
  ;; Turn SYM, a symbol denoting a Scheme name, into a string denoting a C
  ;; name.
  (string-map (lambda (chr)
                (if (eq? chr #\-) #\_ chr))
              (symbol->string sym)))


;;; Local Variables:
;;; mode: scheme
;;; coding: latin-1
;;; End:

;;; arch-tag: 56919ee1-7cce-46b9-b90f-ae6fbcfe4159
