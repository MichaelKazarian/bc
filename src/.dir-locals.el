;;; .dir-locals.el --- 

;; Copyright (C) Michael Kazarian
;;
;; Author: Michael Kazarian <michael.kazarian@gmail.com>
;; Keywords: 
;; Requirements: 
;; Status: not intended to be distributed yet

((nil . ((company-clang-arguments . (
                                     "-I~/.platformio/packages/framework-arduino-avr"
                                     "-I~/bc/.pio/libdeps/pro16MHzatmega328"
                                     "-I~/bc/src"))
         (eval . (progn
                   (add-to-list 'company-backends '(company-etags company-clang))
                   (add-hook 'after-save-hook 'etags-tag-create)))
         (eval . (git-gutter-mode))
         (eval . (yas-minor-mode-on))
         (eval . (message ".dir-locals.el was loaded"))
         )))

;;; .dir-locals.el ends here
