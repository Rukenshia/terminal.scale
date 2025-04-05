.PHONY: fonts
fonts:
	fontconvert assets/fonts/GeistMono-VariableFont_wght.ttf 18 > include/GeistMonoVariableFont_wght18.h
	fontconvert assets/fonts/GeistMono-VariableFont_wght.ttf 16 > include/GeistMonoVariableFont_wght16.h