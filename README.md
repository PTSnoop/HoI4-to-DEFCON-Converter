# Hearts Of Iron IV to DEFCON Converter

So you looked at the long list of [Paradox game converters](https://forum.paradoxplaza.com/forum/index.php?threads/paradox-game-converters.743404/), able to convert one single historical timeline of save games all the way through [Attila: Total War](https://github.com/ijedi1234/Attila2CK2), [Crusader Kings 2](https://store.steampowered.com/app/226665/DLC__Crusader_Kings_II_Europa_Universalis_IV_Converter/), [Europa Universalis IV](https://github.com/kingofmen/CK2toEU4/), [Victoria 2, Hearts Of Iron 4](https://github.com/Idhrendur/paradoxGameConverters) and [Stellaris](https://github.com/PTSnoop/HoI4-to-Stellaris-Converter). 

And you wondered... "what about that big 250-year gap between the end of World War 2 and the start of interstellar travel? Shouldn't there be *something* there?"

Yes. Yes, there should. But to carry us over until Paradox decide to un-cancel [East vs West](https://en.wikipedia.org/wiki/East_vs._West_%E2%80%93_A_Hearts_of_Iron_Game), here's something slightly different. 

The HoI4 to Defcon converter takes save files from the World War 2 strategy game [Hearts Of Iron 4](https://store.steampowered.com/app/394360/Hearts_of_Iron_IV/), and converts them into mods for the nuclear war strategy game [DEFCON](https://store.steampowered.com/app/1520/DEFCON/).

To use the converter, edit configuration.txt to add the correct paths and options (explained therein), then run HoI4ToDEFCON.exe .

Using CImg (CeCILL-licensed) for image-wrangling. Paradox-save-format parser and logging code taken from the other [Paradox Game Converters](https://github.com/Idhrendur/paradoxGameConverters), used under MIT licence.

# Screenshots

![Alternate History](https://i.imgur.com/6d2CynH.jpg)

If 1939 had nukes: Axis vs Allies vs Comintern.

![Border Gore](https://i.imgur.com/0NWxJOv.jpg)

From EU4: a slightly different set of colonial empires.

![Colonial Empires](https://i.imgur.com/gr8Z9xk.jpg)

War of the Atlantic: alternate-universe France vs Spain.

# Features

* The sides in DEFCON can be generated from the HoI4 factions, the six strongest nations, or chosen manually in the config file.
* Supports vanilla HoI4, Vic2toHoI4-converter games, or other HoI4 mods.
* Should continue to illustrate why nuclear war is a really really bad idea.

# Limitations

* The border calculation isn't as exact as I'd like. Occasionally cities next to your country's border will hop sides and convert across as one of yours. I'd just put this down to the border conflicts that sparked off this nuclear war in the first place.
* All players start off with equal populations and units, regardless of population or industrial capacity. There's no way to change this without delving into the DEFCON source code itself.
* I can't think of any reason why this wouldn't work with the big total-conversion HoI4 mods like Kaiserreich or Millenium Dawn. But I've not really tested it properly.
* No converter frontend support yet. It shouldn't be too hard to add this, though.
* DEFCON has no save games. So make sure you write down what the end-of-game numbers were if you want to put them into the [HoI4 to Stellaris converter](https://github.com/PTSnoop/HoI4-to-Stellaris-Converter). 
