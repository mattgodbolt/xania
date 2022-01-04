>greet_prog 40~
if isgood($n)
	if rand(50)
		if level($n) > 12
		   mpechoat '$n' $i snaps to attention at your arrival.
		mpechoaround '$n' $i snaps to attention at the arrival of $n.
	else
		emote salutes.
	endif
else
	if rand(30)
		growl $n
		say Evil is not wanted here!
	endif
endif
~
|
