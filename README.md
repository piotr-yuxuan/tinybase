tinybase
========

Bon il faut qu'on se mette au travail là. Pierre plus je relis ce que tu as fait moins je comprends comment tu comptes implémenter l'algorithme de l'arbre B+. A mon avis **un index correspond à un fichier qui sera alloué par le IX_IndexManager** et ensuite dans le **IX_IndexHandle** il faut qu'on ait un *PF_FileHandle** en attribut pour gérer le fichier qui nous est donnée. Ensuite il faudrait qu'on ait les structures suivantes:

* une structure **fileHeader** pour le fichier qui contienne entre autres le **numéro de la page de la racine**
* une structure **nodeHeader** pour un noeud qui contienne entre autres **le niveau (feuille, racine, noeud interne)** du noeud, le **numero de la page du noeud parent**, le **numéro de la page précédente** et le **numéro de la page suivante**.
* Et ensuite pour le bucket (voir la page du projet sur IX) il faudrait une structure **bucketHeader** qui contienne le **nombre de RID stockés**, le **nombre max** et éventuellement le **bucket suivant** si on fait des chaînes de buckets.

Et après pour les algos d'insertions/suppression on loop dans les noeuds avec les pages et on écrit quand il y a besoin, etc. Parce que dans ce que tu as fais c'est gentil mais je ne vois pas du tout le rapport avec le fichier, l'index doit quand même être stocké **dans un fichier** à la base et on utilise PF pour récupérer les pages au fur et à mesure (et si le buffer est assez grand toutes les pages se retrouveront bufferisées tant mieux).

* Pour cloner le dépôt ou monter à distance votre répertoire de l'école, voir [ici](Trucs techniques.md).
* Sources RedBase : [yifeih](https://github.com/yifeih/redbase) et [junkumar](https://github.com/junkumar/redbase)
* Livres : [base de données](http://libgen.org/book/index.php?md5=E60B59A176028E4C66E2C42265A06427) et [C++](libgen.org/get.php?md5=2c10708a8337097ada6a36dc5b0efd24)
[//]: # (guide de Markdown ici : https://help.github.com/articles/github-flavored-markdown/)

La manière dont sont numérotés les clés et pointeurs dans les différents noeuds:

* **In internal node (or root):**
Header Pointer-1 Key0 Pointer0 Key1 Pointer1 ... KeyN PointerN

* **In leaf node:**
Header Key0 Pointer0 Key1 Pointer1 ... KeyN PointerN

* **In Bucket:**
Header RID0 RID1...RIDN