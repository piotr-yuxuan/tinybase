tinybase
========

![SM kit](http://www.sm-sim.com/uploads/6/4/6/6/6466686/458006.jpg)

###Quelques précisions
Quelques précisions concernant le code:
* Pour tout ce qui est donné par l'utilisateur on fait le choix de **tout passer en minuscules** pour éviter les problèmes. Donc quand l'utilisateur input un truc on le transforme automatiquement en minuscules, et il ne peut pas y avoir deux tables appelées "voiTures" et "voitures" par exemple ou deux attributs "Couleur" et "couleur".
* Dans la description du sm parfois ils disent **redbase** au lieu de **tinybase** (juste au cas où)
* Et il y a peut être un problème aussi avec le fichier **redbase.h** ce devrait plutôt être **tinybase.h** Pierre est-ce que ça vient du setup 3? J'ai renommé `redbase.h` en `tinybase.h`. Quelques fichiers sont touchés. Je pense que c'est le prof qui n'a pas fait gaffe.
* Je (@piort2b) me suis permis d'ajouter un type de retour à la fonction main de dbcreate, sinon Eclipse me dit qu'il y a une erreur de syntaxe.

### Composant SM
On en est donc à la troisième partie, le composant SM. La deadline est fixée au **3 février** par le prof et la séance d'aide au 30 janvier. Cela dit, étant donné ce qui s'est passé pour les autres composants si on essaie de le faire pour la deadline ça va merder donc je vous propose que l'on se fixe que **tout doit être terminé pour le 29 au soir** de cette manière on pourra poser nos questions au prof le lendemain et l'on pourra débugguer tous ensemble sur le temps qu'il nous reste (croyez-moi il y en aura du débuggage). Si vraiment on est trop forts et que tout marche alors tant mieux on pourra profiter de notre dur labeur ou passer au composant suivant qui sera normalement très tendu à faire (théoriquement seulement 4 jours pour le faire d'après la page du prof).

### Répartition des tâches
**Concernant la répartition des tâches.** J'ai attribué les tâches comme suit mais bien sûr c'est juste arbitraire si quelqu'un n'est pas content de sa partie il peut le dire et on échange sans aucun souci. Il y a quatre parties différentes donc chacun se charge de l'une d'entre elles. Voilà la répartition:

* **Camille** (moi^^) se charge de faire **Unix command line utilities** (trois utilitaires UNIX à implémenter *dbcreate*, *dbdestroy* et *TinyBase*. C'est ce qui a l'air le plus simple et donc comme j'ai beaucoup travaillé avant...)

* **Pierre** je t'ai mis sur le **TinyBase System Commands**. Il ya *create table*, *drop table*, *create index* et *drop index*. Là ce ne sont pas des commandes UNIX, c'est simplement des méthodes qu'il implémenter dans la seule classe du SM, SM_Manager. [Voir section sur la page du SM](http://www.infres.enst.fr/~griesner/tinybase/sm.html#system)

Pour ce qui est de la partie **System Utilities** je l'ai coupée en deux entre Pauline et Yixin car elle a l'air assez chaude.
* **Yixin** tu te chargeras de faire **load relName("FileName");** qui permet de charger une table depuis un fichier écrit en ASCII. Je ne t'ai mis que cette méthode là car ça n'a pas l'air simple il faut manipuler le ASCII, prendre en compte Int, loat, String, tronquer si nécessaire, etc. C'est bien sûr aussi à toi de créer des fichiers ASCII pour les tests associés à ça.
* Et donc pour finir **Pauline** tu feras les deux autres fonctions des utilitaires qui sont:
  * **help** qui affiche des infos sur les table ou sur une table en particulier si elle est précisée en argument.
  * **print relName;** affiche les tuples dans une relation, en sachant que le prof file déjà ce qu'il appelle [The printer class](http://www.infres.enst.fr/~griesner/tinybase/sm.html#printer) donc tu ne doit pas tout refaire à zéro du tout. Il faut regarder la classe du prof et faire des appels pour printer la table.

Si vous lisez la page du SM vous remarquerez qu'il reste une fonction **set Param = "Value";** dont il faudra qu'on discute pour savoir quels paramètres on choisit de laisser modifier à l'utilisateur, je pense que c'est des trucs comme la taille des pages, etc. mais dans un premier temps c'est peut-être pas la peine d'implémenter celle-là puisqu'il est marqué qu'ils ne la mette que si on a besoin de laisser l'utilisateur choisir des trucs...

### Rappels
Sinon rappel très important ***ON NE COPIE PAS SUR LES GITS DEJA EXISTANT***. De toute manière l'intérêt c'est aussi de faire soi-même et on n'a pas envie de se faire épingler au détecteur de plagiat. On commente bien son code aussi et on commit régulièrement. Et puis si tout le monde s'y met ça ne devrait être si dur que ça!

### Ressources
Livres : [base de données](http://libgen.org/book/index.php?md5=E60B59A176028E4C66E2C42265A06427) et [C++](libgen.org/get.php?md5=2c10708a8337097ada6a36dc5b0efd24)
[//]: # (guide de Markdown ici : https://help.github.com/articles/github-flavored-markdown/)

