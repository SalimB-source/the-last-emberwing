# Content

Les assets definitifs d'Unreal seront ranges ici, selon la structure decrite dans
`../../UNREAL_GAME_CONCEPT.md`.

Le prototype ne contient volontairement **aucun `.uasset`** : l'arene, le personnage et les
ennemis sont montes a l'execution a partir des primitives du moteur
(`/Engine/BasicShapes/Cube|Sphere|Cylinder|Cone|Plane|Torus`). Le depot reste donc lisible
en Git, et les placeholders se remplacent un par un sans retoucher le gameplay.

## Ce qui doit apparaitre ici

| Chemin | Role | Remplace |
|---|---|---|
| `Maps/Prototype.umap` | niveau de test ; il n'apporte qu'un brush BSP et des lumieres que le C++ neutralise | — |
| `Emberwing/Materials/M_BasicColor` | materiau a 2 parametres (`Color`, `Emissive`) qui donne leurs couleurs aux primitives, sans lui tout est gris moteur | rien, il est optionnel |
| `Emberwing/Characters/M_EMBERWING_Mantis` (plus tard) | mesh squelettique + animations du mante | `UEmberwingMantisRig` |
| `Emberwing/UI/` (plus tard) | HUD de vie, menu pause, joystick maison | joystick virtuel du moteur |

La recette de `M_BasicColor` est dans le `README.md` a la racine (paragraphe « Couleurs »).
Tant que le materiau n'existe pas, le code cree un `MID` sur `BasicShapeMaterial` et
l'avertit a l'ecran : c'est un etat normal, pas une erreur.

## Contenu genere a l'execution (ne pas creer « pour aider »)

- l'arene complete (sol, plateformes, piliers, lanternes, autel) : `AEmberwingPrototypeWorld`
- les lumieres, le brouillard et le post-process lisibles : `AEmberwingLightingRig`

Ces acteurs sont crees par le GameMode a chaque `Play` : les poser a la main dans le niveau
produirait des doublons (le rig est idempotent par type, mais pas les lumieres que tu ajouterais).
