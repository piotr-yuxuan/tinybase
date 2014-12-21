tinybase
========

**Update 21/12**

Le rendu est dans deux jours, donc il va falloir commencer à faire la documentation. J'ai créé un fichier **rm_DOC** dans src dans lequelle j'ai déjà écrit un premier documentation pour les classes que j'ai faites, ce qui serait bien c'est que chacun écrive dedans les documentation pour ses classes en expliquant bien.

Pour ce qui est des tests, après pas mal de debug **les 7 tests donnés dans le fichier rm_test.cc envoyé par le prof passent sans problème**. Donc normalement c'est bon, étant donné qu'on sera évalué sur ça! Cela dit si vous lancez tous les tests à la suite vous remqrquerez que ça ne passe pas, pas d'inquiétude je crois que c'est dû au fait que dans les tests 5 et 6 ils ont oublié d'appeler **DestroyFile** et donc ça crée une erreur UNIX car le fichier existe déjà... J'ai envoyé un mail au prof pour en avoir le coeur net^^

Troisième et dernière chose, étant donné que pour certaines classes ça ressemble fortement au truc de Stanford, ce serait bien d'**ajouter des commentaires** et de **changer un peu la manière dont le code est structuré** (pas ce qu'il fait^^, juste un peu la forme). J'ai commencé à faire un peu ça dans le **RM_FileHandle** notamment en changeant un peu la forme des tests pour les Return Code qui étaient strcitement les mêmes que Stanford. Les commentaires aussi, ça n'est pas très malin de laisser les même commentaires, d'autant plus qu'écrire les comments soi-même permet en plus de bien comprendre comment le truc marche...

**Voilà. Pour résumer, les deux objectifs d'ici la deadline : Faire la doc et commenter/changer le code.**


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
