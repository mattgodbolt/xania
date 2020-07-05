>greet_prog 30~
if ispc($n)
	beg $n
	say Spare some gold?
endif
~
>bribe_prog 10000~
dance $n
hug $n
~
>bribe_prog 1000~
say Oh my GOD!  Thank you! Thank you!
smile $n
~
>bribe_prog 100~
say Wow!  Thank you! Thank you!
~
>bribe_prog 1~
thank $n
~
>fight_prog 20~
say Help!  Please somebody help me!
if rand(50)
	say Ouch!
else
	say I'm bleeding.
endif
~
>death_prog 50~
if rand(50)
	mpechoaround $i $I says 'Now I go to a better place.'
else
	mpechoaround $i $I says 'Forgive me God for I have sinned...'
endif
~
|

