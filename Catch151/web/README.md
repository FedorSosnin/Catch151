# Catch 151 for Knomi: the web installer

`web/knomi/` is the Knomi's web installer: a page that flashes the game from Chrome or Edge with
[ESP Web Tools](https://esphome.github.io/esp-web-tools/), with no Arduino or drivers. Knomis also download their
Wi-Fi updates from the same place.

## Contents

- `knomi/index.html`: the page. One button installs and updates.
- `knomi/img/`: its photo (`docs/images/v2/knomi-on-stand.gif`, the Knomi in its case on its stand, as WebM and MP4 videos, a fifth of the size, and a still), and the Poké Ball for the steps.
- `knomi/brand/`: the CATCH 151 FOR KNOMI sign and the icons, copied from `tools/assets/brand/`.
- `knomi/manifest.json`: ESP Web Tools' config, pointing at the firmware.
- `knomi/firmware/`: the firmware, and the sprites packed for the Knomi's flash.
- `knomi/update.json`: what a Knomi checks for a Wi-Fi update.

The site also has the **Catch 151 Dex** at `/sprites/`, every sprite the game uses in every view, with its artists. It isn't
kept here: the deploy builds it from the packs with `tools/make_sprite_gallery.py` (the page is
`tools/sprite_gallery.html`; a tap on a Pokémon plays its voice, from `tools/voices.json`, which `make -C host voices`
writes). To see it locally, `python3 tools/make_sprite_gallery.py` (it needs Pillow) and serve `build/sprites/`.

`web/play/` holds the last Catch 151 for Web build (`webgame/build.sh`), which `tools/publish_web.sh` publishes to its
own site.

## Rebuild

After a firmware change, rebuild `web/knomi/firmware/` with `tools/build_knomi_web.sh`. For each release also run `tools/build_xl_web.sh`: it makes `web/knomi/xl/`, Catch 151 XL's Wi-Fi updates (no page).

## Test locally

ESP Web Tools needs a secure context, `https://` or `http://localhost`:

```bash
cd web/knomi && python3 -m http.server 8000
# open http://localhost:8000 in Chrome or Edge
```

## Deploy (GitHub Pages)

`.github/workflows/pages.yml` publishes `web/knomi/` at https://fedorsosnin.github.io/Catch151/, and the Shiny
Pokédex at `/sprites/`, on every push to `main` that touches `web/`, the packs or the gallery's two files.

## Limitations

- Desktop Chrome and Edge only: Web Serial isn't in Firefox or Safari.
