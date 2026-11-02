# Live Love Laugh

Een dromerige stereo delay-pedaal als VST3-plugin, voor synths en vocals.

| Knop | Wat het doet |
|---|---|
| **live** | Delay. Hoe hoger, hoe meer pingpong en hoe breder het stereobeeld. Bovenin komen er chorus en een shimmer (octaaf omhoog) in de staart bij. |
| **love** | Granulaire reverse-octaaf delay, geïnspireerd op de Crystallizer. Een deel gaat door de live-delay en versterkt zo de shimmer. |
| **laugh** | Feedback voor live en love, met een ingebouwde zachte limiter. |
| **lo · hi** | DJ-filter in de feedback: naar links worden de herhalingen donkerder, naar rechts dunner. Klikt vast in het midden; dubbelklik zet hem terug. |
| **time** | Delaytijd in nootwaarden, gesynct met het tempo van je DAW (1/32 tot 4 maten). |
| **duck** | Het effect wordt zachter zolang je droge signaal speelt. |
| **kill dry** | Zet het droge signaal uit (100% wet, handig op een send). |
| **voetschakelaar** | Klik op het zwarte vlak om de pedaal aan of uit te zetten. Bij uit sterven de herhalingen natuurlijk uit. |

## Installeren (Windows / FL Studio)

1. Open op GitHub het tabblad **Actions**, klik op de laatste groene run en download **LiveLoveLaugh-VST3-Windows**.
2. Pak de zip uit en kopieer de map `Live Love Laugh.vst3` naar `C:\Program Files\Common Files\VST3\`.
3. Ga in FL Studio naar **Options → Manage plugins → Find more plugins**. Daarna vind je de plugin onder Effects.

## Bouwen

Dit gebeurt automatisch via GitHub Actions (`.github/workflows/build.yml`). Handmatig gaat het zo:

```
cmake -S . -B build -A x64
cmake --build build --config Release --target LiveLoveLaugh_VST3
```
