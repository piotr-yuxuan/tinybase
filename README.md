tinybase
========

Pour copier le dépôt :
``` bash
$ git clone https://github.com/piotr2b/tinybase.git tinybase
```
Mais j'ai l'impression que git ne copie pas le dossier build qu'il faut donc récupérer à la main. Pour copier tout un répertoire d'un compte de l'école vers un ordi :
``` bash
$ scp -r username@ssh.enst.fr:/cal/homes/username/tinybase tinybase
```

Si vous avez besoin de réveiller une machine de l'école pour travailler dessus (je crois qu'il est mal vu de trop faire travailler la passerelle ssh.enst.fr).
``` bash
$ ./wakeonlan -i 137.194.35.38 f8:bc:12:a6:50:ae
Sending magic packet to 137.194.35.38:9 with f8:bc:12:a6:50:ae
$ ssh c130-38
```

Classe			|	Codeur  | Etat
:---------------|:------------:|:--------
RM_Manager		|	Pauline |
RM_FileHandle	|	Pierre  | Ca ne compile pas sur mon pc, je ne peux pas travailler ><
RM_FileScan		|	Camille |
RM_Record		|	Yixin |
RID				|	Pauline |
RM_PrintError	|	  |

