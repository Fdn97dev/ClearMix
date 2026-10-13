# Architecture

## Vue d'ensemble

Chaque instance du plugin fait **à chaque bloc audio** (toutes les ~5 à 10 ms) :

```
 audio in ──► 1. ANALYSE  (SpectrumAnalyzer : FFT → 24 niveaux en dB)
              2. PUBLIER  (SharedBus.publish : rôle, priorité, 24 niveaux)
              3. DÉCIDER  (SharedBus.snapshotOthers → MaskingEngine → 24 coupes cibles)
              4. LISSER   (attack / release, une fois par bloc)
              5. FILTRER  (24 cloches auto  →  HPF + 5 bandes manuelles)
              6. GAIN     (output, lissé)  ──► audio out
```

Point clé : **l'analyse porte sur le signal d'ENTRÉE** (avant tout filtrage de ClearMix). Ainsi, quand la piste A
creuse la piste B, la mesure de A ne change pas → **aucune boucle de rétroaction**, aucun pompage instable.

## Les fichiers

### `Constants.h`
Constantes et `BandLayout` : 24 bandes réparties de façon logarithmique de 30 Hz à 16 kHz (~0,4 octave par bande).
`centre[i]` = fréquence centrale, `edge[i]` = bornes. Singleton `BandLayout::get()` (construit une seule fois).

### `Roles.h`
`enum class Role` + table `{nom, priorité de base}`. Plus la priorité est haute, moins la piste est creusée
(Voix lead 0.95, Kick 0.70, 808 0.65, … FX 0.20).

### `MaskingEngine.h/.cpp` — le cerveau (C++ standard uniquement)
- `kRules[]` : table de règles « ces deux rôles se gênent entre X et Y Hz avec un poids W ».
- `ConflictTable` : la table est **pré-calculée** (rôle × rôle × bande) pour ne rien calculer de coûteux dans le thread audio.
- `computeTargetCuts(me, others, n, settings, out)` : produit 24 valeurs en dB (≤ 0). Détail dans
  [MASQUAGE_ET_DSP.md](MASQUAGE_ET_DSP.md).
- Aucun accès JUCE, aucune allocation → **testable** (`tests/MaskingEngineTests.cpp`).

### `SharedBus.h/.cpp` — la communication entre instances
Un tableau statique de 64 `SharedSlot`. Chaque instance :
- **réserve** un slot à sa création (`acquireSlot`, compare-and-swap atomique),
- **publie** à chaque bloc (`publish`) : rôle, priorité, groupe, link, 24 niveaux, horodatage,
- **libère** le slot à sa destruction.

Les autres instances **lisent** (`snapshotOthers`) en ignorant : leur propre slot, les autres groupes, les pistes « link »
désactivé, et les pistes muettes depuis plus de 500 ms (bypass, piste supprimée, DAW à l'arrêt).
Tout est en `std::atomic` : **pas de mutex dans le thread audio**.

> Le bus est un *singleton statique* : il est partagé par toutes les instances **chargées dans le même processus**.
> C'est le cas dans FL Studio, Ableton, Reaper, Logic, Studio One par défaut. Voir les limites dans [ROADMAP.md](ROADMAP.md).

### `SpectrumAnalyzer.h/.cpp`
Ring buffer de 4096 samples → toutes les 1024 samples : fenêtre de Hann → `juce::dsp::FFT` → somme des puissances par bande →
dBFS (≈ RMS dans la bande) → lissage dB (attaque 40 ms, relâchement 300 ms). Mixe L+R en mono.

### `Biquad.h`
Filtres du 2e ordre (RBJ Cookbook) : cloche, shelf grave/aigu, passe-haut. `BiquadCascade<N>` les enchaîne avec un état par canal
(forme transposée directe II, **double précision**). Écrit à la main plutôt qu'avec `juce::dsp::IIR` : changer les coefficients
**sans allocation** dans le thread audio. `magnitudeDb()` calcule la réponse en fréquence pour dessiner les courbes.
Phase minimale → **latence 0**.

### `PluginProcessor.h/.cpp`
- `createLayout()` : déclare tous les paramètres (rôle, priorité, amount, max cut, attack, release, output, HPF, 5×{freq, gain, Q}…).
- `processBlock()` : les 6 étapes ci-dessus.
- Communication avec l'UI : uniquement des `std::atomic` (`autoCutDisplay`, `ownLevelDisplay`).

### `PluginEditor.h/.cpp`
- `EqDisplay` : dessine grille, spectres, coupes auto (orange), courbe manuelle, courbe finale, poignées. Timer à 30 Hz.
  Interaction souris → modifie les paramètres via `beginChangeGesture / setValueNotifyingHost / endChangeGesture`
  (indispensable pour que l'automation et l'annulation du DAW fonctionnent).
- `ClearMixAudioProcessorEditor` : en-tête (rôle, groupe, link, auto) + 6 knobs reliés aux paramètres par des `Attachment`.

## Threads

| Thread | Ce qu'il fait | Interdit |
|---|---|---|
| **Audio** (`processBlock`) | DSP, publication, décision | `new`/`malloc`, mutex, I/O, log |
| **Message / UI** | Interface, création/destruction du plugin | Bloquer longtemps |

Échanges audio → UI : atomiques en lecture « relaxed » (une valeur légèrement périmée ne gêne pas l'affichage).
Échanges instance ↔ instance : atomiques aussi.

## Sauvegarde
`getStateInformation` sérialise l'`AudioProcessorValueTreeState` en XML binaire : le DAW stocke l'état dans le projet.
Les IDs de paramètres (`Parameters.h`) ne doivent **jamais** être renommés après distribution.
