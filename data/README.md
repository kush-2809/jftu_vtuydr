# SmartTank portal branding

Put **two PNG images** in this `data/` directory (not in `include/` or `src/`):

- `data/logo.png` — product logo shown in the fixed portal header. Recommended transparent PNG about 256×256 pixels (or a compact horizontal logo).
- `data/favicon.png` — square browser tab and home-screen icon. Recommended PNG 64×64 or 192×192 pixels.

The current HTML and firmware serve these exact paths: `/logo.png` and `/favicon.png`.

After copying the images:

```powershell
pio run -e esp32-s3-devkitc-1 -t upload
pio run -e esp32-s3-devkitc-1 -t uploadfs
```

Reload the page (hard-refresh on desktop) to update cached images. If the logo is missing, the portal shows a built-in `ST` badge instead.

**Do not run `erase_flash` or change the partition table.** The code uses `LittleFS.begin(false)` and never formats the filesystem automatically. `uploadfs` **replaces the contents of the LittleFS partition** (so back up any pre-existing files stored there). Normal application firmware updates and LittleFS asset uploads are distinct from the NVS partition containing tank settings, provided the partition layout stays unchanged.

If you have SVG, JPG, WEBP or ICO originals, export the copies you place here as PNG using the names above. Do not rename your source originals. The firmware currently serves PNG only.

If the asset does not appear, open `http://smarttank.local/logo.png` and `http://smarttank.local/favicon.png` directly to diagnose whether they were uploaded.
