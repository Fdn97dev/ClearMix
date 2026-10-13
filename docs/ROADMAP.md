# Limites connues et feuille de route

## Limites actuelles (v0.1)
1. **Partage en mémoire statique** : fonctionne quand toutes les instances sont dans le même processus. Ne fonctionne pas si le DAW sandboxe chaque plugin dans un processus séparé (Bitwig par défaut, Reaper en option, certains modes d'Ableton/Logic). → voir étape « mémoire partagée inter-processus ».
2. **Pas de communication entre formats** : une instance VST3 ne voit pas une instance AU (binaires séparés).
3. **Mono pour l'analyse** : (L+R)/2. Un contenu purement latéral hors phase est sous-estimé.
4. **Coefficients mis à jour une fois par bloc** : correct jusqu'à ~1024 samples/bloc. Au-delà (blocs de 2048+), les coupes peuvent « marcher ». Solution : sous-diviser le bloc (voir étape 3).
5. **Pas d'auto-détection du rôle** : à régler à la main.
6. **Pas de presets** ni de sidechain externe.
7. Réglages Q des filtres auto fixes (Q = 3).
8. Vérifié par compilation, tests unitaires et test de bout en bout automatisé ; **jamais écouté dans un DAW** et l'interface n'a pas été inspectée visuellement : prévois une phase de tests d'écoute et de réglage fin des valeurs par défaut.

## Retours des premiers tests (Logic Pro, 13/10/2026)
- **Presets** : le menu de presets du DAW est vide (le plugin n'en déclare pas). À faire : presets d'usine (Trap, Afro, RnB, Drill) via les « programmes » du plugin.
- **Comparaison des conflits** : pas de vue pour voir *quelles pistes* se battent *dans quelles bandes*. À faire : vue « Conflits » (carte de chaleur piste × bande) et affichage des noms de pistes.
- Interface : accents manquants, pas d'unités sur les boutons, échelle ±12 dB alors que les poignées vont à ±18.

## Étapes suivantes (par ordre de valeur)
1. **Presets de rôle/genre** (Trap, Afro, RnB, Drill) : valeurs d'Amount/Max Cut/Attack/Release et jeux de règles alternatifs.
2. **Sous-division des blocs** (blocs de 64-128 samples) pour interpoler les coefficients.
3. **Mémoire partagée inter-processus** (`juce::InterProcessLock` + `boost::interprocess` ou `shm_open`/`CreateFileMapping`) en repli quand le bus local est vide.
4. **Auto-détection du rôle** : features simples (centroïde spectral, énergie sub, crest factor) → classifieur léger.
5. **Mode stéréo M/S** : creuser seulement le Mid, ou seulement le Side.
6. **Sidechain fréquentiel kick → 808** à l'échelle temporelle des transitoires (look-ahead optionnel).
7. **Mode phase linéaire** optionnel (STFT / FIR) pour le mastering.
8. **Affichage « conflits »** : carte de chaleur montrant quelles pistes se battent dans quelles bandes.
9. **Interface** : redimensionnement par ratio, thèmes, tooltips sur chaque contrôle.
10. **Distribution** : signature de code (Windows/macOS), notarisation Apple, installeur, pluginval pour la validation.

## Valider la qualité du plugin
- **pluginval** (Tracktion) : outil gratuit qui stresse un plugin (niveau 5+ conseillé).
- `ctest` pour la logique de masquage.
- Tests d'écoute A/B : bypass vs auto, sur des beats trap/afro/RnB variés.
