# Canonical reconstruction sources

- When implementing or correcting canonical behavior in this repository, derive behavior only from authoritative local artifacts such as available PDB symbols, types, and data or direct reverse engineering of the shipped executable.
- Do not use online references, videos, guides, visual approximation, or inferred look-and-feel as implementation evidence for canonical behavior in this repository.

# Live-review deployment

- Deploy every test-ready build to the game folder by default before marking an item ready for live review.
- If deployment is blocked because the user is actively testing or the executable is locked, report deployment as pending and complete it as soon as the game process exits.
