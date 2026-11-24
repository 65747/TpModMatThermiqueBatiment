TP thermique du bâtiment
Première version du programme

FICHIERS
- Thermique du bâtiment-1.pdf : sujet du TP.
- LE_PUY_CHADRAC.csv : données météo fournies.
- thermique.cpp : premier calcul statique des pertes par les murs.

COMPILER
Dans PowerShell, depuis le dossier du projet :

g++ -std=c++17 -O2 -Wall -Wextra -pedantic -o thermique.exe thermique.cpp

LANCER

.\thermique.exe

Le programme affiche les pertes d'un mur témoin de 50 m2, composé de 50 cm de béton.
