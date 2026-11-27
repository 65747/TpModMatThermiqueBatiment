TP thermique du bâtiment

FICHIERS
- Thermique du bâtiment-1.pdf : sujet du TP.
- LE_PUY_CHADRAC.csv : données météo fournies.
- Etape3.txt : équations et valeurs de l'étape 3.
- thermique.cpp : pertes statiques et évolution de la température d'une pièce.

COMPILER
Dans PowerShell, depuis le dossier du projet :

g++ -std=c++17 -O2 -Wall -Wextra -pedantic -o thermique.exe thermique.cpp

PERTES STATIQUES

.\thermique.exe

SIMULATION AVEC LA METEO

.\thermique.exe dynamic LE_PUY_CHADRAC.csv 20 600 explicit > temperatures.csv

Remplacer explicit par implicit pour utiliser Euler implicite. Le pas est en secondes.
Le programme affiche la température extérieure et celle de la pièce chaque pas. Les
valeurs minimale et maximale de la pièce s'affichent dans la console.
