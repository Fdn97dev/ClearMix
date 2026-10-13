# Installer et tester dans un DAW

## 1. Compiler en Release
```bash
cmake --preset release
cmake --build --preset release -j
```
Artefacts : `build/release/ClearMix_artefacts/Release/VST3/ClearMix.vst3` (+ `Standalone/`, + `AU/` sur macOS).

## 2. Copier le VST3
| OS | Dossier |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3\` (droits admin) |
| macOS | `~/Library/Audio/Plug-Ins/VST3/` (AU : `~/Library/Audio/Plug-Ins/Components/`) |
| Linux | `~/.vst3/` |

Rescanne les plugins dans ton DAW.
> **FL Studio** : *Options → Manage plugins → Find installed plugins*. **Ableton** : *Preferences → Plug-ins → Rescan*. **Reaper** : *Preferences → Plug-ins → VST → Re-scan*.

## 3. Scénario de test (5 minutes)
1. Crée 3 pistes : un **kick**, un **808**, une **voix** (ou un sample mélodique).
2. Charge ClearMix sur chacune, règle les **rôles**, laisse le même **Groupe**.
3. Lance la lecture en boucle. Chaque instance doit afficher « *2 autre(s) piste(s) liée(s)* » et les spectres des autres pistes (traits colorés).
4. Observe la zone **orange** dans le sub (30-90 Hz) quand kick et 808 jouent ensemble.
5. Mets **Amount à 0** puis à 100 : compare à l'oreille. Bascule **Auto EQ** off/on.
6. Ajoute une coupe manuelle (glisse la poignée 2 vers le bas) : la courbe blanche = auto + manuel.

## 4. Si les pistes ne se « voient » pas
- Toutes les instances sont-elles dans le **même groupe** et **Link** activé ?
- Ton DAW isole-t-il les plugins dans des processus séparés (sandbox) ? Désactive l'option (Reaper : *Run as separate process* ; Bitwig : mode de sandbox « Within plug-in host » ou « Together »). Voir [ROADMAP](ROADMAP.md).
- La lecture tourne-t-elle ? Une piste silencieuse depuis > 0,5 s est ignorée.
- Freeze/bounce hors-ligne : l'ordre de traitement des pistes n'est pas garanti, le résultat peut différer légèrement du temps réel.

## 5. Latence / compensation
ClearMix est **à phase minimale, latence 0** : rien à compenser. Chaque piste est analysée sur son propre signal du même bloc.
