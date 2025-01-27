# JUCE_FLAIR

## Description/Résumé du projet
Cette petite application développée avec JUCE permet d'inférer sur un modèle type FLAIR (3 canaux RGB).
L'application charge des dalles orthophotos de la GéoPlateforme et applique le modèle.
On récupère donc une image de même résolution avec les codes couleurs suivants :
1   : ['building','#db0e9a'] ,
2   : ['pervious surface','#938e7b'],
3   : ['impervious surface','#f80c00'],
4   : ['bare soil','#a97101'],
5   : ['water','#1553ae'],
6   : ['coniferous','#194a26'],
7   : ['deciduous','#46e483'],
8   : ['brushwood','#f3a60d'],
9   : ['vineyard','#660082'],
10  : ['herbaceous vegetation','#55ff00'],
11  : ['agricultural land','#fff30d'],
12  : ['plowed land','#e4df7c'],
13  : ['swimming_pool','#3de6eb'],
14  : ['snow','#ffffff'],
15  : ['clear cut','#8ab3a0'],
16  : ['mixed','#6b714f'],
17  : ['ligneous','#c5dc42'],
18  : ['greenhouse','#9999ff'],
19  : ['other','#000000'],

## Dépendances
Il faut installer JUCE (https://juce.com/) et lancer le Projucer pour créer le projet.
Le projet est compatible Windows, LINUX, MacOSX.
Pour Windows, JUCE crée un projet VisualStudio 2022.
Pour LINUX, JUCE crée un fichier Makefile. Pour compiler, il suffit de faire make CONFIG=Release.
Sous MacOSX, JUCE crée un projet XCode.

Il faut de plus installer la bibliothèque ONNX Runtime : https://github.com/microsoft/onnxruntime

JUCE_FLAIR a été testé avec la version 1.20.1 de ONNX Runtime.

## Exécution
Pour exécuter le logiciel, il faut disposer d'un modèle FLAIR au format ONNX.