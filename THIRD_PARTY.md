# Composants tiers et licences

ClearMix est publié sous **GNU AGPL-3.0-or-later** (voir `LICENSE`).

| Composant | Rôle | Licence |
|---|---|---|
| [JUCE 8](https://juce.com) | Framework audio / interface, téléchargé au build par CMake | AGPL-3.0 (ou licence commerciale JUCE) |
| VST3 SDK (inclus dans JUCE) | Format de plugin VST3 | GPL-3.0 / licence propriétaire Steinberg |
| *Audio EQ Cookbook* de Robert Bristow-Johnson | Formules des filtres biquad (`Biquad.h`) | Formules mathématiques, citées pour référence |

## Conséquences pratiques
- Tu peux utiliser, étudier, modifier et redistribuer ClearMix librement, **à condition** que toute version distribuée
  (ou proposée en service réseau) reste sous AGPL-3.0 avec son code source disponible.
- Si tu veux faire une version **fermée / commerciale**, il te faudra une licence commerciale JUCE (et respecter la licence du VST3 SDK).
- Les contributeurs acceptent que leurs contributions soient publiées sous la même licence.

## Marques
« VST » est une marque de Steinberg Media Technologies GmbH. « Audio Units » est une marque d'Apple Inc.
« Logic Pro », « FL Studio », « Ableton Live » et « Reaper » appartiennent à leurs propriétaires respectifs ; ClearMix n'est affilié à aucun d'eux.
