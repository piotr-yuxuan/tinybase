tinybase
========

**Update:**

Après de nombreuses corrections pour moi ça a l'air de compiler (make s'execute sans erreur). Attention surtout pour Pierre 
**ça ne sert à rien de copier le code de Stanford sans le modifier** certe la classe RM_FileHandle est la plus dure pas de doute là dessus mais à la limite il faut mieux aller doucement méthode par méthode et nous après on vient t'aider à terminer celles qui manquent car là c'était vraiment la grosse merde dedans (par exemple par pitié arrête de créer un bitmap pour le pageHeader il en a déjà un en attribut public qui s'appelle .freeSlots donc ça ne rîme à rien!) 

Idem il faut commenter le code plus je pense (c'est pas parce que Stanford il n'ont rien mis que c'est forcément bien)

* Pour cloner le dépôt ou monter à distance votre répertoire de l'école, voir [ici](Trucs techniques.md).
* RedBase corrigé : https://github.com/junkumar/redbase
* Livre de référence du cours : http://libgen.org/get.php?md5=00E22B5CB10BC9859A3D389EA77BDD80

Classe			|	Codeur  | Etat
:---------------|:------------:|:--------
RM_Manager		|	Pauline | Being processed
RM_FileHandle	|	Pierre  | Le code devrait être terminé
RM_FileScan		|	Camille | Implémentation de base terminé, pas compilable
RM_Record		|	Yixin |
RID				|	Camille | J'ai fait une première version dans rm_rid.h et rm_rid.cc
RM_PrintError	|	  |

<h2>De la part de Pauline</h2>

Love Jean-Benoît ---> Deadline = mardi prochain :-D

C'est cool, tu vas pouvoir committer du Japon \o/ ah ah

<h2>De la part de Camille</h2>

J'ai ajouté la classe Bitmap comme recommandé dans l'énoncé en m'inspirant largement de https://github.com/junkumar/redbase/blob/master/src/bitmap.cc mais j'ai changé quelques trucs pour qu'elle soit plus simple (un peu moins efficace mais on pourra améliorer ça par la suite)
Les interfaces sont dans rm.h
J'ai aussi créé la classe PageHeader qui représente le header qu'il y a sur chaque page (à ne pas confondre avec la première page d'un fichier qui est un header tout entière).
La classe PageHeader possède des méthodes qui lui permettent de s'écrire sur un buffer et de lire depuis le buffer.
Tout n'est pas parfait et pas encore testé mais vous pouvez utiliser les interface de ces deux classes dans vos fichiers à vous (je les ai faites parce que j'en avais besoin pour le FileScan d'ailleurs^^)

<h2>Méthodes à implémenter</h2>

Il faut aussi qu'on se mette d'accord sur les methodes qu'on a besoin dans les classes des uns et des autres. Pour le FileScan par exemple j'ai "terminé" l'implémentation grossièrement mais il me manque des fonctions d'autres classes :
- fileHandle->fullRecordSize()
- RID->getPageNumber()
- fileHandle->getNbPages()
- fileHandle->getPageHeader(ph, pHeader);

<h2>Une autre source suffisante pour se référer </h2>

J'ai trouvé un projet avec les docs rendus qui bien explique la structure de cette base et toutes les autre choses. Une bonne source supplémentaire.
https://github.com/yifeih/redbase

J'ai l'impression que le code du prof est très similaire à ce projet (avec des goto). Peut-être qu'il s'en est directement inspiré :p

<!-- Désolé pour le html je ne savais pas comment faire autrement^^ -->
<!-- ah ah, mais ça marche aussi le html. Sinon tu as un guide du markdown ici  : https://help.github.com/articles/github-flavored-markdown/ -->
