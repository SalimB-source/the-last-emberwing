# The Last Emberwing — projet de demarrage Unreal Engine

Ce dossier contient la premiere base jouable du concept decrit dans
[`../../UNREAL_GAME_CONCEPT.md`](../../UNREAL_GAME_CONCEPT.md).

## Etat du prototype

Le projet est volontairement **sans assets binaires** : tout le visuel est genere au
runtime a partir des primitives du moteur (`/Engine/BasicShapes`), ce qui le rend
versionnable dans Git et remplace proprement par de l'art definitive plus tard.

- Descripteur de projet UE5 (Windows + Android)
- Module C++ de gameplay, classes exposees aux Blueprints
- **Mante jouable, modele en C++** : `UEmberwingMantisRig` monte un corps d'insecte
  complet (tete, thorax, abdomen en 3 segments, 6 pattes tripodales, 2 pattes
  raptoriales, 2 ailes, yeux et coeur d'ember emissifs) a partir de cubes, spheres,
  cylindres, cones et tores. Pas de cube geant : la silhouette est lisible et animee.
- Course, saut, regard, attaque luminescente, plane (les ailes s'ouvrent en chute)
- Ennemis « Drowned Strider » partagent le meme rig (autre palette, proportions plus
  hautes) avec poursuite, degats de contact et reaction aux degats
- Arene generee au runtime : sol de securite, route ascendante a 5 plateformes,
  9 piliers-racines, 5 lanternes d'ember, autel final
- **Rig de lumiere autonome** (`AEmberwingLightingRig`) — voir plus bas
- Configuration rendu pensee pour mobile + joystick virtuel

## Pourquoi un ecran noir, et ce que fait le rig de lumiere

L'`Prototype.umap` herite du template ne contient qu'une `DirectionalLight` **en mobilite
Statique** : sans « Build Lighting » (et Lumen est active), une lumiere Statique n'eclaire
RIEN. Le brouillard volumetrique et l'auto-exposition y allaient aussi d'un projet precedent
a l'autre. Le projet ne depend donc plus du niveau pour la lumiere :

`AEmberwingLightingRig::EnsureLightingRig()` est appele par le GameMode dans `InitGame`
(et re-appelle par l'arene au cas ou). Il est idempotent : une seule instance par monde.

1. **Detruit** les `ADirectionalLight`, `ASkyLight`, `AExponentialHeightFog` et
   `APostProcessVolume` poses dans le niveau (les siennes sont protegees), puis re-passe
   toutes les 6 s au cas ou le niveau en regenererait.
2. **Installe 3 directionnelles Movable** : une lune froide qui porte les ombres,
   un contre-jour bleute sans ombres, un renvoi doux du sol ; plus un halo tiede
   au-dessus du debut de la route, pour que le point de depart reste visible meme
   sans aucune autre source.
3. **Regenere** un `SkyAtmosphere` + un brouillard de hauteur non volumetrique
   (`FogDensity = 0.00035`, `FogMaxOpacity = 0.72`) : de la brume, mais qui ne tue pas
   la lisibilite a 300 units de la camera.
4. **Exposition figee** dans un `APostProcessVolume` non lie : min = max =
   `ExposureBrightness`, donc aucune auto-exposition ne peut plonger l'ecran dans le noir
   pendant les premieres frames.
5. **Affiche des diagnostics** a l'ecran pendant 8 s (`[EMBERWING] ...`) :
   la position du pawn, le nombre d'acteurs d'arene, la mobilite de la lune, le materiau utilise. C'est le premier reflexe a avoir.

Tous les reglages sont des `UPROPERTY EditAnywhere` : selectionne l'acteur
**LightingRig** dans l'outliner pendant le PIE et ajuste `MoonIntensity`,
`FillIntensity`, `ExposureBrightness`, `FogDensity`, `bPrintDiagnostics` a la volee.
Si l'image est trop laiteuse, baisse `ExposureBrightness` vers `0.35` ; si elle est
encore sombre, monte `MoonIntensity` (42 → 90) et `FillIntensity` (6.5 → 14).

## Couleurs : creer `M_BasicColor` (facultatif, 2 minutes)

Sans materiau a soi, les primitives utilisent `BasicShapeMaterial` du moteur : les formes
sont correctes mais **gris neutre** (le parametre `Color` n'existe que sur certains
materiaux, et `BasicShapeMaterial` n'a ni emissif ni couleur). Le code essaie dans l'ordre :

1. `/Game/Emberwing/Materials/M_BasicColor`
2. `MID` de `/Engine/BasicShapes/BasicShapeMaterial` (parametres `Color` → `BaseColor` → `Tint`)
3. `MID` de `WorldGridMaterial` en dernier recours

Le diagnostic a l'ecran indique lequel a ete pris. Pour obtenir les vraies couleurs
(carapace verte, yeux a l'orange emissif) :

1. Content Browser → dossier `Emberwing/Materials` → **Material** → nom `M_BasicColor`.
2. Dans l'editeur de materiau, cree deux **Vector Parameter** : `Color` (Default Value
   blanche) et `Emissive` (Default Value noire).
3. `Color` → **Base Color**.
4. `Color` × `Emissive` (noeud *Multiply*) → **Emissive Color**.
5. Metallic `0`, Roughness `0.55`, **Domain : Surface**, puis Save + Apply.

Rien d'autre a brancher : `FEmberwingAssets::ApplyTint()` detecte les parametres presents
(`Color`/`BaseColor`/`Tint`, `Emissive*`, `Roughness`) et ne cree un `MID` que pour ceux
qui existent. Les materiaux de type `Domain : Deferred Decal`, lumieres, etc. ne sont pas
touches.

## Configuration requise

- Unreal Engine 5.x : le `.uproject` vise **5.8** (`EngineAssociation`). Le projet n'a **pas
  pu etre compile** dans le bac a sable qui a ecrit ce code (pas de moteur installe) — si tu
  es sur une autre version 5.x, attends-toi a devoir ajuster un include, pas la logique.
- Visual Studio 2022 avec la charge de travail **Developpement de jeux en C++**
- Android Studio, SDK/NDK et OpenJDK pour un packaging Android

Le bac a sable qui a prepare ce depot n'embarque **pas** le moteur : le projet n'a donc pas
pu etre compile ici. Il doit etre genere puis compile sur une machine qui a UE5 installe.

## Ouvrir le projet

1. Installe la version d'Unreal Engine requise.
2. Clic droit sur `Emberwing.uproject` → **Generate Visual Studio project files**.
3. Ouvre `Emberwing.slnx` (ou `.sln`) et compile la cible **`EmberwingEditor Win64 Development`**.
4. Ouvre `Emberwing.uproject` avec l'editeur. S'il demande de compiler/recharger le module
   `Emberwing`, reponds **Yes**.
5. Le niveau ouvert doit etre `Content/Maps/Prototype`. Sinon : **File → Open Level → Prototype**.
6. Appuie sur **Play**.

Au chargement, la console doit afficher `[Emberwing] module de gameplay charge` : sans
cette ligne, le C++ n'a pas ete compile et l'editeur fait tourner des classes absentes — c'est l'ecran noir garanti.

## Depannage : « j'ai toujours un ecran noir »

Dans l'ordre, chaque etape prend moins d'une minute :

1. **Supprime `Binaries/` et `Intermediate/`** a la racine du projet, puis relance
   *Generate Project Files* et recompile `EmberwingEditor Win64 Development`. Les vieilles
   DLL d'un module renomme sont la cause n°1 d'un module qui ne se charge pas.
2. **Lis les lignes `[EMBERWING]`** a l'ecran (8 s) : mobilite de la lune, nombre de
   SkyLight restants (0 = propre), acteurs d'arene, position du pawn, materiau resolu.
   L'Output Log (filtre `Emberwing`) ajoute le detail des lumieres neutralisees dans le niveau.
3. **`Window → Developer Tools → Output Log`** : cherche `error C` / `unresolved external`.
   Une erreur de compilation = module absent = ecran noir. Copie l'erreur exacte avant de
   modifier quoi que ce soit : les messages UBT/MSVC disent presque toujours la ligne.
4. **Chemin de rendu** : le projet force `r.Mobile.EnableMovableSpotlights=1` et
   `r.Mobile.EnableMovableLightFX=1` dans `Config/DefaultEngine.ini` (sinon Android ignore
   les lumieres locales). Pour un test sur PC, verifie dans *Project Settings → Rendering →
   Shading Path* que **Default Rendering Path = Deferred** : en Mobile/Forward+, les
   ombres des lumieres Movable sont ignorees et la scene parait plat.
5. **`Build → Build Lighting` n'est pas necessaire** (tout est Movable, Lumen s'en charge).
   En lancer un n'apporte rien et fait patienter gratuitement.
6. **Pawn et camera** : le diagnostic affiche `pawn: X=... Y=... Z=...`. S'il affiche
   `pawn: aucun`, le GameMode n'a pas pu posseder le personnage (regarde alors si
   `DefaultPawnClass` n'est pas ecrase par les World Settings du niveau). S'il affiche une
   position correcte mais que l'ecran est noir, la camera n'est pas sur le pawn : le GameMode
   re-force le view target pendant les 5 premieres secondes. Enfin, si tu tombes dans le
   vide, le garde-fou `FallResetZ` doit afficher
   « Emberwing - chute dans le vide, retour au dernier appui » : si tu le vois en boucle,
   la geometrie de l'arene n'a pas ete construite (vois le point 7).
7. **Le niveau ne doit pas etre vide par hasard** : `Prototype.umap` contient un brush BSP
   et des lumieres, mais l'arene jouable est generee par le C++ au `Play`. Si tu joues
   depuis un autre niveau, le meme GameMode le construira quand meme — verifie juste que
   *Project Settings → Maps → GameDefaultMap* pointe sur `/Game/Maps/Prototype`.

## Controles

| Action | Clavier / souris | Gamepad / tactile |
|---|---|---|
| Deplacement | W A S D | Stick gauche / joystick virtuel |
| Camera | Souris | Stick droit |
| Saut | Espace | Bouton bas |
| Attaque | Clic gauche | Gachette droite |
| Plane | Shift gauche | Bouton gauche |

Le HUD mobile definitif remplacera le joystick virtuel du moteur a la passe UI.

## Prochaine passe

1. Remplacer `UEmberwingMantisRig` par le vrai mesh squelettique et ses animations :
   le rig est une **seule composante**, donc un seul fichier a toucher.
2. UI de vie, menu pause, checkpoint d'autel, sauvegarde.
3. Puzzle de clair de lune et boss Water Strider.
4. Niveau « Drowned Root » en contenu autorise (plus de geometrie runtime).
5. Materiaux stylises, vegetations, Niagara, son, cine.
6. Profilage sur vrais appareils Android avant d'augmenter la complexite visuelle.
