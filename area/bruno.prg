>greet_prog 40~
if ispc($n)
	if isgood($n)
		if rand(50)
			say Alright 'arry!
			shake $n
		else
			'arry my man!
			hug $n
		endif
	else
		if rand(50)
			bow $n
		else
			flex $n
		endif
	endif
endif
~
>rand_prog 5~
if rand(50)
	if rand(50)
		emote runs around waving $l arms in a victory salute!
		emote pulls up $l boxer shorts.
	else
		say Anyone seen Tyson?
		growl
	endif
else
	remove gloves
	emote polishes $l gloves until they gleam.
	wear gloves
endif
~
>speech_prog p xania is~
if ispc($n)
	if rand(50)
		say Yeah? What you gotta say about Xania?
		peer $n
	else
		say Hey mate, you don't know of any pantomime theatres do you?
	endif
endif
~
>fight_prog 50~
if rand(10)
	if ispc($n)
		bonk $n
		say You'll never make 10 rounds, you wimp!
	endif
endif
if rand(10)
	if ispc($n)
		flex
		say HAHA! You think you can go the whole distance do you?!
	endif
endif
~
>hitprcnt_prog 20~
if rand(50)
	if rand(50)
		emote hides behind $l gloves.
	else
		emote ducks and weaves to avoid your blows.
	endif
endif
~
|
