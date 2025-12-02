Martin (Nitorami), 1.3.2025

Hi all
These are the milkdrop presets I have made since I started this in 2007. Including (most of) my early attempts... yuk ! I went through all of them and made minor corrections where necessary to cope with the much faster modern GPUs, or to fix odd bugs. So if you happen to use my presets already, I recommend to replace them. They should all run ok with milkdrop v2.25c, although there's a few caveats:

- Do not use milkdrop v2.25d (comes with winamp 5.9x). There is no benefit, but it is buggy and cannot handle float to integer conversion. 
- Shader model PSVERSION 4 (see at the top of the .milk file) does not work with ATI video cards, due to an old bug in the milkdrop dll I am unable to fix. Therefore I avoided it where possible but there is one exception:

My latest work "emergency power supply only" is based on Nivush's ray marching code and needs PSVERSION_COMP=4, so won't run on ATI. Sorry for that. And please be aware that it will take a few seconds to compile, so I recommed not to store it in your standard presets folder, as it will interrupt the smooth transition between presets. Other than that, it needs its "skin" martin_skin_emergency_power_supply.jpg, and loads three random textures found in its own folder or under ...\milkdrop\textures. 

Enjoy!
Martin


