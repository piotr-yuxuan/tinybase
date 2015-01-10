tinybase
========

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