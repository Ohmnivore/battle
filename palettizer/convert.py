import os

MAPS = ['battle_highway', 'emerald_beach', 'holy_summit', 'tails_lab']

for map in MAPS:
    os.system('python palettizer.py 0 {0}.pal {0}.png out/{0}_palette.png out/{0}.png'.format(map))
