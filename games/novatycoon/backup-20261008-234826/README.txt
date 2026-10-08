Nova Tycoon — module original pour NumPlay

Fonctionnalités :
- ville sur grille 5x4
- 6 types de commerces
- achat, amélioration, embauche, marketing et revente
- économie journalière et progression par niveaux
- réputation, clientèle, bénéfices et statistiques
- recherche à 6 branches
- événements économiques aléatoires
- interface graphique 320x240 avec rendu plein écran et buffer 16 bits
- commandes clavier NumWorks : flèches, OK, Back/Home
- architecture sans flottants et avec calculs entiers pour limiter le coût CPU

Le fichier NovaTycoon.c est volontairement autonome vis-à-vis du moteur : il utilise uniquement
les primitives de rendu/input de NumPlay et les fonctions EADK déjà exposées par l'application.

Intégration : compiler ce module en ARM Thumb/Cortex-M4 avec le même environnement que NumPlay,
puis l'enregistrer dans la table des jeux comme un module dont l'entrée est np_tycoon_main.
