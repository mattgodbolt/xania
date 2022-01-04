>greet_prog 30~
if isgood($n)
	if level($n) > 18
		mpechoat '$n' $i snaps to attention at your arrival.
		mpechoaround '$n' $i snaps to attention at the arrival of $n.
	endif
else
	if rand(30)
		growl $n
		say Evil is not wanted here!
	endif
endif
~
|
