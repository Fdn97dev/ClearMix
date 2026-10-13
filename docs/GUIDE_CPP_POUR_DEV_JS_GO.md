# C++ et audio temps réel, expliqués à un dev JS / Go

## 1. Les 4 différences qui comptent

| Concept | JS / Go | C++ (dans ce projet) |
|---|---|---|
| Mémoire | Garbage collector | Pas de GC : **RAII**. Un objet est détruit à la sortie de sa portée. On utilise `std::unique_ptr`, `std::vector`, `std::array` ; presque jamais `new`/`delete` à la main. |
| Fichiers | `import` / `package` | `.h` (déclaration, « l'interface ») + `.cpp` (implémentation). `#include` colle le contenu du `.h`. `#pragma once` évite les inclusions en double. |
| Références | Objets passés par référence implicitement | `T&` = référence, `const T&` = référence en lecture seule, `T*` = pointeur (peut être nul). Une copie est faite si tu écris `T` tout court. |
| Erreurs | exceptions / `error` | Les erreurs de logique plantent (segfault). On évite les exceptions dans le code audio. |

## 2. Le thread audio — la seule vraie difficulté

Le DAW appelle `processBlock()` **toutes les 2 à 20 ms** avec un bloc de samples. Si on est en retard : **craquements**.
Donc dans `processBlock` :

| ❌ Interdit | Pourquoi | ✅ À la place |
|---|---|---|
| `new`, `std::vector::push_back`, `std::string` qui grandit | L'allocation peut bloquer | Tout pré-allouer dans le constructeur / `prepareToPlay` (`std::array`, buffers fixes) |
| `std::mutex::lock` | Peut attendre l'UI | `std::atomic` |
| Lire un fichier, `printf`, réseau | Bloquant | Rien — ou un FIFO lock-free vers un autre thread |

Analogie Go : `processBlock` est une goroutine temps réel qui n'a droit ni au `mutex.Lock()` ni au `make()`, ni à un channel bloquant.

### `std::atomic` = variable partagée sans mutex
```cpp
std::atomic<float> level { 0.0f };
level.store (0.5f);          // thread A
float v = level.load();      // thread B — jamais de valeur « à moitié écrite »
```
C'est l'équivalent de `sync/atomic` en Go. Tout le `SharedBus` repose dessus.

## 3. Lire le code JUCE

- `juce::String` ≈ `string`. `juce::Array`/`std::vector` ≈ slice. `juce::Slider`, `juce::Component` ≈ composants UI.
- **`AudioProcessor`** = le plugin (audio, paramètres). **`AudioProcessorEditor`** = son interface. Ils sont séparés : l'UI peut être fermée, le plugin continue.
- **`AudioProcessorValueTreeState` (apvts)** = le store de paramètres (comme un Redux). Les `Attachment` (`SliderAttachment`…) relient automatiquement un widget à un paramètre, **dans les deux sens**, et gèrent l'automation du DAW.
- **`juce::Component::paint(Graphics&)`** = méthode de dessin (comme un `canvas.getContext('2d')`). `repaint()` demande un nouveau dessin.
- **`Timer`** : callback périodique sur le thread UI (≈ `setInterval`).

## 4. Petites constructions C++ que tu verras
```cpp
const auto& layout = BandLayout::get();   // 'auto' = type déduit ; 'const&' = pas de copie, lecture seule
for (int b = 0; b < kNumBands; ++b)       // boucle classique
for (auto& a : autoCutDisplay) a.store(0) // for...of, mais avec référence (&) pour modifier l'élément
std::array<float, 24> arr {};             // tableau de taille fixe, initialisé à 0 — pas d'allocation
static const Foo& get() { static const Foo f; return f; }   // singleton : construit au 1er appel
[](float x) { return x * 2; }             // lambda = fonction anonyme
template <int N> class BiquadCascade      // générique (comme les generics Go) : N connu à la compilation
```

## 5. Déboguer
- **Build Debug** + `F5` sur le Standalone : points d'arrêt, variables. Ne mets **pas** de point d'arrêt dans `processBlock` du Standalone sans mettre l'audio en pause : il va hoqueter.
- `DBG ("texte" << valeur);` affiche dans la console (version Debug uniquement). À ne pas laisser dans `processBlock`.
- Crash au lancement : vérifie que les `std::unique_ptr<Attachment>` sont déclarés **après** les widgets (ils sont détruits en ordre inverse).

## 6. Parcours d'apprentissage à partir de ce projet
1. Lis `Constants.h` → `Roles.h` → `MaskingEngine.cpp` (C++ « simple », pas de JUCE).
2. Modifie une règle dans `kRules`, relance `ctest`, écris un test.
3. Lis `SharedBus.cpp` : c'est le plus bel exemple de code lock-free du projet.
4. Lis `Biquad.h`, puis `PluginProcessor.cpp::processBlock`.
5. Enfin `PluginEditor.cpp`.
Ressources : tutoriels JUCE (juce.com/learn/tutorials), chaîne YouTube *The Audio Programmer*, « Audio EQ Cookbook » (RBJ) pour les filtres.
