>entry_prog 30~
emote keeps an eye out for wanted criminals.
~
>greet_prog 40~
if isgood($n)
	say Good day.
	smile $n
else
	mpechoaround '$n' $I wonders if $N is among the most wanted.
	mpechoat '$n' $I wonders if you are among the most wanted.
endif
~
>rand_prog 20~
if rand(50)
	emote whistles a little song.
else
	sing
endif
~
>act_prog p bursts into~
say What is wrong?
~
>speech_prog p My girlfriend left me~
comfort $n 
~
>speech_prog hello hi~
say Greetings $N!
~
>act_prog p zaps a~
if objtype($p) == 25
	say Oi! Be careful zapping $P with yer magic wand!
endif
~
|