# ClearMix — EQ anti-conflit de fréquences pour la production moderne

[![Build](https://github.com/VOTRE-PSEUDO/ClearMix/actions/workflows/build.yml/badge.svg)](https://github.com/Fdn97dev/ClearMix/actions)
[![Licence: AGPL v3](https://img.shields.io/badge/licence-AGPL--3.0-blue.svg)](LICENSE)

> Projet **open source** (AGPL-3.0). Statut : **v0.1 alpha** — fonctionne, mais encore peu testé dans les DAW. Les retours sont les bienvenus.

Plugin audio (VST3 / AU / Standalone) pour beatmakers : tu charges **ClearMix sur chaque piste**,
tu choisis le **rôle** de la piste (Kick, 808, Voix lead, Hi-Hats…), et les instances **se parlent entre elles**
pour creuser automatiquement les fréquences en conflit. Par-dessus, tu gardes un **EQ manuel complet**.

Orienté trap / afro / RnB / drill : règles Kick vs 808, boue 100-400 Hz, présence voix 1.5-5 kHz, air des hats et percussions.

```
 Piste 808  ──[ClearMix: rôle 808]──┐
 Piste Kick ──[ClearMix: rôle Kick]─┼──►  mémoire partagée (SharedBus)  ──► chaque instance calcule
 Piste Voix ──[ClearMix: rôle Voix]─┘        rôle + priorité + spectre        ses propres coupes
```

## Démarrage rapide (VS Code)

1. Installe les prérequis (voir plus bas) puis ouvre le dossier `ClearMix` dans VS Code.
2. Accepte d'installer les extensions recommandées (**C/C++**, **CMake Tools**).
3. Palette de commandes (`Ctrl+Shift+P`) → **CMake: Select Configure Preset** → `debug`.
4. **CMake: Build** (ou `F7`). Le premier build télécharge JUCE (~1 min) et compile (~5-10 min).
5. Lance le **Standalone** : `build/debug/ClearMix_artefacts/Debug/Standalone/ClearMix`
   (ou `F5` avec la config « Debug ClearMix Standalone »).
6. Le **VST3** est dans `build/debug/ClearMix_artefacts/Debug/VST3/ClearMix.vst3` : copie-le dans le dossier VST3 de ton DAW
   (voir [docs/TESTER_DANS_UN_DAW.md](docs/TESTER_DANS_UN_DAW.md)).

En ligne de commande :
```bash
cmake --preset release
cmake --build --preset release -j
ctest --test-dir build/release --output-on-failure   # tests du moteur de masquage
```

**Test de bout en bout** (deux/trois vraies instances du plugin qui s'échangent leurs spectres, sans DAW) :
```bash
cmake --preset release -DCLEARMIX_BUILD_SMOKE_TEST=ON
cmake --build --preset release --target ClearMixSmokeTest
./build/release/ClearMixSmokeTest_artefacts/Release/ClearMixSmokeTest      # doit afficher « Smoke test OK »
```
Résultat attendu : kick et 808 jouant la même note de sub se creusent mutuellement (~ −3 dB sur la bande),
une voix seule à 2 kHz n'est pas touchée.

### Prérequis
| OS | À installer |
|---|---|
| **Windows** | Visual Studio 2022 Build Tools (charge de travail « Développement Desktop en C++ »), CMake ≥ 3.22, Git |
| **macOS** | Xcode (ou Command Line Tools), `brew install cmake` |
| **Linux** | `sudo apt install build-essential cmake ninja-build git libasound2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libgl1-mesa-dev libcurl4-openssl-dev` |

> Hors-ligne ? Clone JUCE toi-même et configure avec `-DCLEARMIX_JUCE_DIR=/chemin/vers/JUCE`.

## Utilisation

1. Sur **chaque piste** du mix, charge ClearMix (en dernier ou en premier de la chaîne, peu importe).
2. Choisis le **Rôle** (menu en haut). C'est la seule chose indispensable.
3. Lance la lecture : l'écran affiche le spectre de la piste (turquoise), celui des autres pistes (traits colorés)
   et les **coupes automatiques** (zone orange).
4. Ajuste :
   - **Priorité** : au centre = priorité normale du rôle. Vers la droite = cette piste est protégée (les autres se creusent plus).
   - **Amount** : intensité globale de l'EQ auto. **Max Cut** : coupe maximale par bande (dB).
   - **Attack / Release** : vitesse à laquelle les coupes arrivent / s'en vont.
   - **Groupe** : si tu as plusieurs mix ouverts, mets-les dans des groupes différents.
   - **Auto EQ** coupe seulement l'automatique. **Link** déconnecte complètement la piste du réseau.
5. EQ manuel (appliqué après l'auto) : **glisse** les poignées 1-5 (fréquence/gain), **molette** = Q,
   **double-clic** = reset. Poignée **HP** (en haut) = filtre passe-haut.

## Arborescence

```
ClearMix/
├─ CMakeLists.txt          build (JUCE téléchargé automatiquement)
├─ CMakePresets.json       presets debug / release
├─ .vscode/                tâches, debug, extensions recommandées
├─ Source/
│  ├─ Constants.h          constantes + répartition des 24 bandes
│  ├─ Roles.h              rôles et priorités de base
│  ├─ MaskingEngine.*      règles de conflit + calcul des coupes (C++ pur, testé)
│  ├─ SharedBus.*          mémoire partagée entre instances (atomiques, sans mutex)
│  ├─ SpectrumAnalyzer.*   FFT 4096 -> énergie en 24 bandes
│  ├─ Biquad.h             filtres RBJ (phase minimale, latence nulle)
│  ├─ Parameters.h         IDs des paramètres
│  ├─ PluginProcessor.*    thread audio : analyse → publication → coupes → filtrage
│  └─ PluginEditor.*       interface (courbes, poignées, knobs)
├─ tests/                  tests unitaires (sans JUCE)
└─ docs/                   documentation détaillée
```

## Documentation
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — comment le plugin fonctionne, fichier par fichier, avec le chemin d'un bloc audio.
- [docs/MASQUAGE_ET_DSP.md](docs/MASQUAGE_ET_DSP.md) — l'algorithme de masquage, les règles trap/afro/RnB, comment les modifier.
- [docs/GUIDE_CPP_POUR_DEV_JS_GO.md](docs/GUIDE_CPP_POUR_DEV_JS_GO.md) — les concepts C++/audio temps réel expliqués à partir de JS/Go.
- [docs/TESTER_DANS_UN_DAW.md](docs/TESTER_DANS_UN_DAW.md) — installation et tests dans FL Studio, Ableton, Reaper…
- [docs/ROADMAP.md](docs/ROADMAP.md) — limites connues et suite du projet.

## Contribuer
Voir [CONTRIBUTING.md](CONTRIBUTING.md). Idées bienvenues : règles pour d'autres genres (drill, amapiano, dancehall…), presets, tests dans différents DAW.

## Licence
**GNU AGPL-3.0-or-later** — voir [LICENSE](LICENSE) et [THIRD_PARTY.md](THIRD_PARTY.md) (JUCE est lui aussi sous AGPL-3.0 ; une version fermée/commerciale nécessiterait une licence JUCE).
« VST » est une marque de Steinberg ; ClearMix n'est affilié à aucun éditeur de DAW.
