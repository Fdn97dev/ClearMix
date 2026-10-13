# Contribuer à ClearMix

Merci de ton intérêt ! Toutes les contributions sont bienvenues : bugs, idées, règles de conflit pour d'autres genres
(drill, amapiano, dancehall…), presets, traductions, tests dans différents DAW.

## Signaler un bug
Ouvre une *issue* avec : ton OS, ton DAW (+ version), le format (VST3 / AU), les étapes pour reproduire, et si possible une capture.

## Proposer du code
1. Fork + branche : `git checkout -b ma-fonctionnalite`
2. Compile et lance les tests :
   ```bash
   cmake --preset debug && cmake --build --preset debug -j
   ctest --test-dir build/debug --output-on-failure
   ```
3. Respecte les **règles du thread audio** (`processBlock` : pas d'allocation, pas de mutex, pas d'I/O) — voir `docs/GUIDE_CPP_POUR_DEV_JS_GO.md`.
4. Ajoute un test dans `tests/` si tu touches à `MaskingEngine` ou `SharedBus`.
5. Ouvre une *pull request* qui explique **pourquoi** (et, pour un changement de son, avec quel signal tu as testé).

## Ajouter une règle de conflit pour un genre musical
Tout se passe dans le tableau `kRules` de `Source/MaskingEngine.cpp` (voir `docs/MASQUAGE_ET_DSP.md`).
Une règle = deux rôles + une plage de fréquences + un poids. Joins à ta PR un petit test.

## Compatibilité des projets
Ne **renomme jamais** un ID de paramètre (`Source/Parameters.h`) et n'insère pas un rôle au milieu de `enum class Role` :
cela casserait les projets déjà sauvegardés. Ajoute à la fin.

## Licence
En contribuant, tu acceptes que ta contribution soit publiée sous **AGPL-3.0-or-later**. Chaque fichier source commence par
`// SPDX-License-Identifier: AGPL-3.0-or-later`.
