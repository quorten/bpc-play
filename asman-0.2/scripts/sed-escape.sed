# Escape user-input strings so that they will be safe to use within a
# sed script.  This will only work for basic regular expressions.

s.\\.\\\\.g
s./.\\/.g
s.\*.\\*.g
s.\^.\\^.g
s.\$.\\$.g
s.\[.\\[.g
s.\].\\].g
