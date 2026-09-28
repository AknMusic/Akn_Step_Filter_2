# Build avec GitHub Actions

1. Créer un repository GitHub vide.
2. Envoyer **tout le contenu** de ce dossier à la racine du repository, y compris `.github/`.
3. Ouvrir l'onglet **Actions** du repository.
4. Lancer `Build Windows VST3` puis `Build macOS Universal VST3` avec **Run workflow**.
5. À la fin du run, télécharger l'Artifact correspondant.

Résultats attendus:

- `AKN-Step-Filter-Windows-x64.zip`
- `AKN-Step-Filter-macOS-Universal.zip`

Le workflow macOS vérifie que le binaire contient bien `arm64` **et** `x86_64` avec `lipo -archs`, puis applique une signature ad-hoc avant de zipper le bundle.

Pour une distribution publique sans avertissement Gatekeeper, remplace ensuite la signature ad-hoc par une signature Developer ID et une notarisation Apple.
