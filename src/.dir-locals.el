;;; .dir-locals.el --- 

;; Copyright (C) Michael Kazarian
;;
;; Author: Michael Kazarian <michael.kazarian@gmail.com>
;; Keywords: 
;; Requirements: 
;; Status: not intended to be distributed yet


((nil . ((eval . (progn
                   (setq-local tags-candidate '("."))
                   (setq-local clang-tags-candidate
                               '("~/.platformio/packages/framework-arduino-avr"
                                 "~/bc/.pio/libdeps/pro16MHzatmega328"
                                 "."))
                   (setq-local company-clang-arguments (clang-src-dirs))
                   (add-to-list 'company-backends '(company-etags company-clang))
                   (add-hook 'after-save-hook
                             (lambda ()
                               (etags-tag-create (find-dirs-str))))
                   ))
         (eval . (git-gutter-mode))
         (eval . (yas-minor-mode-on))
         (eval . (message ".dir-locals.el was loaded"))
         )))

;;; .dir-locals.el ends here
