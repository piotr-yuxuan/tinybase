tinybase
========

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

<!-- Désolé pour le html je ne savais pas comment faire autrement^^ -->
