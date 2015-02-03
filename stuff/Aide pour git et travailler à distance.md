## Utiliser ce dépôt

Pour copier le dépôt :
``` bash
$ git clone https://github.com/piotr2b/tinybase.git tinybase
```
Il n'y a rien besoin de faire de plus, le dépôt est prêt à être utilisé.

Pour mettre à jour à la main le dépôt :
 * `git status` pour voir les fichiers qui ont été modifiés ;
 * `git add .` pour donner tous les fichiers modifiés ou `git add src/rm.h src/bla` pour choisir les fichiers à donner ;
 * `git commit -m "message du commit"` pour sauvegarder les changements dans un « commit ». Les changements ne sont pas encore donnés au dépôt.
 * `git pull` pour vérifier que personne n'a modifié les mêmes fichiers entre temps. Si c'est le cas, git essaiera automatiquement de faire une synthèse des deux versions.
 * `git  push` enfin pour publier les changements.

## Travailler à distance sur des machines de l'école

Mais j'ai l'impression que git ne copie pas le dossier build qu'il faut donc récupérer à la main. Pour copier tout un répertoire d'un compte de l'école vers un ordi :
``` bash
$ scp -r username@ssh.enst.fr:/cal/homes/username/tinybase tinybase
```
je crois qu'il est mal vu de trop faire travailler la passerelle ssh.enst.fr donc si vous avez besoin de réveiller une machine de l'école pour travailler dessus et compiler par exemple, lancez les commandes suivantes depuis la passerelle.

`wakeonlan` est un script que j'ai mis dans ce dépôt. C'est donc ce fichier qu'il faut exécuter. Détail : une machine  un peu de temps à s'allumer, il faut donc attendre une minute ou deux avant de pouvoir effectivement s'y connecter.

* Pour la machine c130-24 : `./wakeonlan -i 137.194.35.24 f8:bc:12:a6:50:a8`
* Pour la machine c130-38 : `./wakeonlan -i 137.194.35.38 f8:bc:12:a6:50:ae`

Monter le répertoire de l'école permet d'éditer les fichiers sur son ordis et de les compiler facilement sur une machine de l'école (les # indiquent une commande à lancer en administrateur et les $ une commande à lancer en simple utilisateur).
``` bash
# mkdir -p /tmp/tmpfusefs/username/
# sshfs username@ssh.enst.fr: /tmp/tmpfusefs/username/
$ ls -hal /tmp/tmpfusefs/username/
```
et pour le démonter :
``` bash
# umount /tmp/tmpfusefs/username/
# rmdir /tmp/tmpfusefs/username/ # (devrait être vide)
```
