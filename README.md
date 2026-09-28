# GBA video ROM template

A clean-slate GBA video player: title menu (PLAY / CHAPTERS / CONTROLS), a main video split into 3 chapters,
and a hidden second video plus a hidden secret screen, all unlocked with button codes on the main menu.
It ships with tiny placeholder assets (test-pattern video + beep, plain backgrounds) so it builds and runs
straight away. Swap in your own content and rebuild.

## What's in the box
| File | What it is |
|---|---|
| `main.c`, `Makefile`, `.github/workflows/build.yml` | The player + build (GitHub Actions builds `video.gba` on every push) |
| `frames.bin` `palette.bin` `audio.bin` | **Main video** (frames, 256-colour palette, ADPCM audio) |
| `vid2_frames.bin` `vid2_palette.bin` `vid2_audio.bin` | **Hidden second video** |
| `menu_bg.bin` `menu_pal.bin` | **Menu background** |
| `secret_bg.bin` `secret_pal.bin` | **Secret screen background** |
| `assets_src/` | Editable PNG sources for the two backgrounds |
| `tools/gbatool.py` | Converter: video / image -> the `.bin` files |

## Making a new ROM
Needs Python 3, `pip install pillow numpy`, and `ffmpeg` on your PATH.

```
python3 tools/gbatool.py video  my_movie.mp4                 # main video + audio
python3 tools/gbatool.py video  my_clip.mp4 --slot secret    # hidden second video
python3 tools/gbatool.py bg     my_menu.png --slot menu      # menu background
python3 tools/gbatool.py bg     my_secret.png --slot secret  # secret screen background
python3 tools/gbatool.py placeholders                        # reset everything to the test assets
```
Then commit and push (GitHub builds the ROM), or run `make` locally with devkitARM installed.

Video options: `--stretch` (fill 120x68 instead of letterboxing), `--dither` (smoother gradients), `--gain 1.5` (louder audio).
The tool prints the running total; a GBA ROM holds 32 MB, roughly 11-12 minutes of main video in total.

## Facts worth knowing
* Video is **120x68, 5 fps, one shared 256-colour palette**, drawn 2x in Mode 4. Audio is mono 4-bit ADPCM at ~9078 Hz.
* Video length is read from the file sizes, so there is nothing to edit in `main.c` when clips change length.
* Menu/secret backgrounds are 240x160 with **colours 254 (black) and 255 (white) reserved** for the menu text; the converter handles this.
* The three menu buttons sit at x=192, y=15 / 38 / 61 (top right). `assets_src/menu_bg.png` shows the slots; keep art there clear or matching.
* Chapters (PART 1/2/3) simply split the main video into thirds.
* Codes on the main menu: `U U D D L R L R B A` shows the secret screen; `D D U U L R L R B A` plays the second video.
* Menu labels are set in `main()` (`main_items`, `chap_items`); the built-in font has only A-Z and 1-3.
* In-video controls: A+Up pause, A+Right fast-forward, A+Down rewind, Select = menu.
* Removed from the original: both videos, the audio, all three background images, and the "Woover" 4-channel jingle (with its orchestra-hit sample). The secret screen now just waits for a button press.

Note: I couldn't compile for ARM in my sandbox (no devkitARM), so the first GitHub Actions build is the real test.
