>greet_prog 100~
if ispc($n)
    say I can help you practice your skills, train your attributes and gain new abilities, $N.
    if level($n) >= 60
        say As you are beyond your sixtieth season you are experienced enough to learn newer arts too.
    endif
endif
~
>rand_prog 15~
if rand(50)
    strut
    blink
else
    flex
endif
~
|
