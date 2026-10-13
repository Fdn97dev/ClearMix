# Algorithme de masquage

## Principe en une phrase
Pour chaque bande de fréquence, si **une autre piste dont le rôle entre en conflit avec le mien est aussi forte (ou plus) que moi**,
je me creuse, **proportionnellement à mon manque de priorité**.

## Calcul pour une bande `b`, ma piste `me`, une autre piste `o`

| Terme | Formule | Sens |
|---|---|---|
| `conflict` | table de règles (0..1) | Ces deux rôles se gênent-ils à cette fréquence ? |
| `myAudibility` | `clamp((mon_niveau + 80) / 20)` | Je suis vraiment présent dans la bande (sinon rien à creuser) |
| `otherAudibility` | idem pour l'autre | L'autre est présent |
| `competition` | `clamp((niveau_autre − mon_niveau + 12) / 12)` | 1 si l'autre est ≥ moi ; 0 s'il est 12 dB plus faible |
| `share` | `prio_autre / (prio_autre + ma_prio)` | Part de l'effort que je prends (0.5 si priorités égales) |

`c = conflict × myAudibility × otherAudibility × competition × share`

Avec plusieurs pistes, on combine par « probabilité cumulée » : `masking = 1 − Π(1 − cᵢ)`.
Borné entre 0 et 1, même avec 40 pistes.

**Coupe finale** : `cut_dB = − MaxCut × Amount × masking`  (≤ 0, jamais plus que Max Cut).

Puis lissage attack/release, puis une cloche (Q = 3) par bande à ce gain.

### Pourquoi c'est bien conçu
- **Pas de boucle** : tout est calculé sur les signaux d'entrée.
- **Dynamique** : le 808 qui joue seul n'est pas touché ; dès que le kick frappe, le 808 se creuse brièvement dans le sub.
- **Équitable et réglable** : priorités égales → chacun cède la moitié. Le slider « Priorité » déplace l'équilibre.
- **Borné** : jamais de coupe délirante.

## Priorité effective
`prio = priorité_de_base_du_rôle + (slider − 0.5)`, bornée dans [0.05, 1].
Slider centré = comportement par défaut du rôle.

## Règles de conflit (extrait de `MaskingEngine.cpp`)

| Rôles | Zone | Poids | Pourquoi |
|---|---|---|---|
| Kick ↔ 808 | 30–90 Hz | 1.0 | Bataille classique du sub |
| 808 ↔ Basse | 30–200 Hz | 1.0 | Deux sources de grave |
| Kick ↔ Basse | 35–120 Hz | 0.9 | idem |
| 808/Basse ↔ Mélodie/Pad/Sample | 80–350 Hz | 0.6–0.7 | La « boue » |
| Voix lead ↔ Mélodie/Pad/Sample | 1.5–5 kHz | 0.9 | Présence / intelligibilité |
| Voix lead ↔ Backs | 300–4000 Hz | 0.6 | Placer la lead devant |
| Voix lead ↔ Hi-Hats | 5–10 kHz | 0.4 | Sibilance |
| Hi-Hats ↔ Percussions | 6–16 kHz | 0.8 | Air et transitoires (shakers, congas…) |
| Tout ↔ Tout | 200–500 Hz | 0.35 | Filet de sécurité bas-médium |

Les bords de zone sont **adoucis** (cosinus sur une demi-octave) : pas de marche d'escalier.

### Ajouter / modifier une règle
Dans `Source/MaskingEngine.cpp`, tableau `kRules` :
```cpp
{ Role::SnareClap, Role::Melody, 1000.f, 3000.f, 0.5f },   // { rôleA, rôleB, Hz min, Hz max, poids 0..1 }
```
Les règles sont **symétriques** (A↔B = B↔A). `Role::Any` est un joker. Relance les tests : `ctest`.

### Ajouter un rôle
1. `Roles.h` : ajoute une valeur dans l'enum **avant `Count`** + une ligne dans la table (même ordre !).
2. `MaskingEngine.cpp` : ajoute les règles qui l'impliquent.
3. Ajoute-le **à la fin** de la liste pour ne pas décaler les projets déjà sauvegardés.

## Analyse spectrale
- FFT 4096 points, Hann, une mesure toutes les 1024 samples (≈ 21 ms à 48 kHz). Résolution ≈ 11,7 Hz → utile dans le sub.
- 24 bandes log de 30 Hz à 16 kHz. Chaque bande = somme des puissances de ses bins (au moins 1 bin).
- Niveau en dB ≈ RMS de la bande. Plancher −100 dB.

## EQ appliqué
- **Auto** : 24 cloches (Q = 3) aux centres des bandes. Les cloches voisines se chevauchent légèrement, donc la coupe réelle
  entre deux bandes est un peu plus profonde que la valeur d'une seule bande (c'est voulu : courbe continue, sans trous).
- **Manuel** : HPF (Butterworth 2e ordre), shelf grave, 3 cloches, shelf aigu.
- Phase minimale, latence 0, double précision. Les coefficients sont recalculés une fois par bloc ; comme les coupes sont lissées,
  il n'y a pas de craquement.

## Réglages conseillés
| Situation | Réglage |
|---|---|
| Mix déjà propre, juste du polish | Amount 30-40 %, Max Cut 3 dB |
| Beat dense trap | Amount 70 %, Max Cut 6 dB (défaut) |
| 808 qui écrase le kick | 808 : Priorité ← ; Kick : Priorité → |
| Voix noyée dans les samples | Voix lead : Priorité ↑ ; samples : Amount plus haut |
| Pompage audible | Release ↑ (300-500 ms), Attack ↑ |
