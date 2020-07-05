#include "colors.inc"  
#include "stones.inc"
#include "skies.inc"

background { color rgb 1 }

camera {
	location <-3, 3, -8>
	look_at < -.3, .5, 0 >
	angle 25
}

light_source { <500,500,-1000> White }

difference {
	box { <-2.0, -.5, 0.1>, <1.95, .8, 1>
		texture { T_Stone10 }
	}
	text { ttf "timrom.ttf" "Players" 0.15, 0
		pigment { BrightGold }
		finish { reflection.25 specular 1 }
		translate -1.6*x
	}
}

sky_sphere { S_Cloud5 }