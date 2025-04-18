.PHONY: fonts
fonts:
	fontconvert assets/fonts/GeistMono-VariableFont_wght.ttf 18 > include/GeistMonoVariableFont_wght18.h
	fontconvert assets/fonts/GeistMono-VariableFont_wght.ttf 16 > include/GeistMonoVariableFont_wght16.h
	fontconvert assets/fonts/GeistMono-VariableFont_wght.ttf 14 > include/GeistMonoVariableFont_wght14.h
	fontconvert assets/fonts/GeistMono-VariableFont_wght.ttf 12 > include/GeistMonoVariableFont_wght12.h
	fontconvert assets/fonts/GeistMono-VariableFont_wght.ttf 10 > include/GeistMonoVariableFont_wght10.h
	fontconvert assets/fonts/HelvetiPixel.ttf 12 > include/HelvetiPixelFont_wght12.h